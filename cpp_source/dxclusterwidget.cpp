// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "dxclusterwidget.h"
#include "qrzcache.h"
#include "mainwindow.h"
#include "paths.h"
#include "prefix_parser.h"
#include <QPainter>
#include <QSettings>
#include <QCoreApplication>
#include <QCheckBox>
#include <QRegularExpression>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMouseEvent>
#include <QMessageBox>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QXmlStreamReader>
#include <QUrl>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QDir>

// ============================================================================
// COMPONENT: DxClusterWidget
// ROLE: Dynamic DX Cluster spot aggregator and presentation feed.
//       Establishes asynchronous Telnet TCP socket links with configured logbook
//       servers, parses incoming spot feeds via regex patterns, performs local
//       QRZ country cache mapping or asynchronous SVG flag lookups, and paints
//       a custom scroll-animated table with smooth alpha-channel bottom fading.
// INVOKED BY: MainWindow (via "Spots / DX Cluster" action).
// PRODUCES: Live DX spot entries, flag icons, and click-to-lookup spot signals.
// ============================================================================

// [ENTRY POINT: DX Cluster Constructor]
// Restores saved window geometry, registers socket TCP callbacks, and configures
// the periodic connection heartbeat and scrolling animation timers.
DxClusterWidget::DxClusterWidget(QWidget *parent) 
    : QWidget(parent, Qt::Window)
    , m_scrollOffset(0)
    , m_socket(new QTcpSocket(this))
    , m_reconnectTimer(new QTimer(this))
    , m_animationTimer(new QTimer(this))
    , m_netManager(new QNetworkAccessManager(this))
{
    setWindowTitle("DX Cluster Spots");
    setObjectName("dxClusterWindow");
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    if (settings.contains("dxClusterWindow/size")) {
        resize(settings.value("dxClusterWindow/size").toSize());
    } else {
        resize(500, 600);
    }
    if (settings.contains("dxClusterWindow/pos")) {
        move(settings.value("dxClusterWindow/pos").toPoint());
    }
    
    setMouseTracking(true);

    connect(m_socket, &QTcpSocket::readyRead, this, &DxClusterWidget::onReadyRead);
    connect(m_socket, &QTcpSocket::connected, this, &DxClusterWidget::onConnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_socket, &QTcpSocket::errorOccurred, this, &DxClusterWidget::onError);
#else
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this, &DxClusterWidget::onError);
#endif

    m_reconnectTimer->setInterval(5000);
    connect(m_reconnectTimer, &QTimer::timeout, this, &DxClusterWidget::connectToServer);

    m_animationTimer->setInterval(30);
    connect(m_animationTimer, &QTimer::timeout, this, &DxClusterWidget::animateScroll);
}

DxClusterWidget::~DxClusterWidget() {
    stopClustering();
}

void DxClusterWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    startClustering();
}

void DxClusterWidget::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);
    stopClustering();
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.setValue("dxClusterWindow/size", size());
    settings.setValue("dxClusterWindow/pos", pos());
}

void DxClusterWidget::closeEvent(QCloseEvent *event) {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.setValue("dxClusterWindow/size", size());
    settings.setValue("dxClusterWindow/pos", pos());
    QWidget::closeEvent(event);
}

// [ENTRY POINT: Feed Polling & Connection Manager]
// Orchestrates startup and teardown of the background TCP feed connection loops,
// scheduling recurrent socket heartbeats and starting smooth visual interpolation timers.
void DxClusterWidget::startClustering() {
    connectToServer();
    m_reconnectTimer->start();
    m_animationTimer->start();
}

void DxClusterWidget::stopClustering() {
    m_reconnectTimer->stop();
    m_animationTimer->stop();
    m_socket->disconnectFromHost();
    m_socket->close();
}

