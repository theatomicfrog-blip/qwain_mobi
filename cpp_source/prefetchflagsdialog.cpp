// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "prefetchflagsdialog.h"
#include "qrzcache.h"
#include "dxclusterwidget.h"
#include "mainwindow.h"
#include "paths.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>
#include <QHeaderView>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDateTime>
#include <QImage>

// ============================================================================
// COMPONENT: PrefetchFlagsDialog
// ROLE: Bulk country flag asset synchronization and pre-fetching engine.
//       This component crawl-indexes worked QSO countries from local caches,
//       correlates names to ISO 3166 codes, presents a status-resolved grid,
//       and coordinates chained sequential async network downloads from FlagCDN.
// INVOKED BY: MainWindow (via "Pre-fetch Flags" action).
// PRODUCES: Populated flag caches in qrz_cache.ini under the 'icons' section.
// ============================================================================

// [ENTRY POINT: Prefetch Flags Dialog Constructor]
// Configures layout grids, instantiates network download managers, and triggers 
// the initial scan of cached worked countries and mapped ISO indexes.
PrefetchFlagsDialog::PrefetchFlagsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Pre-fetch Flags");
    resize(600, 600);
    
    m_netManager = new QNetworkAccessManager(this);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Table
    m_table = new QTableWidget(0, 3, this);
    m_table->setHorizontalHeaderLabels({"Country", "ISO code", "Cached Flag"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_table->verticalHeader()->setVisible(false);
    mainLayout->addWidget(m_table, 2);
    
    // Get Flags button
    m_btnGetFlags = new QPushButton("Get Flags", this);
    connect(m_btnGetFlags, &QPushButton::clicked, this, &PrefetchFlagsDialog::getSelectedFlags);
    mainLayout->addWidget(m_btnGetFlags);
    
    // Status text
    m_statusText = new QTextEdit(this);
    m_statusText->setReadOnly(true);
    mainLayout->addWidget(m_statusText, 1);
    
    scanCountries();
}

PrefetchFlagsDialog::~PrefetchFlagsDialog() {}

// [ENTRY POINT: Local Logbook Country Crawler]
// Queries static country registries and iterates local QrzCache callgroups to build a
// unique list of all worked countries, checking existing cached icons, and populating table cells.
void PrefetchFlagsDialog::scanCountries() {
    QrzCache qrzCache;
    
    QSet<QString> uniqueCountries;
    
    QList<QString> allMapped = MainWindow::getAllCountries();
    for (const QString &c : allMapped) {
        uniqueCountries.insert(c);
    }
    
    // We scan all callsigns to find countries
    QStringList groups = qrzCache.childGroups();
    for (const QString &group : groups) {
        if (group == "icons") continue;
        qrzCache.beginGroup(group);
        QString country = qrzCache.value("country").toString();
        if (country.isEmpty()) country = qrzCache.value("qrz_country").toString();
        qrzCache.endGroup();
        
        if (!country.isEmpty()) {
            uniqueCountries.insert(country);
        }
    }
    
    // Intelligent search fallback if they don't have many countries in cache
    m_countries = uniqueCountries.values();
    m_countries.sort();
    
    m_table->setRowCount(m_countries.size());
    
    qrzCache.beginGroup("icons");
    for (int i=0; i<m_countries.size(); ++i) {
        QString c = m_countries[i];
        QString iso = MainWindow::countryToIso(c);
        m_countryToIso[c] = iso;
        
        QTableWidgetItem *itemC = new QTableWidgetItem(c);
        QTableWidgetItem *itemI = new QTableWidgetItem(iso);
        
        QTableWidgetItem *itemF = new QTableWidgetItem();
        
        if (!iso.isEmpty()) {
            QByteArray flagData = qrzCache.value("flag_" + iso).toByteArray();
            if (!flagData.isEmpty() && !flagData.startsWith("empty")) {
                m_hasCache[c] = true;
                QImage img;
                if (img.loadFromData(flagData)) {
                    itemF->setData(Qt::DecorationRole, QPixmap::fromImage(img.scaledToHeight(20, Qt::SmoothTransformation)));
                    itemF->setText("Cached");
                }
            } else {
                m_hasCache[c] = false;
                itemF->setText("Not Cached");
            }
        } else {
            m_hasCache[c] = false;
            itemF->setText("Unknown ISO");
        }
        
        itemC->setFlags(itemC->flags() & ~Qt::ItemIsEditable);
        itemI->setFlags(itemI->flags() & ~Qt::ItemIsEditable);
        itemF->setFlags(itemF->flags() & ~Qt::ItemIsEditable);
        
        m_table->setItem(i, 0, itemC);
        m_table->setItem(i, 1, itemI);
        m_table->setItem(i, 2, itemF);
    }
    qrzCache.endGroup();
}

// [ENTRY POINT: Batch Pre-fetch Action Callback]
// Evaluates selected table rows. If no rows are highlighted, intelligently auto-selects all
// missing cache items with valid ISO codes, initializing the sequential download pipeline.
void PrefetchFlagsDialog::getSelectedFlags() {
    auto selectedItems = m_table->selectedItems();
    if (selectedItems.isEmpty()) {
        // Intelligently select/search if nothing is selected
        // We will select all countries that do NOT have a cached flag but DO have an ISO
        m_pendingCountries.clear();
        for (int i=0; i<m_table->rowCount(); ++i) {
            QString c = m_table->item(i, 0)->text();
            QString iso = m_countryToIso[c];
            if (!iso.isEmpty() && !m_hasCache[c]) {
                m_pendingCountries.append(c);
                m_table->item(i, 0)->setSelected(true);
            }
        }
        if (m_pendingCountries.isEmpty()) {
            m_statusText->append("No flags currently missing ISO. Please select manually if you still want to download.");
            return;
        } else {
            m_statusText->append(QString("Intelligently auto-selected %1 missing flags.").arg(m_pendingCountries.size()));
        }
    } else {
        QSet<QString> cSet;
        for (auto *item : selectedItems) {
            int row = item->row();
            cSet.insert(m_table->item(row, 0)->text());
        }
        m_pendingCountries = cSet.values();
    }
    
    // Filter out unknown ISOs
    QList<QString> validPendings;
    for (const QString &c : m_pendingCountries) {
        if (!m_countryToIso[c].isEmpty()) {
            validPendings.append(c);
        } else {
            m_statusText->append(QString("Skipping %1 (Unknown ISO)").arg(c));
        }
    }
    m_pendingCountries = validPendings;
    
    if (m_pendingCountries.isEmpty()) return;
    
    m_btnGetFlags->setEnabled(false);
    m_downloadIndex = 0;
    
    m_statusText->append(QString("Starting download of %1 flags...").arg(m_pendingCountries.size()));
    
    // Start downloading the first one
    onDownloadFinished(); // kick off
}

// [ENTRY POINT: Asynchronous Download Chain Router]
// Triggered on individual network download completions. Handles the response, writes downloaded flag
// bytes to the icons group in qrz_cache.ini, and immediately schedules the next pending country item.
void PrefetchFlagsDialog::onDownloadFinished() {
    // If reply was sent to this slot, process it
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        QString c = m_pendingCountries[m_downloadIndex - 1]; // because we incremented
        QString iso = m_countryToIso[c];
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            m_statusText->append(QString("SUCCESS: %1 (%2 bytes)").arg(c, QString::number(data.size())));
            
            QrzCache qrzCache;
            qrzCache.beginGroup("icons");
            qrzCache.setValue("flag_" + iso, data);
            qrzCache.endGroup();
            
            m_hasCache[c] = true;
        } else {
            m_statusText->append(QString("ERROR: %1 (%2)").arg(c, reply->errorString()));
        }
        reply->deleteLater();
        
        updateTableIcons();
    }
    
    if (m_downloadIndex < m_pendingCountries.size()) {
        QString nextC = m_pendingCountries[m_downloadIndex];
        QString iso = m_countryToIso[nextC];
        m_downloadIndex++;
        
        m_statusText->append(QString("Fetching: %1 (%2)...").arg(nextC, iso));
        
        m_currentDownloadStartMs = QDateTime::currentMSecsSinceEpoch();
        m_totalBytesReceived = 0;
        m_lastPrintMs = 0;
        
        QNetworkRequest req((QUrl(QString("https://flagcdn.com/w80/%1.png").arg(iso))));
        req.setHeader(QNetworkRequest::UserAgentHeader, "Qwain/8.2");
        QNetworkReply *nextReply = m_netManager->get(req);
        connect(nextReply, &QNetworkReply::finished, this, &PrefetchFlagsDialog::onDownloadFinished);
        connect(nextReply, &QNetworkReply::downloadProgress, this, &PrefetchFlagsDialog::onDownloadProgress);
    } else {
        m_statusText->append("All downloads finished!");
        m_btnGetFlags->setEnabled(true);
    }
}

