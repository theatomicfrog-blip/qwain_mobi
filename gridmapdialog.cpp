#include <QCoreApplication>
#include "paths.h"
#include "gridmapdialog.h"
#include <QStandardPaths>
#include <QMessageBox>
// Force file sync to fix multiple definition linker errors
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QGraphicsPixmapItem>
#include <QtMath>
#include <QFile>
#include <QDir>
#include <QPainter>
#include <QDebug>
#include <QToolTip>
#include <QMouseEvent>
#include <QStringList>

#if __has_include("world.png.h")
namespace WorldMapGrid {
#include "world.png.h"
}
#endif

#include <QEvent>
#include <QTimer>
#include <QLabel>
#include <QSettings>
#include <QFileDialog>
#include <QPushButton>
#include <QScrollBar>
#include <QRegularExpression>

GridMapView::GridMapView(QWidget *parent) : QGraphicsView(parent), m_mapItem(nullptr), m_drawMode(0)
{
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 3600, 1800);
    setScene(m_scene);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    setMouseTracking(true);

    m_gridTooltip = new QLabel(viewport());
    m_gridTooltip->setWordWrap(true);
    m_gridTooltip->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    m_gridTooltip->setStyleSheet("QLabel { background-color: #ffffe0; border: 1.5px solid black; padding: 4px; color: black; }");
    m_gridTooltip->hide();
    m_gridTooltip->installEventFilter(this);
    connect(m_gridTooltip, &QLabel::linkActivated, this, &GridMapView::onGridCallsignClicked);

    m_callTooltip = new QLabel(viewport());
    m_callTooltip->setWordWrap(true);
    m_callTooltip->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_callTooltip->setStyleSheet("QLabel { background-color: #e0f7fa; border: 1.5px solid black; padding: 4px; color: black; }");
    m_callTooltip->hide();
    m_callTooltip->installEventFilter(this);
}

