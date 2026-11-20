// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
// ============================================================================
// COMPONENT: MainWindow Declaration (mainwindow.h)
// ROLE: Defines the central controller for the Qwain ecosystem.
//       Declares socket listeners, database handles (`qwain.sqlite`), QRZ.com
//       queue managers, viewports, rolling backup engine, and UI slots.
// INVOKED BY: main.cpp
// PRODUCES: MainWindow class declaration.
// ============================================================================

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QSettings>
#include <QSet>
#include <QMap>
#include <QTimer>
#include <QDateTime>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QToolButton>
#include "qsodata.h"

class QString;
class QTextEdit;
class QTcpServer;
class QTcpSocket;
class QProgressBar;
class QNetworkAccessManager;
class QNetworkReply;
class QLabel;
class QTextBrowser;
class QTableWidget;
class QAction;
class LoadingSplash;

#include "globewidget.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    static QString countryToIso(QString c);
    static QList<QString> getAllCountries();
    MainWindow(LoadingSplash *splash = nullptr, QWidget *parent = nullptr);
    ~MainWindow();
    bool checkLotwDatabase(const QString &callsign);

    
    void testQrzLogin(bool silent = false);
    void forceSyncQrzLog();
    void startQslMailCheck();
    void prepareNewAdi();
    void updateLocalAdifQslStatus(const QList<QsoDetail> &selectedQsos);
    QString getQrzSessionKey() const { return m_qrzSessionKey; }
    
    void logSystem(const QString& msg);
    void logDebug(const QString& msg);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    void onSettingsApplied();
    void backupData(const QString &userDir);
    void restoreData(const QString &userDir);
    void loadLogfile();

private slots:
    void updateMenuLabels();
    void openConfigDialog();
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();
    void onUdpReadyRead();
    void setQrzUsername();
    void setQrzPassword();
    void onQrzLoginReply(QNetworkReply *reply, bool silent = false);
    void openCacheViewer();
    void openIconTools();
    void openAwardMap(const QString &awStr);
    void updateClock();
    void setMapStyleSatellite();
    void setMapStyleSimplified();
    void updateLeaderboard();
    void onXmlTestClicked();
    void onCountyPrepClicked();

public:
    void doQrzLookup(const QString &callsign, const QString &adifName, const QString &adifAddr2, const QString &adifCity, const QString &adifState, const QString &adifZip, const QString &adifCountry, qint64 startMs, const QString &pota, bool adifLotw, bool adifMqsl, bool updateDisplay = true, bool isUdp = false, const QString &originalCall = QString());
    void doFreeLookup(const QString &callsign, const QString &adifName, const QString &adifAddr2, const QString &adifCity, const QString &adifState, const QString &adifZip, const QString &adifCountry, qint64 startMs, const QString &pota, bool adifLotw, bool adifMqsl, bool updateDisplay = true, bool isUdp = false, const QString &originalCall = QString());
    
    int port() const { return m_port; }
    QString ipAddress() const { return m_ipAddress; }
    QDateTime sessionStartTime() const { return m_sessionStartTime; }
    static void incrementTemplatesPrinted(int count = 1);

public slots:
    void openStatsDialog();

signals:
    void dataReceived(const QString &protocol, const QString &senderAddress, int senderPort, const QString &data);

private slots:
    void openLiveStreamViewer();

