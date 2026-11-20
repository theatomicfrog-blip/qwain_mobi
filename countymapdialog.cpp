#include <QCoreApplication>
#include "paths.h"
#include "countymapdialog.h"
#include "fips_data.h"
#include <QVBoxLayout>
#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QFile>
#include <QWheelEvent>
#include <QtMath>
#include <QRegularExpression>
#include <QCryptographicHash>

#include <QSettings>
#include <QScrollBar>
#include <QTimer>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>

CountyMapDialog::CountyMapDialog(const QSet<QString> &workedCounties, QWidget *parent)
    : QDialog(parent), m_workedCounties(workedCounties)
{
    setWindowTitle(tr("Worked US Counties Map"));
    resize(1000, 600);

    QVBoxLayout *layout = new QVBoxLayout(this);
    m_view = new QGraphicsView(this);
    m_scene = new QGraphicsScene(this);
    m_view->setScene(m_scene);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    
    // Install event filter for zooming and tooltips
    m_view->viewport()->installEventFilter(this);
    m_view->installEventFilter(this);
    m_view->viewport()->setMouseTracking(true);
    m_view->setMouseTracking(true);
    
    m_netMan = new QNetworkAccessManager(this);
    
    layout->addWidget(m_view);

    QSettings settings(QCoreApplication::organizationName(), "AdifMonitor");
    if (settings.contains("Zoom/CountyMapTransform")) {
        m_view->setTransform(qvariant_cast<QTransform>(settings.value("Zoom/CountyMapTransform")));
    }
    QTimer::singleShot(0, this, [this]() {
        QSettings s(QCoreApplication::organizationName(), "AdifMonitor");
        if (s.contains("Zoom/CountyMapHScroll") && s.contains("Zoom/CountyMapVScroll")) {
            m_view->horizontalScrollBar()->setValue(s.value("Zoom/CountyMapHScroll").toInt());
            m_view->verticalScrollBar()->setValue(s.value("Zoom/CountyMapVScroll").toInt());
        }
    });

    loadMap();
}

void CountyMapDialog::closeEvent(QCloseEvent *event)
{
    QSettings settings(QCoreApplication::organizationName(), "AdifMonitor");
    settings.setValue("Zoom/CountyMapTransform", QVariant::fromValue(m_view->transform()));
    settings.setValue("Zoom/CountyMapHScroll", m_view->horizontalScrollBar()->value());
    settings.setValue("Zoom/CountyMapVScroll", m_view->verticalScrollBar()->value());
    QDialog::closeEvent(event);
}