// [ENTRY POINT: Telnet Server Handshake Dispatcher]
// Connects to the host/port configured in Feeds/dxClusterServer, logs the connection event 
// to the system log, and writes the user's callsign token as the Telnet handshake.
void DxClusterWidget::connectToServer() {
    if (m_socket->state() == QAbstractSocket::ConnectedState || m_socket->state() == QAbstractSocket::ConnectingState) return;

    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString serverConfig = settings.value("Feeds/dxClusterServer", "dx.ve7cc.net:23").toString();
    if (serverConfig.isEmpty()) return;

    QString host;
    int port = 7373;
    if (serverConfig.contains(":")) {
        QStringList parts = serverConfig.split(":");
        host = parts[0];
        port = parts[1].toInt();
    } else {
        host = serverConfig;
    }
    
    m_serverName = host;
    
    QSettings logSettings(QCoreApplication::organizationName(), "Qwain");
    QString sysLogPath = logSettings.value("Files/systemLogfile", "").toString();
    if (!sysLogPath.isEmpty()) {
        QFile file(sysLogPath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << QDateTime::currentDateTimeUtc().toString("dd-MM-yy HH:mm:ss.zzz UTC") << " DXCluster attempting connect to " << host << ":" << port << "\n";
        }
    }
    
    m_socket->connectToHost(host, port);
}

// [ENTRY POINT: Connection Connected Callback]
// Logs successful connection telemetry and dispatches the initial login credentials block to the host socket.
void DxClusterWidget::onConnected() {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString sysLogPath = settings.value("Files/systemLogfile", "").toString();
    if (!sysLogPath.isEmpty()) {
        QFile file(sysLogPath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << QDateTime::currentDateTimeUtc().toString("dd-MM-yy HH:mm:ss.zzz UTC") << " DXCluster Connected to " << m_serverName << "\n";
        }
    }

    QString call = settings.value("Credentials/username", "").toString();
    if(call.isEmpty() || call == "N0CALL") {
        QMessageBox::warning(this, "No Callsign configured", "No callsign is configured in the Configuration Dialog.\nUsing N0CALL, which may be rejected by the DX cluster.");
        call = "N0CALL";
    }
    m_socket->write((call + "\n").toUtf8());
}

// [ENTRY POINT: Connection Error Callback]
// Handles connection dropouts or handshake rejections by logging socket error state and aborting the stream.
void DxClusterWidget::onError(QAbstractSocket::SocketError err) {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString sysLogPath = settings.value("Files/systemLogfile", "").toString();
    if (!sysLogPath.isEmpty()) {
        QFile file(sysLogPath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << QDateTime::currentDateTimeUtc().toString("dd-MM-yy HH:mm:ss.zzz UTC") << " DXCluster Connection Error/Disconnected. Code: " << err << "\n";
        }
    }
    m_socket->abort();
}

// [ENTRY POINT: TCP Stream Buffer Callback]
// Accumulates raw incoming socket bytes, processes line-separated Telnet outputs,
// and trims trailing whitespace before feeding lines to the regex parser pipeline.
void DxClusterWidget::onReadyRead() {
    m_buffer += QString::fromLatin1(m_socket->readAll());
    int idx;
    while ((idx = m_buffer.indexOf('\n')) != -1) {
        QString line = m_buffer.left(idx).trimmed();
        m_buffer.remove(0, idx + 1);
        processLine(line);
    }
}

