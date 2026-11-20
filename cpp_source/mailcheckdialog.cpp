// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "mailcheckdialog.h"
#include "paths.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QCheckBox>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSettings>
#include <QFile>
#include <QRegularExpression>
#include <QTimer>
#include <QMessageBox>
#include <QSet>
#include <QApplication>

MailCheckDialog::MailCheckDialog(QWidget *parent, const QString &sessionKey, const QString &logfilePath)
    : QDialog(parent), m_sessionKey(sessionKey), m_logfilePath(logfilePath), 
      m_totalCacheCalls(0), m_cacheChecked(0), m_cacheFound(0), m_cacheReply(nullptr)
{
    setWindowTitle("QSL by Mail Check");
    setMinimumWidth(450);
    
    m_netManager = new QNetworkAccessManager(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    
    m_statusLabel = new QLabel("Initializing...", this);
    layout->addWidget(m_statusLabel);
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    layout->addWidget(m_progressBar);
    
    m_speedLabel = new QLabel("Speed: --", this);
    layout->addWidget(m_speedLabel);
    
    m_timeLabel = new QLabel("Estimated Time: --", this);
    layout->addWidget(m_timeLabel);
    
    m_chkBackground = new QCheckBox("Run in the Background", this);
    connect(m_chkBackground, &QCheckBox::clicked, this, [this](bool checked) {
        if (checked) this->hide();
    });
    layout->addWidget(m_chkBackground);
    
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    
    m_btnCancel = new QPushButton("Cancel", this);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(m_btnCancel);
    
    layout->addLayout(btnLayout);

    QTimer::singleShot(100, this, &MailCheckDialog::startCacheCheck);
}

MailCheckDialog::~MailCheckDialog() {
}

bool MailCheckDialog::isBackgroundOptionSelected() const {
    return m_chkBackground->isChecked();
}

void MailCheckDialog::startCacheCheck() {
    if (m_sessionKey.isEmpty()) {
        QMessageBox::warning(this, "QRZ Error", "No QRZ Session Key available.\nPlease sync logbook first or ensure your credentials are correct in Settings.");
        reject();
        return;
    }

    m_statusLabel->setText("Scanning local logfile...");
    QApplication::processEvents();

    QSet<QString> callsgns;
    QSettings mailCache(getAppDataDir() + "/qrz_mailing_cache.ini", QSettings::IniFormat);
    QStringList groups = mailCache.childGroups();
    QSet<QString> cachedCalls(groups.begin(), groups.end());

    QFile logFile(m_logfilePath);
    if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = logFile.readAll();
        logFile.close();
        QRegularExpression callRe("<CALL:(\\d+)[^>]*>([^<]+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator i = callRe.globalMatch(content);
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString c = match.captured(2).left(match.captured(1).toInt()).trimmed().toUpper();
            if (!c.isEmpty() && !cachedCalls.contains(c)) {
                callsgns.insert(c);
            }
        }
    }
    
    m_cacheQueue = callsgns.values();
    m_totalCacheCalls = m_cacheQueue.size();
    
    if (m_totalCacheCalls == 0) {
        QMessageBox::information(this, "Update Complete", "All calls in logfile are already in the mailing cache.");
        accept();
        return;
    }

    m_progressBar->setMaximum(m_totalCacheCalls);
    m_progressBar->setValue(0);
    
    m_cacheTimer.start();
    m_cacheChecked = 0;
    m_cacheFound = 0;
    checkNextCacheCall();
}

void MailCheckDialog::checkNextCacheCall() {
    if (m_cacheQueue.isEmpty()) {
        if (isVisible()) m_progressBar->setValue(m_totalCacheCalls);
        if (isBackgroundOptionSelected()) {
            QMessageBox::information(nullptr, "Background Mail Check Complete", QString("Finished background mail checking.\nFound %1 new records.").arg(m_cacheFound));
        } else {
            QMessageBox::information(this, "Mail Check Complete", QString("Finished checking.\nFound %1 new records.").arg(m_cacheFound));
        }
        accept();
        return;
    }
    
    QString call = m_cacheQueue.takeFirst();
    if (isVisible()) m_progressBar->setValue(m_totalCacheCalls - m_cacheQueue.size());
    
    qint64 elapsedMs = m_cacheTimer.elapsed();
    if (isVisible() && elapsedMs > 0 && m_cacheChecked > 0) {
        double speed = static_cast<double>(m_cacheChecked) / (static_cast<double>(elapsedMs) / 1000.0);
        m_speedLabel->setText(QString("Speed: %1 calls/s").arg(speed, 0, 'f', 1));
        
        if (speed > 0) {
            double remainingSec = static_cast<double>(m_cacheQueue.size()) / speed;
            m_timeLabel->setText(QString("Estimated Time: %1 sec").arg(static_cast<int>(remainingSec)));
        }
    }
    
    if (isVisible()) m_statusLabel->setText(QString("Fetching mailing info for %1...").arg(call));
    
    QString urlStr = QString("https://xmldata.qrz.com/xml/current/?s=%1;callsign=%2").arg(m_sessionKey).arg(call);
    QNetworkRequest req((QUrl(urlStr)));
    
    if (m_cacheReply) {
        m_cacheReply->deleteLater();
        m_cacheReply = nullptr;
    }
    
    m_cacheReply = m_netManager->get(req);
    m_cacheReply->setProperty("callsign", call);
    connect(m_cacheReply, &QNetworkReply::finished, this, &MailCheckDialog::onCacheReplyFinished);
}

void MailCheckDialog::onCacheReplyFinished() {
    if (!m_cacheReply) return;
    
    QString call = m_cacheReply->property("callsign").toString();
    
    if (m_cacheReply->error() == QNetworkReply::NoError) {
        QByteArray xmlData = m_cacheReply->readAll();
        if (xmlData.contains("<call>")) {
            m_cacheFound++;
            
            QSettings mailCache(getAppDataDir() + "/qrz_mailing_cache.ini", QSettings::IniFormat);
            mailCache.beginGroup(call);
            
            auto extractTag = [&](const QString &tag) {
                QRegularExpression re("<" + tag + ">([^<]+)</" + tag + ">", QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch match = re.match(xmlData);
                return match.hasMatch() ? match.captured(1).trimmed() : QString();
            };
            
            mailCache.setValue("name", extractTag("name"));
            mailCache.setValue("addr1", extractTag("addr1"));
            mailCache.setValue("addr2", extractTag("addr2"));
            mailCache.setValue("city", extractTag("city"));
            mailCache.setValue("state", extractTag("state"));
            mailCache.setValue("zip", extractTag("zip"));
            mailCache.setValue("country", extractTag("country"));
            
            mailCache.endGroup();
        }
    }
    
    m_cacheReply->deleteLater();
    m_cacheReply = nullptr;
    
    m_cacheChecked++;
    QTimer::singleShot(20, this, &MailCheckDialog::checkNextCacheCall);
}