// [ENTRY POINT: Network Download Progress Hook]
// Receives socket packet progress signals, throttling status printouts to a maximum of 
// once every 500ms, and calculates active transfer speeds in KB/s.
void PrefetchFlagsDialog::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (bytesTotal <= 0) return;
    
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastPrintMs > 500) { // Print every 500ms max
        m_lastPrintMs = now;
        
        double kbps = 0;
        qint64 elapsed = now - m_currentDownloadStartMs;
        if (elapsed > 0) {
            kbps = (double)bytesReceived / 1024.0 / ((double)elapsed / 1000.0);
        }
        
        m_statusText->append(QString("  -> Progress: %1 / %2 bytes (%3 KB/s)")
                                 .arg(bytesReceived).arg(bytesTotal).arg(QString::number(kbps, 'f', 1)));
    }
}

void PrefetchFlagsDialog::updateTableIcons() {
    QrzCache qrzCache;
    qrzCache.beginGroup("icons");
    
    for (int i=0; i<m_table->rowCount(); ++i) {
        QString c = m_table->item(i, 0)->text();
        QString iso = m_countryToIso[c];
        
        if (m_hasCache[c]) {
            QByteArray flagData = qrzCache.value("flag_" + iso).toByteArray();
            if (!flagData.isEmpty() && !flagData.startsWith("empty")) {
                m_table->item(i, 2)->setText("Cached");
                QImage img;
                if (img.loadFromData(flagData)) {
                    m_table->item(i, 2)->setData(Qt::DecorationRole, QPixmap::fromImage(img.scaledToHeight(20, Qt::SmoothTransformation)));
                }
            }
        } else {
            m_table->item(i, 2)->setText(iso.isEmpty() ? "Unknown ISO" : "Not Cached");
            m_table->item(i, 2)->setData(Qt::DecorationRole, QVariant());
        }
    }
    qrzCache.endGroup();
}
