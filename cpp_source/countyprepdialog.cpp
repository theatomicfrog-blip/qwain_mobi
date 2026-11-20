#include "countyprepdialog.h"
#include "qrzcache.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QXmlStreamReader>
#include <QUrl>
#include <QNetworkRequest>
#include <QMessageBox>
#include <QDateTime>

CountyPrepDialog::CountyPrepDialog(QWidget *parent, const QString &sessionKey, const QString &appDataDir, const QStringList &callsigns, QSet<QString> *usCountiesWorked)
    : QDialog(parent)
    , m_sessionKey(sessionKey)
    , m_appDataDir(appDataDir)
    , m_queue(callsigns)
    , m_totalCount(callsigns.size())
    , m_checkedCount(0)
    , m_skippedCount(0)
    , m_successCount(0)
    , m_failCount(0)
    , m_pendingCount(0)
    , m_usCountiesWorked(usCountiesWorked)
{
    setWindowTitle(tr("Initialize US County Cache"));
    resize(600, 450);

    // Dark-themed, modern palette matching Qwain styling
    setStyleSheet(
        "QDialog { background-color: #1a1e24; color: #e2e8f0; font-family: 'Segoe UI', Arial, sans-serif; }"
        "QLabel { color: #cbd5e1; font-size: 13px; }"
        "QProgressBar { border: 1px solid #4a5568; border-radius: 4px; text-align: center; background-color: #2d3748; color: #fff; font-weight: bold; height: 22px; }"
        "QProgressBar::chunk { background-color: #3182ce; border-radius: 3px; }"
        "QTextEdit { background-color: #11151c; color: #a0aec0; border: 1px solid #2d3748; border-radius: 4px; font-family: 'Courier New', monospace; font-size: 12px; }"
        "QPushButton { background-color: #e53e3e; color: white; border: none; padding: 8px 16px; border-radius: 4px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #c53030; }"
        "QPushButton:disabled { background-color: #4a5568; color: #a0aec0; }"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    m_lblInfo = new QLabel(tr("Scanning US-based callsigns (DXCC 291, 6, 110, 285) in active logfile..."), this);
    QFont infoFont = m_lblInfo->font();
    infoFont.setBold(true);
    m_lblInfo->setFont(infoFont);
    mainLayout->addWidget(m_lblInfo);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, m_totalCount);
    m_progressBar->setValue(0);
    mainLayout->addWidget(m_progressBar);

    m_lblStats = new QLabel(this);
    m_lblStats->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_lblStats);

    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    mainLayout->addWidget(m_logEdit);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    m_btnCancel = new QPushButton(tr("Cancel / Stop"), this);
    connect(m_btnCancel, &QPushButton::clicked, this, &CountyPrepDialog::onCancelClicked);
    btnLayout->addWidget(m_btnCancel);
    mainLayout->addLayout(btnLayout);

    m_netManager = new QNetworkAccessManager(this);

    m_timer = new QTimer(this);
    m_timer->setInterval(50); // 50ms interval as specified by user
    connect(m_timer, &QTimer::timeout, this, &CountyPrepDialog::onTick);

    updateUI();
    logMessage(tr("Scanning starting with %1 unique callsigns in queue...").arg(m_totalCount));

    // Kick off the processing loop on constructor complete
    QTimer::singleShot(500, m_timer, qOverload<>(&QTimer::start));
}

CountyPrepDialog::~CountyPrepDialog()
{
    m_timer->stop();
    for (QNetworkReply *reply : m_activeReplies) {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
    }
}

void CountyPrepDialog::updateUI()
{
    m_progressBar->setValue(m_checkedCount);
    QString stats = tr("Total: %1  |  Processed: %2  |  Cached: %3  |  Network Success: %4  |  Failures: %5  |  Pending: %6")
                        .arg(m_totalCount)
                        .arg(m_checkedCount)
                        .arg(m_skippedCount)
                        .arg(m_successCount)
                        .arg(m_failCount)
                        .arg(m_pendingCount);
    m_lblStats->setText(stats);
}

