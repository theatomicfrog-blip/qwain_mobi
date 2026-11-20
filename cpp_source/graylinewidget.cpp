// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

// ============================================================================
// COMPONENT: GraylineWidget
// ROLE: Real-time 2D equirectangular solar terminator (grayline) projection.
//       Calculates astronomical solar declination/equation-of-time to map
//       the daylight terminator over a flat Earth texture, centering on the
//       user's configured Maidenhead grid coordinates.
// INVOKED BY: MainWindow (via "View Grayline Map" action/dialog).
// PRODUCES: Custom 2D equirectangular map with dynamic day/night shading overlays.
// ============================================================================

#include "graylinewidget.h"
#include "paths.h"
#include <QPainter>
#include <QDateTime>
#include <QSettings>
#include <QCoreApplication>
#include <qmath.h>
#include <QDebug>
#include <QFile>

// [ENTRY POINT: Grayline Widget Constructor]
// Initializes layout sizes, reads cached textures, and schedules a 60s periodic shadow update loop.
GraylineWidget::GraylineWidget(QWidget *parent)
    : QWidget(parent), m_updateTimer(new QTimer(this)), m_netManager(new QNetworkAccessManager(this))
{
    setWindowTitle("Grayline Map");
    resize(401, 167);
    
    // Load earth map
    loadEarthMap();
    
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        updateShadow();
        update();
    });
    // Update every minute
    m_updateTimer->start(60000);
}

GraylineWidget::~GraylineWidget() {}

// [ENTRY POINT: Texture Asset Loader & Cache Manager]
// Attempts to load a cached JPEG map of the Earth. Falls back to an asynchronous
// HTTP fetch with redirect handling, persisting the downloaded texture back to disk.
void GraylineWidget::loadEarthMap() {
    QString cachePath = getAppDataDir() + "/earth_texture.jpg";
    if (QFile::exists(cachePath)) {
        if (m_earthMap.load(cachePath) && !m_earthMap.isNull()) {
            qDebug() << "Grayline map loaded from cache.";
            updateShadow();
            update();
            return;
        } else {
            QFile::remove(cachePath);
        }
    }

    // Download small map
    QUrl url("https://raw.githubusercontent.com/mrdoob/three.js/master/examples/textures/planets/earth_atmos_2048.jpg");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "Qwain/1.0 (https://github.com/qwain)");
    QNetworkReply *reply = m_netManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            // Handle redirect
            QVariant redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
            if (!redirectUrl.isNull()) {
                QNetworkRequest req(redirectUrl.toUrl());
                req.setHeader(QNetworkRequest::UserAgentHeader, "Qwain/1.0 (https://github.com/qwain)");
                QNetworkReply *redirectReply = m_netManager->get(req);
                connect(redirectReply, &QNetworkReply::finished, this, [this, redirectReply]() {
                    if (redirectReply->error() == QNetworkReply::NoError) {
                        QByteArray data = redirectReply->readAll();
                        if (m_earthMap.loadFromData(data)) {
                            QString cachePath = getAppDataDir() + "/earth_texture.jpg";
                            m_earthMap.save(cachePath, "JPG");
                            updateShadow();
                            update();
                        }
                    }
                    redirectReply->deleteLater();
                });
                reply->deleteLater();
                return;
            }

            QByteArray data = reply->readAll();
            if (m_earthMap.loadFromData(data)) {
                QString cachePath = getAppDataDir() + "/earth_texture.jpg";
                m_earthMap.save(cachePath, "JPG");
                updateShadow();
                update();
            }
        }
        reply->deleteLater();
    });
}


void GraylineWidget::showEvent(QShowEvent *event) {
    updateShadow();
    QWidget::showEvent(event);
}

// [ENTRY POINT: Astronomical Solar Position Solver]
// Approximates the current sub-solar latitude and longitude using standard Julian/orbital
// orbital mechanics equations, including Equation of Time (EoT) correction.
void GraylineWidget::calculateSunPosition(double &subSolarLat, double &subSolarLon) {
    QDateTime now = QDateTime::currentDateTimeUtc();
    int days = now.date().dayOfYear();
    double timeOfDay = now.time().hour() + now.time().minute() / 60.0 + now.time().second() / 3600.0;
    
    // Simple declination (sub-solar latitude) approximation
    // Dec = 23.44 * sin(360/365 * (d - 81))
    double angle = 2.0 * M_PI * (days - 81) / 365.0;
    subSolarLat = 23.44 * qSin(angle);
    
    // Equation of time and sub-solar longitude
    // B = 360/365 * (d - 81)
    double B = 2.0 * M_PI * (days - 81) / 365.0;
    double EoT = 9.87 * qSin(2.0 * B) - 7.53 * qCos(B) - 1.5 * qSin(B);
    
    double trueSolarTime = timeOfDay + EoT / 60.0;
    subSolarLon = (12.0 - trueSolarTime) * 15.0;
    if (subSolarLon < -180.0) subSolarLon += 360.0;
    if (subSolarLon > 180.0) subSolarLon -= 360.0;
}

