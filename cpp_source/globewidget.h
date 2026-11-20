// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef GLOBEWIDGET_H
#define GLOBEWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QTimer>
#include <QList>
#include <QString>
#include <QColor>
#include <QImage>

struct GlobeMarker {
    double lat;
    double lon;
    QString callsign;
    QColor color;
};

class QNetworkAccessManager;

class GlobeWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit GlobeWidget(QWidget *parent = nullptr);
    ~GlobeWidget();

    void setMarkers(const QList<GlobeMarker> &markers);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
    void updateAnimation();
    void endExplosion();
    void closeWarning();
    void onTextureDownloaded();
    void copyDebugInfo();

private:
    void drawSphere(double r, int lats, int longs);
    void downloadEarthTexture();
    
    QTimer *m_timer;
    QTimer *m_explosionTimer;

    double m_latRot;
    double m_lonRot;
    double m_zoomRadius;
    bool m_dragging;
    bool m_exploding;
    int m_explosionFrame;
    QWidget *m_warningWidget;
    
    QPoint m_lastMousePos;

    QList<GlobeMarker> m_markers;
    QNetworkAccessManager *m_netManager;
    
    QImage m_earthImage;
    QOpenGLTexture *m_texture;
    bool m_textureDirty;
};

#endif // GLOBEWIDGET_H

