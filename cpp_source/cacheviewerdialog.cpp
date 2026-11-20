// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "cacheviewerdialog.h"
#include "qrzcache.h"
#include "prefetchflagsdialog.h"
#include "paths.h"
#include "mainwindow.h"
#include "qrzsyncdialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QScrollArea>
#include <QSettings>
#include <QPushButton>
#include <QImage>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QInputDialog>
#include <QFileDialog>
#include <QTextEdit>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QProgressBar>

class RebuildProgressDialog : public QDialog {
public:
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QCheckBox *updateDetailsCheckbox;
    QPushButton *cancelButton;
    bool wasCanceled;

    RebuildProgressDialog(QWidget *parent) : QDialog(parent), wasCanceled(false) {
        setWindowTitle(QDialog::tr("Rebuilding Cache"));
        setModal(true);
        resize(400, 150);

        QVBoxLayout *layout = new QVBoxLayout(this);

        statusLabel = new QLabel(QDialog::tr("Preparing cache database..."), this);
        layout->addWidget(statusLabel);

        progressBar = new QProgressBar(this);
        progressBar->setRange(0, 100);
        progressBar->setValue(0);
        layout->addWidget(progressBar);

        QHBoxLayout *bottomLayout = new QHBoxLayout();
        updateDetailsCheckbox = new QCheckBox(QDialog::tr("Update Details After Rebuild"), this);
        updateDetailsCheckbox->setChecked(true);
        bottomLayout->addWidget(updateDetailsCheckbox);

        cancelButton = new QPushButton(QDialog::tr("Cancel"), this);
        bottomLayout->addWidget(cancelButton);

        layout->addLayout(bottomLayout);

        connect(cancelButton, &QPushButton::clicked, this, [this]() {
            wasCanceled = true;
            reject();
        });
    }

    void setProgress(int value, int total, const QString &callsign) {
        progressBar->setRange(0, total);
        progressBar->setValue(value);
        statusLabel->setText(QDialog::tr("Caching details for %1... (%2 of %3)").arg(callsign).arg(value + 1).arg(total));
    }
};

CacheViewerDialog::CacheViewerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Cache Tools"));
    resize(600, 420);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    QVBoxLayout *leftLayout = new QVBoxLayout();
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    leftLayout->addWidget(m_listWidget);

    QPushButton *deleteButton = new QPushButton(tr("Delete Cached Entry"), this);
    leftLayout->addWidget(deleteButton);
    
    QPushButton *refreshButton = new QPushButton(tr("Refresh Cached Entry"), this);
    leftLayout->addWidget(refreshButton);

    QPushButton *editButton = new QPushButton(tr("Edit Cached Entry"), this);
    leftLayout->addWidget(editButton);
    
    QPushButton *rebuildButton = new QPushButton(tr("Rebuild Cache"), this);
    rebuildButton->setStyleSheet("QPushButton { background-color: #27ae60; color: white; font-weight: bold; } QPushButton:hover { background-color: #2ecc71; }");
    leftLayout->addWidget(rebuildButton);

    QPushButton *refreshDataButton = new QPushButton(tr("Refresh Data"), this);
    refreshDataButton->setStyleSheet("QPushButton { background-color: #2980b9; color: white; font-weight: bold; } QPushButton:hover { background-color: #3498db; }");
    leftLayout->addWidget(refreshDataButton);
    
    mainLayout->addLayout(leftLayout, 1);

    m_infoBrowser = new QTextBrowser(this);
    m_infoBrowser->setOpenLinks(false); // we handle clicks manually
    mainLayout->addWidget(m_infoBrowser, 2);

    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &CacheViewerDialog::onSelectionChanged);
    connect(deleteButton, &QPushButton::clicked, this, &CacheViewerDialog::onDeleteClicked);
    connect(refreshButton, &QPushButton::clicked, this, &CacheViewerDialog::onRefreshClicked);
    connect(editButton, &QPushButton::clicked, this, &CacheViewerDialog::onEditClicked);
    connect(rebuildButton, &QPushButton::clicked, this, &CacheViewerDialog::onRebuildClicked);
    connect(refreshDataButton, &QPushButton::clicked, this, &CacheViewerDialog::onRefreshDataClicked);
    connect(m_infoBrowser, &QTextBrowser::anchorClicked, this, &CacheViewerDialog::onAnchorClicked);

    populateList();

    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void CacheViewerDialog::populateList()
{
    m_listWidget->clear();
    QrzCache qrzCache;
    QStringList groups = qrzCache.childGroups();
    groups.removeAll("ICONS");
    groups.removeAll("icons");
    groups.sort();
    m_listWidget->addItems(groups);
}

