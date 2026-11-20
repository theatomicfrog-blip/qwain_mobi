// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "qrzsyncdialog.h"
#include "paths.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QCheckBox>
#include <QToolTip>

#include <QTextEdit>

// ============================================================================
// COMPONENT: QrzSyncDialog
// ROLE: Asynchronous XML QRZ Logbook synchronization download pipeline.
//       This component checks logbook state entries, contacts the QRZ Logbook
//       HTTP API, retrieves ADIF formatted call records incrementally or fully,
//       parses entries, writes to disk, and updates GUI speed gauges and progress bars.
// INVOKED BY: MainWindow (toolbar "Sync QRZ Log" action).
// PRODUCES: Freshly synchronized local ADIF logs and updated UI table views.
// ============================================================================

// [ENTRY POINT: QRZ Sync Dialog Constructor]
// Sets up layout elements, binds socket callback slots, and launches startDownload via a single-shot timer.
QrzSyncDialog::QrzSyncDialog(QWidget *parent, const QString &apiKey, const QString &logfilePath, bool isFull)
    : QDialog(parent), m_apiKey(apiKey), m_logfilePath(logfilePath), m_isFull(isFull), m_successful(false), m_reply(nullptr), m_bytesReceived(0)
{
    setWindowTitle(m_isFull ? "QRZ Full Sync" : "QRZ Incremental Sync");
    setMinimumWidth(500);

    m_netManager = new QNetworkAccessManager(this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    
    m_statusLabel = new QLabel("Initializing download...", this);
    layout->addWidget(m_statusLabel);
    
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    layout->addWidget(m_progressBar);
    
    m_speedLabel = new QLabel("Speed: 0 KB/s", this);
    layout->addWidget(m_speedLabel);
    
    m_timeLabel = new QLabel("Estimated Time: --", this);
    layout->addWidget(m_timeLabel);

    m_debugText = new QTextEdit(this);
    m_debugText->setReadOnly(true);
    m_debugText->hide();
    layout->addWidget(m_debugText);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    m_btnDebug = new QPushButton("Debug", this);
    m_btnDebug->setCheckable(true);
    m_btnRetry = new QPushButton("Retry", this);
    m_btnCancel = new QPushButton("Cancel", this);
    m_btnClose = new QPushButton("Close", this);
    
    m_btnRetry->setEnabled(false);
    m_btnClose->setEnabled(false);
    
    btnLayout->addWidget(m_btnDebug);
    btnLayout->addStretch();
    btnLayout->addWidget(m_btnRetry);
    btnLayout->addWidget(m_btnCancel);
    btnLayout->addWidget(m_btnClose);
    layout->addLayout(btnLayout);
    
    connect(m_btnDebug, &QPushButton::toggled, this, [this](bool checked) {
        m_debugText->setVisible(checked);
        if (checked) {
            QString debugInfo = "=== REQUEST ===\n" + m_lastRequestDump + "\n\n=== RESPONSE ===\n" + m_lastResponseDump;
            m_debugText->setPlainText(debugInfo);
        }
    });
    connect(m_btnRetry, &QPushButton::clicked, this, &QrzSyncDialog::retryDownload);
    connect(m_btnCancel, &QPushButton::clicked, this, &QrzSyncDialog::cancelDownload);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::accept);

    QTimer::singleShot(100, this, &QrzSyncDialog::startDownload);
}

QrzSyncDialog::~QrzSyncDialog() {
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
    }
}

