// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "icontoolsdialog.h"
#include "qrzcache.h"
#include "prefetchflagsdialog.h"
#include "paths.h"
#include "mainwindow.h"
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
#include <QLineEdit>
#include <QSpinBox>

IconToolsDialog::IconToolsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Icon Tools"));
    resize(600, 420);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    QVBoxLayout *leftLayout = new QVBoxLayout();
    m_listWidget = new QListWidget(this);
    m_listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    leftLayout->addWidget(m_listWidget);

    QPushButton *deleteButton = new QPushButton(tr("Delete Icon"), this);
    leftLayout->addWidget(deleteButton);

    QPushButton *editButton = new QPushButton(tr("Edit Icon"), this);
    leftLayout->addWidget(editButton);
    
    QPushButton *dlFlagsButton = new QPushButton(tr("Pre-fetch Flags"), this);
    leftLayout->addWidget(dlFlagsButton);
    
    mainLayout->addLayout(leftLayout, 1);

    m_infoBrowser = new QTextBrowser(this);
    m_infoBrowser->setOpenLinks(false);
    mainLayout->addWidget(m_infoBrowser, 2);

    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &IconToolsDialog::onSelectionChanged);
    connect(deleteButton, &QPushButton::clicked, this, &IconToolsDialog::onDeleteClicked);
    connect(editButton, &QPushButton::clicked, this, &IconToolsDialog::onEditClicked);
    connect(dlFlagsButton, &QPushButton::clicked, this, &IconToolsDialog::onDownloadFlagsClicked);
    connect(m_infoBrowser, &QTextBrowser::anchorClicked, this, &IconToolsDialog::onAnchorClicked);

    populateList();
}

void IconToolsDialog::populateList()
{
    m_listWidget->clear();
    QrzCache qrzCache;
    qrzCache.beginGroup("ICONS");
    QStringList keys = qrzCache.childKeys();

    // Self-healing database scrub: if we find any leftover callsign fields inside the ICONS group, delete them!
    static const QStringList callsignFields = {
        "qrz_fname", "qrz_name", "qrz_addr1", "qrz_addr2", "qrz_city",
        "qrz_state", "qrz_zip", "country", "qrz_country", "grid",
        "lat", "lon", "qslmgr", "lotw", "mqsl", "htmlInfo", "imgData", "county", "dxcc"
    };

    bool needsReselect = false;
    for (const QString &field : callsignFields) {
        for (const QString &k : keys) {
            if (k.compare(field, Qt::CaseInsensitive) == 0) {
                qrzCache.remove(k);
                needsReselect = true;
            }
        }
    }

    if (needsReselect) {
        keys = qrzCache.childKeys();
    }
    qrzCache.endGroup();

    // Filter out _W and _H keys to only display raw icon entry keys
    QStringList iconKeys;
    for (const QString &key : keys) {
        if (!key.endsWith("_W") && !key.endsWith("_H")) {
            iconKeys << key;
        }
    }
    iconKeys.sort();
    m_listWidget->addItems(iconKeys);
}