void CacheViewerDialog::onCallsignSelected(const QString &callsign)
{
    if (callsign.isEmpty()) {
        m_infoBrowser->clear();
        return;
    }

    QrzCache qrzCache;
    qrzCache.beginGroup(callsign);

    QString htmlInfo = qrzCache.value("htmlInfo").toString();
    QByteArray imgData = qrzCache.value("imgData").toByteArray();
    qrzCache.endGroup();

    QString imgTag;
    if (!imgData.isEmpty()) {
        QImage img = QImage::fromData(imgData).scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QString resName = "img_cache_" + callsign;
        m_infoBrowser->document()->addResource(QTextDocument::ImageResource, QUrl(resName), img);
        imgTag = QString("<img src='%1'/>").arg(resName);
    } else {
        imgTag = "<div style='width: 150px; height: 150px; background-color: #f0f0f0; text-align: center; line-height: 150px; border: 1px solid #ccc; color: #888;'>No Image</div>";
    }

    QString html = QString("<table border='0' cellspacing='10'><tr><td valign='top'>%1</td><td valign='top'>%2</td></tr></table>")
                   .arg(imgTag, htmlInfo);
    m_infoBrowser->setHtml(html);
}

void CacheViewerDialog::onDeleteClicked()
{
    QList<QListWidgetItem*> items = m_listWidget->selectedItems();
    if (items.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Delete Cached Entries"),
        tr("Are you sure you want to delete %1 selected cached entries?").arg(items.size()),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    QrzCache qrzCache;
    m_listWidget->blockSignals(true);
    for (QListWidgetItem *item : items) {
        QString callsign = item->text();
        qrzCache.beginGroup(callsign);
        qrzCache.remove("");
        qrzCache.endGroup();
    }
    qrzCache.sync();
    m_listWidget->blockSignals(false);

    populateList();
    m_infoBrowser->clear();
}

void CacheViewerDialog::onRefreshClicked()
{
    QList<QListWidgetItem*> items = m_listWidget->selectedItems();
    if (items.isEmpty()) return;

    QWidget *p = parentWidget();
    MainWindow *mw = qobject_cast<MainWindow*>(p);
    if (!mw) {
        QMessageBox::warning(this, "Error", "Could not find main window to trigger lookup.");
        return;
    }

    QStringList callsigns;
    for (QListWidgetItem *item : items) {
        QString call = item->text();
        if (call != "icons") {
            callsigns << call;
        }
    }

    if (callsigns.isEmpty()) {
        QMessageBox::information(this, "Info", "Cannot refresh icons from this button.");
        return;
    }

    if (callsigns.size() == 1) {
        QString group = callsigns.first();
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        bool hasQrzXml = settings.value("Credentials/qrzXmlSubscriber", false).toBool() && !settings.value("Credentials/username").toString().isEmpty();
        
        if (hasQrzXml) {
            mw->doQrzLookup(group, "", "", "", "", "", "", QDateTime::currentMSecsSinceEpoch(), "", false, false);
        } else {
            mw->doFreeLookup(group, "", "", "", "", "", "", QDateTime::currentMSecsSinceEpoch(), "", false, false);
        }
        QMessageBox::information(this, "Refreshing", "Lookup triggered for " + group + ".");
    } else {
        mw->startQrzQueue(callsigns);
        QMessageBox::information(this, tr("Background Refresh"), tr("%1 callsigns added to background queue worker (1.5s interval).").arg(callsigns.size()));
    }
}

void CacheViewerDialog::onSelectionChanged()
{
    QList<QListWidgetItem*> selected = m_listWidget->selectedItems();
    if (selected.isEmpty()) {
        m_infoBrowser->clear();
        return;
    }

    if (selected.size() == 1) {
        onCallsignSelected(selected.first()->text());
        return;
    }

    // Multiple selections
    QString html = QString("<h3>Selected %1 Entries</h3>").arg(selected.size());
    html += "<ul>";
    int count = 0;
    for (QListWidgetItem *item : selected) {
        if (++count > 20) {
            html += QString("<li>... and %1 more</li>").arg(selected.size() - 20);
            break;
        }
        html += QString("<li><b>%1</b></li>").arg(item->text());
    }
    html += "</ul>";
    html += "<p style='color: #555;'>You can perform batch actions (Delete, Refresh) on these entries using the buttons on the left.</p>";
    m_infoBrowser->setHtml(html);
}

void CacheViewerDialog::onEditClicked()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (!item) return;

    QString group = item->text();
    QrzCache qrzCache;

    qrzCache.beginGroup(group);
    QByteArray imgData = qrzCache.value("imgData").toByteArray();
    bool lotw = qrzCache.value("lotw", false).toBool();
    bool mqsl = qrzCache.value("mqsl", false).toBool();

    QDialog dlg(this);
    dlg.setWindowTitle("Edit Cache Entry for " + group);
    dlg.resize(480, 520);
    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);

    // Image preview at the top (left justified)
    QLabel *imgLabel = new QLabel(&dlg);
    imgLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
    auto updatePreview = [&]() {
        if (!imgData.isEmpty()) {
            QImage img = QImage::fromData(imgData).scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            imgLabel->setPixmap(QPixmap::fromImage(img));
        } else {
            imgLabel->setText("No Image Available");
            imgLabel->setAlignment(Qt::AlignCenter);
            imgLabel->setFixedSize(120, 120);
        }
    };
    updatePreview();

    QHBoxLayout *imgTopLayout = new QHBoxLayout();
    imgTopLayout->addWidget(imgLabel);
    imgTopLayout->addStretch();
    mainLayout->addLayout(imgTopLayout);

    // Below, a table / form of fields
    QScrollArea *scrollArea = new QScrollArea(&dlg);
    scrollArea->setWidgetResizable(true);
    QWidget *scrollWidget = new QWidget(scrollArea);
    QFormLayout *formLayout = new QFormLayout(scrollWidget);

    QLineEdit *leFname = new QLineEdit(qrzCache.value("qrz_fname").toString(), &dlg);
    QLineEdit *leLname = new QLineEdit(qrzCache.value("qrz_name").toString(), &dlg);
    QLineEdit *leAddr1 = new QLineEdit(qrzCache.value("qrz_addr1").toString(), &dlg);
    QLineEdit *leAddr2 = new QLineEdit(qrzCache.value("qrz_addr2").toString(), &dlg);
    QLineEdit *leCity = new QLineEdit(qrzCache.value("qrz_city").toString(), &dlg);
    QLineEdit *leState = new QLineEdit(qrzCache.value("qrz_state").toString(), &dlg);
    QLineEdit *leZip = new QLineEdit(qrzCache.value("qrz_zip").toString(), &dlg);
    QLineEdit *leCountry = new QLineEdit(qrzCache.value("country", qrzCache.value("qrz_country")).toString(), &dlg);
    QLineEdit *leGrid = new QLineEdit(qrzCache.value("grid").toString(), &dlg);
    QLineEdit *leLat = new QLineEdit(qrzCache.value("lat").toString(), &dlg);
    QLineEdit *leLon = new QLineEdit(qrzCache.value("lon").toString(), &dlg);
    QLineEdit *leQslmgr = new QLineEdit(qrzCache.value("qslmgr").toString(), &dlg);
    
    QCheckBox *cbLotw = new QCheckBox("LoTW User Status", &dlg);
    cbLotw->setChecked(lotw);
    QCheckBox *cbMqsl = new QCheckBox("eQSL/Mail Status", &dlg);
    cbMqsl->setChecked(mqsl);

    formLayout->addRow("First Name:", leFname);
    formLayout->addRow("Last Name/Name:", leLname);
    formLayout->addRow("Address 1:", leAddr1);
    formLayout->addRow("Address 2:", leAddr2);
    formLayout->addRow("City:", leCity);
    formLayout->addRow("State:", leState);
    formLayout->addRow("Zip/Postcode:", leZip);
    formLayout->addRow("Country:", leCountry);
    formLayout->addRow("Grid Square:", leGrid);
    formLayout->addRow("Latitude:", leLat);
    formLayout->addRow("Longitude:", leLon);
    formLayout->addRow("QSL Manager:", leQslmgr);
    formLayout->addRow("LoTW Status:", cbLotw);
    formLayout->addRow("eQSL/Mail Status:", cbMqsl);

    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);

    // Image operations
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnLocal = new QPushButton("From Local File", &dlg);
    QPushButton *btnUrl = new QPushButton("From URL", &dlg);
    QPushButton *btnRemoveImg = new QPushButton("Remove Image", &dlg);
    btnLayout->addWidget(btnLocal);
    btnLayout->addWidget(btnUrl);
    btnLayout->addWidget(btnRemoveImg);
    mainLayout->addLayout(btnLayout);

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    mainLayout->addWidget(bbox);

    connect(bbox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bbox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    connect(btnLocal, &QPushButton::clicked, [&]() {
        QString path = QFileDialog::getOpenFileName(&dlg, "Select Image", "", "Images (*.png *.jpg *.jpeg *.svg)");
        if (path.isEmpty()) return;
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            imgData = file.readAll();
            file.close();
            updatePreview();
            QMessageBox::information(&dlg, "Success", "Local image loaded into dialog buffer.");
        }
    });

    connect(btnUrl, &QPushButton::clicked, [&]() {
        bool ok;
        QString urlStr = QInputDialog::getText(&dlg, "URL", "Enter image URL:", QLineEdit::Normal, "", &ok);
        if (!ok || urlStr.isEmpty()) return;
        QNetworkAccessManager manager;
        QNetworkRequest req((QUrl(urlStr)));
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        req.setHeader(QNetworkRequest::UserAgentHeader, "Qwain/8.2");
        QNetworkReply *reply = manager.get(req);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() == QNetworkReply::NoError) {
            imgData = reply->readAll();
            updatePreview();
            QMessageBox::information(&dlg, "Success", "URL image loaded into dialog buffer.");
        } else {
            QMessageBox::warning(&dlg, "Error", "Failed to download image: " + reply->errorString());
        }
        reply->deleteLater();
    });

    connect(btnRemoveImg, &QPushButton::clicked, [&]() {
        imgData.clear();
        updatePreview();
        QMessageBox::information(&dlg, "Success", "Image removed from dialog buffer.");
    });

    if (dlg.exec() == QDialog::Accepted) {
        // Regenerate htmlInfo based on edited fields
        QString finalName = leLname->text().trimmed();
        QString finalCountry = leCountry->text().trimmed();
        QString finalCity = leCity->text().trimmed();
        QString finalState = leState->text().trimmed();
        QString finalZip = leZip->text().trimmed();
        QString finalAddr1 = leAddr1->text().trimmed();
        QString finalAddr2 = leAddr2->text().trimmed();
        QString finalGrid = leGrid->text().trimmed();

        QString cityState = (!finalCity.isEmpty() && !finalState.isEmpty()) ? QString("%1, %2").arg(finalCity, finalState) : finalCity + finalState;
        if (!finalZip.isEmpty()) cityState += " " + finalZip;

        QString addressLine = !finalAddr1.isEmpty() ? finalAddr1 : finalAddr2;

        QString htmlInfo = QString("<h2>%1</h2><table border='0' cellspacing='0' cellpadding='2'>"
                                   "<tr><td align='right'><b>Name:</b></td><td align='left'>%2</td></tr>"
                                   "<tr><td align='right'><b>Address:</b></td><td align='left'>%3</td></tr>"
                                   "<tr><td align='right'><b>City:</b></td><td align='left'>%4</td></tr>"
                                   "<tr><td align='right'><b>Country:</b></td><td align='left'>%5</td></tr>"
                                   "</table>")
                               .arg(group.toHtmlEscaped(), finalName.toHtmlEscaped(), addressLine.toHtmlEscaped(), cityState.toHtmlEscaped(), finalCountry.toHtmlEscaped());

        if (!finalGrid.isEmpty()) {
            htmlInfo += QString("<br><b>Grid:</b> %1").arg(finalGrid.toHtmlEscaped());
        }

        qrzCache.setValue("htmlInfo", sanitizeHtmlInfo(htmlInfo));
        qrzCache.setValue("lotw", cbLotw->isChecked());
        qrzCache.setValue("mqsl", cbMqsl->isChecked());
        qrzCache.setValue("imgData", imgData);
        
        qrzCache.setValue("qrz_fname", leFname->text().trimmed());
        qrzCache.setValue("qrz_name", finalName);
        qrzCache.setValue("qrz_addr1", finalAddr1);
        qrzCache.setValue("qrz_addr2", finalAddr2);
        qrzCache.setValue("qrz_city", finalCity);
        qrzCache.setValue("qrz_state", finalState);
        qrzCache.setValue("qrz_zip", finalZip);
        qrzCache.setValue("country", finalCountry);
        qrzCache.setValue("qrz_country", finalCountry);
        qrzCache.setValue("grid", finalGrid);
        qrzCache.setValue("lat", leLat->text().trimmed());
        qrzCache.setValue("lon", leLon->text().trimmed());
        qrzCache.setValue("qslmgr", leQslmgr->text().trimmed());

        qrzCache.sync();
        qrzCache.endGroup();
        onCallsignSelected(group);
    } else {
        qrzCache.endGroup();
    }
}

