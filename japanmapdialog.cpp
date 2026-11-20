#include <QCoreApplication>
#include "paths.h"
#include "japanmapdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTableWidget>
#include <QHeaderView>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

#if __has_include("world.png.h")
namespace WorldMapJapan {
#include "world.png.h"
}
#endif

#include <QLabel>
#include <QSettings>
#include <QTimer>
#include <QScrollBar>

JapanMapDialog::JapanMapDialog(const GridQsoMap &gridQsos, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Worked All Japan"));
    resize(1200, 700);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    
    // Table (WAJA requirements)
    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"Pref#", "Prefecture", "Status", "Callsign"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    populateTable(gridQsos);
    
    // Map View
    m_view = new GridMapView(this);
    // Filter gridQsos for JA
    GridQsoMap jaQsos;
    for (auto gridIt = gridQsos.begin(); gridIt != gridQsos.end(); ++gridIt) {
        for (auto callIt = gridIt.value().begin(); callIt != gridIt.value().end(); ++callIt) {
            for (const QsoDetail &d : callIt.value()) {
                if (d.country == "JAPAN" || d.dxcc == "339" || callIt.key().startsWith("JA")) {
                    jaQsos[gridIt.key()][callIt.key()].append(d);
                }
            }
        }
    }
    m_view->setGridQsos(jaQsos);
    
    // Set view scene rect to focus on Japan
    m_view->setSceneRect(0, 0, 3600, 1800);
    
    QSettings settings(QCoreApplication::organizationName(), "AdifMonitor");
    if (settings.contains("Zoom/JapanMapTransform")) {
        m_view->setTransform(qvariant_cast<QTransform>(settings.value("Zoom/JapanMapTransform")));
        QTimer::singleShot(0, this, [this]() {
            QSettings s(QCoreApplication::organizationName(), "AdifMonitor");
            if (s.contains("Zoom/JapanMapHScroll") && s.contains("Zoom/JapanMapVScroll")) {
                m_view->horizontalScrollBar()->setValue(s.value("Zoom/JapanMapHScroll").toInt());
                m_view->verticalScrollBar()->setValue(s.value("Zoom/JapanMapVScroll").toInt());
            }
        });
    } else {
        m_view->fitInView(QRectF(3020, 440, 260, 220), Qt::KeepAspectRatio);
    }

    m_netMan = new QNetworkAccessManager(this);
    connect(m_netMan, &QNetworkAccessManager::finished, this, &JapanMapDialog::onMapDownloaded);

    QString colorMapPath = getAppDataDir() + "/world_map.jpg";
    QString simpleMapPath = getAppDataDir() + "/world_map_simple.png";

#if __has_include("world.png.h")
    QImage builtinImage;
    if (builtinImage.loadFromData((const uchar*)WorldMapJapan::world_png, WorldMapJapan::world_png_len)) {
         m_colorMap = QPixmap::fromImage(builtinImage);
         m_simpleMap = QPixmap::fromImage(builtinImage);
    }
#else
    if (QFile::exists(colorMapPath)) {
        m_colorMap.load(colorMapPath);
    }
    if (QFile::exists(simpleMapPath)) {
        m_simpleMap.load(simpleMapPath);
    }
#endif
    
    int mapMode = settings.value("MapStyle", 0).toInt();
    m_mapStyleMode = mapMode;
    loadMapStyle(mapMode);

    splitter->addWidget(m_table);
    splitter->addWidget(m_view);
    
    QList<int> sizes;
    sizes << 400 << 800;
    splitter->setSizes(sizes);
    
    layout->addWidget(splitter);
}

void JapanMapDialog::closeEvent(QCloseEvent *event)
{
    QSettings settings(QCoreApplication::organizationName(), "AdifMonitor");
    settings.setValue("Zoom/JapanMapTransform", QVariant::fromValue(m_view->transform()));
    settings.setValue("Zoom/JapanMapHScroll", m_view->horizontalScrollBar()->value());
    settings.setValue("Zoom/JapanMapVScroll", m_view->verticalScrollBar()->value());
    QDialog::closeEvent(event);
}