void CountyPrepDialog::logMessage(const QString &msg)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    m_logEdit->append(QString("[%1] %2").arg(timestamp, msg));
}

void CountyPrepDialog::onTick()
{
    // Fast skip processing for cached ones
    while (!m_queue.isEmpty()) {
        QString callsign = m_queue.takeFirst();
        
        QrzCache cache(m_appDataDir + "/qrz_cache.ini", QrzCache::IniFormat);
        cache.beginGroup(callsign);
        QString state = cache.value("qrz_state").toString().toUpper();
        QString county = cache.value("county").toString().toUpper();
        QString country = cache.value("country").toString().toUpper();
        QString dxcc = cache.value("dxcc").toString();
        cache.endGroup();

        if (!state.isEmpty() && !county.isEmpty()) {
            // Already Cached! Populate inMainWindow
            m_checkedCount++;
            m_skippedCount++;

            QString c = county;
            if (!c.contains(",")) {
                c = state + "," + c;
            }
            c.replace(", ", ",");
            m_usCountiesWorked->insert(c);

            logMessage(QString(tr("[CACHED] %1 -> %2, %3")).arg(callsign, state, county));
            updateUI();
            
            // Continue the loop to handle cached ones instantly without pausing
            continue;
        }

        // Needs network query
        m_checkedCount++;
        m_pendingCount++;
        updateUI();

        logMessage(QString(tr("[QUERYING] %1 via QRZ XML API...")).arg(callsign));

        QString urlStr = QString("https://xmldata.qrz.com/xml/current/?s=%1;callsign=%2").arg(m_sessionKey).arg(callsign);
        QNetworkRequest req((QUrl(urlStr)));
        QNetworkReply *reply = m_netManager->get(req);
        m_activeReplies.append(reply);

        connect(reply, &QNetworkReply::finished, this, [this, reply, callsign]() {
            m_activeReplies.removeOne(reply);
            reply->deleteLater();
            onReplyFinished(reply, callsign);
        });

        // Break loop to respect 50ms pacing for next network request
        return;
    }

    // Queue empty
    if (m_pendingCount == 0) {
        m_timer->stop();
        logMessage(tr("Batch operation completed successfully!"));
        m_btnCancel->setText(tr("Close"));
        m_btnCancel->setStyleSheet("background-color: #3182ce; color: white;");
        m_btnCancel->disconnect();
        connect(m_btnCancel, &QPushButton::clicked, this, &CountyPrepDialog::accept);
        
        QMessageBox::information(this, tr("Scan Complete"), tr("US County XML scan completed! Total callsigns: %1").arg(m_totalCount));
    }
}