void IconToolsDialog::onIconSelected(const QString &key)
{
    if (key.isEmpty()) {
        m_infoBrowser->clear();
        return;
    }

    QrzCache qrzCache;
    qrzCache.beginGroup("ICONS");
    QByteArray data = qrzCache.value(key).toByteArray();
    int w = qrzCache.value(key + "_W", -1).toInt();
    int h = qrzCache.value(key + "_H", 24).toInt();
    qrzCache.endGroup();

    QString html = QString("<h3>Icon: %1</h3>").arg(key);
    
    QString status;
    if (data.startsWith("empty")) {
        status = "<b style='color:#c0392b;'>FAILED/EMPTY: " + QString::fromUtf8(data) + "</b>";
    } else if (data.isEmpty()) {
        status = "<i style='color:#7f8c8d;'>Empty/No Image Data Saved</i>";
    } else {
        QImage img = QImage::fromData(data);
        if (img.isNull()) {
            status = "<b style='color:#c0392b;'>Error: Invalid or corrupt image data!</b>";
        } else {
            QString resName = "icon_view_" + key;
            m_infoBrowser->document()->addResource(QTextDocument::ImageResource, QUrl(resName), img);
            
            status = QString("<p><img src='%1'/></p>"
                             "<table border='0' cellspacing='0' cellpadding='4'>"
                             "<tr><td><b>Original Dimensions:</b></td><td>%2 x %3 px</td></tr>"
                             "<tr><td><b>Render Width (display):</b></td><td>%4 px</td></tr>"
                             "<tr><td><b>Render Height (display):</b></td><td>%5 px</td></tr>"
                             "<tr><td><b>Data Size:</b></td><td>%6 bytes</td></tr>"
                             "</table>")
                         .arg(resName)
                         .arg(img.width()).arg(img.height())
                         .arg(w == -1 ? "Auto Scale" : QString::number(w))
                         .arg(h)
                         .arg(data.size());
        }
    }
    
    html += status;
    m_infoBrowser->setHtml(html);
}

void IconToolsDialog::onDeleteClicked()
{
    QList<QListWidgetItem*> items = m_listWidget->selectedItems();
    if (items.isEmpty()) return;

    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Delete Icons"),
        tr("Are you sure you want to delete %1 selected icons from the graphics database?").arg(items.size()),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    QrzCache qrzCache;
    m_listWidget->blockSignals(true);
    qrzCache.beginGroup("ICONS");
    for (QListWidgetItem *item : items) {
        QString key = item->text();
        qrzCache.remove(key);
        qrzCache.remove(key + "_W");
        qrzCache.remove(key + "_H");
    }
    qrzCache.endGroup();
    m_listWidget->blockSignals(false);

    populateList();
    m_infoBrowser->clear();
}

void IconToolsDialog::onRefreshClicked()
{
    // No-op for icons (not bound to standard QRZ callsign lookups)
}

void IconToolsDialog::onSelectionChanged()
{
    QList<QListWidgetItem*> selected = m_listWidget->selectedItems();
    if (selected.isEmpty()) {
        m_infoBrowser->clear();
        return;
    }

    if (selected.size() == 1) {
        onIconSelected(selected.first()->text());
        return;
    }

    // Multiple selections
    QString html = QString("<h3>Selected %1 Icons</h3>").arg(selected.size());
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
    html += "<p style='color: #555;'>You can perform batch actions (Delete) on these icons using the buttons on the left.</p>";
    m_infoBrowser->setHtml(html);
}

void IconToolsDialog::onEditClicked()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (!item) return;

    QString key = item->text();
    editIcon(key);
}

void IconToolsDialog::onAnchorClicked(const QUrl &url)
{
    QString urlStr = url.toString();
    if (urlStr.startsWith("edit:")) {
        editIcon(urlStr.mid(5));
    }
}

void IconToolsDialog::onDownloadFlagsClicked()
{
    PrefetchFlagsDialog dlg(this);
    dlg.exec();
    populateList();
}

