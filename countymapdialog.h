#ifndef COUNTYMAPDIALOG_H
#define COUNTYMAPDIALOG_H

#include <QDialog>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QSet>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSvgRenderer>

class CountyMapDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CountyMapDialog(const QSet<QString> &workedCounties, QWidget *parent = nullptr);

private slots:
    void onMapDownloaded(QNetworkReply *reply);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QSet<QString> m_workedCounties;
    QGraphicsView *m_view;
    QGraphicsScene *m_scene;
    QNetworkAccessManager *m_netMan;
    QSvgRenderer *m_renderer;
    void loadMap();
};

#endif // COUNTYMAPDIALOG_H