void GridMapView::setMapImage(const QPixmap &pixmap)
{
    if (m_mapItem) {
        m_scene->removeItem(m_mapItem);
        delete m_mapItem;
    }
    m_mapItem = m_scene->addPixmap(pixmap.scaled(3600, 1800, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    m_mapItem->setZValue(-1);
}

void GridMapView::setGridQsos(const GridQsoMap &gridQsos)
{
    m_gridQsos = gridQsos;
    m_scene->invalidate(); // Request redraw
}

void GridMapView::setPotaQsos(const QList<QsoDetail> &potaQsos)
{
    m_potaQsos = potaQsos;
    m_scene->invalidate();
}

void GridMapView::setDrawMode(int mode)
{
    m_drawMode = mode;
    m_scene->invalidate();
}

void GridMapView::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
    
    QPointF scenePos = mapToScene(event->pos());
    
    if (m_drawMode == 1) {
        // POTA Mode: check pota list
        QsoDetail hoveredPota;
        qreal minDistance = 5.0; // hit radius
        for (const QsoDetail &d : m_potaQsos) {
            bool hasLoc = !d.lat.isEmpty() && !d.lon.isEmpty();
            QPointF p = hasLoc ? getLatLonPoint(d.lat, d.lon) : getGridRect(d.grid).center();
            if (p.isNull()) continue;
            qreal dist = QLineF(scenePos, p).length();
            if (dist < minDistance) {
                minDistance = dist;
                hoveredPota = d;
            }
        }
        
        if (!hoveredPota.call.isEmpty()) {
            if (hoveredPota.call != m_currentHoveredGrid) {
                m_currentHoveredGrid = hoveredPota.call;
                
                QString html = QString("<b>Callsign: %1</b><br/>POTA: %2<br/>Grid: %3").arg(hoveredPota.call).arg(hoveredPota.pota).arg(hoveredPota.grid);
                m_gridTooltip->setText(html);
                m_gridTooltip->adjustSize();
                
                QPoint pos = event->pos() + QPoint(15, 15);
                if (pos.x() + m_gridTooltip->width() > viewport()->width()) pos.setX(viewport()->width() - m_gridTooltip->width());
                if (pos.y() + m_gridTooltip->height() > viewport()->height()) pos.setY(viewport()->height() - m_gridTooltip->height());
                
                m_gridTooltip->move(pos);
                m_gridTooltip->show();
                m_callTooltip->hide(); // Not needed for POTA simple tooltip
            }
        } else {
            m_currentHoveredGrid.clear();
            m_gridTooltip->hide();
            m_callTooltip->hide();
        }
        return;
    }
    
    QString hoveredGrid;
    
    for (auto it = m_gridQsos.begin(); it != m_gridQsos.end(); ++it) {
        if (getGridRect(it.key()).contains(scenePos)) {
            hoveredGrid = it.key();
            break;
        }
    }
    
    if (hoveredGrid != m_currentHoveredGrid) {
        m_currentHoveredGrid = hoveredGrid;
        if (!hoveredGrid.isEmpty()) {
            QStringList calls = m_gridQsos[hoveredGrid].keys();
            calls.sort();
            QStringList links;
            for (const QString &c : calls) {
                links << QString("<a href=\"%1\">%1</a>").arg(c);
            }
            QString html = QString("<b>Grid: %1</b><br/>Calls: %2").arg(hoveredGrid).arg(links.join(", "));
            
            m_gridTooltip->setText(html);
            m_gridTooltip->adjustSize();
            
            QPoint pos = event->pos() + QPoint(15, 15);
            if (pos.x() + m_gridTooltip->width() > viewport()->width()) {
                pos.setX(viewport()->width() - m_gridTooltip->width());
            }
            if (pos.y() + m_gridTooltip->height() > viewport()->height()) {
                pos.setY(viewport()->height() - m_gridTooltip->height());
            }
            
            m_gridTooltip->move(pos);
            m_gridTooltip->show();
            m_callTooltip->hide();
        } else {
            m_gridTooltip->hide();
            m_callTooltip->hide();
        }
    }
}

void GridMapView::leaveEvent(QEvent *event)
{
    QGraphicsView::leaveEvent(event);
    QTimer::singleShot(50, this, [this]() {
        if (!m_gridTooltip->underMouse() && !m_callTooltip->underMouse() && !viewport()->underMouse()) {
            m_currentHoveredGrid.clear();
            m_gridTooltip->hide();
            m_callTooltip->hide();
        }
    });
}

bool GridMapView::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == m_gridTooltip || watched == m_callTooltip) && event->type() == QEvent::Leave) {
        QTimer::singleShot(50, this, [this]() {
            if (!m_gridTooltip->underMouse() && !m_callTooltip->underMouse() && !viewport()->underMouse()) {
                m_currentHoveredGrid.clear();
                m_gridTooltip->hide();
                m_callTooltip->hide();
            }
        });
    }
    return QGraphicsView::eventFilter(watched, event);
}

QString GridMapView::formatDateString(const QString &dateStr)
{
    if (dateStr.length() == 8) {
        return QString("%1-%2-%3").arg(dateStr.mid(0,4)).arg(dateStr.mid(4,2)).arg(dateStr.mid(6,2));
    }
    return dateStr;
}

QString GridMapView::formatTimeString(const QString &timeStr)
{
    if (timeStr.length() == 4) {
        return QString("%1:%2").arg(timeStr.mid(0,2)).arg(timeStr.mid(2,2));
    } else if (timeStr.length() >= 6) {
        return QString("%1:%2:%3").arg(timeStr.mid(0,2)).arg(timeStr.mid(2,2)).arg(timeStr.mid(4,2));
    }
    return timeStr;
}