void CountyPrepDialog::onReplyFinished(QNetworkReply *reply, const QString &callsign)
{
    m_pendingCount--;
    
    if (reply->error() != QNetworkReply::NoError) {
        m_failCount++;
        logMessage(QString(tr("[FAILED] %1 (Network Error: %2)")).arg(callsign, reply->errorString()));
        updateUI();
        
        // If timer was waiting for queue to empty, trigger tick to finish
        if (m_queue.isEmpty() && m_pendingCount == 0) {
            onTick();
        }
        return;
    }

    QByteArray data = reply->readAll();
    QXmlStreamReader xml(data);

    QString qrzFname, qrzLastName, qrzAddr1, qrzAddr2, qrzCity, qrzState, qrzZip, qrzCountry, qrzCounty, qrzGrid, qrzDxcc, qrzLat, qrzLon, qrzQslmgr;
    bool hasError = false;
    QString errorString;
    bool lotw = false;
    bool mqsl = false;

    while (!xml.atEnd() && !xml.hasError()) {
        if (xml.readNext() == QXmlStreamReader::StartElement) {
            QString tag = xml.name().toString();
            if (tag == "fname") qrzFname = xml.readElementText();
            else if (tag == "name") qrzLastName = xml.readElementText();
            else if (tag == "addr1") qrzAddr1 = xml.readElementText();
            else if (tag == "addr2") qrzAddr2 = xml.readElementText();
            else if (tag == "city") qrzCity = xml.readElementText();
            else if (tag == "state") qrzState = xml.readElementText().toUpper();
            else if (tag == "zip") qrzZip = xml.readElementText();
            else if (tag == "country") qrzCountry = xml.readElementText().toUpper();
            else if (tag == "county") qrzCounty = xml.readElementText().toUpper();
            else if (tag == "grid") qrzGrid = xml.readElementText();
            else if (tag == "dxcc") qrzDxcc = xml.readElementText();
            else if (tag == "lat") qrzLat = xml.readElementText();
            else if (tag == "lon") qrzLon = xml.readElementText();
            else if (tag == "lotw") { if (xml.readElementText() == "1") lotw = true; }
            else if (tag == "mqsl") { if (xml.readElementText() == "1") mqsl = true; }
            else if (tag == "qslmgr") qrzQslmgr = xml.readElementText();
            else if (tag.compare("Error", Qt::CaseInsensitive) == 0) {
                errorString = xml.readElementText();
                hasError = true;
            }
        }
    }

    if (hasError) {
        m_failCount++;
        logMessage(QString(tr("[FAILED] %1 (QRZ Error: %2)")).arg(callsign, errorString));
        updateUI();
    } else if (qrzState.isEmpty() || qrzCounty.isEmpty()) {
        m_failCount++;
        logMessage(QString(tr("[INCOMPLETE] %1 -> No county or state returned by QRZ.")).arg(callsign));
        updateUI();
    } else {
        m_successCount++;
        logMessage(QString(tr("[SUCCESS] %1 -> %2, %3")).arg(callsign, qrzState, qrzCounty));

        // Store to QrzCache
        QrzCache cache(m_appDataDir + "/qrz_cache.ini", QrzCache::IniFormat);
        cache.beginGroup(callsign);
        cache.setValue("qrz_state", qrzState);
        cache.setValue("county", qrzCounty);
        if (!qrzCountry.isEmpty()) cache.setValue("country", qrzCountry);
        if (!qrzGrid.isEmpty()) cache.setValue("grid", qrzGrid);
        if (!qrzDxcc.isEmpty()) cache.setValue("dxcc", qrzDxcc);
        if (!qrzLat.isEmpty()) cache.setValue("lat", qrzLat);
        if (!qrzLon.isEmpty()) cache.setValue("lon", qrzLon);
        if (!qrzFname.isEmpty()) cache.setValue("qrz_fname", qrzFname);
        if (!qrzLastName.isEmpty()) cache.setValue("qrz_name", qrzLastName);
        if (!qrzAddr1.isEmpty()) cache.setValue("qrz_addr1", qrzAddr1);
        if (!qrzAddr2.isEmpty()) cache.setValue("qrz_addr2", qrzAddr2);
        if (!qrzCity.isEmpty()) cache.setValue("qrz_city", qrzCity);
        if (!qrzZip.isEmpty()) cache.setValue("qrz_zip", qrzZip);
        cache.setValue("lotw", lotw);
        cache.setValue("mqsl", mqsl);
        if (!qrzQslmgr.isEmpty()) cache.setValue("qslmgr", qrzQslmgr);
        cache.endGroup();
        cache.sync();

        // Populate in MainWindow m_usCountiesWorked
        QString c = qrzCounty;
        if (!c.contains(",")) {
            c = qrzState + "," + c;
        }
        c.replace(", ", ",");
        m_usCountiesWorked->insert(c);

        updateUI();
    }

    // Trigger tick on completion to finish
    if (m_queue.isEmpty() && m_pendingCount == 0) {
        onTick();
    }
}

void CountyPrepDialog::onCancelClicked()
{
    m_timer->stop();
    for (QNetworkReply *reply : m_activeReplies) {
        if (reply && reply->isRunning()) reply->abort();
    }
    logMessage(tr("Batch operation aborted by user."));
    QMessageBox::warning(this, tr("Cancelled"), tr("Bulk scan cancelled. Processed callsigns will remain cached."));
    reject();
}