void IconToolsDialog::editIcon(const QString &key)
{
    QrzCache qrzCache;
    qrzCache.beginGroup("ICONS");
    QByteArray imgData = qrzCache.value(key).toByteArray();
    int w = qrzCache.value(key + "_W", -1).toInt();
    int h = qrzCache.value(key + "_H", 24).toInt();
    qrzCache.endGroup();

    QDialog dlg(this);
    dlg.setWindowTitle("Edit Icon: " + key);
    dlg.resize(350, 380);
    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);
    
    // Live preview inside edit dialog
    QLabel *imgLabel = new QLabel(&dlg);
    imgLabel->setFrameStyle(QFrame::Box | QFrame::Sunken);
    imgLabel->setAlignment(Qt::AlignCenter);
    imgLabel->setFixedSize(120, 120);
    
    auto updatePreview = [&]() {
        if (!imgData.isEmpty() && !imgData.startsWith("empty")) {
            QImage img = QImage::fromData(imgData);
            if (!img.isNull()) {
                imgLabel->setPixmap(QPixmap::fromImage(img.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            } else {
                imgLabel->setText("Invalid Image");
            }
        } else {
            imgLabel->setText("No Image/Empty");
        }
    };
    updatePreview();

    QHBoxLayout *imgTopLayout = new QHBoxLayout();
    imgTopLayout->addWidget(imgLabel);
    imgTopLayout->addStretch();
    mainLayout->addLayout(imgTopLayout);

    QLineEdit *leKey = new QLineEdit(&dlg);
    leKey->setText(key);
    mainLayout->addWidget(new QLabel("Icon Key (change to rename):"));
    mainLayout->addWidget(leKey);
    
    QSpinBox *sbW = new QSpinBox(&dlg);
    sbW->setRange(-1, 500);
    sbW->setValue(w);
    mainLayout->addWidget(new QLabel("Display Width (-1 = auto scale):"));
    mainLayout->addWidget(sbW);

    QSpinBox *sbH = new QSpinBox(&dlg);
    sbH->setRange(1, 500);
    sbH->setValue(h);
    mainLayout->addWidget(new QLabel("Display Height:"));
    mainLayout->addWidget(sbH);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnLocal = new QPushButton("From Local File", &dlg);
    QPushButton *btnUrl = new QPushButton("From URL", &dlg);
    QPushButton *btnRemoveImg = new QPushButton("Remove Icon Data", &dlg);
    btnLayout->addWidget(btnLocal);
    btnLayout->addWidget(btnUrl);
    btnLayout->addWidget(btnRemoveImg);
    mainLayout->addLayout(btnLayout);

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    mainLayout->addWidget(bbox);

    connect(bbox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bbox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    connect(btnLocal, &QPushButton::clicked, [&]() {
        QString path = QFileDialog::getOpenFileName(&dlg, "Select Icon File", "", "Icons (*.png *.svg *.jpg *.jpeg)");
        if (path.isEmpty()) return;
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            imgData = file.readAll();
            file.close();
            updatePreview();
            QMessageBox::information(&dlg, "Success", "Icon loaded into dialog buffer.");
        }
    });

    connect(btnUrl, &QPushButton::clicked, [&]() {
        bool ok;
        QString urlStr = QInputDialog::getText(&dlg, "URL", "Enter icon image URL:", QLineEdit::Normal, "", &ok);
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
            QMessageBox::information(&dlg, "Success", "URL icon loaded into dialog buffer.");
        } else {
            QMessageBox::warning(&dlg, "Error", "Failed to download icon: " + reply->errorString());
        }
        reply->deleteLater();
    });

    connect(btnRemoveImg, &QPushButton::clicked, [&]() {
        imgData.clear();
        updatePreview();
        QMessageBox::information(&dlg, "Success", "Icon data removed from dialog buffer.");
    });

    if (dlg.exec() == QDialog::Accepted) {
        QString newKey = leKey->text().trimmed();
        if (newKey.isEmpty()) return;

        qrzCache.beginGroup("ICONS");
        if (newKey != key) {
            qrzCache.remove(key);
            qrzCache.remove(key + "_W");
            qrzCache.remove(key + "_H");
        }
        qrzCache.setValue(newKey, imgData);
        qrzCache.setValue(newKey + "_W", sbW->value());
        qrzCache.setValue(newKey + "_H", sbH->value());
        qrzCache.endGroup();

        populateList();
        
        // Select the edited key
        for (int i = 0; i < m_listWidget->count(); ++i) {
            if (m_listWidget->item(i)->text() == newKey) {
                m_listWidget->setCurrentRow(i);
                break;
            }
        }
    }
}
