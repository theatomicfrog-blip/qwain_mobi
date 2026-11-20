// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef DXCLUSTERWIDGET_H
#define DXCLUSTERWIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QListWidget>
#include <QTimer>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>

struct DxSpot {
    QString spooter;
    QString spotted;
    QString frequency;
    QString mode_comment;
    QString time;
    QString country;
    QImage flagImage;
};

class DxClusterWidget : public QWidget {
    Q_OBJECT
public:
    explicit DxClusterWidget(QWidget *parent = nullptr);
    ~DxClusterWidget();

    void startClustering();
    void stopClustering();

signals:
    void spotClicked(const QString &callsign);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onReadyRead();
    void onConnected();
    void onError(QAbstractSocket::SocketError error);
    void connectToServer();
    void animateScroll();

private:
    void processLine(const QString &line);
    void addSpot(const DxSpot &spot);
    void fetchFlagForSpot(int index);

    QTcpSocket *m_socket;
    QListWidget *m_listWidget;
    QList<DxSpot> m_spots;
    QString m_serverName;
    QString m_buffer;
    QTimer *m_reconnectTimer;
    QNetworkAccessManager *m_netManager;
    
    // For smooth scrolling animation
    double m_scrollOffset;
    QTimer *m_animationTimer;
};

#endif // DXCLUSTERWIDGET_H