void GridMapView::onGridCallsignClicked(const QString &callsign)
{
    if (m_currentHoveredGrid.isEmpty()) return;
    if (!m_gridQsos.contains(m_currentHoveredGrid) || !m_gridQsos[m_currentHoveredGrid].contains(callsign)) return;
    
    const QList<QsoDetail> &qsos = m_gridQsos[m_currentHoveredGrid][callsign];
    
    QString html = QString("<b>Callsign: %1</b><br/>").arg(callsign);
    html += "<table border='1' cellspacing='0' cellpadding='2'>";
    html += "<tr><th style='white-space:nowrap;'>Date</th><th style='white-space:nowrap;'>Time (UTC)</th><th style='white-space:nowrap;'>Freq (MHz)</th><th style='white-space:nowrap;'>Band</th><th style='white-space:nowrap;'>Mode</th><th style='white-space:nowrap;'>POTA</th></tr>";
    
    for (const QsoDetail &d : qsos) {
        QString fmtDate = formatDateString(d.date);
        QString fmtTime = formatTimeString(d.time);
        
        html += QString("<tr><td style='white-space:nowrap;'>%1</td><td style='white-space:nowrap;'>%2</td><td style='white-space:nowrap;'>%3</td><td style='white-space:nowrap;'>%4</td><td style='white-space:nowrap;'>%5</td><td style='white-space:nowrap;'>%6</td></tr>")
                .arg(fmtDate).arg(fmtTime).arg(d.freq).arg(d.band).arg(d.mode).arg(d.pota);
    }
    html += "</table>";
    
    m_callTooltip->setText(html);
    m_callTooltip->adjustSize();
    
    QPoint pos = m_gridTooltip->pos() + QPoint(m_gridTooltip->width() + 5, 0);
    
    // Adjust if goes out of viewport
    if (pos.x() + m_callTooltip->width() > viewport()->width()) {
        pos.setX(m_gridTooltip->pos().x() - m_callTooltip->width() - 5);
    }
    if (pos.y() + m_callTooltip->height() > viewport()->height()) {
        pos.setY(viewport()->height() - m_callTooltip->height());
    }
    
    m_callTooltip->move(pos);
    m_callTooltip->show();
}

void GridMapView::wheelEvent(QWheelEvent *event)
{
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    qreal angle = event->angleDelta().y();
#else
    qreal angle = event->delta();
#endif
    qreal zoomFactor = qPow(1.15, angle / 120.0);
    scale(zoomFactor, zoomFactor);
}

QRectF GridMapView::getGridRect(const QString &gridCode)
{
    if (gridCode.length() < 2) return QRectF();
    
    qreal lon = -180.0;
    qreal lat = -90.0;
    qreal lonWidth = 360.0;
    qreal latHeight = 180.0;
    
    if (gridCode.length() >= 2) {
        lon = -180.0 + (gridCode[0].toUpper().unicode() - 'A') * 20.0;
        lat = -90.0 + (gridCode[1].toUpper().unicode() - 'A') * 10.0;
        lonWidth = 20.0;
        latHeight = 10.0;
    }
    if (gridCode.length() >= 4) {
        lon += (gridCode[2].unicode() - '0') * 2.0;
        lat += (gridCode[3].unicode() - '0') * 1.0;
        lonWidth = 2.0;
        latHeight = 1.0;
    }
    if (gridCode.length() >= 6) {
        lon += (gridCode[4].toUpper().unicode() - 'A') * (2.0/24.0);
        lat += (gridCode[5].toUpper().unicode() - 'A') * (1.0/24.0);
        lonWidth = (2.0/24.0);
        latHeight = (1.0/24.0);
    }
    if (gridCode.length() >= 8) {
        lon += (gridCode[6].unicode() - '0') * (2.0/240.0);
        lat += (gridCode[7].unicode() - '0') * (1.0/240.0);
        lonWidth = (2.0/240.0);
        latHeight = (1.0/240.0);
    }
    
    qreal x = (lon + 180.0) * 10.0;
    qreal y = (90.0 - lat - latHeight) * 10.0;
    return QRectF(x, y, lonWidth * 10.0, latHeight * 10.0);
}

QPointF GridMapView::getLatLonPoint(const QString &latStr, const QString &lonStr)
{
    if (latStr.isEmpty() || lonStr.isEmpty()) return QPointF();
    
    // Parse ADIF Lat (e.g. N045 12.345)
    auto parseDegree = [](const QString &s) -> qreal {
        if (s.isEmpty()) return 0;
        QChar dir = s[0].toUpper();
        int spaceIdx = s.indexOf(' ');
        if (spaceIdx > 1) {
            qreal deg = s.mid(1, spaceIdx - 1).toDouble();
            qreal min = s.mid(spaceIdx + 1).toDouble();
            qreal val = deg + min / 60.0;
            if (dir == 'S' || dir == 'W') val = -val;
            return val;
        }
        return 0;
    };
    
    qreal lat = parseDegree(latStr);
    qreal lon = parseDegree(lonStr);
    if (lat == 0 && lon == 0) return QPointF();
    
    qreal x = (lon + 180.0) * 10.0;
    qreal y = (90.0 - lat) * 10.0;
    return QPointF(x, y);
}