void JapanMapDialog::populateTable(const GridQsoMap &gridQsos)
{
    QStringList prefNames = {
        "Hokkaido", "Aomori", "Iwate", "Miyagi", "Akita",
        "Yamagata", "Fukushima", "Ibaraki", "Tochigi", "Gunma",
        "Saitama", "Chiba", "Tokyo", "Kanagawa", "Niigata",
        "Toyama", "Ishikawa", "Fukui", "Yamanashi", "Nagano",
        "Gifu", "Shizuoka", "Aichi", "Mie", "Shiga",
        "Kyoto", "Osaka", "Hyogo", "Nara", "Wakayama",
        "Tottori", "Shimane", "Okayama", "Hiroshima", "Yamaguchi",
        "Tokushima", "Kagawa", "Ehime", "Kochi", "Fukuoka",
        "Saga", "Nagasaki", "Kumamoto", "Oita", "Miyazaki",
        "Kagoshima", "Okinawa"
    };

    QMap<int, QString> workedPrefectures;

    // Scan for worked prefectures
    for (auto gridIt = gridQsos.begin(); gridIt != gridQsos.end(); ++gridIt) {
        for (auto callIt = gridIt.value().begin(); callIt != gridIt.value().end(); ++callIt) {
            for (const QsoDetail &d : callIt.value()) {
                if (d.country == "JAPAN" || d.dxcc == "339" || callIt.key().startsWith("J")) {
                    bool ok;
                    int prefNum = d.state.toInt(&ok);
                    if (ok && prefNum >= 1 && prefNum <= 47) {
                        workedPrefectures[prefNum] = callIt.key();
                    } else {
                        // Some alternative formats might provide full name or abbreviations
                        for (int i = 0; i < prefNames.size(); ++i) {
                            if (d.state.compare(prefNames[i], Qt::CaseInsensitive) == 0) {
                                workedPrefectures[i+1] = callIt.key();
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    m_table->setRowCount(47);
    int workedCount = 0;

    for (int i = 0; i < 47; ++i) {
        int prefNum = i + 1;
        
        QTableWidgetItem *numItem = new QTableWidgetItem(QString("%1").arg(prefNum, 2, 10, QChar('0')));
        numItem->setTextAlignment(Qt::AlignCenter);
        
        QTableWidgetItem *nameItem = new QTableWidgetItem(prefNames[i]);
        
        QTableWidgetItem *statusItem = new QTableWidgetItem();
        statusItem->setTextAlignment(Qt::AlignCenter);
        
        QTableWidgetItem *callItem = new QTableWidgetItem();
        
        if (workedPrefectures.contains(prefNum)) {
            statusItem->setText("WORKED");
            statusItem->setForeground(QBrush(Qt::darkGreen));
            callItem->setText(workedPrefectures[prefNum]);
            workedCount++;
        } else {
            statusItem->setText("NEEDED");
            statusItem->setForeground(QBrush(Qt::red));
        }
        
        m_table->setItem(i, 0, numItem);
        m_table->setItem(i, 1, nameItem);
        m_table->setItem(i, 2, statusItem);
        m_table->setItem(i, 3, callItem);
    }
    m_table->resizeColumnsToContents();
    
    // Add summary to title
    setWindowTitle(QString("Worked All Japan - %1 of 47 Prefectures Worked").arg(workedCount));
}

void JapanMapDialog::loadMapStyle(int mode)
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

void JapanMapDialog::onMapDownloaded(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() == QNetworkReply::NoError) {
        QPixmap pix;
        pix.loadFromData(reply->readAll());
        if (!pix.isNull()) {
            if (m_mapStyleMode == 0) {
                m_colorMap = pix;
                m_colorMap.save(getAppDataDir() + "/world_map.jpg");
                m_view->setMapImage(m_colorMap);
            } else {
                m_simpleMap = pix;
                m_simpleMap.save(getAppDataDir() + "/world_map_simple.png");
                m_view->setMapImage(m_simpleMap);
            }
        }
    }
}
