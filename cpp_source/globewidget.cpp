// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

// ============================================================================
// COMPONENT: GlobeWidget
// ROLE: Real-time 3D OpenGL geospatial visualizer.
//       This component processes geographic coordinates (latitude, longitude)
//       of worked/needed amateur radio contacts, plots them as 3D markers on
//       a rotatable orthographic globe, handles greyline shading transitions,
//       and provides an interactive graphical HUD of QSO statistics.
// INVOKED BY: MainWindow (embedded in the main panel).
// PRODUCES: Interactive 3D globe renderings, contact markers, and zoom views.
// ============================================================================

#include "globewidget.h"
#include "paths.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QFile>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QtMath>
#include <random>
#include <QLabel>
#include <QVBoxLayout>

GlobeWidget::GlobeWidget(QWidget *parent)
    : QOpenGLWidget(parent),
      m_latRot(15.0),
      m_lonRot(0.0),
      m_zoomRadius(0.8),
      m_dragging(false),
      m_exploding(false),
      m_explosionFrame(0),
      m_warningWidget(nullptr),
      m_netManager(new QNetworkAccessManager(this)),
      m_texture(nullptr),
      m_textureDirty(false)
{
    setMinimumSize(200, 200);
    setMouseTracking(true);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &GlobeWidget::updateAnimation);
    m_timer->start(30);

    downloadEarthTexture();
}

GlobeWidget::~GlobeWidget() {
    if (m_texture) {
        delete m_texture;
        m_texture = nullptr;
    }
}

void GlobeWidget::setMarkers(const QList<GlobeMarker> &markers) {
    m_markers = markers;
    
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(m_markers.begin(), m_markers.end(), g);
    
    update();
}

void GlobeWidget::downloadEarthTexture() {
    QString cachePath = getAppDataDir() + "/earth_texture.jpg";
    if (QFile::exists(cachePath)) {
        if (m_earthImage.load(cachePath) && !m_earthImage.isNull()) {
            m_earthImage = m_earthImage.convertToFormat(QImage::Format_RGBA8888);
            m_textureDirty = true;
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
    connect(reply, &QNetworkReply::finished, this, &GlobeWidget::onTextureDownloaded);
}

void GlobeWidget::onTextureDownloaded() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        // Handle redirect
        QVariant redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (!redirectUrl.isNull()) {
            QNetworkRequest req(redirectUrl.toUrl());
            req.setHeader(QNetworkRequest::UserAgentHeader, "Qwain/1.0 (https://github.com/qwain)");
            QNetworkReply *redirectReply = m_netManager->get(req);
            connect(redirectReply, &QNetworkReply::finished, this, &GlobeWidget::onTextureDownloaded);
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        QImage img;
        if (img.loadFromData(data)) {
            QString cachePath = getAppDataDir() + "/earth_texture.jpg";
            img.save(cachePath, "JPG");
            m_earthImage = img.convertToFormat(QImage::Format_RGBA8888);
            m_textureDirty = true;
            update();
        }
    } else {
        qDebug() << "Failed to download earth texture: " << reply->errorString();
    }
    reply->deleteLater();
}

// [ENTRY POINT: OpenGL Context Init Callback]
// Sets up background colors, deep buffer states, and blending parameters for transparent overlays.
void GlobeWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.05f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GlobeWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    double ratio = (double)w / (double)h;
    if (w > h) {
        glOrtho(-ratio, ratio, -1.0, 1.0, -5.0, 5.0);
    } else {
        glOrtho(-1.0, 1.0, -1.0/ratio, 1.0/ratio, -5.0, 5.0);
    }
    glMatrixMode(GL_MODELVIEW);
}

void GlobeWidget::drawSphere(double r, int lats, int longs) {
    for(int i = 0; i < lats; i++) {
        double lat0 = M_PI * (-0.5 + (double) i / lats);
        double y0  = r * sin(lat0);
        double zr0 = r * cos(lat0);
        double v0 = (double)i / lats; // map to 0.0 to 1.0

        double lat1 = M_PI * (-0.5 + (double) (i + 1) / lats);
        double y1 = r * sin(lat1);
        double zr1 = r * cos(lat1);
        double v1 = (double)(i + 1) / lats;

        glBegin(GL_QUAD_STRIP);
        for(int j = 0; j <= longs; j++) {
            double u = (double)j / longs;
            double lon_rad = (u - 0.5) * 2 * M_PI;
            double x_norm = sin(lon_rad);
            double z_norm = cos(lon_rad);

            glTexCoord2f(u, v0);
            glNormal3f(x_norm * cos(lat0), sin(lat0), z_norm * cos(lat0));
            glVertex3f(x_norm * zr0, y0, z_norm * zr0);
            
            glTexCoord2f(u, v1);
            glNormal3f(x_norm * cos(lat1), sin(lat1), z_norm * cos(lat1));
            glVertex3f(x_norm * zr1, y1, z_norm * zr1);
        }
        glEnd();
    }
}