// [ENTRY POINT: Regex Pattern Spot Resolver]
// Parses individual Telnet lines against multi-tiered regex definitions to extract spotter,
// DX frequency, target callsign, mode comment, and timestamps.
void DxClusterWidget::processLine(const QString &line) {
    if (line.contains("login:", Qt::CaseInsensitive) || line.contains("callsign:", Qt::CaseInsensitive)) {
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        QString call = settings.value("Credentials/username", "").toString();
        if(call.isEmpty() || call == "N0CALL") {
            call = "N0CALL";
        }
        m_socket->write((call + "\n").toUtf8());
        return;
    }

    // Try several regex patterns to parse spots, from strict to permissive.
    // 1. Strict DX pattern: DX de [spotter]: [freq] [dx] [comment] [time]
    QRegularExpression rxStrict("DX\\s+de\\s+([A-Za-z0-9\\-\\/#]+)[-\\s:]+\\s*(\\d+(?:\\.\\d+)?)\\s+([A-Za-z0-9\\-\\/]+)\\s*(.*?)\\s*(\\d{4}Z?)$", QRegularExpression::CaseInsensitiveOption);
    // 2. DX without time: DX de [spotter]: [freq] [dx] [comment]
    QRegularExpression rxNoTime("DX\\s+de\\s+([A-Za-z0-9\\-\\/#]+)[-\\s:]+\\s*(\\d+(?:\\.\\d+)?)\\s+([A-Za-z0-9\\-\\/]+)\\s*(.*)", QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatch match;
    bool matched = false;
    
    match = rxStrict.match(line);
    if (match.hasMatch()) matched = true;
    else {
        match = rxNoTime.match(line);
        if (match.hasMatch()) matched = true;
    }

    if (matched) {
        DxSpot spot;
        spot.spooter = match.captured(1);
        spot.frequency = match.captured(2);
        spot.spotted = match.captured(3);
        spot.mode_comment = match.captured(4).trimmed();
        
        // if time was extracted in group 5, otherwise, check if it's trapped at end of group 4
        if (match.lastCapturedIndex() >= 5) {
            spot.time = match.captured(5);
        } else {
            QRegularExpression timeEnd("(\\d{4}Z?)$");
            QRegularExpressionMatch tmMatch = timeEnd.match(spot.mode_comment);
            if (tmMatch.hasMatch()) {
                spot.time = tmMatch.captured(1);
                spot.mode_comment = spot.mode_comment.left(spot.mode_comment.length() - spot.time.length()).trimmed();
            }
        }
        
        addSpot(spot);
    }
}

// [ENTRY POINT: Feed Spot Queue Controller]
// Adds newly parsed spots to the prepend list, limits maximum spots queue to 15 entries, 
// sets up visual scroll animation offsets, and initiates flag lookup spools.
void DxClusterWidget::addSpot(const DxSpot &spot) {
    m_spots.prepend(spot);
    if (m_spots.size() > 15) m_spots.takeLast();
    
    m_scrollOffset -= 40.0; // Assume itemH is 40
    if (m_scrollOffset < -160.0) {
        m_scrollOffset = -160.0;
    }
    fetchFlagForSpot(0);
}

// [ENTRY POINT: Scroll Animation Timer Slot]
// Performs smooth pixel interpolation adjustments over negative scrollOffset indexes,
// creating a slide-down animation effect whenever a new DX spot is prepended.
void DxClusterWidget::animateScroll() {
    if (m_scrollOffset < 0) {
        m_scrollOffset += 1.5; // smoother and faster
        if (m_scrollOffset > 0) m_scrollOffset = 0;
        update();
    }
}

// [ENTRY POINT: Callsign Country Flag Resolver]
// Maps spotted callsigns to countries via local QRZ cached profiles. If a match is found,
// fetches and caches country flag PNG assets asynchronously from FlagCDN, scaling the result to fit rows.
void DxClusterWidget::fetchFlagForSpot(int index) {
    if (index < 0 || index >= m_spots.size()) return;
    QString callsign = m_spots[index].spotted;
    
    QString appDataDir = getAppDataDir();
    QrzCache qrzCache(appDataDir + "/qrz_cache.ini", QSettings::IniFormat);
    
    // First, try to get country from local QRZ cache
    qrzCache.beginGroup(callsign);
    QString country = qrzCache.value("country").toString();
    QString qrzCountry = qrzCache.value("qrz_country").toString();
    qrzCache.endGroup();
    
    if (country.toUpper() == "NOT_FOUND") country = "";
    if (qrzCountry.toUpper() == "NOT_FOUND") qrzCountry = "";
    if (country.isEmpty()) country = qrzCountry;

    auto fetchImageByIso = [this, callsign, appDataDir](const QString &iso) {
        if (iso.isEmpty()) {
            // Placeholder
            QImage ph(30, 20, QImage::Format_ARGB32);
            ph.fill(Qt::darkGray);
            for (int i = 0; i < m_spots.size(); ++i) {
                if (m_spots[i].spotted == callsign) {
                    m_spots[i].flagImage = ph;
                    update();
                }
            }
            return;
        }
        QrzCache cacheConfig(appDataDir + "/qrz_cache.ini", QSettings::IniFormat);
        cacheConfig.beginGroup("icons");
        QByteArray flagData = cacheConfig.value("flag_" + iso).toByteArray();
        cacheConfig.endGroup();
        
        if (!flagData.isEmpty() && !flagData.startsWith("empty")) {
            QImage img;
            img.loadFromData(flagData);
            for (int i = 0; i < m_spots.size(); ++i) {
                if (m_spots[i].spotted == callsign) {
                    m_spots[i].flagImage = img.scaledToHeight(20, Qt::SmoothTransformation);
                    update();
                }
            }
        } else if (flagData.isEmpty()) {
            qDebug() << "[DxCluster Flag Debug] Downloading from flagcdn for iso:" << iso;
            QNetworkRequest req(QUrl("https://flagcdn.com/w80/" + iso + ".png"));
            req.setHeader(QNetworkRequest::UserAgentHeader, "Qwain/8.2");
            QNetworkReply *reply = m_netManager->get(req);
            connect(reply, &QNetworkReply::finished, this, [this, reply, callsign, iso, appDataDir]() {
                reply->deleteLater();
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray d = reply->readAll();
                    QrzCache cm(appDataDir + "/qrz_cache.ini", QSettings::IniFormat);
                    cm.beginGroup("icons");
                    cm.setValue("flag_" + iso, d);
                    cm.endGroup();
                    QImage img;
                    if (img.loadFromData(d)) {
                        for (int i = 0; i < m_spots.size(); ++i) {
                            if (m_spots[i].spotted == callsign) {
                                m_spots[i].flagImage = img.scaledToHeight(20, Qt::SmoothTransformation);
                                update();
                            }
                        }
                    } else {
                        qDebug() << "[DxCluster Flag Debug] API returned ok, but invalid image data for iso:" << iso;
                        // Placeholder
                        QImage ph(30, 20, QImage::Format_ARGB32);
                        ph.fill(Qt::darkGray);
                        for (int i = 0; i < m_spots.size(); ++i) {
                            if (m_spots[i].spotted == callsign) {
                                m_spots[i].flagImage = ph;
                                update();
                            }
                        }
                    }
                } else {
                    qDebug() << "[DxCluster Flag Debug] API request failed for iso:" << iso << "error:" << reply->errorString();
                    // Placeholder
                    QImage ph(30, 20, QImage::Format_ARGB32);
                    ph.fill(Qt::darkGray);
                    for (int i = 0; i < m_spots.size(); ++i) {
                        if (m_spots[i].spotted == callsign) {
                            m_spots[i].flagImage = ph;
                            update();
                        }
                    }
                }
            });
        }
    };
    
    if (!country.isEmpty()) {
        QString mappedIso = MainWindow::countryToIso(country);
        if (!mappedIso.isEmpty()) {
            fetchImageByIso(mappedIso);
            return;
        }
    }
    
    qrzCache.beginGroup(callsign);
    bool hamdbFailed = qrzCache.value("hamdb_not_found", false).toBool();
    qrzCache.endGroup();
    
    if (hamdbFailed || !hamdbFailed) { // Always guess ISO if not in cache to avoid blocking HTTP requests
        QString guessedIso = guessIsoFromCallsign(callsign);
        if (!guessedIso.isEmpty()) {
            fetchImageByIso(guessedIso);
        } else {
            // Placeholder
            QImage ph(30, 20, QImage::Format_ARGB32);
            ph.fill(Qt::darkGray);
            for (int i = 0; i < m_spots.size(); ++i) {
                if (m_spots[i].spotted == callsign) {
                    m_spots[i].flagImage = ph;
                    update();
                }
            }
        }
        return;
    }
}

void DxClusterWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        // Check if user clicked on a spot's bounding box
        int headerHeight = 60;
        int y = headerHeight + 5;
        int itemH = 40;
        double currentY = y + m_scrollOffset;
        
        for (int i = 0; i < m_spots.size(); ++i) {
            QRectF itemRect(0, currentY, width(), itemH);
            if (itemRect.contains(event->pos())) {
                emit spotClicked(m_spots[i].spotted);
                event->accept();
                return;
            }
            currentY += itemH;
            if (currentY > height()) break; 
        }
        event->accept();
    }
}

void DxClusterWidget::mouseMoveEvent(QMouseEvent *event) {
    int headerHeight = 60;
    int y = headerHeight + 5;
    int itemH = 40;
    double currentY = y + m_scrollOffset;
    
    bool overRow = false;
    for (int i = 0; i < m_spots.size(); ++i) {
        QRectF itemRect(0, currentY, width(), itemH);
        if (itemRect.contains(event->pos())) {
            overRow = true;
            break;
        }
        currentY += itemH;
        if (currentY > height()) break; 
    }
    
    if (overRow) {
        setCursor(Qt::PointingHandCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    QWidget::mouseMoveEvent(event);
}

void DxClusterWidget::mouseReleaseEvent(QMouseEvent *event) {
    QWidget::mouseReleaseEvent(event);
}

// [ENTRY POINT: Spot Paint Render Loop]
// Custom draws rows with smooth scroll interpolation offsets, parses HTML-rich spot layouts,
// and applies gradient transparency opacity clipping as rows approach screen limits.
void DxClusterWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // Draw background
    p.fillRect(rect(), QColor(25, 25, 25, 230)); 
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString serverConfig = settings.value("Feeds/dxClusterServer", "Disconnected").toString();
    if (serverConfig.isEmpty()) serverConfig = "Disconnected";
    
    p.setPen(Qt::white);
    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    p.setFont(titleFont);
    
    int headerHeight = 60;
    QRect headerRect(10, 10, width() - 20, headerHeight);
    p.drawText(headerRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, QString("Latest DXCluster spots from %1").arg(serverConfig));
    p.drawLine(10, headerHeight, width() - 10, headerHeight);
    
    int y = headerHeight + 5;
    p.setClipRect(0, y, width(), height() - y);
    
    int itemH = 40;
    double currentY = y + m_scrollOffset;
    
    QFont textFont = font();
    textFont.setPointSize(10);
    QFont boldFont = textFont;
    boldFont.setBold(true);
    
    for (int i = 0; i < m_spots.size(); ++i) {
        const auto &spot = m_spots[i];
        
        QRectF itemRect(0, currentY, width(), itemH);
        if (i % 2 == 0) {
            p.fillRect(itemRect, QColor(40, 40, 40, 200));
        }
        
        p.save();
        
        // Opacity fade when dropping off the bottom
        double fadeStart = height() - itemH;
        if (currentY > fadeStart) {
            double opacity = 1.0 - (currentY - fadeStart) / itemH;
            if (opacity < 0) opacity = 0;
            p.setOpacity(opacity);
        }
        
        QTextDocument doc;
        int textOffset = 10;
        QString flagHtml = "";
        if (!spot.flagImage.isNull()) {
            doc.addResource(QTextDocument::ImageResource, QUrl("flag"), spot.flagImage);
            flagHtml = "<img src='flag' /> ";
        }
        
        QString html = QString("<div style='font-family: monospace; font-size: 13px; color: white; white-space: pre;'>"
                               "<font color='#95a5a6'>%1</font> DX de %2<a href='#' style='color:#3498db; text-decoration:none;'><b>%3</b></a>  <font color='#e74c3c'>%4</font>  <font color='#2ecc71'>%5</font> <font color='#bdc3c7'>%6</font>"
                               "</div>")
                       .arg((spot.spooter).leftJustified(10, ' ').toHtmlEscaped(),
                            flagHtml,
                            spot.spotted.leftJustified(12, ' ').toHtmlEscaped(),
                            spot.frequency.rightJustified(9, ' ').toHtmlEscaped(),
                            spot.mode_comment.leftJustified(30, ' ').toHtmlEscaped(),
                            spot.time.rightJustified(5, ' ').toHtmlEscaped());

        doc.setHtml(html);
        
        p.save();
        
        p.translate(textOffset, currentY + (itemH - doc.size().height()) / 2.0);
        doc.drawContents(&p);
        p.restore();
        p.restore(); // Restore the outer p.save() (from line 376)
        
        currentY += itemH;
        if (currentY > height() + itemH) break; // Optimization
    }
}