// [ENTRY POINT: Local Logbook High-watermark Scanner]
// Scans the active logfile on disk to locate the largest `<APP_QRZLOG_LOGID>` tag value,
// establishing the incremental download boundary to fetch only newer records from the server.
void QrzSyncDialog::startDownload() {
    m_btnRetry->setEnabled(false);
    m_btnCancel->setEnabled(true);
    m_btnClose->setEnabled(false);
    m_progressBar->setValue(0);
    m_speedLabel->setText("Speed: 0 KB/s");
    m_timeLabel->setText("Estimated Time: --");
    
    m_maxLogId = 0;
    if (!m_isFull) {
        m_statusLabel->setText("Checking current logfile for latest QRZ Log ID...");
        QCoreApplication::processEvents();
        QFile file(m_logfilePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();
            QRegularExpression re("<APP_QRZLOG_LOGID:(\\d+)[^>]*>([^<]+)", QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatchIterator i = re.globalMatch(content);
            while (i.hasNext()) {
                QRegularExpressionMatch match = i.next();
                int len = match.captured(1).toInt();
                int logId = match.captured(2).left(len).trimmed().toInt();
                if (logId > m_maxLogId) m_maxLogId = logId;
            }
        }
        m_statusLabel->setText(QString("Found latest ID: %1. Downloading newer records...").arg(m_maxLogId));
    } else {
        m_statusLabel->setText("Connecting to QRZ...");
    }
    QCoreApplication::processEvents();
    
    requestFetch();
}

// [ENTRY POINT: HTTP FETCH API Dispatches]
// Launches an asynchronous HTTP POST request containing the authentication API keys
// to the QRZ logbook endpoint, establishing progress tracking and signal hooks.
void QrzSyncDialog::requestFetch() {
    QUrl url("https://logbook.qrz.com/api");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    QUrlQuery postData;
    postData.addQueryItem("KEY", m_apiKey);
    postData.addQueryItem("ACTION", "FETCH");
    
    QByteArray postBytes = postData.toString(QUrl::FullyEncoded).toUtf8();
    
    m_lastRequestDump = "URL: " + url.toString() + "\n";
    m_lastRequestDump += "Method: POST\n";
    m_lastRequestDump += "Headers:\n";
    const QList<QByteArray> headers = req.rawHeaderList();
    for (const QByteArray &h : headers) {
        m_lastRequestDump += QString(h) + ": " + QString(req.rawHeader(h)) + "\n";
    }
    m_lastRequestDump += "\nBody:\n" + QString(postBytes);
    
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    
    m_reply = m_netManager->post(req, postBytes);
    
    connect(m_reply, &QNetworkReply::downloadProgress, this, &QrzSyncDialog::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &QrzSyncDialog::onDownloadFinished);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_reply, &QNetworkReply::errorOccurred, this, &QrzSyncDialog::onDownloadError);
#else
    connect(m_reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::error), this, &QrzSyncDialog::onDownloadError);
#endif
    
    m_timer.start();
    m_bytesReceived = 0;
}

void QrzSyncDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    m_bytesReceived = bytesReceived;
    if (bytesTotal > 0) {
        m_progressBar->setMaximum(100);
        m_progressBar->setValue(static_cast<int>((bytesReceived * 100) / bytesTotal));
    } else {
        m_progressBar->setMaximum(0); // Indeterminate if size not known
        m_progressBar->setValue(0);
    }
    
    qint64 elapsedMs = m_timer.elapsed();
    if (elapsedMs > 0) {
        double speed = (static_cast<double>(bytesReceived) / 1024.0) / (static_cast<double>(elapsedMs) / 1000.0);
        m_speedLabel->setText(QString("Speed: %1 KB/s").arg(speed, 0, 'f', 1));
        
        if (bytesTotal > 0 && speed > 0) {
            double remainingKb = static_cast<double>(bytesTotal - bytesReceived) / 1024.0;
            double remainingSec = remainingKb / speed;
            m_timeLabel->setText(QString("Estimated Time: %1 sec").arg(static_cast<int>(remainingSec)));
        } else {
            m_timeLabel->setText("Estimated Time: --");
        }
    }
    
    m_statusLabel->setText(QString("Downloading... %1 KB").arg(bytesReceived / 1024));
}

void QrzSyncDialog::onDownloadFinished() {
    if (!m_reply || m_reply->error() != QNetworkReply::NoError) {
        return; // Handled in error slot
    }
    
    m_statusLabel->setText("Download complete. Processing data...");
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(100);
    
    m_btnCancel->setEnabled(false);
    
    QByteArray rawData = m_reply->readAll();
    QString adifData = QString::fromUtf8(rawData);
    
    adifData.replace("&lt;", "<", Qt::CaseInsensitive);
    adifData.replace("&gt;", ">", Qt::CaseInsensitive);
    adifData.replace("&lt", "<", Qt::CaseInsensitive);
    adifData.replace("&gt", ">", Qt::CaseInsensitive);
    adifData.replace("&amp;", "&", Qt::CaseInsensitive);
    
    m_lastResponseDump = "HTTP Status Code: " + m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString() + "\n";
    m_lastResponseDump += "Headers:\n";
    const QList<QByteArray> headers = m_reply->rawHeaderList();
    for (const QByteArray &h : headers) {
        m_lastResponseDump += QString(h) + ": " + QString(m_reply->rawHeader(h)) + "\n";
    }
    
    QStringList adifLines = adifData.split('\n');
    if (adifLines.size() > 300) {
        QStringList lastLines = adifLines.mid(adifLines.size() - 300);
        m_lastResponseDump += "\nBody:\n... (first " + QString::number(adifLines.size() - 300) + " lines truncated) ...\n" + lastLines.join('\n');
    } else {
        m_lastResponseDump += "\nBody:\n" + adifData;
    }
    
    if (m_btnDebug->isChecked()) m_btnDebug->toggled(true); // refresh if open
    
    m_reply->deleteLater();
    m_reply = nullptr;
    
    if (!adifData.contains("<eor>", Qt::CaseInsensitive)) {
        m_statusLabel->setText("Error: Unexpected response format (no <EOR> tags).");
        m_btnRetry->setEnabled(true);
        m_btnClose->setEnabled(true);
        m_btnDebug->setChecked(true); // auto show debug
        return;
    }
    
    processAdif(adifData);
}

