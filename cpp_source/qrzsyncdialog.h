// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef QRZSYNCDIALOG_H
#define QRZSYNCDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

class QrzSyncDialog : public QDialog {
    Q_OBJECT
public:
    QrzSyncDialog(QWidget *parent, const QString &apiKey, const QString &logfilePath, bool isFull);
    ~QrzSyncDialog();

    bool syncSuccessful() const { return m_successful; }
    void setSessionKey(const QString &key) { m_sessionKey = key; }

private slots:
    void startDownload();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onDownloadError(QNetworkReply::NetworkError code);
    void cancelDownload();
    void retryDownload();

private:
    void requestFetch();
    void processAdif(const QString &adifData);

    QString m_apiKey;
    QString m_logfilePath;
    QString m_sessionKey;
    bool m_isFull;
    bool m_successful;

    QNetworkAccessManager *m_netManager;
    QNetworkReply *m_reply;
    QElapsedTimer m_timer;
    
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QLabel *m_speedLabel;
    QLabel *m_timeLabel;
    
    QPushButton *m_btnCancel;
    QPushButton *m_btnRetry;
    QPushButton *m_btnClose;
    QPushButton *m_btnDebug;
    class QTextEdit *m_debugText;
    QString m_lastRequestDump;
    QString m_lastResponseDump;
    
    qint64 m_bytesReceived;
    int m_maxLogId;
    int m_closeCountdown;
};

#endif // QRZSYNCDIALOG_H