private:
    void readSettings();
    void applyThemeStyle();
    void writeSettings();
    void performRollingConfigBackup();
    void buildAwardsMenu();
    void restartServer();
    void displayCallInfo(const QString &htmlInfo, const QByteArray &imgData, const QString &callsign, const QString &pota, const QString &country, bool lotw = false, bool mqsl = false);
    void trackQsoStats(const QsoDetail &d);
    void openCountriesOfTheWorld();
    void openTestingWorldMap();
    void openWAJAMap();
    QString extractAdifField(const QString &record, const QString &field);
    void updateQsoCount();
    QString formatElapsedMs(qint64 elapsedMs);

    void initDatabase();
    void importLogfileToDb(const QString &logContent);
    int getAwardCount(const QJsonObject &awardObj);
    
    QSqlDatabase m_db;
    QString m_ipAddress;
    int m_port;
    QString m_protocol;
    QString m_logfilePath;
    QDateTime m_logLastModified;
    QString m_receiveBuffer;
    
    QString m_qrzUsername;
    QString m_qrzPassword;
    QString m_qrzSessionKey;
    bool m_qrzXmlSubscriber;
    
    QTextEdit *m_textEdit = nullptr;
    QTextBrowser *m_infoBrowser = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_qsoCountLabel = nullptr;
    QLabel *m_timeLabel = nullptr;
    QLabel *m_dateLabel = nullptr;
    QLabel *m_newQsoLabel = nullptr;
    QPropertyAnimation *m_newQsoAnim = nullptr;
    QLabel *m_newCountyLabel = nullptr;
    QPropertyAnimation *m_newCountyAnim = nullptr;
    QTimer *m_newCountyTimer = nullptr;
    QLabel *m_newGridLabel = nullptr;
    QPropertyAnimation *m_newGridAnim = nullptr;
    QTimer *m_newGridTimer = nullptr;
    
    QSet<QString> m_dxccWorked;
    QSet<QString> m_dxccDetailWorked;
    QSet<QString> m_was5BandWorked;
    QSet<QString> m_wasDetailWorked;
    QSet<QString> m_japanPrefecturesWorked;
    QSet<QString> m_japanDetailWorked;
    QSet<QString> m_japanCitiesWorked;
    QSet<QString> m_usCountiesWorked;
    QSet<QString> m_potaWorked;
    QSet<QString> m_potaDetailWorked;
    QSet<QString> m_gridsWorked;
    
    QMenu *m_trackingMenu = nullptr;
    
    QList<QAction*> m_dynamicAwardActions;
    
    QAction *m_ipAction = nullptr;
    QAction *m_portAction = nullptr;
    QAction *m_logFileAction = nullptr;
    QAction *m_userAction = nullptr;
    
    QString m_newsText;
    int m_newsTickerPos;
    
    QTcpServer *m_tcpServer = nullptr;
    QList<QTcpSocket*> m_clients;
    class QUdpSocket *m_udpSocket = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;
    QSet<QString> m_uniqueCallsigns;
    GridQsoMap m_gridQsos;
    QList<QsoDetail> m_potaQsos;
    QMap<QString, QList<QsoDetail>> m_allQsos;
    class QLineEdit *m_lookupEdit = nullptr;
    class QListWidget *m_searchMatchesList = nullptr;
    LoadingSplash *m_splash = nullptr;
    
    QString m_lastHtmlInfoRaw;
    QByteArray m_lastImgData;
    QString m_lastPota;
    QString m_lastCountry;
    bool m_lastLotw = false;
    bool m_lastMqsl = false;
    bool m_useHeatmapMode = false;
    
public:
    void processAdifRecord(const QString &record, bool isManualLookup = false, bool updateDisplay = true, bool isUdp = false);
    void lookupCallsign(const QString &callsign);
    
    const QSet<QString>& uniqueCallsigns() const { return m_uniqueCallsigns; }
    const QMap<QString, QList<QsoDetail>>& allQsos() const { return m_allQsos; }
    QMap<QString, QList<QsoDetail>>& allQsos() { return m_allQsos; }
    QString logfilePath() const { return m_logfilePath; }
    void setLogfilePath(const QString &path, bool persist = false);

private:
    GlobeWidget *m_globeWidget;
    void updateGlobe();

    QString m_currentDisplayedCallsign;
    QDateTime m_sessionStartTime;

    QTimer *m_qrzQueueTimer = nullptr;
    QStringList m_qrzQueue;
    int m_qrzQueueTotal = 0;
    int m_qrzQueueSuccessCount = 0;
    QString m_qrzQueueLastCall;
    int m_qrzQueueIntervalMs = 1500;

public:
    void startQrzQueue(const QStringList &callsigns);
    void stopQrzQueueWorker();
    bool isQrzQueueRunning() const { return m_qrzQueueTimer && m_qrzQueueTimer->isActive(); }
    bool hasPausedQrzQueue() const { return !m_qrzQueue.isEmpty(); }
    void resumeQrzQueue();
    void logQueueProgress(const QString &msg);

private slots:
    void processNextQrzQueueItem();
    void loadQrzQueueTickFile();
    void saveQrzQueueTickFile(const QString &state);
};

#endif // MAINWINDOW_H