void CacheViewerDialog::onAnchorClicked(const QUrl &url)
{
    Q_UNUSED(url);
}

void CacheViewerDialog::onRebuildClicked() {
    QWidget *p = parentWidget();
    MainWindow *mw = qobject_cast<MainWindow*>(p);
    if (!mw) {
        QMessageBox::warning(this, tr("Error"), tr("Could not find main window to access QSO logs."));
        return;
    }

    // 1. Ask for confirmation
    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Rebuild Cache"),
        tr("Warning: Rebuilding will delete your existing local qrz_cache.db, re-download your full QRZ logbook if credentials are set, and rebuild your local cache. Proceed?"),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    // Log Start
    mw->logSystem("[Cache Rebuild] Rebuild initiated by user.");

    // 2. Delete qrz_cache.db
    QString dbPath = getAppDataDir() + "/qrz_cache.db";
    {
        QSqlDatabase db = QSqlDatabase::database("qrz_cache_db", false);
        if (db.isOpen()) db.close();
    }
    QSqlDatabase::removeDatabase("qrz_cache_db");
    
    bool dbDeleted = false;
    if (QFile::exists(dbPath)) {
        dbDeleted = QFile::remove(dbPath);
    } else {
        dbDeleted = true; // didn't exist anyway
    }

    mw->logSystem(QString("[Cache Rebuild] Database file '%1' deletion status: %2")
        .arg(dbPath).arg(dbDeleted ? "Success" : "Failed or already deleted"));

    // 3. Re-download a full logfile
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString apiKey = settings.value("Credentials/qrzApiKey", "").toString();
    QString logfilePath = mw->logfilePath();

    if (apiKey.isEmpty() || logfilePath.isEmpty()) {
        mw->logSystem("[Cache Rebuild] Sync skipped: No QRZ API Key or active logfile path set in Settings. Proceeding with existing loaded log.");
    } else {
        mw->logSystem(QString("[Cache Rebuild] Initiating full QRZ Logbook sync to path: %1").arg(logfilePath));
        QrzSyncDialog syncDlg(this, apiKey, logfilePath, true);
        syncDlg.setSessionKey(mw->getQrzSessionKey());
        syncDlg.exec();
        if (syncDlg.syncSuccessful()) {
            mw->logSystem("[Cache Rebuild] QRZ Logbook full sync completed successfully. Consolidating records...");
            mw->prepareNewAdi(); // Consolidation & reload
        } else {
            mw->logSystem("[Cache Rebuild] QRZ Logbook sync failed or was canceled. Rebuilding with current loaded logbook.");
        }
    }

    // 4. Proceed with rebuild
    const QMap<QString, QList<QsoDetail>> &qsosMap = mw->allQsos();
    if (qsosMap.isEmpty()) {
        QMessageBox::warning(this, tr("Empty Log"), tr("No QSOs are currently loaded in Qwain. Rebuild aborted."));
        return;
    }

    // Create and configure our custom RebuildProgressDialog
    RebuildProgressDialog progress(this);
    progress.show();

    int count = 0;
    QrzCache qrzCache;

    for (auto it = qsosMap.begin(); it != qsosMap.end(); ++it) {
        QString callsign = it.key().trimmed().toUpper();
        if (callsign.isEmpty()) continue;

        // Update progress dialog
        progress.setProgress(count, qsosMap.size(), callsign);
        QCoreApplication::processEvents(); // Keep UI fully responsive and updated

        if (progress.wasCanceled) {
            break;
        }

        const QList<QsoDetail> &list = it.value();
        if (list.isEmpty()) continue;

        // Use first QSO where operator details are somewhat populated, otherwise fallback to first record
        QsoDetail qso = list.first();
        for (const QsoDetail &q : list) {
            if (!q.name.isEmpty() || !q.address.isEmpty() || !q.city.isEmpty()) {
                qso = q;
                break;
            }
        }

        // Split name into first and last name
        QString fname = "";
        QString lname = qso.name.trimmed();
        int spaceIdx = lname.indexOf(' ');
        if (spaceIdx != -1) {
            fname = lname.left(spaceIdx).trimmed();
            lname = lname.mid(spaceIdx + 1).trimmed();
        }

        QString cityState = (!qso.city.isEmpty() && !qso.state.isEmpty()) ? QString("%1, %2").arg(qso.city, qso.state) : qso.city + qso.state;
        if (!qso.zip.isEmpty()) cityState += " " + qso.zip;

        QString htmlInfo = QString("<h2>%1</h2><table border='0' cellspacing='0' cellpadding='2'>"
                                   "<tr><td align='right'><b>Name:</b></td><td align='left'>%2</td></tr>"
                                   "<tr><td align='right'><b>Address:</b></td><td align='left'>%3</td></tr>"
                                   "<tr><td align='right'><b>City:</b></td><td align='left'>%4</td></tr>"
                                   "<tr><td align='right'><b>Country:</b></td><td align='left'>%5</td></tr>"
                                   "</table>")
                               .arg(callsign.toHtmlEscaped(), qso.name.toHtmlEscaped(), qso.address.toHtmlEscaped(), cityState.toHtmlEscaped(), qso.country.toHtmlEscaped());

        if (!qso.grid.isEmpty()) htmlInfo += QString("<br><b>Grid:</b> %1").arg(qso.grid);

        qrzCache.beginGroup(callsign);
        qrzCache.setValue("htmlInfo", htmlInfo);
        qrzCache.setValue("imgData", QByteArray());
        qrzCache.setValue("lotw", qso.lotwRcvd);
        qrzCache.setValue("mqsl", qso.mqslRcvd);
        qrzCache.setValue("country", qso.country);
        qrzCache.setValue("county", qso.county);
        qrzCache.setValue("grid", qso.grid);
        qrzCache.setValue("dxcc", qso.dxcc);
        qrzCache.setValue("lat", qso.lat);
        qrzCache.setValue("lon", qso.lon);
        qrzCache.setValue("qrz_fname", fname);
        qrzCache.setValue("qrz_name", lname);
        qrzCache.setValue("qrz_addr1", qso.address);
        qrzCache.setValue("qrz_addr2", qso.addr2.isEmpty() ? cityState : qso.addr2);
        qrzCache.setValue("qrz_city", qso.city);
        qrzCache.setValue("qrz_state", qso.state);
        qrzCache.setValue("qrz_zip", qso.zip);
        qrzCache.setValue("qrz_country", qso.country);
        qrzCache.endGroup();
        count++;
    }
    
    qrzCache.sync();
    progress.progressBar->setValue(qsosMap.size());
    progress.close();

    // Log Rebuilt Total
    mw->logSystem(QString("[Cache Rebuild] Successfully rebuilt %1 contacts in qrz_cache.db.").arg(count));

    if (progress.wasCanceled) {
        QMessageBox::information(this, tr("Canceled"), tr("Cache rebuilding was canceled. Fully cached details for %1 callsigns.").arg(count));
    } else {
        QMessageBox::information(this, tr("Success"), tr("Successfully rebuilt and cached details for %1 callsigns!").arg(count));
    }

    populateList();

    // 5. Trigger autonomous background queue worker if checkbox is checked
    if (progress.updateDetailsCheckbox->isChecked() && !progress.wasCanceled) {
        onRefreshDataClicked();
    }
}