void GridMapView::drawForeground(QPainter *painter, const QRectF &rect)
{
    qreal currentScale = transform().m11();
    
    // Draw shaded worked grids or Pota icons
    painter->setPen(Qt::NoPen);
    if (m_drawMode == 1) {
        painter->setBrush(QColor(0, 150, 0, 150)); // differs: darker green for worked, though here all grids have POTAs worked
        
        for (const QsoDetail &d : m_potaQsos) {
            bool hasLoc = !d.lat.isEmpty() && !d.lon.isEmpty();
            QPointF p = hasLoc ? getLatLonPoint(d.lat, d.lon) : getGridRect(d.grid).center();
            if (p.isNull()) continue;
            
            QRectF iconRect(p.x() - 1.5, p.y() - 1.5, 3.0, 3.0);
            if (iconRect.intersects(rect)) {
                painter->setPen(QPen(QColor(0, 100, 0), qMax(1.0 / currentScale, 0.5)));
                painter->setBrush(QColor(0, 150, 0, 200));
                QPainterPath path;
                path.moveTo(iconRect.center().x(), iconRect.top());
                path.lineTo(iconRect.right(), iconRect.bottom());
                path.lineTo(iconRect.left(), iconRect.bottom());
                path.closeSubpath();
                painter->drawPath(path);
                
                // trunk
                painter->setBrush(QColor(139, 69, 19, 200)); // saddle brown
                painter->drawRect(iconRect.center().x() - iconRect.width() * 0.1, iconRect.bottom(),
                                  iconRect.width() * 0.2, iconRect.height() * 0.2);
                painter->setPen(Qt::NoPen);
            }
        }
    } else {
        painter->setBrush(QColor(255, 0, 0, 80)); // Light red shade with some transparency
        for (const QString &grid : m_gridQsos.keys()) {
            QRectF gridRect = getGridRect(grid);
            if (gridRect.intersects(rect)) {
                painter->drawRect(gridRect);
            }
        }
    }

    bool drawSquares = (currentScale > 1.5);
    bool drawSubsquares = (currentScale > 15.0);

    QPen fieldPen(QColor(0, 0, 0, 200), qMax(1.0 / currentScale, 0.5));
    QPen squarePen(QColor(50, 50, 50, 150), qMax(0.5 / currentScale, 0.2));
    QPen subPen(QColor(100, 100, 100, 100), qMax(0.2 / currentScale, 0.1));

    // Field lines
    painter->setPen(fieldPen);
    for (int x = 0; x <= 3600; x += 200) {
        if (x >= rect.left() && x <= rect.right()) painter->drawLine(QLineF(x, 0, x, 1800));
    }
    for (int y = 0; y <= 1800; y += 100) {
        if (y >= rect.top() && y <= rect.bottom()) painter->drawLine(QLineF(0, y, 3600, y));
    }

    // Square lines
    if (drawSquares) {
        painter->setPen(squarePen);
        for (int x = 0; x <= 3600; x += 20) {
            if (x % 200 == 0) continue;
            if (x >= rect.left() && x <= rect.right()) painter->drawLine(QLineF(x, 0, x, 1800));
        }
        for (int y = 0; y <= 1800; y += 10) {
            if (y % 100 == 0) continue;
            if (y >= rect.top() && y <= rect.bottom()) painter->drawLine(QLineF(0, y, 3600, y));
        }
    }

    // Subsquare lines
    if (drawSubsquares) {
        painter->setPen(subPen);
        qreal subW = 20.0 / 24.0;
        qreal subH = 10.0 / 24.0;
        for (qreal x = 0; x <= 3600; x += subW) {
            if (qRound(x) % 20 == 0) continue;
            if (x >= rect.left() && x <= rect.right()) painter->drawLine(QLineF(x, 0, x, 1800));
        }
        for (qreal y = 0; y <= 1800; y += subH) {
            if (qRound(y) % 10 == 0) continue;
            if (y >= rect.top() && y <= rect.bottom()) painter->drawLine(QLineF(0, y, 3600, y));
        }
    }

    // Labels
    QFont font = painter->font();
    if (!drawSquares) {
        font.setPointSizeF(qMax(20.0 / currentScale, 5.0));
        painter->setFont(font);
        painter->setPen(Qt::black);
        for (int lon = 0; lon < 18; ++lon) {
            for (int lat = 0; lat < 18; ++lat) {
                qreal x = lon * 200;
                qreal y = 1800 - (lat + 1) * 100;
                if (QRectF(x, y, 200, 100).intersects(rect)) {
                    QString label = QString("%1%2").arg(QChar('A' + lon)).arg(QChar('A' + lat));
                    painter->drawText(QRectF(x, y, 200, 100), Qt::AlignCenter, label);
                }
            }
        }
    } else if (drawSquares && !drawSubsquares) {
        font.setPointSizeF(qMax(4.0 / currentScale, 1.0));
        painter->setFont(font);
        painter->setPen(Qt::black);
        int startLon = qMax(0, (int)(rect.left() / 20.0));
        int endLon = qMin(180, (int)(rect.right() / 20.0 + 1));
        int startLat = qMax(0, (int)((1800 - rect.bottom()) / 10.0));
        int endLat = qMin(180, (int)((1800 - rect.top()) / 10.0 + 1));
        
        for (int xIdx = startLon; xIdx < endLon; ++xIdx) {
            for (int yIdx = startLat; yIdx < endLat; ++yIdx) {
                int fieldLon = xIdx / 10;
                int fieldLat = yIdx / 10;
                int sqLon = xIdx % 10;
                int sqLat = yIdx % 10;
                
                qreal x = xIdx * 20;
                qreal y = 1800 - (yIdx + 1) * 10;
                if (QRectF(x, y, 20, 10).intersects(rect)) {
                    QString label = QString("%1%2%3%4")
                                    .arg(QChar('A' + fieldLon))
                                    .arg(QChar('A' + fieldLat))
                                    .arg(sqLon)
                                    .arg(sqLat);
                    painter->drawText(QRectF(x, y, 20, 10), Qt::AlignCenter, label);
                }
            }
        }
    }
}