void QrzSyncDialog::onDownloadError(QNetworkReply::NetworkError code) {
    Q_UNUSED(code);
    if (!m_reply) return;
    
    if (m_reply->error() == QNetworkReply::OperationCanceledError) {
        m_statusLabel->setText("Download canceled.");
    } else {
        m_statusLabel->setText("Error: " + m_reply->errorString());
        
        m_lastResponseDump = "Network Error: " + m_reply->errorString() + "\n";
        m_lastResponseDump += m_reply->readAll();
        if (m_btnDebug->isChecked()) m_btnDebug->toggled(true);
        m_btnDebug->setChecked(true); // auto show debug on fail
    }
    
    m_progressBar->setValue(0);
    m_speedLabel->setText("Speed: 0 KB/s");
    m_timeLabel->setText("Estimated Time: --");
    
    m_btnRetry->setEnabled(true);
    m_btnCancel->setEnabled(false);
    m_btnClose->setEnabled(true);
    
    m_reply->deleteLater();
    m_reply = nullptr;
}

void QrzSyncDialog::cancelDownload() {
    if (m_reply) {
        m_reply->abort();
    }
}

void QrzSyncDialog::retryDownload() {
    startDownload();
}

// [ENTRY POINT: Log merger & ADIF Writer]
// Overwrites the local logbook with fresh entries in a full-sync scenario, or merges 
// incremental logs by skipping existing elements based on logged dates/times.
void QrzSyncDialog::processAdif(const QString &adifData) {
    if (m_isFull) {
        QFile out(m_logfilePath);
        if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
            out.write(adifData.toUtf8());
            out.close();
            m_successful = true;
            m_statusLabel->setText("Full Sync successful! Logfile overwritten.");
        } else {
            m_statusLabel->setText("Error: Could not write to logfile.");
        }
    } else {
        // Incremental
        m_statusLabel->setText("Merging / checking for duplicates...");
        QCoreApplication::processEvents();
        QFile file(m_logfilePath);
        QString existingData;
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            existingData = file.readAll();
            file.close();
        }
        
        QString cleanAdif = adifData;
        int qIdx = cleanAdif.indexOf('Q', 0, Qt::CaseInsensitive);
        int eohIdx = cleanAdif.indexOf("<eoh>", 0, Qt::CaseInsensitive);
        if (qIdx != -1 && eohIdx != -1 && qIdx < eohIdx) {
            cleanAdif.remove(qIdx, eohIdx - qIdx + 5);
        }
        
        QStringList newRecords;
        QStringList lines = cleanAdif.split('\n');
        QString currentRecord;
        for (const QString &rawline : lines) {
            QString line = rawline.trimmed();
            if (line.isEmpty()) continue;
            currentRecord += line + " ";
            
            if (line.contains("<eor>", Qt::CaseInsensitive)) {
                QString rec = currentRecord.trimmed();
                currentRecord.clear();
                
                QString qrzLogId;
                QRegularExpression logIdRe("<APP_QRZLOG_LOGID:(\\d+)[^>]*>([^<]+)", QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch match = logIdRe.match(rec);
                if (match.hasMatch()) {
                    int len = match.captured(1).toInt();
                    qrzLogId = match.captured(2).left(len).trimmed();
                    int numId = qrzLogId.toInt();
                    if (numId > 0) {
                        if (numId <= m_maxLogId) {
                            continue; // Skip older or duplicate items
                        }
                        QSettings settings(getAppDataDir() + "/settings.ini", QSettings::IniFormat);
                        int lastId = settings.value("qrz_last_sync_id", 0).toInt();
                        if (numId > lastId) {
                            settings.setValue("qrz_last_sync_id", numId);
                            settings.sync();
                        }
                    }
                }
                
                if (!qrzLogId.isEmpty() && existingData.contains("<APP_QRZLOG_LOGID:" + match.captured(1) + ">" + qrzLogId)) {
                    continue; // exists
                } else if (qrzLogId.isEmpty()) {
                    QString call;
                    QRegularExpression callRe("<CALL:(\\d+)[^>]*>([^<]+)", QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatch cmatch = callRe.match(rec);
                    if (cmatch.hasMatch()) call = cmatch.captured(2).left(cmatch.captured(1).toInt());
                    
                    QString timeOn;
                    QRegularExpression timeRe("<TIME_ON:(\\d+)[^>]*>([^<]+)", QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatch tmatch = timeRe.match(rec);
                    if (tmatch.hasMatch()) timeOn = tmatch.captured(2).left(tmatch.captured(1).toInt());
                    
                    if (!call.isEmpty() && existingData.contains(call) && existingData.contains(timeOn)) {
                        continue;
                    }
                }
                
                newRecords.append(rec);
            }
        }
        
        if (newRecords.isEmpty()) {
            m_statusLabel->setText("Incremental Sync: Checking completed, no new contacts found.");
            m_successful = true;
        } else {
            m_statusLabel->setText(QString("Incremental Sync: Found %1 new unique contacts. Appending...").arg(newRecords.size()));
            QCoreApplication::processEvents();
            if (file.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream out(&file);
                for (const QString &r : newRecords) {
                    out << r << "\n";
                }
                file.close();
                m_successful = true;
                m_statusLabel->setText(QString("Appended %1 new contacts successfully.").arg(newRecords.size()));
            } else {
                m_statusLabel->setText("Error: Could not append to logfile.");
                m_successful = false;
            }
        }
    }
    
    if (m_successful) {
        // --- DUPLICATE CHECK ---
        QDialog *dupePopup = new QDialog(this);
        dupePopup->setWindowTitle("Dupe Check");
        QVBoxLayout *dupeLayout = new QVBoxLayout(dupePopup);
        QLabel *dupeLbl = new QLabel("Running Dupe Check...", dupePopup);
        dupeLbl->setAlignment(Qt::AlignCenter);
        dupeLayout->addWidget(dupeLbl);
        dupePopup->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        dupePopup->show();
        QCoreApplication::processEvents();

        QFile file(m_logfilePath);
        int dupesRemoved = 0;
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = file.readAll();
            file.close();

            QRegularExpression eorRe("<eor>", QRegularExpression::CaseInsensitiveOption);
            QStringList records = content.split(eorRe);
            QStringList uniqueRecords;
            QSet<QString> seenRecords;
            
            // To properly preserve header, split the first record
            QString header;
            if (!records.isEmpty() && records.first().contains("<eoh>", Qt::CaseInsensitive)) {
                int eohIdx = records.first().indexOf("<eoh>", 0, Qt::CaseInsensitive);
                header = records.first().left(eohIdx + 5);
                records.first() = records.first().mid(eohIdx + 5);
            }

            for (const auto &rawRec : records) {
                QString rec = rawRec.trimmed();
                if (rec.isEmpty()) continue;

                QString call;
                QRegularExpression callRe("<CALL:(\\d+)[^>]*>([^<]+)", QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch cmatch = callRe.match(rec);
                if (cmatch.hasMatch()) call = cmatch.captured(2).left(cmatch.captured(1).toInt()).trimmed().toUpper();

                QString timeOn;
                QRegularExpression timeRe("<TIME_ON:(\\d+)[^>]*>([^<]+)", QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch tmatch = timeRe.match(rec);
                if (tmatch.hasMatch()) timeOn = tmatch.captured(2).left(tmatch.captured(1).toInt()).trimmed();

                QString qsoDate;
                QRegularExpression dateRe("<QSO_DATE:(\\d+)[^>]*>([^<]+)", QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch dmatch = dateRe.match(rec);
                if (dmatch.hasMatch()) qsoDate = dmatch.captured(2).left(dmatch.captured(1).toInt()).trimmed();

                QString key = call + "|" + qsoDate + "|" + timeOn;
                if (key == "||") {
                    uniqueRecords.append(rec);
                    continue;
                }

                if (seenRecords.contains(key)) {
                    dupesRemoved++;
                } else {
                    seenRecords.insert(key);
                    uniqueRecords.append(rec);
                }
            }

            if (dupesRemoved > 0) {
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&file);
                    if (!header.isEmpty()) {
                        out << header << "\n";
                    }
                    for (const auto &r : uniqueRecords) {
                        out << r << "\n<eor>\n";
                    }
                    file.close();
                }
            }
        }
        
        dupeLbl->setText(QString("Running Dupe Check\nDuplicates removed: %1").arg(dupesRemoved));
        QTimer::singleShot(1500, dupePopup, &QDialog::accept);

        // --- END DUPLICATE CHECK ---

        m_statusLabel->setText(m_statusLabel->text() + " (Closing in 5 seconds...)");
        m_closeCountdown = 5;
        
        QTimer *countdownTimer = new QTimer(this);
        connect(countdownTimer, &QTimer::timeout, this, [this, countdownTimer]() {
            m_closeCountdown--;
            if (m_closeCountdown <= 0) {
                countdownTimer->stop();
                this->accept();
            } else {
                QString existing = m_statusLabel->text();
                int idx = existing.indexOf(" (Closing in");
                if (idx != -1) existing = existing.left(idx);
                m_statusLabel->setText(existing + QString(" (Closing in %1 seconds...)").arg(m_closeCountdown));
            }
        });
        countdownTimer->start(1000);
    }
    
    m_btnClose->setEnabled(true);
}
