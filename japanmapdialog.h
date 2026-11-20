#ifndef JAPANMAPDIALOG_H
#define JAPANMAPDIALOG_H

#include <QDialog>
#include "gridmapdialog.h"

class QTableWidget;
class QNetworkAccessManager;
class QNetworkReply;

class JapanMapDialog : public QDialog
{
    Q_OBJECT
public:
    JapanMapDialog(const GridQsoMap &gridQsos, QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onMapDownloaded(QNetworkReply *reply);

private:
    void populateTable(const GridQsoMap &gridQsos);
    void loadMapStyle(int mode);
    
    QTableWidget *m_table;
    GridMapView *m_view;
    QNetworkAccessManager *m_netMan;
    QPixmap m_colorMap;
    QPixmap m_simpleMap;
    int m_mapStyleMode;
};

#endif // JAPANMAPDIALOG_H