// [ENTRY POINT: Terminator Projection Rasterizer]
// Computes daylight/twilight/night boundaries across the image raster by calculating the 
// spherical zenith angle between each latitude/longitude point and the sub-solar coordinates.
void GraylineWidget::updateShadow() {
    if (m_earthMap.isNull()) return;
    
    int w = m_earthMap.width();
    int h = m_earthMap.height();
    m_shadowLayer = QImage(w, h, QImage::Format_ARGB32);
    m_shadowLayer.fill(Qt::transparent);
    
    double sunLat, sunLon;
    calculateSunPosition(sunLat, sunLon);
    
    double sunLatRad = sunLat * M_PI / 180.0;
    double sunLonRad = sunLon * M_PI / 180.0;
    
    for (int y = 0; y < h; ++y) {
        double latRad = (90.0 - (180.0 * y / (double)h)) * M_PI / 180.0;
        QRgb* row = (QRgb*)m_shadowLayer.scanLine(y);
        for (int x = 0; x < w; ++x) {
            double lonRad = ((360.0 * x / (double)w) - 180.0) * M_PI / 180.0;
            
            // Zenith angle to sun
            // cos(Zenith) = sin(lat)*sin(sunLat) + cos(lat)*cos(sunLat)*cos(lon - sunLon)
            double cosZenith = qSin(latRad) * qSin(sunLatRad) + 
                               qCos(latRad) * qCos(sunLatRad) * qCos(lonRad - sunLonRad);
            
            if (cosZenith < -0.1) {
                // night
                row[x] = qRgba(0, 0, 0, 140);
            } else if (cosZenith < 0.1) {
                // twilight (fade 0 to 140)
                double factor = (0.1 - cosZenith) / 0.2; // 0 to 1
                row[x] = qRgba(0, 0, 0, (int)(140 * factor));
            } else {
                // day
                row[x] = qRgba(0, 0, 0, 0);
            }
        }
    }
}

// [ENTRY POINT: Equirectangular Projection Renderer]
// Parses the user's home Maidenhead grid location to shift/center the 2D map projection,
// wraps the Earth map texture horizontally, and renders the dynamic twilight/night shade overlays.
void GraylineWidget::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    
    if (m_earthMap.isNull()) {
        p.fillRect(rect(), Qt::black);
        return;
    }
    
    // Center the map on grid location entered in the configuration
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString myGrid = settings.value("Credentials/myGrid", "").toString();
    
    double myLat = 0.0;
    double myLon = 0.0;
    bool hasGrid = false;
    
    // Parse maidenhead grid to lat/lon
    if (myGrid.length() >= 4) {
        myGrid = myGrid.toUpper();
        myLon = (myGrid[0].unicode() - 'A') * 20.0 - 180.0 + (myGrid[2].unicode() - '0') * 2.0 + 1.0;
        myLat = (myGrid[1].unicode() - 'A') * 10.0 - 90.0 + (myGrid[3].unicode() - '0') * 1.0 + 0.5;
        hasGrid = true;
    }
    
    int mapW = m_earthMap.width();
    int mapH = m_earthMap.height();
    
    // User requested "Center the map on the grid location"
    // Calculate the pixel coordinate of myLat/myLon on the earthMap
    int centerPixelX = (int)((myLon + 180.0) / 360.0 * mapW);
    int centerPixelY = (int)((90.0 - myLat) / 180.0 * mapH);
    
    if (!hasGrid) {
        centerPixelX = mapW / 2;
        centerPixelY = mapH / 2;
    }
    
    int drawW = width();
    int drawH = height();
    
    // To center, we calculate the offset. Map will wrap horizontally.
    // The scale should keep the map flat and geographically accurate.
    // The user requested window size 401x167. So we map the whole world or scale it?
    // "Open a new window sized 401x167 pixels showing a clean equirectangular projected Earth map. Center the map on the grid location... Keep the map flat, rectangular, geographically accurate"
    // If the window is 401x167 and the whole world is shown, does it mean we scale to fit, but it might not be 401x167 aspect ratio exactly. Wait, earth is 2:1, so 401x200? Or maybe we just draw the scaled earth mapping over wrapping boundaries.
    
    // We want the earthmap scaled to fit height:
    double scale = (double)drawH / (double)mapH; // or maybe they want 1:1, but the window size is 401x167
    
    // Let's scale map to 401 wide? 401 is vaguely 2x167 width (334).
    // Let's scale and wrap continuously.
    // draw full scaled map, wrapping horizontally, and centered at centerPixelX, centerPixelY
    
    double scaledW = mapW * scale;
    double scaledH = mapH * scale; // this will be exactly drawH
    
    // We want centerPixelX to be at drawW / 2
    // We want centerPixelY to be at drawH / 2
    int scaledCenterX = centerPixelX * scale;
    int scaledCenterY = centerPixelY * scale;
    
    int ox = drawW / 2 - scaledCenterX;
    int oy = drawH / 2 - scaledCenterY;
    
    // Wrap around horizontally
    while (ox > 0) ox -= scaledW;
    
    for (int cx = ox; cx < drawW; cx += scaledW) {
        p.drawImage(QRect(cx, oy, scaledW, scaledH), m_earthMap);
        p.drawImage(QRect(cx, oy, scaledW, scaledH), m_shadowLayer);
    }
    
    // Overlay a thin gray reference line exactly through that center point
    p.setPen(QPen(Qt::lightGray, 1));
    p.drawLine(drawW / 2, 0, drawW / 2, drawH);
    p.drawLine(0, drawH / 2, drawW, drawH / 2);
}