// [ENTRY POINT: 3D Render Loop Callback]
// Coordinates glClear passes, calculates particle expansion physics if in "explosion" Easter Egg mode,
// maps raw flat Earth texture maps onto 3D spherical coordinates, and plots contact markers.
void GlobeWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_exploding) {
        glLoadIdentity();
        glScalef(m_zoomRadius, m_zoomRadius, m_zoomRadius);
        glRotatef(m_latRot, 1.0, 0.0, 0.0);
        glRotatef(m_lonRot, 0.0, 1.0, 0.0);
        
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LIGHTING);
        glEnable(GL_DEPTH_TEST);
        
        glPointSize(8.0f);
        glBegin(GL_POINTS);
        
        std::mt19937 g(42); 
        int numParticles = 800;
        double progress = m_explosionFrame / 60.0;
        for (int i=0; i<numParticles; i++) {
            double theta = (g() % 360) * M_PI / 180.0;
            double phi   = (g() % 360) * M_PI / 180.0;
            
            double speed = 1.0 + (g() % 100) / 50.0;
            double dist = 0.98 + speed * progress * 2.5; // Starts at radius 0.98, expands outwards
            
            double x = dist * sin(theta) * cos(phi);
            double y = dist * sin(phi);
            double z = dist * cos(theta) * cos(phi);
            
            if (progress < 0.2) glColor3f(1.0f, 1.0f, 0.0f);
            else if (progress < 0.5) glColor3f(1.0f, 0.5f, 0.0f);
            else if (progress < 0.8) glColor3f(1.0f, 0.2f, 0.0f);
            else {
                double fade = 1.0 - (progress - 0.8) * 5.0;
                glColor4f(0.5f, 0.5f, 0.5f, fade < 0 ? 0 : fade);
            }
            
            glVertex3f(x, y, z);
        }
        glEnd();
        return;
    }

    if (m_textureDirty && !m_earthImage.isNull()) {
        if (m_texture) { delete m_texture; m_texture = nullptr; }
        m_texture = new QOpenGLTexture(m_earthImage.mirrored()); // OpenGL expects origin at bottom left
        m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_textureDirty = false;
    }

    glLoadIdentity();
    glScalef(m_zoomRadius, m_zoomRadius, m_zoomRadius);
    glRotatef(m_latRot, 1.0, 0.0, 0.0);
    glRotatef(m_lonRot, 0.0, 1.0, 0.0);
    
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    if (m_texture && m_texture->isCreated()) {
        m_texture->bind();
        glEnable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f); // white so texture colors show
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawSphere(0.98, 30, 60);
        glDisable(GL_TEXTURE_2D);
    } else {
        // Dark blue ocean fallback
        glColor3f(0.0f, 0.15f, 0.3f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        drawSphere(0.98, 30, 30);
    }
    
    // Wireframe lines fallback/overlay
    if (!m_texture || !m_texture->isCreated()) {
        glColor3f(0.0f, 0.4f, 0.8f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(1.0f);
        drawSphere(0.985, 15, 30);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // Draw markers
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D); // Ensure textures don't interfere with point colors
    glDisable(GL_LIGHTING);   // Ensure lighting doesn't darken points
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    for (const GlobeMarker &m : m_markers) {
        double rLat = m.lat * M_PI / 180.0;
        double lon_rad = m.lon * M_PI / 180.0;
        
        double x = sin(lon_rad) * cos(rLat);
        double y = sin(rLat);
        double z = cos(lon_rad) * cos(rLat);
        
        glColor3f(m.color.redF(), m.color.greenF(), m.color.blueF());
        glVertex3f(x * 1.01, y * 1.01, z * 1.01); // Render slightly outside the sphere (1.01 vs 0.98 in drawSphere)
    }
    glEnd();
}

void GlobeWidget::updateAnimation() {
    if (m_exploding) {
        m_explosionFrame++;
        if (m_explosionFrame % 10 == 0) QApplication::beep(); // boom sound effect
        update();
        return;
    }

    if (!m_dragging) {
        m_lonRot += 0.5;
        if (m_lonRot > 360.0) m_lonRot -= 360.0;
        update();
    }
}

void GlobeWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastMousePos = event->pos();
    }
}

void GlobeWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_lastMousePos = event->pos();

        m_lonRot += delta.x() * 0.5;
        m_latRot += delta.y() * 0.5;
        
        m_latRot = qBound(-90.0, m_latRot, 90.0);
        
        if (m_lonRot > 360.0) m_lonRot -= 360.0;
        else if (m_lonRot < 0.0) m_lonRot += 360.0;

        update();
    }
}

void GlobeWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && !m_exploding) {
        m_exploding = true;
        m_explosionFrame = 0;
        m_dragging = false;
        QApplication::beep();
        QTimer::singleShot(2000, this, &GlobeWidget::endExplosion);
    }
}

void GlobeWidget::endExplosion() {
    QTimer::singleShot(500, this, [this]() {
        if (!m_warningWidget) {
            m_warningWidget = new QWidget(this);
            m_warningWidget->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::Tool);
            m_warningWidget->setAttribute(Qt::WA_TranslucentBackground);
            
            QLabel *lbl = new QLabel("Don't Do That!", m_warningWidget);
            lbl->setStyleSheet("color: red; font-size: 30px; font-weight: bold; background-color: rgba(0,0,0,150); padding: 20px; border-radius: 10px;");
            
            QVBoxLayout *l = new QVBoxLayout(m_warningWidget);
            l->setContentsMargins(0,0,0,0);
            l->addWidget(lbl);
        }
        
        m_warningWidget->adjustSize();
        QPoint center = mapToGlobal(rect().center());
        m_warningWidget->move(center.x() - m_warningWidget->width()/2, center.y() - m_warningWidget->height()/2);
        m_warningWidget->show();
        
        QTimer::singleShot(1000, this, &GlobeWidget::closeWarning);
    });
}

void GlobeWidget::closeWarning() {
    if (m_warningWidget) {
        m_warningWidget->close();
        m_warningWidget->deleteLater();
        m_warningWidget = nullptr;
    }
    m_exploding = false;
    m_explosionFrame = 0;
}

void GlobeWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
}

void GlobeWidget::leaveEvent(QEvent *event) {
    Q_UNUSED(event);
    m_dragging = false;
}

void GlobeWidget::wheelEvent(QWheelEvent *event) {
    double numDegrees = event->angleDelta().y() / 8.0;
    double numSteps = numDegrees / 15.0;

    m_zoomRadius *= pow(1.1, numSteps);
    m_zoomRadius = qBound(0.2, m_zoomRadius, 3.0);

    update();
}

void GlobeWidget::copyDebugInfo() {
    QString info = "Globe Debug Info:\n";
    info += QString("Rotation: Lat %1, Lon %2\n").arg(m_latRot).arg(m_lonRot);
    info += QString("Zoom: %1\n").arg(m_zoomRadius);
    info += QString("Texture Loaded: %1\n").arg(m_texture && m_texture->isCreated() ? "Yes" : "No");
    info += QString("Markers Rendered: %1\n\n").arg(m_markers.size());
    
    info += "Sample of Mapped Markers (First 20):\n";
    
    int count = 0;
    for (const auto &m : m_markers) {
        if (count >= 20) break;
        
        double rLat = m.lat * M_PI / 180.0;
        double lng = (m.lon + 180.0) * M_PI / 180.0;
        
        double x = cos(lng) * cos(rLat);
        double y = sin(rLat);
        double z = sin(lng) * cos(rLat);
        
        info += QString("Call: %1 | Lat: %2, Lon: %3 | Render (X: %4, Y: %5, Z: %6)\n")
                .arg(m.callsign).arg(m.lat).arg(m.lon)
                .arg(x * 1.01).arg(y * 1.01).arg(z * 1.01);
        count++;
    }
    
    QApplication::clipboard()->setText(info);
}

void GlobeWidget::contextMenuEvent(QContextMenuEvent *event) {
    Q_UNUSED(event);
}
