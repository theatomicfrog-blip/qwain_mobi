// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef MAILCHECKDIALOG_H
#define MAILCHECKDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QElapsedTimer>
#include <QNetworkAccessManager>

class QLabel;
class QProgressBar;
class QCheckBox;
class QPushButton;
class QNetworkReply;

class MailCheckDialog : public QDialog {
    Q_OBJECT
public:
    explicit MailCheckDialog(QWidget *parent, const QString &sessionKey, const QString &logfilePath);
    ~MailCheckDialog();

    bool isBackgroundOptionSelected() const;
    QStringList getRemainingCacheCalls() const { return m_cacheQueue; }
    int getCacheChecked() const { return m_cacheChecked; }
    int getCacheFound() const { return m_cacheFound; }

private slots:
    void startCacheCheck();
    void checkNextCacheCall();
    void onCacheReplyFinished();

private:
    QString m_sessionKey;
    QString m_logfilePath;
    QStringList m_cacheQueue;
    int m_totalCacheCalls;
    int m_cacheChecked;
    int m_cacheFound;

    QNetworkAccessManager *m_netManager;
    QNetworkReply *m_cacheReply;
    QElapsedTimer m_cacheTimer;

    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QLabel *m_speedLabel;
    QLabel *m_timeLabel;
    QCheckBox *m_chkBackground;
    QPushButton *m_btnCancel;
};

#endif // MAILCHECKDIALOG_H
