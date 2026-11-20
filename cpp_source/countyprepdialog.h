#ifndef COUNTYPREPDIALOG_H
#define COUNTYPREPDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QSet>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QPushButton>

class CountyPrepDialog : public QDialog {
    Q_OBJECT
public:
    explicit CountyPrepDialog(QWidget *parent, const QString &sessionKey, const QString &appDataDir, const QStringList &callsigns, QSet<QString> *usCountiesWorked);
    ~CountyPrepDialog();

private slots:
    void onTick();
    void onCancelClicked();
    void onReplyFinished(QNetworkReply *reply, const QString &callsign);

private:
    QString m_sessionKey;
    QString m_appDataDir;
    QStringList m_queue;
    int m_totalCount;
    int m_checkedCount;
    int m_skippedCount;
    int m_successCount;
    int m_failCount;
    int m_pendingCount;

    QSet<QString> *m_usCountiesWorked;
    QNetworkAccessManager *m_netManager;
    QTimer *m_timer;
    QList<QNetworkReply*> m_activeReplies;

    QLabel *m_lblInfo;
    QLabel *m_lblStats;
    QProgressBar *m_progressBar;
    QTextEdit *m_logEdit;
    QPushButton *m_btnCancel;

    void updateUI();
    void logMessage(const QString &msg);
};

#endif // COUNTYPREPDIALOG_H