GridMapDialog::GridMapDialog(const GridQsoMap &gridQsos, const QList<QsoDetail> &potaQsos, int drawMode, const QString &awardName, const QString &customMapImage, bool isMaidenhead, QWidget *parent) : QDialog(parent)
{
    m_awardName = awardName;
    m_customMapImage = customMapImage;
    m_isMaidenhead = isMaidenhead;
    m_cacheBtn = nullptr;

    if (!m_awardName.isEmpty()) {
        setWindowTitle(tr("Award Map - %1").arg(m_awardName));
    } else if (drawMode == 1) {
        setWindowTitle(tr("Worked POTAs Map"));
    } else {
        setWindowTitle(tr("Worked Grids Map"));
    }
    resize(1000, 600);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *topLayout = new QHBoxLayout;
    layout->addLayout(topLayout);

    QPushButton *infoBtn = new QPushButton("Map Debug Info", this);
    topLayout->addWidget(infoBtn);
    topLayout->addStretch();

    connect(infoBtn, &QPushButton::clicked, this, [this]() {
        QString info;
        info += "Award Processing Source: " + (m_awardName.isEmpty() ? "Internal (All Grid QSOs)" : m_awardName) + "\n";
        info += "Shown As: " + QString(m_isMaidenhead || m_awardName.isEmpty() ? "Maidenhead Grids" : "Polygons/Points (Custom Map)") + "\n";
        
        QString mapPath;
        bool isBuiltin = false;
        
#if __has_include("world.png.h")
        isBuiltin = true;
#endif
        
        QSettings settings(QCoreApplication::organizationName(), "AdifMonitor");
        QString defaultMaidenhead = settings.value("Files/maidenheadImage", "").toString();
        
        if (!m_awardName.isEmpty()) {
            if (!m_customMapImage.isEmpty() && QFile::exists(m_customMapImage)) {
                mapPath = "Custom Image Override: " + m_customMapImage;
                isBuiltin = false;
            } else {
                QString fileBaseName = m_awardName;
                fileBaseName.remove(QRegularExpression("[^a-zA-Z0-9_]"));
                mapPath = "Cached Image Storage: " + getAppDataDir() + "/" + fileBaseName + "_map.png";
                isBuiltin = false;
            }
        } else {
            if (!defaultMaidenhead.isEmpty() && QFile::exists(defaultMaidenhead)) {
                mapPath = "User Override: " + defaultMaidenhead;
                isBuiltin = false;
            } else if (isBuiltin) {
                mapPath = "Built-in Compiled Source: world.png.h";
            } else {
                mapPath = "Downloaded Cache: " + getAppDataDir() + (m_mapStyleMode == 0 ? "/world_map.jpg" : "/world_map_simple.png");
            }
        }
        
        info += "Effective Base Map Resolution Path:\n" + mapPath + "\n";
        
        QMessageBox::information(this, "Map Debug Info", info);
    });

    m_cacheBtn = new QPushButton("Map Image Missing. Cache Now?");
    m_cacheBtn->hide();
    layout->addWidget(m_cacheBtn);

    m_view = new GridMapView(this);
    m_view->setGridQsos(gridQsos);
    m_view->setPotaQsos(potaQsos);
    if (!m_awardName.isEmpty()) {
        m_view->setDrawMode(m_isMaidenhead ? 0 : 2);
    } else {
        m_view->setDrawMode(drawMode);
    }
    layout->addWidget(m_view);

    QSettings settings(QCoreApplication::organizationName(), "AdifMonitor");
    QString prefix = drawMode == 1 ? "PotaMap" : "GridMap";
    if (settings.contains("Zoom/" + prefix + "Transform")) {
        m_view->setTransform(qvariant_cast<QTransform>(settings.value("Zoom/" + prefix + "Transform")));
    }
    QTimer::singleShot(0, this, [this, prefix]() {
        QSettings s(QCoreApplication::organizationName(), "AdifMonitor");
        if (s.contains("Zoom/" + prefix + "HScroll") && s.contains("Zoom/" + prefix + "VScroll")) {
            m_view->horizontalScrollBar()->setValue(s.value("Zoom/" + prefix + "HScroll").toInt());
            m_view->verticalScrollBar()->setValue(s.value("Zoom/" + prefix + "VScroll").toInt());
        } else {
            m_view->fitInView(QRectF(0, 0, 3600, 1800), Qt::KeepAspectRatio);
        }
    });

    m_netMan = new QNetworkAccessManager(this);
    connect(m_netMan, &QNetworkAccessManager::finished, this, &GridMapDialog::onMapDownloaded);

    QString appDataDir = getAppDataDir();
    QDir().mkpath(appDataDir);
    QString m_colorMapPath = appDataDir + "/world_map.jpg";
    QString m_simpleMapPath = appDataDir + "/world_map_simple.png";

#if __has_include("world.png.h")
    QImage builtinImage;
    if (builtinImage.loadFromData((const uchar*)WorldMapGrid::world_png, WorldMapGrid::world_png_len)) {
         m_colorMap = QPixmap::fromImage(builtinImage);
         m_simpleMap = QPixmap::fromImage(builtinImage);
    }
#else
    if (QFile::exists(m_colorMapPath)) {
        m_colorMap.load(m_colorMapPath);
    }
    if (QFile::exists(m_simpleMapPath)) {
        m_simpleMap.load(m_simpleMapPath);
    }
#endif
    
    int mapMode = settings.value("MapStyle", 0).toInt();
    m_mapStyleMode = mapMode;
    m_drawMode = drawMode;
    
    if (m_awardName == "TestMapBuiltin") {
        loadMapStyle(mapMode);
    } else if (!m_awardName.isEmpty()) {
        QString fileBaseName = m_awardName;
        fileBaseName.remove(QRegularExpression("[^a-zA-Z0-9_]"));
        QString cachedMap = appDataDir + "/" + fileBaseName + "_map.png";
        
        QPixmap customPix;
        if (QFile::exists(cachedMap)) {
            customPix.load(cachedMap);
        } else if (!m_customMapImage.isEmpty() && QFile::exists(m_customMapImage)) {
            customPix.load(m_customMapImage);
            customPix.save(cachedMap);
        }
        
        if (!customPix.isNull()) {
            m_view->setMapImage(customPix);
        } else {
            m_view->setMapImage(QPixmap());
            m_cacheBtn->show();
            connect(m_cacheBtn, &QPushButton::clicked, this, [this, cachedMap]() {
                QString chosenImage;
                if (m_isMaidenhead) {
                    QSettings s(QCoreApplication::organizationName(), "AdifMonitor");
                    s.beginGroup("Files");
                    chosenImage = s.value("maidenheadImage", "").toString();
                    s.endGroup();
                }
                
                if (chosenImage.isEmpty() || !QFile::exists(chosenImage)) {
                    chosenImage = QFileDialog::getOpenFileName(this, "Select Map Image to Cache", "", "Images (*.png *.jpg *.svg)");
                }
                
                if (!chosenImage.isEmpty()) {
                    QPixmap p(chosenImage);
                    if (!p.isNull()) {
                        p.save(cachedMap);
                        m_view->setMapImage(p);
                        m_cacheBtn->hide();
                    }
                }
            });
        }
    } else {
        QString defaultMaidenhead = settings.value("Files/maidenheadImage", "").toString();
        if (!defaultMaidenhead.isEmpty() && QFile::exists(defaultMaidenhead)) {
            QPixmap pix(defaultMaidenhead);
            if (!pix.isNull()) {
                m_view->setMapImage(pix);
            } else {
                loadMapStyle(mapMode);
            }
        } else {
            loadMapStyle(mapMode);
        }
    }
}