void CacheViewerDialog::onRefreshDataClicked() {
    QWidget *p = parentWidget();
    MainWindow *mw = qobject_cast<MainWindow*>(p);
    if (!mw) {
        QMessageBox::warning(this, tr("Error"), tr("Could not find main window to start background refresh."));
        return;
    }

    QStringList callsigns;
    for (int i = 0; i < m_listWidget->count(); ++i) {
        QString call = m_listWidget->item(i)->text();
        if (call != "icons") {
            callsigns << call;
        }
    }

    if (callsigns.isEmpty()) {
        QMessageBox::information(this, tr("Refresh Data"), tr("No callsigns available in the cache to refresh. Run a rebuild first."));
        return;
    }

    if (mw->isQrzQueueRunning()) {
        mw->stopQrzQueueWorker();
        QMessageBox::information(this, tr("Background Refresh"), tr("Background refresh process paused. You can resume it anytime."));
    } else if (mw->hasPausedQrzQueue()) {
        mw->resumeQrzQueue();
        QMessageBox::information(this, tr("Background Refresh"), tr("Background queue worker resumed successfully from where you left off!"));
    } else {
        mw->startQrzQueue(callsigns);
        QMessageBox::information(this, tr("Background Refresh"), tr("Background queue worker started successfully! Process running autonomously in the background. Feel free to close this dialog."));
    }
}