void CountyMapDialog::loadMap()
{
    QStringList cs;
    for (const QString& c : m_workedCounties) cs << c;
    cs.sort();
    
    QSettings settings(QCoreApplication::organizationName(), "AdifMonitor");
    settings.beginGroup("Files");
    QString logfilePath = settings.value("logfile", "").toString();
    settings.endGroup();
    QString logDir;
    if (logfilePath.isEmpty()) {
        logDir = getAppDataDir();
    } else {
        logDir = QFileInfo(logfilePath).absolutePath();
    }
    QDir().mkpath(logDir);
    
    QByteArray hashData = QCryptographicHash::hash(cs.join(",").toUtf8(), QCryptographicHash::Md5);
    QString cacheKey = QString::number(m_workedCounties.size()) + "_" + QString(hashData.toHex());
    QString cachePath = QDir(logDir).filePath("countymap_" + cacheKey + ".png");

    if (QFile::exists(cachePath)) {
        QPixmap pm(cachePath);
        QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pm);
        m_scene->addItem(item);
        m_scene->setSceneRect(item->boundingRect());
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
        return;
    }

    QString svgFilePath = QDir(logDir).filePath("USA_Counties_with_FIPS_and_names.svg");
    if (!QFile::exists(svgFilePath)) {
        // Download it
        QNetworkRequest req(QUrl("https://upload.wikimedia.org/wikipedia/commons/5/5f/USA_Counties_with_FIPS_and_names.svg"));
        req.setHeader(QNetworkRequest::UserAgentHeader, "AdifMonitor/8.2");
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply *reply = m_netMan->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, svgFilePath]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                if (data.contains("<svg")) {
                    QFile f(svgFilePath);
                    if (f.open(QIODevice::WriteOnly)) {
                        f.write(data);
                        f.close();
                    }
                    loadMap();
                }
            }
            reply->deleteLater();
        });
        return;
    }

    QString svgStr;
    QFile resourceFile(svgFilePath);
    if (resourceFile.open(QIODevice::ReadOnly)) {
        svgStr = resourceFile.readAll();
        resourceFile.close();
    }
    
    // Validate if the file is an actual SVG file (e.g. not a 403 Forbidden HTML from Wikimedia)
    if (svgStr.isEmpty() || !svgStr.contains("<svg", Qt::CaseInsensitive)) {
        QFile::remove(svgFilePath);
        loadMap();
        return;
    }

    QMap<QString, QString> fipsMap = getFipsMap();
    QStringList fipsKeys = fipsMap.keys();
    
    for (const QString &county : m_workedCounties) {
        QString matchedCid;
        if (fipsMap.contains(county)) {
            matchedCid = fipsMap[county];
        } else {
            // Fuzzy match for Alaska, Louisiana, Virginia etc.
            QString cleanAdif = county;
            cleanAdif.remove(" COUNTY").remove(" PARISH").remove(" BOROUGH").remove(" CENSUS AREA")
                     .remove(" MUNICIPALITY").remove(" CITY AND BOROUGH").remove(" (CITY)");
            
            for (const QString &fk : fipsKeys) {
                QString cleanFk = fk;
                cleanFk.remove(" COUNTY").remove(" PARISH").remove(" BOROUGH").remove(" CENSUS AREA")
                       .remove(" MUNICIPALITY").remove(" CITY AND BOROUGH").remove(" (CITY)");
                if (cleanFk == cleanAdif || cleanFk.endsWith("," + cleanAdif)) {
                    matchedCid = fipsMap[fk];
                    break;
                }
            }
        }

        if (!matchedCid.isEmpty()) {
            QString numCid = matchedCid;
            if (numCid.startsWith("c")) numCid = numCid.mid(1);
            
            QRegularExpression reId(QString("<(?:path|polygon|g)[^>]*?id=\"(?:c|FIPS_)?%1\"[^>]*>").arg(numCid), QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = reId.match(svgStr);
            if (match.hasMatch()) {
                QString fullTag = match.captured(0);
                QString modifiedTag = fullTag;
                if (modifiedTag.contains("style=\"")) {
                    if (modifiedTag.contains("fill:")) {
                        modifiedTag.replace(QRegularExpression("fill:\\s*#[0-9a-fA-F]{3,6}"), "fill:#ff8080");
                        modifiedTag.replace(QRegularExpression("fill:\\s*(none|url\\([^)]+\\)[^;]*|[a-zA-Z]+)"), "fill:#ff8080");
                    } else {
                        modifiedTag.replace("style=\"", "style=\"fill:#ff8080;");
                    }
                } else {
                    if (modifiedTag.endsWith("/>")) {
                        modifiedTag.replace("/>", " style=\"fill:#ff8080\"/>");
                    } else {
                        modifiedTag.replace(">", " style=\"fill:#ff8080\">");
                    }
                }
                svgStr.replace(fullTag, modifiedTag);
            }
        }
    }

    m_renderer = new QSvgRenderer(svgStr.toUtf8(), this);
    
    QSize s = m_renderer->defaultSize();
    s.scale(4000, 4000, Qt::KeepAspectRatio);
    QImage image(s, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter p(&image);
    m_renderer->render(&p);
    p.end();

    QPixmap pm = QPixmap::fromImage(image);
    pm.save(cachePath);

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(pm);
    m_scene->addItem(item);
    m_scene->setSceneRect(item->boundingRect());
    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void CountyMapDialog::onMapDownloaded(QNetworkReply *)
{
    // Deprecated
}

#include <QToolTip>

bool CountyMapDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        qreal angle = wheelEvent->angleDelta().y();
#else
        qreal angle = wheelEvent->angleDelta().y();
#endif
        qreal zoomFactor = qPow(1.15, angle / 120.0);
        m_view->scale(zoomFactor, zoomFactor);
        return true;
    }
    return QDialog::eventFilter(watched, event);
}