void GridMapDialog::closeEvent(QCloseEvent *event)
{
    QSettings settings(QCoreApplication::organizationName(), "AdifMonitor");
    QString prefix = m_drawMode == 1 ? "PotaMap" : "GridMap";
    settings.setValue("Zoom/" + prefix + "Transform", QVariant::fromValue(m_view->transform()));
    settings.setValue("Zoom/" + prefix + "HScroll", m_view->horizontalScrollBar()->value());
    settings.setValue("Zoom/" + prefix + "VScroll", m_view->verticalScrollBar()->value());
    QDialog::closeEvent(event);
}

void GridMapDialog::loadMapStyle(int mode)
{
    if (mode == 0) {
        if (!m_colorMap.isNull()) {
            m_view->setMapImage(m_colorMap);
        } else {
            m_netMan->get(QNetworkRequest(QUrl("https://upload.wikimedia.org/wikipedia/commons/8/83/Equirectangular_projection_SW.jpg")));
        }
    } else {
        if (!m_simpleMap.isNull()) {
            m_view->setMapImage(m_simpleMap);
        } else {
            m_netMan->get(QNetworkRequest(QUrl("https://upload.wikimedia.org/wikipedia/commons/thumb/c/c3/Equirectangular_projection_SW_-_white.jpg/1280px-Equirectangular_projection_SW_-_white.jpg")));
        }
    }
}

void GridMapDialog::onMapDownloaded(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() == QNetworkReply::NoError) {
        QPixmap pix;
        pix.loadFromData(reply->readAll());
        if (!pix.isNull()) {
            QString appDataDir = getAppDataDir();
            if (m_mapStyleMode == 0) {
                m_colorMap = pix;
                m_colorMap.save(appDataDir + "/world_map.jpg");
                m_view->setMapImage(m_colorMap);
            } else {
                m_simpleMap = pix;
                m_simpleMap.save(appDataDir + "/world_map_simple.png");
                m_view->setMapImage(m_simpleMap);
            }
        }
    }
}
