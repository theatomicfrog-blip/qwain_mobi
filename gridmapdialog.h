#ifndef GRIDMAPDIALOG_H
#define GRIDMAPDIALOG_H

#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QSet>
#include <QMap>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QLabel>
#include <QList>

struct QsoDetail {
    QString date;
    QString time;
    QString freq;
    QString band;
    QString mode;
    QString country;
    QString state;
    QString dxcc;
    QString call;
    QString pota;
    QString grid;
    QString lat;
    QString lon;
    QString rst_rcvd;
    QString rst_sent;
    QString tx_pwr;
    QString name;
    QString address;
    QString city;
    QString zip;
    QString county;
};

typedef QMap<QString, QMap<QString, QList<QsoDetail>>> GridQsoMap;

class GridMapView : public QGraphicsView
{
    Q_OBJECT
public:
    GridMapView(QWidget *parent = nullptr);
    void setMapImage(const QPixmap &pixmap);
    void setGridQsos(const GridQsoMap &gridQsos);
    void setPotaQsos(const QList<QsoDetail> &potaQsos);
    void setDrawMode(int mode); // 0 = default, 1 = POTA (pine trees)

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onGridCallsignClicked(const QString &callsign);

private:
    QRectF getGridRect(const QString &gridCode);
    QString formatDateString(const QString &dateStr);
    QString formatTimeString(const QString &timeStr);

    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_mapItem;
    GridQsoMap m_gridQsos;
    QList<QsoDetail> m_potaQsos;
    
    QPointF getLatLonPoint(const QString &latStr, const QString &lonStr);
    
    QLabel *m_gridTooltip;
    QLabel *m_callTooltip;
    QString m_currentHoveredGrid;
    int m_drawMode;
};

class GridMapDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GridMapDialog(const GridQsoMap &gridQsos, const QList<QsoDetail> &potaQsos = QList<QsoDetail>(), int drawMode = 0, const QString &awardName = "", const QString &customMapImage = "", bool isMaidenhead = false, QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onMapDownloaded(QNetworkReply *reply);

private:
    void loadMapStyle(int mode);

    GridMapView *m_view;
    QNetworkAccessManager *m_netMan;
    QPixmap m_colorMap;
    QPixmap m_simpleMap;
    int m_mapStyleMode;
    int m_drawMode;
    QString m_awardName;
    QString m_customMapImage;
    bool m_isMaidenhead;
    QPushButton *m_cacheBtn;
};

#endif // GRIDMAPDIALOG_H
