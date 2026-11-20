// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#include <QMap>
#include <QTabWidget>
#include <QScrollArea>
#include <QCoreApplication>
#include "configdialog.h"
#include "mainwindow.h"
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QVariant>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QCheckBox>
#include <QComboBox>
#include <QFontComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QSettings>
#include <QTimeZone>
#include <QFileDialog>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTextEdit>
#include <QTextBrowser>
#include <QInputDialog>
#include <QApplication>
#include <QClipboard>
#include <QStyle>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QtMath>
#include <QDir>
#include <QTimer>
#include <QStandardPaths>
#include "paths.h"
#include <QToolTip>
#include <QShortcut>
#ifndef QT_NO_XMLPATTERNS
#include <QXmlSchema>
#include <QXmlSchemaValidator>
#endif
#include <QSplitter>
#include <QXmlStreamReader>
#include <QAbstractItemView>
#include <QTableWidget>
#include <QHeaderView>
#include <QSlider>

class ClickEventFilter : public QObject {
public:
    std::function<void()> onClick;
    ClickEventFilter(QObject* parent = nullptr) : QObject(parent) {}
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (onClick) onClick();
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

#include <QPainter>

class UnderDevelopmentOverlay : public QWidget {
public:
    UnderDevelopmentOverlay(QWidget *parent = nullptr) : QWidget(parent) {
        // Intercept mouse events so they don't reach underlying widgets
    }
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        // 25% gray, 75% transparent (alpha 64/255)
        painter.fillRect(rect(), QColor(64, 64, 64, 64));
        
        painter.setRenderHint(QPainter::Antialiasing);
        painter.translate(rect().center());
        painter.rotate(-30); // diagonally
        QFont font("Impact", 48, QFont::Bold);
        font.setStyleStrategy(QFont::PreferAntialias);
        painter.setFont(font);
        painter.setPen(QColorConstants::Svg::yellow);
        painter.drawText(QRect(-width(), -height(), width()*2, height()*2), Qt::AlignCenter, "Under Development");
    }
    void mousePressEvent(QMouseEvent *event) override { event->accept(); }
    void mouseReleaseEvent(QMouseEvent *event) override { event->accept(); }
    void mouseDoubleClickEvent(QMouseEvent *event) override { event->accept(); }
    void mouseMoveEvent(QMouseEvent *event) override { event->accept(); }
};

ConfigDialog::ConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    m_netManager = new QNetworkAccessManager(this);
    setWindowTitle(tr("Configure User Interface"));
    setMinimumSize(600, 450);
    resize(960, 620);

    m_contentsWidget = new QListWidget(this);
    m_contentsWidget->setViewMode(QListView::ListMode);
    m_contentsWidget->setIconSize(QSize(32, 32));
    m_contentsWidget->setMovement(QListView::Static);
    m_contentsWidget->setMaximumWidth(200);
    m_contentsWidget->setSpacing(5);

    m_pagesWidget = new QStackedWidget(this);

    createNetworkPage();
    createCredentialsPage();
    createFeedsPage();
    createLoggingPage();
    createBackupPage();
    createUIPage();
    createPrintersPage();
    createQslCardOptionsPage();
    createStationPage();
    // Debug Information page removed per user request

    createIcons();
    m_contentsWidget->setCurrentRow(0);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &ConfigDialog::applySettings);

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(m_contentsWidget);
    horizontalLayout->addWidget(m_pagesWidget, 1);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(horizontalLayout);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
}

void ConfigDialog::createIcons()
{
    QListWidgetItem *networkItem = new QListWidgetItem(m_contentsWidget);
    networkItem->setText(tr("Network Configuration"));
    networkItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *credItem = new QListWidgetItem(m_contentsWidget);
    credItem->setText(tr("Credentials"));
    credItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *feedsItem = new QListWidgetItem(m_contentsWidget);
    feedsItem->setText(tr("Feeds"));
    feedsItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *logItem = new QListWidgetItem(m_contentsWidget);
    logItem->setText(tr("Logging"));
    logItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *backupItem = new QListWidgetItem(m_contentsWidget);
    backupItem->setText(tr("Backup/Restore"));
    backupItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *uiItem = new QListWidgetItem(m_contentsWidget);
    uiItem->setText(tr("User Interface"));
    uiItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *printerItem = new QListWidgetItem(m_contentsWidget);
    printerItem->setText(tr("Printers"));
    printerItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *qslOptionsItem = new QListWidgetItem(m_contentsWidget);
    qslOptionsItem->setText(tr("QSL Card Options"));
    qslOptionsItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *stationItem = new QListWidgetItem(m_contentsWidget);
    stationItem->setText(tr("Station Information"));
    stationItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    // Note: Debug Information sidebar item removed per user request

    connect(m_contentsWidget, &QListWidget::currentItemChanged, this, &ConfigDialog::changePage);
}

void ConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;
    m_pagesWidget->setCurrentIndex(m_contentsWidget->row(current));
}

void ConfigDialog::createNetworkPage()
{
    QWidget *page = new QWidget;
    QFormLayout *layout = new QFormLayout(page);
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    
    m_protocolCombo = new QComboBox;
    m_protocolCombo->addItem("TCP");
    m_protocolCombo->addItem("UDP");
    m_protocolCombo->addItem("BOTH (TCP & UDP)");
    m_protocolCombo->setCurrentText(settings.value("Network/protocol", "TCP").toString());
    
    m_ipEdit = new QLineEdit(settings.value("Network/ip", "127.0.0.1").toString());
    m_portSpin = new QSpinBox();
    m_portSpin->setRange(1, 65535);
    m_portSpin->setValue(settings.value("Network/port", 2345).toInt());
    
    layout->addRow(tr("Protocol:"), m_protocolCombo);
    layout->addRow(tr("Listen IP Address:"), m_ipEdit);
    layout->addRow(tr("Listen Port:"), m_portSpin);
    
    m_pagesWidget->addWidget(page);
}

void ConfigDialog::createCredentialsPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    
    QFormLayout *topLayout = new QFormLayout;
    // We instantiate m_stationCallsign here now instead of createStationPage
    m_stationCallsign = new QLineEdit(settings.value("Station/Callsign", "N0CALL").toString());
    m_gridEdit = new QLineEdit(settings.value("Station/Grid", "EM73").toString());
    m_configDirEdit = new QLineEdit(settings.fileName());
    m_configDirEdit->setReadOnly(true);
    
    QHBoxLayout *configFileLayout = new QHBoxLayout;
    configFileLayout->addWidget(m_configDirEdit);
    QPushButton *browseConfigFileBtn = new QPushButton(tr("Browse..."), this);
    configFileLayout->addWidget(browseConfigFileBtn);
    
    topLayout->addRow(tr("Callsign:"), m_stationCallsign);
    topLayout->addRow(tr("Grid:"), m_gridEdit);
    topLayout->addRow(tr("Configuration file:"), configFileLayout);

    QHBoxLayout *confButtonsLayout = new QHBoxLayout;
    QPushButton *backupConfBtn = new QPushButton(tr("Backup Conf"), this);
    QPushButton *restoreConfBtn = new QPushButton(tr("Restore Conf"), this);
    confButtonsLayout->addWidget(backupConfBtn);
    confButtonsLayout->addWidget(restoreConfBtn);
    topLayout->addRow(tr(""), confButtonsLayout);

    connect(backupConfBtn, &QPushButton::clicked, this, [this](){
        applySettings();
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        settings.sync();
        QString srcFile = settings.fileName();
        if (srcFile.isEmpty() || !QFile::exists(srcFile)) {
            srcFile = m_configDirEdit->text();
        }
        if (!srcFile.isEmpty() && QFile::exists(srcFile)) {
            QString backupPath = srcFile + ".bak";
            if (QFile::exists(backupPath)) QFile::remove(backupPath);
            if (QFile::copy(srcFile, backupPath)) {
                QMessageBox::information(this, tr("Success"), tr("Configuration backed up successfully to:\n%1").arg(backupPath));
            } else {
                QMessageBox::warning(this, tr("Error"), tr("Failed to copy configuration file to:\n%1").arg(backupPath));
            }
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Configuration file does not exist on disk yet."));
        }
    });

    connect(restoreConfBtn, &QPushButton::clicked, this, [this](){
        QString path = QFileInfo(m_configDirEdit->text()).absolutePath();
        QFileDialog dialog(this, tr("Select Configuration to Restore"), path, "Config Files (*.conf *.ini);;All Files (*)");
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setFilter(dialog.filter() | QDir::Hidden);
        if (dialog.exec() == QDialog::Accepted) {
            QString file = dialog.selectedFiles().first();
            QMessageBox::StandardButton reply = QMessageBox::question(this, "Restore", QString("OK to Apply Configuration %1?").arg(file), QMessageBox::Ok | QMessageBox::Cancel);
            if (reply == QMessageBox::Ok) {
                QSettings importedSettings(file, QSettings::IniFormat);
                QSettings settings(QCoreApplication::organizationName(), "Qwain");
                settings.clear();
                for (const QString &key : importedSettings.allKeys()) {
                    settings.setValue(key, importedSettings.value(key));
                }
                settings.sync();
                
                m_stationCallsign->setText(settings.value("Station/Callsign", "N0CALL").toString());
                m_gridEdit->setText(settings.value("Station/Grid", "EM73").toString());
                
                auto updateList = [](QListWidget *list, const QStringList &vals) {
                    list->clear();
                    list->addItems(vals);
                };
                updateList(m_stationRadios, settings.value("Station/Radios").toStringList());
                updateList(m_stationAntennas, settings.value("Station/Antennas").toStringList());
                updateList(m_stationBands, settings.value("Station/Bands").toStringList());
                updateList(m_stationChecks, settings.value("Station/Checks").toStringList());
                updateList(m_stationAdifTags, settings.value("Station/AdifTags").toStringList());

                m_protocolCombo->setCurrentText(settings.value("Network/protocol", "UDP").toString());
                m_ipEdit->setText(settings.value("Network/ip", "127.0.0.1").toString());
                m_portSpin->setValue(settings.value("Network/port", 2224).toInt());
                
                m_qrzUserEdit->setText(settings.value("Credentials/username", "").toString());
                m_qrzPassEdit->setText(settings.value("Credentials/password", "").toString());
                m_qrzApiKeyEdit->setText(settings.value("Credentials/qrzApiKey", "").toString());
                m_qrzXmlSubCheckbox->setChecked(settings.value("Credentials/qrzXmlSubscriber", false).toBool());
                m_arrlMemberCheckbox->setChecked(settings.value("Credentials/arrlMember", false).toBool());
                m_lotwUserEdit->setText(settings.value("Credentials/lotwUser", "").toString());
                m_lotwPassEdit->setText(settings.value("Credentials/lotwPass", "").toString());
                m_clublogEmailEdit->setText(settings.value("Credentials/clublogEmail", "").toString());
                m_clublogCallEdit->setText(settings.value("Credentials/clublogCall", "").toString());
                m_clublogPassEdit->setText(settings.value("Credentials/clublogPass", "").toString());
                m_clublogApiKeyEdit->setText(settings.value("Credentials/clublogApiKey", "").toString());
                m_eqslUserEdit->setText(settings.value("Credentials/eqslUser", "").toString());
                m_eqslPassEdit->setText(settings.value("Credentials/eqslPass", "").toString());
                
                m_feed1Check->setChecked(settings.value("Feeds/feed1Enabled", false).toBool());
                m_feed1UrlEdit->setText(settings.value("Feeds/feed1Url", "").toString());
                m_feed1RefreshCombo->setCurrentText(settings.value("Feeds/feed1Refresh", "30 Minutes").toString());
                m_feed2Check->setChecked(settings.value("Feeds/feed2Enabled", false).toBool());
                m_feed2UrlEdit->setText(settings.value("Feeds/feed2Url", "").toString());
                m_feed2RefreshCombo->setCurrentText(settings.value("Feeds/feed2Refresh", "30 Minutes").toString());
                m_feed3Check->setChecked(settings.value("Feeds/feed3Enabled", false).toBool());
                m_feed3UrlEdit->setText(settings.value("Feeds/feed3Url", "").toString());
                m_feed3RefreshCombo->setCurrentText(settings.value("Feeds/feed3Refresh", "30 Minutes").toString());
                
                m_dxClusterCombo->setCurrentText(settings.value("Feeds/dxClusterEnabled", "Disabled").toString());
                m_dxClusterEdit->setText(settings.value("Feeds/dxClusterHost", "dxc.nc7j.com:7373").toString());
                

                
                m_logfileEdit->setText(settings.value("Paths/Logfile", "qwain.adif").toString());
                m_userDirectoryEdit->setText(settings.value("Paths/UserDirectory", "").toString());
                m_systemLogfileEdit->setText(settings.value("Paths/SystemLog", "qwain_system.log").toString());
                m_debugLogfileEdit->setText(settings.value("Paths/DebugLog", "qwain_debug.log").toString());
                
                m_scrollDirCombo1->setCurrentText(settings.value("UI/feed1Dir", "Down").toString());
                m_scrollSpeedSlider1->setValue(settings.value("UI/feed1Speed", 5).toInt());
                m_scrollDirCombo2->setCurrentText(settings.value("UI/feed2Dir", "Down").toString());
                m_scrollSpeedSlider2->setValue(settings.value("UI/feed2Speed", 5).toInt());
                m_scrollDirCombo3->setCurrentText(settings.value("UI/feed3Dir", "Down").toString());
                m_scrollSpeedSlider3->setValue(settings.value("UI/feed3Speed", 5).toInt());
                
                m_statusScrollLimit->setValue(settings.value("UI/StatusScrollLimit", 50).toInt());
                m_mapStyleCombo->setCurrentText(settings.value("UI/MapStyle", "Dark Map").toString());
                
                auto loadFontColor = [&](QFont &f, QColor &c, const QString &fkey, const QString &ckey, QFont defFont, QColor defColor) {
                    f = qvariant_cast<QFont>(settings.value(fkey, defFont));
                    c = qvariant_cast<QColor>(settings.value(ckey, defColor));
                };
                loadFontColor(m_clockFont, m_clockColor, "UI/clockFont", "UI/clockColor", QFont("Arial", 10), Qt::white);
                loadFontColor(m_feed1Font, m_feed1Color, "UI/feed1Font", "UI/feed1Color", QFont("Arial", 10), Qt::white);
                loadFontColor(m_feed2Font, m_feed2Color, "UI/feed2Font", "UI/feed2Color", QFont("Arial", 10), Qt::white);
                loadFontColor(m_feed3Font, m_feed3Color, "UI/feed3Font", "UI/feed3Color", QFont("Arial", 10), Qt::white);
                loadFontColor(m_callInfoFont, m_callInfoColor, "UI/callInfoFont", "UI/callInfoColor", QFont("Arial", 10), Qt::white);
                loadFontColor(m_qsoListFont, m_qsoListColor, "UI/qsoListFont", "UI/qsoListColor", QFont("Arial", 10), Qt::white);

                if (settings.contains("SkinColor")) {
                    m_selectedColor = settings.value("SkinColor").toString();
                    m_currentColorLabel->setStyleSheet(QString("background-color: %1; border: 1px solid black;").arg(m_selectedColor));
                }
                
                for(auto &cb : m_styleLabelRefreshers) cb();
                
                if (m_uiPreview) updateUIPreview();
                applySettings();
                QMessageBox::information(this, "Configuration Restored", "Configuration has been restored and applied.");
            }
        }
    });

    connect(browseConfigFileBtn, &QPushButton::clicked, this, [this](){
        QString path = QFileInfo(m_configDirEdit->text()).absolutePath();
        QFileDialog dialog(this, tr("Select Configuration File"), path);
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setFilter(dialog.filter() | QDir::Hidden);
        if (dialog.exec() == QDialog::Accepted) {
            m_configDirEdit->setText(dialog.selectedFiles().first());
        }
    });
    
    layout->addLayout(topLayout);
    
    QFrame *line1 = new QFrame; line1->setFrameShape(QFrame::HLine); line1->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line1);
    
    QFormLayout *qrzCredForm = new QFormLayout;
    m_qrzUserEdit = new QLineEdit(settings.value("Credentials/username", "").toString());
    m_qrzPassEdit = new QLineEdit(settings.value("Credentials/password", "").toString());
    m_qrzPassEdit->setEchoMode(QLineEdit::Password);
    qrzCredForm->addRow(tr("QRZ Username:"), m_qrzUserEdit);
    qrzCredForm->addRow(tr("QRZ Password:"), m_qrzPassEdit);
    layout->addLayout(qrzCredForm);
    
    m_qrzXmlSubCheckbox = new QCheckBox(tr("QRZ XML or Higher Subscriber?"), this);
    m_qrzXmlSubCheckbox->setChecked(settings.value("Credentials/qrzXmlSubscriber", false).toBool());
    layout->addWidget(m_qrzXmlSubCheckbox);
    
    QHBoxLayout *qrzLayout = new QHBoxLayout;
    m_qrzApiKeyEdit = new QLineEdit(settings.value("Credentials/qrzApiKey", "").toString());
    QPushButton *qrzTestBtn = new QPushButton(tr("Test Connection..."));
    qrzTestBtn->setAutoDefault(false);
    qrzLayout->addWidget(new QLabel(tr("QRZ API Key:")));
    qrzLayout->addWidget(m_qrzApiKeyEdit);
    qrzLayout->addWidget(qrzTestBtn);
    layout->addLayout(qrzLayout);
    connect(qrzTestBtn, &QPushButton::clicked, this, &ConfigDialog::testQrzLogin);
    
    QFrame *line2 = new QFrame; line2->setFrameShape(QFrame::HLine); line2->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line2);

    m_arrlMemberCheckbox = new QCheckBox(tr("Affiliated With A Bureau? (Enables Outgoing Bureau 'B' Cards)"), this);
    m_arrlMemberCheckbox->setChecked(settings.value("Credentials/arrlMember", false).toBool());
    layout->addWidget(m_arrlMemberCheckbox);
    
    QHBoxLayout *lotwLayout = new QHBoxLayout;
    m_lotwUserEdit = new QLineEdit(settings.value("Credentials/lotwUser", "").toString());
    m_lotwPassEdit = new QLineEdit(settings.value("Credentials/lotwPass", "").toString());
    m_lotwPassEdit->setEchoMode(QLineEdit::Password);
    QPushButton *lotwTestBtn = new QPushButton(tr("Test"));
    lotwTestBtn->setAutoDefault(false);
    lotwLayout->addWidget(new QLabel(tr("LoTW:")));
    lotwLayout->addWidget(new QLabel(tr("Username:")));
    lotwLayout->addWidget(m_lotwUserEdit);
    lotwLayout->addWidget(new QLabel(tr("Password:")));
    lotwLayout->addWidget(m_lotwPassEdit);
    lotwLayout->addWidget(lotwTestBtn);
    layout->addLayout(lotwLayout);
    
    QFrame *line3 = new QFrame; line3->setFrameShape(QFrame::HLine); line3->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line3);
    
    QVBoxLayout *clublogContainer = new QVBoxLayout;
    m_clublogEmailEdit = new QLineEdit(settings.value("Credentials/clublogEmail", "").toString());
    m_clublogCallEdit = new QLineEdit(settings.value("Credentials/clublogCall", "").toString());
    m_clublogPassEdit = new QLineEdit(settings.value("Credentials/clublogPass", "").toString());
    m_clublogPassEdit->setEchoMode(QLineEdit::Password);
    m_clublogApiKeyEdit = new QLineEdit(settings.value("Credentials/clublogApiKey", "").toString());
    QPushButton *clublogTestBtn = new QPushButton(tr("Test"));
    clublogTestBtn->setAutoDefault(false);

    QHBoxLayout *clublogRow1 = new QHBoxLayout;
    clublogRow1->addWidget(new QLabel(tr("Clublog:")));
    clublogRow1->addWidget(new QLabel(tr("E-Mail:")));
    clublogRow1->addWidget(m_clublogEmailEdit);
    clublogRow1->addWidget(new QLabel(tr("Call:")));
    clublogRow1->addWidget(m_clublogCallEdit);

    QHBoxLayout *clublogRow2 = new QHBoxLayout;
    clublogRow2->addWidget(new QLabel(tr("Password / App Pass:")));
    clublogRow2->addWidget(m_clublogPassEdit);
    clublogRow2->addWidget(new QLabel(tr("API Key:")));
    clublogRow2->addWidget(m_clublogApiKeyEdit);
    clublogRow2->addWidget(clublogTestBtn);

    clublogContainer->addLayout(clublogRow1);
    clublogContainer->addLayout(clublogRow2);
    layout->addLayout(clublogContainer);

    connect(clublogTestBtn, &QPushButton::clicked, this, [this]() {
        QString email = m_clublogEmailEdit->text().trimmed();
        QString call = m_clublogCallEdit->text().trimmed();
        QString pass = m_clublogPassEdit->text().trimmed();
        QString apiKey = m_clublogApiKeyEdit->text().trimmed();

        if (email.isEmpty() || call.isEmpty() || pass.isEmpty()) {
            QMessageBox::warning(this, tr("Club Log Test"),
                                 tr("Please enter Email, Callsign, and Password / Application Password first."));
            return;
        }

        QNetworkAccessManager *netManager = new QNetworkAccessManager(this);
        QUrl url("https://clublog.org/getadif.php");
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

        QUrlQuery postData;
        postData.addQueryItem("email", email);
        postData.addQueryItem("password", pass);
        postData.addQueryItem("call", call);
        if (!apiKey.isEmpty()) {
            postData.addQueryItem("api", apiKey);
        }
        postData.addQueryItem("type", "dxqsl");

        QNetworkReply *reply = netManager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
        connect(reply, &QNetworkReply::finished, this, [reply, netManager, this]() {
            reply->deleteLater();
            netManager->deleteLater();
            QString resp = QString::fromUtf8(reply->readAll());
            if (reply->error() != QNetworkReply::NoError && resp.contains("Invalid", Qt::CaseInsensitive)) {
                QMessageBox::warning(this, tr("Club Log Test Failed"),
                                     tr("Club Log test failed:\n%1").arg(resp.trimmed()));
            } else if (reply->error() != QNetworkReply::NoError) {
                QMessageBox::warning(this, tr("Club Log Test Error"),
                                     tr("Club Log connection error:\n%1").arg(reply->errorString()));
            } else {
                QMessageBox::information(this, tr("Club Log Test Success"),
                                         tr("Club Log connection test successful!\nCredentials and API Key accepted."));
            }
        });
    });
    
    QFrame *line4 = new QFrame; line4->setFrameShape(QFrame::HLine); line4->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line4);
    
    QHBoxLayout *eqslLayout = new QHBoxLayout;
    m_eqslUserEdit = new QLineEdit(settings.value("Credentials/eqslUser", "").toString());
    m_eqslPassEdit = new QLineEdit(settings.value("Credentials/eqslPass", "").toString());
    m_eqslPassEdit->setEchoMode(QLineEdit::Password);
    QPushButton *eqslTestBtn = new QPushButton(tr("Test"));
    eqslTestBtn->setAutoDefault(false);
    eqslLayout->addWidget(new QLabel(tr("eQSL.cc:")));
    eqslLayout->addWidget(new QLabel(tr("User:")));
    eqslLayout->addWidget(m_eqslUserEdit);
    eqslLayout->addWidget(new QLabel(tr("Password:")));
    eqslLayout->addWidget(m_eqslPassEdit);
    eqslLayout->addWidget(eqslTestBtn);
    layout->addLayout(eqslLayout);
    
    layout->addStretch();
    m_pagesWidget->addWidget(page);
}

void ConfigDialog::createFeedsPage()
{
    QWidget *page = new QWidget;
    QFormLayout *layout = new QFormLayout(page);
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");

    // Silently initialize the removed feed components to prevent breaking loading/saving logic elsewhere
    m_feed1Check = new QCheckBox; m_feed1UrlEdit = new QLineEdit; m_feed1RefreshCombo = new QComboBox;
    m_feed2Check = new QCheckBox; m_feed2UrlEdit = new QLineEdit; m_feed2RefreshCombo = new QComboBox;
    m_feed3Check = new QCheckBox; m_feed3UrlEdit = new QLineEdit; m_feed3RefreshCombo = new QComboBox;
    m_scrollDirCombo1 = new QComboBox; m_scrollSpeedSlider1 = new QSlider(Qt::Horizontal);
    m_scrollDirCombo2 = new QComboBox; m_scrollSpeedSlider2 = new QSlider(Qt::Horizontal);
    m_scrollDirCombo3 = new QComboBox; m_scrollSpeedSlider3 = new QSlider(Qt::Horizontal);

    QLabel *hdr = new QLabel("<b>DX Cluster Server Configuration</b>");
    layout->addRow(hdr);
    
    QWidget *dxClusterWidget = new QWidget;
    QVBoxLayout *dxLayout = new QVBoxLayout(dxClusterWidget);
    dxLayout->setContentsMargins(0, 0, 0, 0);
    
    QHBoxLayout *svrLayout = new QHBoxLayout;
    svrLayout->addWidget(new QLabel("DX Cluster Server:"));
    m_dxClusterEdit = new QLineEdit;
    m_dxClusterEdit->setText(settings.value("Feeds/dxClusterServer", "dx.ve7cc.net:23").toString());
    svrLayout->addWidget(m_dxClusterEdit);
    
    QPushButton *testDxBtn = new QPushButton("Test Connection");
    testDxBtn->setAutoDefault(false);
    svrLayout->addWidget(testDxBtn);
    
    connect(testDxBtn, &QPushButton::clicked, this, [this, testDxBtn]() {
        testDxBtn->setStyleSheet(""); // reset color
        QString serverConfig = m_dxClusterEdit->text();
        if (serverConfig.isEmpty()) return;
        
        QString host;
        int port = 7373;
        if (serverConfig.contains(":")) {
            QStringList parts = serverConfig.split(":");
            host = parts[0];
            port = parts[1].toInt();
        } else {
            host = serverConfig;
        }
        
        QTcpSocket *socket = new QTcpSocket(this);
        QString *trace = new QString;
        *trace += "Connecting to " + host + " on port " + QString::number(port) + "...\n";
        
        connect(socket, &QTcpSocket::stateChanged, this, [trace](QAbstractSocket::SocketState state) {
            switch(state) {
                case QAbstractSocket::HostLookupState: *trace += "Looking up host...\n"; break;
                case QAbstractSocket::ConnectingState: *trace += "Connecting...\n"; break;
                case QAbstractSocket::ConnectedState: *trace += "Connected!\n"; break;
                case QAbstractSocket::ClosingState: *trace += "Closing connection...\n"; break;
                case QAbstractSocket::UnconnectedState: *trace += "Unconnected.\n"; break;
                default: break;
            }
        });
        
        connect(socket, &QTcpSocket::connected, this, [socket, testDxBtn, trace]() {
            testDxBtn->setStyleSheet("background-color: green; color: white;");
            *trace += "Test connection successful!\n";
            QMessageBox::information(nullptr, "Connection Test", *trace);
            socket->disconnectFromHost();
            socket->deleteLater();
            delete trace;
        });
        
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(socket, &QTcpSocket::errorOccurred, this, [this, socket, testDxBtn, trace](QAbstractSocket::SocketError err) {
#else
        connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this, [this, socket, testDxBtn, trace](QAbstractSocket::SocketError err) {
#endif
            testDxBtn->setStyleSheet("background-color: red; color: white;");
            *trace += "Error: " + socket->errorString() + "\n";
            QMessageBox::critical(this, "Connection Error", *trace);
            socket->deleteLater();
            delete trace;
        });
        
        socket->connectToHost(host, port);
    });
    
    m_dxClusterCombo = new QComboBox; // kept for legacy reference if any

    // List of predefined servers for checking
    QStringList predefinedServers;
    predefinedServers << "dx.ve7cc.net:23 - VE7CC, British Columbia, Canada"
                      << "dxc.nc7j.com:7373 - NC7J, Utah, USA"
                      << "dxc.w9zrx.net:7373 - W9ZRX, Wisconsin, USA"
                      << "k3lr.com:7373 - K3LR, Pennsylvania, USA"
                      << "w3lpl.net:7373 - W3LPL, Maryland, USA"
                      << "dxc.pi4cc.nl:8000 - PI4CC, Netherlands"
                      << "gb7mbc.spoo.org:8000 - GB7MBC, United Kingdom"
                      << "cluster.dl9gtb.de:8000 - DL9GTB, Germany"
                      << "cluster.f5mzn.org:8000 - F5MZN, France"
                      << "cp2.net:7373 - CP2, South Africa"
                      << "cq.ndxc.org:7373 - NDXC, USA"
                      << "dxc.aa4vv.net:7373 - AA4VV, North Carolina, USA"
                      << "dxc.hb9hlm.ch:9000 - HB9HLM, Switzerland"
                      << "dxc.k0xrc.net:7373 - K0XRC, USA"
                      << "dxc.k1ttt.net:7373 - K1TTT, Massachusetts, USA"
                      << "dxc.k8sn.com:7373 - K8SN, Michigan, USA"
                      << "dxc.kx4i.com:7373 - KX4I, Georgia, USA"
                      << "dxc.n0hr.com:7373 - N0HR, Iowa, USA"
                      << "dxc.n4djt.com:7373 - N4DJT, Virginia, USA"
                      << "dxc.n4xh.net:7373 - N4XH, North Carolina, USA"
                      << "dxc.n9nwo.com:7373 - N9NWO, Indiana, USA"
                      << "dxc.w1fm.org:7373 - W1FM, USA"
                      << "gb7tf.spoo.org:8000 - GB7TF, United Kingdom"
                      << "gb7ujs.spoo.org:8000 - GB7UJS, United Kingdom"
                      << "ha6dx.ng3k.com:7373 - HA6DX, Hungary"
                      << "ik5pwj.dyndns.org:8000 - IK5PWJ, Italy"
                      << "k0gw.net:7373 - K0GW, USA"
                      << "k1rgo.net:7373 - K1RGO, USA"
                      << "lw8ar.no-ip.org:8000 - LW8AR, Argentina"
                      << "n1kdo.net:7373 - N1KDO, USA"
                      << "nb.ncdxc.org:7373 - NCDXC, California, USA"
                      << "sk0hb.se:8000 - SK0HB, Sweden"
                      << "sk3w.se:8000 - SK3W, Sweden"
                      << "wa9tt.org:7373 - WA9TT, USA"
                      << "yf1ar.net:8000 - YF1AR, Indonesia"
                      << "dxc.ncdxf.org:7373 - NCDXF, Northern California, USA"
                      << "dxc.ac5aa.com:7373 - AC5AA, Texas, USA"
                      << "dxc.ai9t.com:7373 - AI9T, Florida, USA"
                      << "dxc.k1ea.net:7373 - K1EA, Massachusetts, USA"
                      << "dxc.k1xx.com:7373 - K1XX, Hampshire, UK"
                      << "dxc.n2jg.com:7373 - N2JG, New Jersey, USA"
                      << "dxc.n8kxa.net:7373 - N8KXA, Ohio, USA"
                      << "dxc.w4doc.com:7373 - W4DOC, Georgia, USA"
                      << "dxc.w8wtd.com:7373 - W8WTD, Ohio, USA"
                      << "dxc.w9dxcc.com:7373 - W9DXCC, Illinois, USA"
                      << "pe1pqf.com:8000 - PE1PQF, Netherlands";

    dxLayout->addLayout(svrLayout);
    dxLayout->addWidget(new QLabel("<b>DX Clusters List:</b> (Double-click to set as active server)"));
    
    m_dxClusterListWidget = new QListWidget;
    QStringList savedServers = settings.value("Feeds/dxClusterList").toStringList();
    if (savedServers.isEmpty()) {
        savedServers = predefinedServers;
    }
    m_dxClusterListWidget->addItems(savedServers);
    dxLayout->addWidget(m_dxClusterListWidget);

    QHBoxLayout *removeLayout = new QHBoxLayout;
    m_btnRemoveCluster = new QPushButton("Remove Selected DX Cluster");
    m_btnRemoveCluster->setAutoDefault(false);
    m_btnRemoveCluster->setEnabled(false);
    removeLayout->addStretch();
    removeLayout->addWidget(m_btnRemoveCluster);
    dxLayout->addLayout(removeLayout);

    // Group box to add a custom cluster
    QGroupBox *addGroup = new QGroupBox("Add Custom DX Cluster");
    QFormLayout *addForm = new QFormLayout(addGroup);
    
    m_clusterIpEdit = new QLineEdit;
    m_clusterIpEdit->setPlaceholderText("e.g. dxc.k1ttt.net");
    
    m_clusterPortSpin = new QSpinBox;
    m_clusterPortSpin->setRange(1, 65535);
    m_clusterPortSpin->setValue(7373);
    
    m_clusterStationIdEdit = new QLineEdit;
    m_clusterStationIdEdit->setPlaceholderText("e.g. K1TTT");
    
    m_clusterCommentEdit = new QLineEdit;
    m_clusterCommentEdit->setPlaceholderText("e.g. Massachusetts, USA");
    
    m_btnAddCluster = new QPushButton("Add DX Cluster");
    m_btnAddCluster->setAutoDefault(false);
    
    addForm->addRow("IP/FQDN:", m_clusterIpEdit);
    addForm->addRow("Port:", m_clusterPortSpin);
    addForm->addRow("Station ID:", m_clusterStationIdEdit);
    addForm->addRow("User Comment:", m_clusterCommentEdit);
    addForm->addRow("", m_btnAddCluster);
    
    dxLayout->addWidget(addGroup);

    // Double-click sets active server
    connect(m_dxClusterListWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        if (!item) return;
        QString s = item->text();
        int idx = s.indexOf(" - ");
        if (idx > 0) s = s.left(idx);
        m_dxClusterEdit->setText(s);
    });

    // Single-click selection controls Remove button
    auto updateRemoveButton = [this, predefinedServers]() {
        QListWidgetItem *item = m_dxClusterListWidget->currentItem();
        if (!item) {
            m_btnRemoveCluster->setEnabled(false);
            return;
        }
        QString txt = item->text();
        bool isPredefined = predefinedServers.contains(txt);
        m_btnRemoveCluster->setEnabled(!isPredefined);
    };

    connect(m_dxClusterListWidget, &QListWidget::itemClicked, this, updateRemoveButton);
    connect(m_dxClusterListWidget, &QListWidget::currentItemChanged, this, updateRemoveButton);
    connect(m_dxClusterListWidget, &QListWidget::itemSelectionChanged, this, updateRemoveButton);

    // Remove action
    connect(m_btnRemoveCluster, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = m_dxClusterListWidget->currentItem();
        if (!item) return;
        delete item;
        m_btnRemoveCluster->setEnabled(false);
    });

    // Add action
    connect(m_btnAddCluster, &QPushButton::clicked, this, [this]() {
        QString ip = m_clusterIpEdit->text().trimmed();
        if (ip.isEmpty()) {
            QMessageBox::warning(this, "Input Error", "Please enter an IP/FQDN address.");
            return;
        }
        int port = m_clusterPortSpin->value();
        QString stationId = m_clusterStationIdEdit->text().trimmed();
        QString comment = m_clusterCommentEdit->text().trimmed();
        
        QString entry = QString("%1:%2").arg(ip).arg(port);
        QString desc;
        if (!comment.isEmpty() && !stationId.isEmpty()) {
            desc = QString("%1 [ID: %2]").arg(comment, stationId);
        } else if (!comment.isEmpty()) {
            desc = comment;
        } else if (!stationId.isEmpty()) {
            desc = QString("[ID: %1]").arg(stationId);
        } else {
            desc = "Custom DX Cluster";
        }
        entry += " - " + desc;
        
        // Prevent duplicate
        bool exists = false;
        for (int i = 0; i < m_dxClusterListWidget->count(); ++i) {
            if (m_dxClusterListWidget->item(i)->text() == entry) {
                exists = true;
                break;
            }
        }
        
        if (!exists) {
            m_dxClusterListWidget->addItem(entry);
            m_clusterIpEdit->clear();
            m_clusterStationIdEdit->clear();
            m_clusterCommentEdit->clear();
            m_clusterPortSpin->setValue(7373);
            QMessageBox::information(this, "Success", "Custom DX Cluster added successfully.");
        } else {
            QMessageBox::warning(this, "Duplicate", "This DX Cluster entry already exists in the list.");
        }
    });

    layout->addRow(dxClusterWidget);

    m_pagesWidget->addWidget(page);
}

void ConfigDialog::createLoggingPage()
{
    QWidget *page = new QWidget;
    QFormLayout *layout = new QFormLayout(page);
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("Files");
    QString defUserDir = settings.value("userDirectory", getAppDataDir()).toString();
    QString defLog = settings.value("logfile", "").toString();
    QString defSysLog = settings.value("systemLogfile", "").toString();
    QString defDebugLog = settings.value("debugLogfile", "").toString();
    settings.endGroup();
    
    if (defLog.isEmpty()) {
        QDir dataDir(getAppDataDir());
        if (!dataDir.exists()) dataDir.mkpath(".");
        defLog = dataDir.filePath("Qwain_Active.adi");
    }
    
    // User Directory
    m_userDirectoryEdit = new QLineEdit(defUserDir);
    QHBoxLayout *userDirLayout = new QHBoxLayout;
    userDirLayout->addWidget(m_userDirectoryEdit);
    QPushButton *btnUserDir = new QPushButton("Browse...");
    connect(btnUserDir, &QPushButton::clicked, this, [this]() {
        QFileDialog dialog(this, tr("Select User Directory"), m_userDirectoryEdit->text());
        dialog.setFileMode(QFileDialog::Directory);
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setOption(QFileDialog::ShowDirsOnly, true);
        dialog.setFilter(dialog.filter() | QDir::Hidden);
        if (dialog.exec() == QDialog::Accepted) {
            m_userDirectoryEdit->setText(dialog.selectedFiles().first());
        }
    });
    userDirLayout->addWidget(btnUserDir);
    layout->addRow("User Directory:", userDirLayout);
    
    // Active ADIF file
    m_logfileEdit = new QLineEdit(defLog);
    QHBoxLayout *browseLayout = new QHBoxLayout;
    browseLayout->addWidget(m_logfileEdit);
    
    QPushButton *browseBtn = new QPushButton("Browse...");
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString defDir = m_userDirectoryEdit->text();
        if (defDir.isEmpty()) defDir = QDir::homePath();
        QFileDialog dialog(this, tr("Select ADIF File"), defDir, tr("Log Files (*.adi *.adif *.txt *.csv *.log);;ADIF Files (*.adi *.adif);;All Files (*)"));
        dialog.setFileMode(QFileDialog::ExistingFile);
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setFilter(dialog.filter() | QDir::Hidden);
        if (dialog.exec() == QDialog::Accepted) {
            m_logfileEdit->setText(dialog.selectedFiles().first());
        }
    });
    
    QPushButton *testBtn = new QPushButton("Test Settings");
    connect(testBtn, &QPushButton::clicked, this, [this]() {
        QString path = m_logfileEdit->text().trimmed();
        if (path.isEmpty() || !QFile::exists(path)) {
            QMessageBox::warning(this, "File Not Found", "The specified logfile does not exist.");
            return;
        }
        
        QDialog *previewDlg = new QDialog(this);
        previewDlg->setWindowTitle("Logfile Preview");
        previewDlg->resize(600, 400);
        QVBoxLayout *dlgLayout = new QVBoxLayout(previewDlg);
        QFileInfo fi(path);
        QLabel *infoLbl = new QLabel(QString("<b>File:</b> %1<br/><b>Size:</b> %2 bytes").arg(fi.fileName()).arg(fi.size()));
        dlgLayout->addWidget(infoLbl);
        
        QTextEdit *previewEdit = new QTextEdit;
        previewEdit->setReadOnly(true);
        QFile tFile(path);
        if (tFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&tFile);
            int lineCount = 0;
            while (!in.atEnd() && lineCount < 300) {
                previewEdit->append(in.readLine());
                lineCount++;
            }
            tFile.close();
        }
        dlgLayout->addWidget(previewEdit);
        
        QPushButton *closeBtn = new QPushButton("Close");
        connect(closeBtn, &QPushButton::clicked, previewDlg, &QDialog::accept);
        dlgLayout->addWidget(closeBtn);
        
        previewDlg->exec();
        previewDlg->deleteLater();
    });
    
    browseLayout->addWidget(browseBtn);
    browseLayout->addWidget(testBtn);
    
    layout->addRow("Active ADIF File:", browseLayout);
    
    // System Logfile
    m_systemLogfileEdit = new QLineEdit(defSysLog);
    QHBoxLayout *sysLogLayout = new QHBoxLayout;
    sysLogLayout->addWidget(m_systemLogfileEdit);
    QPushButton *btnSysLog = new QPushButton("Browse...");
    connect(btnSysLog, &QPushButton::clicked, this, [this]() {
        QString defDir = m_userDirectoryEdit->text();
        if (defDir.isEmpty()) defDir = QDir::homePath();
        QString fileName = QFileDialog::getSaveFileName(this, tr("Select System Logfile"), defDir + "/system.log", tr("Log Files (*.log *.txt);;All Files (*)"));
        if (!fileName.isEmpty()) {
            m_systemLogfileEdit->setText(fileName);
        }
    });
    sysLogLayout->addWidget(btnSysLog);
    layout->addRow("System Logfile:", sysLogLayout);
    
    // Debug Logfile
    m_debugLogfileEdit = new QLineEdit(defDebugLog);
    QHBoxLayout *debugLogLayout = new QHBoxLayout;
    debugLogLayout->addWidget(m_debugLogfileEdit);
    QPushButton *btnDebugLog = new QPushButton("Browse...");
    connect(btnDebugLog, &QPushButton::clicked, this, [this]() {
        QString defDir = m_userDirectoryEdit->text();
        if (defDir.isEmpty()) defDir = QDir::homePath();
        QString fileName = QFileDialog::getSaveFileName(this, tr("Select Debug Logfile"), defDir + "/debug.log", tr("Log Files (*.log *.txt);;All Files (*)"));
        if (!fileName.isEmpty()) {
            m_debugLogfileEdit->setText(fileName);
        }
    });
    debugLogLayout->addWidget(btnDebugLog);
    layout->addRow("Debug Logfile:", debugLogLayout);
    
    QHBoxLayout *logOpsLayout = new QHBoxLayout;
    QPushButton *prepBtn = new QPushButton("Prepare/Repair Logfile");
    connect(prepBtn, &QPushButton::clicked, this, &ConfigDialog::prepNewLogfile);
    logOpsLayout->addWidget(prepBtn);
    
    QPushButton *forceSyncBtn = new QPushButton("FORCE SYNC");
    connect(forceSyncBtn, &QPushButton::clicked, this, &ConfigDialog::forceSyncLogfile);
    logOpsLayout->addWidget(forceSyncBtn);
    
    QPushButton *mailCheckBtn = new QPushButton("QSL by Mail Check");
    connect(mailCheckBtn, &QPushButton::clicked, this, &ConfigDialog::startMailCheck);
    logOpsLayout->addWidget(mailCheckBtn);
    
    layout->addRow("", logOpsLayout);
    
    m_pagesWidget->addWidget(page);
}

void ConfigDialog::createBackupPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    
    // High-contrast header description (readable against all dark and light themes)
    QLabel *lbl = new QLabel(tr("Backup and Restore your Log and Configuration Data.\nUse these tools periodically to protect your operating data."));
    lbl->setStyleSheet("font-weight: 500; font-size: 13px; color: #cbd5e1; line-height: 1.4;");
    layout->addWidget(lbl);
    layout->addSpacing(15);
    
    QHBoxLayout *btnLayout = new QHBoxLayout;
    
    // High-contrast primary action button for Full Backup
    QPushButton *btnFull = new QPushButton(tr("Full Backup"));
    btnFull->setMinimumHeight(42);
    btnFull->setCursor(Qt::PointingHandCursor);
    btnFull->setStyleSheet(
        "QPushButton { "
        "  background-color: #0284c7; "
        "  color: #ffffff; "
        "  border: 1px solid #38bdf8; "
        "  border-radius: 6px; "
        "  font-weight: bold; "
        "  font-size: 14px; "
        "  padding: 8px 24px; "
        "} "
        "QPushButton:hover { "
        "  background-color: #0369a1; "
        "  border-color: #7dd3fc; "
        "} "
        "QPushButton:pressed { "
        "  background-color: #075985; "
        "}"
    );
    connect(btnFull, &QPushButton::clicked, this, [this]() {
        if (MainWindow *mainWnd = qobject_cast<MainWindow*>(parentWidget())) {
            // Get path from UI edit box directly
            QString userDir = m_userDirectoryEdit->text();
            if (userDir.isEmpty()) {
                QSettings settings(QCoreApplication::organizationName(), "Qwain");
                settings.beginGroup("Files");
                userDir = settings.value("userDirectory", getAppDataDir()).toString();
                settings.endGroup();
            }
            mainWnd->backupData(userDir);
            updateBackupInfoLabels();
        }
    });
    
    // High-contrast technical secondary button for Restore
    QPushButton *btnRestore = new QPushButton(tr("Restore from Backup"));
    btnRestore->setMinimumHeight(42);
    btnRestore->setCursor(Qt::PointingHandCursor);
    btnRestore->setStyleSheet(
        "QPushButton { "
        "  background-color: #252d38; "
        "  color: #f1f5f9; "
        "  border: 1px solid #475569; "
        "  border-radius: 6px; "
        "  font-weight: bold; "
        "  font-size: 14px; "
        "  padding: 8px 24px; "
        "} "
        "QPushButton:hover { "
        "  background-color: #334155; "
        "  color: #ffffff; "
        "  border-color: #94a3b8; "
        "} "
        "QPushButton:pressed { "
        "  background-color: #1e293b; "
        "}"
    );
    connect(btnRestore, &QPushButton::clicked, this, [this]() {
        if (MainWindow *mainWnd = qobject_cast<MainWindow*>(parentWidget())) {
            QString userDir = m_userDirectoryEdit->text();
            if (userDir.isEmpty()) {
                QSettings settings(QCoreApplication::organizationName(), "Qwain");
                settings.beginGroup("Files");
                userDir = settings.value("userDirectory", getAppDataDir()).toString();
                settings.endGroup();
            }
            mainWnd->restoreData(userDir);
        }
    });
    
    btnLayout->addWidget(btnFull);
    btnLayout->addWidget(btnRestore);
    layout->addLayout(btnLayout);
    
    layout->addSpacing(20);
    
    // Most Recent Backup Details GroupBox with technical workstation card styling
    QGroupBox *infoGroup = new QGroupBox(tr("Most Recent Backup Metadata"));
    infoGroup->setStyleSheet(
        "QGroupBox { "
        "  font-weight: bold; "
        "  font-size: 13px; "
        "  color: #56c7e8; "
        "  border: 1px solid #343e4b; "
        "  background-color: #171c23; "
        "  border-radius: 8px; "
        "  margin-top: 14px; "
        "  padding: 16px 14px 14px 14px; "
        "} "
        "QGroupBox::title { "
        "  subcontrol-origin: margin; "
        "  left: 12px; "
        "  padding: 0 6px; "
        "  color: #56c7e8; "
        "}"
    );
    QFormLayout *infoLayout = new QFormLayout(infoGroup);
    infoLayout->setLabelAlignment(Qt::AlignRight);
    infoLayout->setSpacing(8);
    
    m_lblDate = new QLabel("-");
    m_lblContacts = new QLabel("-");
    m_lblImages = new QLabel("-");
    m_lblFiles = new QLabel("-");
    m_lblCompressedSize = new QLabel("-");
    m_lblUncompressedSize = new QLabel("-");
    m_lblRatio = new QLabel("-");
    m_lblMd5 = new QLabel("-");
    
    // High-contrast values for clear readability
    m_lblDate->setStyleSheet("font-weight: bold; font-size: 12px; color: #56c7e8; background: transparent;");
    m_lblContacts->setStyleSheet("font-weight: bold; font-size: 12px; color: #f8fafc; background: transparent;");
    m_lblImages->setStyleSheet("font-weight: bold; font-size: 12px; color: #f8fafc; background: transparent;");
    m_lblFiles->setStyleSheet("font-weight: bold; font-size: 12px; color: #f8fafc; background: transparent;");
    m_lblUncompressedSize->setStyleSheet("font-weight: bold; font-size: 12px; color: #f8fafc; background: transparent;");
    m_lblCompressedSize->setStyleSheet("font-weight: bold; font-size: 12px; color: #f8fafc; background: transparent;");
    m_lblRatio->setStyleSheet("font-weight: bold; font-size: 12px; color: #4ade80; background: transparent;");
    m_lblMd5->setStyleSheet("font-family: 'DejaVu Sans Mono', 'Courier New', monospace; font-size: 11px; font-weight: bold; color: #e2e8f0; background: #0f1318; padding: 4px 8px; border: 1px solid #2d3748; border-radius: 4px;");
    
    // Helper to ensure row title labels also have high-contrast light-slate styling
    auto addInfoRow = [&](const QString &labelStr, QWidget *valWidget) {
        QLabel *lbl = new QLabel(labelStr, infoGroup);
        lbl->setStyleSheet("font-weight: 500; font-size: 12px; color: #cbd5e1; background: transparent;");
        infoLayout->addRow(lbl, valWidget);
    };

    addInfoRow(tr("Backup Date/Time:"), m_lblDate);
    addInfoRow(tr("Total QSOs/Contacts:"), m_lblContacts);
    addInfoRow(tr("Image Count:"), m_lblImages);
    addInfoRow(tr("Total Files Packed:"), m_lblFiles);
    addInfoRow(tr("Uncompressed Size:"), m_lblUncompressedSize);
    addInfoRow(tr("Compressed Size:"), m_lblCompressedSize);
    addInfoRow(tr("Compression Ratio:"), m_lblRatio);
    addInfoRow(tr("MD5 Checksum:"), m_lblMd5);
    
    layout->addWidget(infoGroup);
    layout->addStretch();
    
    // Load existing metadata from settings
    updateBackupInfoLabels();
    
    m_pagesWidget->addWidget(page);
}

void ConfigDialog::updateBackupInfoLabels()
{
    if (!m_lblDate) return;
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("MostRecentBackup");
    
    QString date = settings.value("date", "No backups recorded").toString();
    QString contacts = settings.value("contacts", "-").toString();
    if (contacts != "-") contacts = QString("%1 QSOs").arg(contacts);
    
    QString images = settings.value("images", "-").toString();
    if (images != "-") images = QString("%1 images").arg(images);
    
    QString files = settings.value("files", "-").toString();
    if (files != "-") files = QString("%1 files").arg(files);
    
    QString uncompressed = settings.value("uncompressedSize", "-").toString();
    QString compressed = settings.value("compressedSize", "-").toString();
    QString ratio = settings.value("ratio", "-").toString();
    QString md5 = settings.value("md5", "-").toString();
    
    settings.endGroup();
    
    m_lblDate->setText(date);
    m_lblContacts->setText(contacts);
    m_lblImages->setText(images);
    m_lblFiles->setText(files);
    m_lblUncompressedSize->setText(uncompressed);
    m_lblCompressedSize->setText(compressed);
    m_lblRatio->setText(ratio);
    m_lblMd5->setText(md5);
}

#include <QFontDialog>
#include <QScrollArea>
#include <QSplitter>
#include <QButtonGroup>
#include <QFrame>
#include "styledialog.h"
#include "gtkfontchooserwidget.h"

// ============================================================================
// COMPONENT: Appearance Workspace (createUIPage)
// ROLE: Implements the "Modern Radio Workstation" preferences page.
//       Replaces the flat form with structured sections (Theme & Color,
//       Text Styles, Status History) and an interactive live miniature radio
//       workspace console preview.
// ============================================================================
void ConfigDialog::createUIPage()
{
    QWidget *mainPage = new QWidget;
    QHBoxLayout *mainPageLayout = new QHBoxLayout(mainPage);
    mainPageLayout->setContentsMargins(8, 8, 8, 8);
    mainPageLayout->setSpacing(14);

    QSettings settings(QCoreApplication::organizationName(), "Qwain");

    // Load initial color and scheme
    m_selectedColor = settings.value("SkinColor", "#221e1f").toString();
    m_initialSelectedColor = m_selectedColor;
    QString initialScheme = settings.value("UI/systemScheme", "None").toString();

    // ------------------------------------------------------------------------
    // LEFT COLUMN: Scrollable Configuration Sections
    // ------------------------------------------------------------------------
    QScrollArea *scrollArea = new QScrollArea(mainPage);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *settingsContainer = new QWidget;
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsContainer);
    settingsLayout->setContentsMargins(6, 6, 12, 6);
    settingsLayout->setSpacing(18);

    // Page Header
    QLabel *pageTitle = new QLabel(tr("Appearance"), settingsContainer);
    pageTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #56c7e8; margin-bottom: 2px;");
    QLabel *pageSubtitle = new QLabel(tr("Personalize your workspace colors, text styles, and status history."), settingsContainer);
    pageSubtitle->setStyleSheet("font-size: 12px; color: #aab5c4; margin-bottom: 6px;");
    settingsLayout->addWidget(pageTitle);
    settingsLayout->addWidget(pageSubtitle);

    // ------------------------------------------------------------------------
    // SECTION 1: Theme & Color
    // ------------------------------------------------------------------------
    QLabel *sec1Header = new QLabel(tr("Theme & Color"), settingsContainer);
    sec1Header->setStyleSheet("font-size: 14px; font-weight: 600; color: #e8edf4;");
    QLabel *sec1Desc = new QLabel(tr("Choose a control style and customize the workspace background."), settingsContainer);
    sec1Desc->setStyleSheet("font-size: 11px; color: #aab5c4;");
    settingsLayout->addWidget(sec1Header);
    settingsLayout->addWidget(sec1Desc);

    // Legacy sync combo box (kept instantiated for compatibility)
    m_schemeCombo = new QComboBox(settingsContainer);
    m_schemeCombo->addItem(tr("Light"));
    m_schemeCombo->addItem(tr("Dark"));
    m_schemeCombo->addItem(tr("None"));
    m_schemeCombo->addItem(tr("Custom"));
    m_schemeCombo->setCurrentText(initialScheme);
    m_schemeCombo->hide(); // visually represented by the 4 theme tiles below

    // Theme Tiles
    m_schemeButtonGroup = new QButtonGroup(settingsContainer);
    m_schemeButtonGroup->setExclusive(true);

    auto makeTile = [this, settingsContainer](const QString &title, const QString &tooltip, const QString &schemeVal) -> QPushButton* {
        QPushButton *tile = new QPushButton(title, settingsContainer);
        tile->setCheckable(true);
        tile->setToolTip(tooltip);
        tile->setFixedHeight(34);
        tile->setMinimumWidth(80);
        tile->setStyleSheet(
            "QPushButton {"
            "  background-color: #252d38; color: #e8edf4; border: 1px solid #343e4b; "
            "  border-radius: 5px; font-size: 12px; font-weight: 600; padding: 4px 10px;"
            "}"
            "QPushButton:hover { background-color: #2f3846; border-color: #56c7e8; }"
            "QPushButton:checked { background-color: #1a3245; color: #56c7e8; border: 2px solid #56c7e8; }"
        );
        m_schemeButtonGroup->addButton(tile);
        connect(tile, &QPushButton::toggled, this, [this, schemeVal](bool checked) {
            if (checked) {
                if (m_schemeCombo->currentText() != schemeVal) {
                    m_schemeCombo->setCurrentText(schemeVal);
                }
                updatePreviewButtonStyle();
                updateMiniConsolePreview();
            }
        });
        return tile;
    };

    m_tileLight = makeTile(tr("Light"), tr("Clean light control styling with dark text"), "Light");
    m_tileDark = makeTile(tr("Dark"), tr("Technical dark charcoal workstation style"), "Dark");
    m_tileNative = makeTile(tr("Native"), tr("Uses the platform's default control styling"), "None");
    m_tileCustom = makeTile(tr("Custom"), tr("Derives controls from the chosen background skin"), "Custom");

    if (initialScheme == "Light") m_tileLight->setChecked(true);
    else if (initialScheme == "Dark") m_tileDark->setChecked(true);
    else if (initialScheme == "Custom") m_tileCustom->setChecked(true);
    else m_tileNative->setChecked(true);

    QHBoxLayout *tilesLayout = new QHBoxLayout;
    tilesLayout->setSpacing(8);
    tilesLayout->addWidget(m_tileLight);
    tilesLayout->addWidget(m_tileDark);
    tilesLayout->addWidget(m_tileNative);
    tilesLayout->addWidget(m_tileCustom);
    tilesLayout->addStretch();
    settingsLayout->addLayout(tilesLayout);

    // Workspace Background Control Row
    QLabel *bgLabel = new QLabel(tr("Workspace Background:"), settingsContainer);
    bgLabel->setStyleSheet("font-size: 12px; font-weight: 500; color: #cbd5e1; margin-top: 4px;");
    settingsLayout->addWidget(bgLabel);

    QHBoxLayout *bgRowLayout = new QHBoxLayout;
    bgRowLayout->setSpacing(8);

    m_swatchFrame = new QFrame(settingsContainer);
    m_swatchFrame->setFixedSize(24, 24);
    m_swatchFrame->setStyleSheet(QString("background-color: %1; border: 1px solid #4f5b66; border-radius: 4px;").arg(m_selectedColor));
    bgRowLayout->addWidget(m_swatchFrame);

    m_currentColorLabel = new QLabel(m_selectedColor, settingsContainer);
    m_currentColorLabel->setStyleSheet(
        "font-family: 'DejaVu Sans Mono', 'Courier New', monospace; "
        "font-size: 11px; font-weight: bold; color: #e8edf4; "
        "background: #1c222b; border: 1px solid #343e4b; border-radius: 4px; "
        "padding: 3px 8px;"
    );
    bgRowLayout->addWidget(m_currentColorLabel);

    // Quick-pick workstation background palette for instant live preview (no dialog popups required)
    const struct BgPreset { const char *hex; const char *name; } bgPresets[] = {
        {"#181b22", "Charcoal"},
        {"#10151f", "Midnight Navy"},
        {"#1a1a22", "Obsidian"},
        {"#252c37", "Slate"},
        {"#221e1f", "Native Dark"},
        {"#f8fafc", "Studio Light"}
    };
    for (const auto &bp : bgPresets) {
        QPushButton *bgDot = new QPushButton(settingsContainer);
        bgDot->setFixedSize(18, 18);
        bgDot->setToolTip(QString("%1 (%2)").arg(bp.name).arg(bp.hex));
        bgDot->setStyleSheet(QString(
            "QPushButton { background-color: %1; border: 1px solid #343e4b; border-radius: 9px; }"
            "QPushButton:hover { border: 2px solid #56c7e8; }"
        ).arg(bp.hex));
        connect(bgDot, &QPushButton::clicked, this, [this, hex = QString(bp.hex)]() {
            m_selectedColor = hex;
            if (m_currentColorLabel) m_currentColorLabel->setText(m_selectedColor);
            if (m_swatchFrame) m_swatchFrame->setStyleSheet(QString("background-color: %1; border: 1px solid #4f5b66; border-radius: 4px;").arg(m_selectedColor));
            updatePreviewButtonStyle();
            updateMiniConsolePreview();
        });
        bgRowLayout->addWidget(bgDot);
    }

    QPushButton *colorBtn = new QPushButton(tr("Choose..."), settingsContainer);
    colorBtn->setStyleSheet(
        "QPushButton { background-color: #252d38; color: #e8edf4; border: 1px solid #343e4b; border-radius: 4px; padding: 4px 12px; font-size: 11px; font-weight: 500; }"
        "QPushButton:hover { background-color: #2f3846; border-color: #56c7e8; }"
    );
    connect(colorBtn, &QPushButton::clicked, this, &ConfigDialog::chooseSkinColor);
    bgRowLayout->addWidget(colorBtn);

    QPushButton *revertBtn = new QPushButton(tr("Restore preset..."), settingsContainer);
    revertBtn->setStyleSheet(
        "QPushButton { background-color: #252d38; color: #aab5c4; border: 1px solid #343e4b; border-radius: 4px; padding: 4px 10px; font-size: 11px; }"
        "QPushButton:hover { background-color: #2f3846; color: #e8edf4; }"
    );
    revertBtn->setToolTip(tr("Reset workspace and text colors to a standard palette."));
    bgRowLayout->addWidget(revertBtn);
    bgRowLayout->addStretch();
    settingsLayout->addLayout(bgRowLayout);

    connect(revertBtn, &QPushButton::clicked, this, [this](){
        QDialog dlg(this);
        dlg.setWindowTitle(tr("Restore Color Preset"));
        dlg.setMinimumWidth(390);
        QVBoxLayout *dlgLayout = new QVBoxLayout(&dlg);
        dlgLayout->setSpacing(12);

        QLabel *descLbl = new QLabel(
            tr("Choose a palette to replace the current workspace and text colors.\n"
               "Font selections and status history settings will be kept."), &dlg);
        descLbl->setWordWrap(true);
        descLbl->setStyleSheet("color: #cbd5e1; font-size: 12px;");
        dlgLayout->addWidget(descLbl);

        QFormLayout *form = new QFormLayout;
        QComboBox *combo = new QComboBox(&dlg);
        combo->addItem(tr("Dark"));
        combo->addItem(tr("Light"));
        combo->addItem(tr("Native / None"));
        form->addRow(tr("Preset Palette:"), combo);
        dlgLayout->addLayout(form);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Cancel, &dlg);
        if (QPushButton *btn = buttonBox->button(QDialogButtonBox::RestoreDefaults)) {
            btn->setText(tr("Restore"));
            btn->setStyleSheet("font-weight: bold;");
        }
        connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        dlgLayout->addWidget(buttonBox);

        if (dlg.exec() == QDialog::Accepted) {
            QString chosen = combo->currentText();
            if (chosen.contains("Dark")) {
                m_clockColor = Qt::white;
                m_callInfoColor = Qt::white;
                m_qsoListColor = Qt::white;
                m_selectedColor = "#181b22";
                if (m_tileDark) m_tileDark->setChecked(true);
                m_schemeCombo->setCurrentText("Dark");
            } else if (chosen.contains("Light")) {
                m_clockColor = Qt::black;
                m_callInfoColor = Qt::black;
                m_qsoListColor = Qt::black;
                m_selectedColor = "#f8fafc";
                if (m_tileLight) m_tileLight->setChecked(true);
                m_schemeCombo->setCurrentText("Light");
            } else {
                m_clockColor = Qt::white;
                m_callInfoColor = Qt::white;
                m_qsoListColor = Qt::white;
                m_selectedColor = "#221e1f";
                if (m_tileNative) m_tileNative->setChecked(true);
                m_schemeCombo->setCurrentText("None");
            }

            if (m_currentColorLabel) m_currentColorLabel->setText(m_selectedColor);
            if (m_swatchFrame) m_swatchFrame->setStyleSheet(QString("background-color: %1; border: 1px solid #4f5b66; border-radius: 4px;").arg(m_selectedColor));
            for (auto &cb : m_styleLabelRefreshers) cb();
            updatePreviewButtonStyle();
            updateMiniConsolePreview();
        }
    });

    // ------------------------------------------------------------------------
    // SECTION 2: Text Styles (Instant Live Inline Controls)
    // ------------------------------------------------------------------------
    // Provides immediate, non-modal manipulation of font family, font size,
    // bold weight, and color themes with direct signal dispatch to the live
    // miniature radio workspace console. Retains the "Adv..." button to preserve
    // full access to GtkFontChooserWidget without forcing modal hops.
    QLabel *sec2Header = new QLabel(tr("Text Styles"), settingsContainer);
    sec2Header->setStyleSheet("font-size: 14px; font-weight: 600; color: #e8edf4; margin-top: 8px;");
    QLabel *sec2Desc = new QLabel(tr("Adjust fonts, sizes, and colors inline with instant live preview."), settingsContainer);
    sec2Desc->setStyleSheet("font-size: 11px; color: #aab5c4;");
    settingsLayout->addWidget(sec2Header);
    settingsLayout->addWidget(sec2Desc);

    auto addFontColor = [&](const QString &label, const QString &sampleText, QFont &f, QColor &c, const QString &fkey, const QString &ckey,
                            QFont defFont = QFont("Arial", 10), QColor defColor = QColor(Qt::white)) {
        f = qvariant_cast<QFont>(settings.value(fkey, defFont));
        c = qvariant_cast<QColor>(settings.value(ckey, defColor));
        if (&f == &m_clockFont) { m_initialClockFont = f; m_initialClockColor = c; }
        else if (&f == &m_feed1Font) { m_initialFeed1Font = f; m_initialFeed1Color = c; }
        else if (&f == &m_feed2Font) { m_initialFeed2Font = f; m_initialFeed2Color = c; }
        else if (&f == &m_feed3Font) { m_initialFeed3Font = f; m_initialFeed3Color = c; }
        else if (&f == &m_callInfoFont) { m_initialCallInfoFont = f; m_initialCallInfoColor = c; }
        else if (&f == &m_qsoListFont) { m_initialQsoListFont = f; m_initialQsoListColor = c; }

        QFrame *rowFrame = new QFrame(settingsContainer);
        rowFrame->setStyleSheet("QFrame { background: #1c222b; border: 1px solid #2d3748; border-radius: 6px; }");
        QVBoxLayout *cardLayout = new QVBoxLayout(rowFrame);
        cardLayout->setContentsMargins(10, 8, 10, 8);
        cardLayout->setSpacing(8);

        // --- Line 1: Header (Label, Summary, Live Sample Badge, Advanced Button) ---
        QHBoxLayout *topRow = new QHBoxLayout;
        topRow->setSpacing(8);

        QLabel *nameLabel = new QLabel(label, rowFrame);
        nameLabel->setStyleSheet("font-weight: 600; font-size: 12px; color: #e8edf4; border: none; background: transparent;");
        topRow->addWidget(nameLabel);

        QString weightStr = (f.weight() >= QFont::Bold) ? "Bold" : "Regular";
        QLabel *summaryLabel = new QLabel(QString("· %1 %2pt %3").arg(f.family()).arg(f.pointSize() > 0 ? f.pointSize() : 10).arg(weightStr), rowFrame);
        summaryLabel->setStyleSheet("font-size: 11px; color: #8892b0; border: none; background: transparent;");
        topRow->addWidget(summaryLabel);

        topRow->addStretch();

        QLabel *lblPreview = new QLabel(sampleText, rowFrame);
        lblPreview->setFont(f);
        lblPreview->setAlignment(Qt::AlignCenter);
        lblPreview->setStyleSheet(QString("color: %1; background: #2b323c; padding: 3px 8px; border: 1px solid #3e4856; border-radius: 4px;").arg(c.name()));
        topRow->addWidget(lblPreview);

        QPushButton *advBtn = new QPushButton(tr("Adv..."), rowFrame);
        advBtn->setToolTip(tr("Open advanced font & color dialog"));
        advBtn->setStyleSheet(
            "QPushButton { background-color: #252d38; color: #94a3b8; border: 1px solid #343e4b; border-radius: 4px; padding: 3px 8px; font-size: 10px; }"
            "QPushButton:hover { background-color: #2f3846; color: #e8edf4; border-color: #56c7e8; }"
        );
        topRow->addWidget(advBtn);
        cardLayout->addLayout(topRow);

        // --- Line 2: Inline Live Control Strip ---
        QHBoxLayout *ctrlRow = new QHBoxLayout;
        ctrlRow->setSpacing(6);

        // 1. Font Family Combo
        QFontComboBox *fontCombo = new QFontComboBox(rowFrame);
        fontCombo->setCurrentFont(f);
        fontCombo->setMaximumWidth(150);
        fontCombo->setStyleSheet(
            "QFontComboBox { background: #13181f; color: #e8edf4; border: 1px solid #343e4b; border-radius: 4px; padding: 2px 6px; font-size: 11px; }"
            "QFontComboBox QAbstractItemView { background: #181d24; color: #e8edf4; selection-background-color: #0284c7; }"
        );
        ctrlRow->addWidget(fontCombo);

        // 2. Font Size SpinBox
        QSpinBox *sizeSpin = new QSpinBox(rowFrame);
        sizeSpin->setRange(6, 48);
        sizeSpin->setValue(f.pointSize() > 0 ? f.pointSize() : 10);
        sizeSpin->setSuffix(" pt");
        sizeSpin->setFixedWidth(64);
        sizeSpin->setStyleSheet(
            "QSpinBox { background: #13181f; color: #e8edf4; border: 1px solid #343e4b; border-radius: 4px; padding: 2px 4px; font-size: 11px; }"
        );
        ctrlRow->addWidget(sizeSpin);

        // 3. Bold Toggle Button
        QPushButton *boldBtn = new QPushButton("B", rowFrame);
        boldBtn->setCheckable(true);
        boldBtn->setChecked(f.weight() >= QFont::Bold);
        boldBtn->setFixedSize(26, 24);
        boldBtn->setToolTip(tr("Toggle bold weight"));
        boldBtn->setStyleSheet(
            "QPushButton { font-weight: bold; background: #13181f; color: #94a3b8; border: 1px solid #343e4b; border-radius: 4px; font-size: 11px; }"
            "QPushButton:checked { background: #0284c7; color: #ffffff; border-color: #38bdf8; }"
            "QPushButton:hover { border-color: #56c7e8; }"
        );
        ctrlRow->addWidget(boldBtn);

        // Separator
        QFrame *sep = new QFrame(rowFrame);
        sep->setFrameShape(QFrame::VLine);
        sep->setStyleSheet("color: #2d3748; background: #2d3748; max-width: 1px;");
        ctrlRow->addWidget(sep);

        // 4. Quick Radio Console Color Swatches (Amber, Green, Cyan, White, Slate, Red)
        const struct ColorPreset { const char *hex; const char *name; } colorPresets[] = {
            {"#ffb000", "Phosphor Amber"},
            {"#39ff14", "Terminal Green"},
            {"#56c7e8", "Ice Cyan"},
            {"#ffffff", "Signal White"},
            {"#cbd5e1", "Cool Gray"},
            {"#ef4444", "Alert Red"}
        };

        auto updateRowPreview = [&f, &c, lblPreview, summaryLabel, fontCombo, sizeSpin, boldBtn]() {
            lblPreview->setFont(f);
            lblPreview->setStyleSheet(QString("color: %1; background: #2b323c; padding: 3px 8px; border: 1px solid #3e4856; border-radius: 4px;").arg(c.name()));
            QString w = (f.weight() >= QFont::Bold) ? "Bold" : "Regular";
            summaryLabel->setText(QString("· %1 %2pt %3").arg(f.family()).arg(f.pointSize() > 0 ? f.pointSize() : 10).arg(w));
            if (fontCombo->currentFont().family() != f.family()) {
                fontCombo->blockSignals(true);
                fontCombo->setCurrentFont(f);
                fontCombo->blockSignals(false);
            }
            if (sizeSpin->value() != f.pointSize() && f.pointSize() > 0) {
                sizeSpin->blockSignals(true);
                sizeSpin->setValue(f.pointSize());
                sizeSpin->blockSignals(false);
            }
            if (boldBtn->isChecked() != (f.weight() >= QFont::Bold)) {
                boldBtn->blockSignals(true);
                boldBtn->setChecked(f.weight() >= QFont::Bold);
                boldBtn->blockSignals(false);
            }
        };

        QPushButton *customColorBtn = new QPushButton(rowFrame);
        customColorBtn->setFixedSize(22, 22);
        customColorBtn->setToolTip(tr("Choose custom text color..."));
        customColorBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #4f5b66; border-radius: 3px; } QPushButton:hover { border-color: #56c7e8; }").arg(c.name()));

        for (const auto &preset : colorPresets) {
            QPushButton *swatch = new QPushButton(rowFrame);
            swatch->setFixedSize(18, 18);
            swatch->setToolTip(QString("%1 (%2)").arg(preset.name).arg(preset.hex));
            swatch->setStyleSheet(QString(
                "QPushButton { background-color: %1; border: 1px solid #343e4b; border-radius: 9px; }"
                "QPushButton:hover { border: 2px solid #ffffff; }"
            ).arg(preset.hex));
            connect(swatch, &QPushButton::clicked, this, [this, &c, hex = QString(preset.hex), updateRowPreview, customColorBtn]() {
                c = QColor(hex);
                customColorBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #4f5b66; border-radius: 3px; } QPushButton:hover { border-color: #56c7e8; }").arg(c.name()));
                updateRowPreview();
                updateMiniConsolePreview();
            });
            ctrlRow->addWidget(swatch);
        }

        // Custom Color Picker Button with Real-time Interactive Scrubbing
        connect(customColorBtn, &QPushButton::clicked, this, [this, &c, updateRowPreview, customColorBtn]() {
            QColor initial = c;
            QColorDialog dlg(c, this);
            dlg.setWindowTitle(tr("Select Text Color"));
            connect(&dlg, &QColorDialog::currentColorChanged, this, [this, &c, updateRowPreview, customColorBtn](const QColor &col) {
                if (col.isValid()) {
                    c = col;
                    customColorBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #4f5b66; border-radius: 3px; } QPushButton:hover { border-color: #56c7e8; }").arg(c.name()));
                    updateRowPreview();
                    updateMiniConsolePreview();
                }
            });
            if (dlg.exec() == QDialog::Accepted) {
                c = dlg.selectedColor();
            } else {
                c = initial;
            }
            customColorBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #4f5b66; border-radius: 3px; } QPushButton:hover { border-color: #56c7e8; }").arg(c.name()));
            updateRowPreview();
            updateMiniConsolePreview();
        });
        ctrlRow->addWidget(customColorBtn);

        ctrlRow->addStretch();
        cardLayout->addLayout(ctrlRow);

        // Connect Inline Controls for Instant Live Response
        connect(fontCombo, &QFontComboBox::currentFontChanged, this, [this, &f, updateRowPreview](const QFont &newF) {
            f.setFamily(newF.family());
            updateRowPreview();
            updateMiniConsolePreview();
        });

        connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, &f, updateRowPreview](int sz) {
            f.setPointSize(sz);
            updateRowPreview();
            updateMiniConsolePreview();
        });

        connect(boldBtn, &QPushButton::toggled, this, [this, &f, updateRowPreview](bool b) {
            f.setWeight(b ? QFont::Bold : QFont::Normal);
            updateRowPreview();
            updateMiniConsolePreview();
        });

        // Advanced Modal Chooser (GtkFontChooserWidget) retained for full fidelity
        connect(advBtn, &QPushButton::clicked, this, [this, &f, &c, updateRowPreview, customColorBtn]() {
            GtkFontChooserWidget sd(f, c, this);
            if (sd.exec() == QDialog::Accepted) {
                f = sd.selectedFont();
                c = sd.selectedColor();
                customColorBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #4f5b66; border-radius: 3px; } QPushButton:hover { border-color: #56c7e8; }").arg(c.name()));
                updateRowPreview();
                updateMiniConsolePreview();
            }
        });

        m_styleLabelRefreshers.push_back([updateRowPreview, customColorBtn, &c]() {
            customColorBtn->setStyleSheet(QString("QPushButton { background-color: %1; border: 1px solid #4f5b66; border-radius: 3px; } QPushButton:hover { border-color: #56c7e8; }").arg(c.name()));
            updateRowPreview();
        });

        settingsLayout->addWidget(rowFrame);
    };

    addFontColor(tr("Clock (UTC)"), "14:28:36 UTC", m_clockFont, m_clockColor, "UI/clockFont", "UI/clockColor");
    addFontColor(tr("Callsign Info"), "W1AW · 20m USB", m_callInfoFont, m_callInfoColor, "UI/callInfoFont", "UI/callInfoColor");
    addFontColor(tr("QSO History"), "K3LR 20m SSB", m_qsoListFont, m_qsoListColor, "UI/qsoListFont", "UI/qsoListColor");

    // ------------------------------------------------------------------------
    // SECTION 3: Status History
    // ------------------------------------------------------------------------
    QLabel *sec3Header = new QLabel(tr("Status History"), settingsContainer);
    sec3Header->setStyleSheet("font-size: 14px; font-weight: 600; color: #e8edf4; margin-top: 8px;");
    settingsLayout->addWidget(sec3Header);

    QHBoxLayout *spinRow = new QHBoxLayout;
    QLabel *spinLabel = new QLabel(tr("Retained status lines:"), settingsContainer);
    spinLabel->setStyleSheet("font-size: 12px; color: #cbd5e1;");
    spinRow->addWidget(spinLabel);

    m_statusScrollLimit = new QSpinBox(settingsContainer);
    m_statusScrollLimit->setRange(10, 200);
    m_statusScrollLimit->setValue(settings.value("UI/statusScrollLimit", 50).toInt());
    m_statusScrollLimit->setSuffix(tr(" lines"));
    m_statusScrollLimit->setFixedWidth(120);
    spinRow->addWidget(m_statusScrollLimit);
    spinRow->addStretch();
    settingsLayout->addLayout(spinRow);

    QLabel *spinHelp = new QLabel(tr("Maximum number of lines retained in the live status pane."), settingsContainer);
    spinHelp->setStyleSheet("font-size: 11px; color: #8892b0; margin-bottom: 8px;");
    settingsLayout->addWidget(spinHelp);

    // Instantiated for compatibility (not shown on UI page)
    m_mapStyleCombo = new QComboBox(settingsContainer);
    m_mapStyleCombo->addItem(tr("Satellite (Color)"));
    m_mapStyleCombo->addItem(tr("Simplified (Black & White)"));
    m_mapStyleCombo->setCurrentIndex(settings.value("MapStyle", 1).toInt());
    m_mapStyleCombo->hide();

    m_alwaysEasternCheck = new QCheckBox(tr("Always adjust times to Eastern timezone"), settingsContainer);
    m_alwaysEasternCheck->setChecked(settings.value("UI/alwaysEastern", false).toBool());
    m_alwaysEasternCheck->hide();

    settingsLayout->addStretch();
    scrollArea->setWidget(settingsContainer);
    mainPageLayout->addWidget(scrollArea, 3);

    // ------------------------------------------------------------------------
    // RIGHT COLUMN: Live Miniature Radio Workspace Preview
    // ------------------------------------------------------------------------
    QWidget *previewContainer = new QWidget(mainPage);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewContainer);
    previewLayout->setContentsMargins(10, 8, 10, 8);
    previewLayout->setSpacing(8);

    QLabel *previewHeading = new QLabel(tr("Live Preview"), previewContainer);
    previewHeading->setStyleSheet("font-size: 14px; font-weight: bold; color: #56c7e8;");
    previewLayout->addWidget(previewHeading);

    QLabel *previewCaption = new QLabel(tr("Preview only — changes are saved with Apply or OK."), previewContainer);
    previewCaption->setStyleSheet("font-size: 11px; color: #aab5c4; margin-bottom: 4px;");
    previewLayout->addWidget(previewCaption);

    m_miniPreviewWorkspace = new QWidget(previewContainer);
    m_miniPreviewWorkspace->setObjectName("miniRadioWorkspace");
    QVBoxLayout *miniLayout = new QVBoxLayout(m_miniPreviewWorkspace);
    miniLayout->setContentsMargins(14, 14, 14, 14);
    miniLayout->setSpacing(12);

    QLabel *stripLabel = new QLabel(tr("📻 Radio Workspace"), m_miniPreviewWorkspace);
    stripLabel->setStyleSheet("font-size: 11px; font-weight: bold; color: #8892b0; letter-spacing: 1px; background: transparent;");
    miniLayout->addWidget(stripLabel);

    m_miniClockLabel = new QLabel("14:28:36 UTC · Jun 27, 2026", m_miniPreviewWorkspace);
    m_miniClockLabel->setAlignment(Qt::AlignCenter);
    miniLayout->addWidget(m_miniClockLabel);

    m_miniCallLabel = new QLabel("W1AW  [20 m · USB]", m_miniPreviewWorkspace);
    m_miniCallLabel->setAlignment(Qt::AlignCenter);
    miniLayout->addWidget(m_miniCallLabel);

    m_miniQsoLabel = new QLabel("14:21  K3LR    20m  SSB\n14:18  G3ABC   20m  SSB\n14:12  JA1XYZ  15m  CW", m_miniPreviewWorkspace);
    m_miniQsoLabel->setAlignment(Qt::AlignLeft);
    miniLayout->addWidget(m_miniQsoLabel);

    QFrame *line = new QFrame(m_miniPreviewWorkspace);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #343e4b; background: transparent;");
    miniLayout->addWidget(line);

    QHBoxLayout *btnBox = new QHBoxLayout;
    btnBox->addStretch();
    m_schemeButtonPreview = new QPushButton(tr("Example Button"), m_miniPreviewWorkspace);
    btnBox->addWidget(m_schemeButtonPreview);
    btnBox->addStretch();
    miniLayout->addLayout(btnBox);

    previewLayout->addWidget(m_miniPreviewWorkspace);
    previewLayout->addStretch();

    mainPageLayout->addWidget(previewContainer, 2);

    m_pagesWidget->addWidget(mainPage);

    updateMiniConsolePreview();
}

void ConfigDialog::updateUIPreview()
{
    if (!m_uiPreview) return;
    
    if (m_currentPreviewPane == 1) {
        // Print QSL Preview mock
        QString techStyle = "font-family: 'Courier New'; font-size: 11pt; font-weight: bold;";
        QString html = "<div style='background-color: #fdfdfd; padding: 10px; color: #333;'>";
        html += "<table width='100%' border='1' cellspacing='0' cellpadding='5' style='border-collapse: collapse;'>";
        html += "<tr style='background-color: #e0e0e0; " + techStyle + "'><th>Callsign</th><th>Date</th><th>Band</th></tr>";
        html += "<tr style='" + techStyle + "'><td align='center'>W4DJF</td><td align='center'>2026-06-27</td><td align='center'>20m</td></tr>";
        html += "</table>";
        html += "</div>";
        m_uiPreview->setHtml(html);
        return;
    } else if (m_currentPreviewPane == 2) {
        // Design QSLs Preview mock
        QString html = "<div style='background-color: #444; padding: 20px; text-align: center; color: white;'>";
        html += "<h3>Design QSLs (WYSIWYG Editor)</h3>";
        html += "<p>Fonts and colors for QSL Cards are customized per-element inside the WYSIWYG Designer.</p>";
        html += "<div style='background-color: white; color: black; padding: 20px; border: 2px solid #888; width: 80%; margin: 0 auto; text-align: left;'>";
        html += "<h2 style='color: navy; border: 1px dashed gray; display: inline-block; padding: 5px;'>W4DJF</h2><br><br>";
        html += "<div style='border: 1px dashed gray; padding: 5px; margin-top: 10px; width: 60%;'>Confirming QSO with ...</div>";
        html += "</div></div>";
        m_uiPreview->setHtml(html);
        return;
    }

    QString html = QString("<div style='background-color: %1; padding: 10px;'>").arg(m_selectedColor);
    
    // Top bar (Clock)
    QString clockStyle = QString("font-family: '%1'; font-size: %2pt; font-weight: %3; color: %4;")
                         .arg(m_clockFont.family()).arg(m_clockFont.pointSize())
                         .arg(m_clockFont.weight() * 8).arg(m_clockColor.name());
    QString clockText = "22:15:30 UTC<br>Jun 27, 2026";
    if (m_alwaysEasternCheck && m_alwaysEasternCheck->isChecked()) {
        clockText = "18:15:30 EDT<br>Jun 27, 2026";
    }
    html += QString("<div align='center' style='margin-bottom: 10px;'><a href='#clock' style='text-decoration:none; %1'>%2</a></div>").arg(clockStyle, clockText);
    
    // Left side (Feeds)
    QString feed1Style = QString("font-family: '%1'; font-size: %2pt; font-weight: %3; color: %4;")
                         .arg(m_feed1Font.family()).arg(m_feed1Font.pointSize())
                         .arg(m_feed1Font.weight() * 8).arg(m_feed1Color.name());
    QString feed2Style = QString("font-family: '%1'; font-size: %2pt; font-weight: %3; color: %4;")
                         .arg(m_feed2Font.family()).arg(m_feed2Font.pointSize())
                         .arg(m_feed2Font.weight() * 8).arg(m_feed2Color.name());
    QString feed3Style = QString("font-family: '%1'; font-size: %2pt; font-weight: %3; color: %4;")
                         .arg(m_feed3Font.family()).arg(m_feed3Font.pointSize())
                         .arg(m_feed3Font.weight() * 8).arg(m_feed3Color.name());
                         
    html += "<table width='100%' border='0' cellspacing='5'><tr>";
    html += QString("<td width='33%' valign='top'><a href='#feed1' style='text-decoration:none; %1'>[Left Feed News Item]</a></td>").arg(feed1Style);
    html += QString("<td width='33%' valign='top'><a href='#feed3' style='text-decoration:none; %1'>[Center Feed Item]</a></td>").arg(feed3Style);
    html += QString("<td width='33%' valign='top'><a href='#feed2' style='text-decoration:none; %1'>[Right Feed News Item]</a></td>").arg(feed2Style);
    html += "</tr></table><hr style='color: #444;'>";
    
    // W4DJF Lookup info
    QString callInfoStyle = QString("font-family: '%1'; font-size: %2pt; font-weight: %3; color: %4;")
                                .arg(m_callInfoFont.family()).arg(m_callInfoFont.pointSize())
                                .arg(m_callInfoFont.weight() * 8).arg(m_callInfoColor.name());
    QString qsoListStyle = QString("font-family: '%1'; font-size: %2pt; font-weight: %3; color: %4;")
                                .arg(m_qsoListFont.family()).arg(m_qsoListFont.pointSize())
                                .arg(m_qsoListFont.weight() * 8).arg(m_qsoListColor.name());
                                
    html += "<table width='100%' border='0' cellspacing='10'><tr><td valign='top' width='60%'>";
    html += QString("<a href='#callInfo' style='text-decoration:none; %1'>")
            .arg(callInfoStyle);
    html += "<h2>W4DJF</h2><table border='0' cellspacing='0' cellpadding='2'>";
    html += "<tr><td align='right'><b>Name:</b></td><td align='left'>Donald J Fox</td></tr>";
    html += "<tr><td align='right'><b>Address:</b></td><td align='left'>238 Graves Hollow Rd</td></tr>";
    html += "<tr><td align='right'><b>City:</b></td><td align='left'>Maynardville, TN 37807</td></tr>";
    html += "<tr><td align='right'><b>Country:</b></td><td align='left'>USA</td></tr>";
    html += "<tr><td align='right'><b>Grid:</b></td><td align='left'>EM86bg</td></tr>";
    html += "<tr><td align='right'><b>County:</b></td><td align='left'>Union</td></tr>";
    html += "</table>";
    html += "</a></td><td valign='top' width='40%'>";
    
    html += "<h3 align='center' style='color:orange; margin-top:0px;'>Past Contacts</h3>";
    html += QString("<a href='#qsoList' style='text-decoration:none; %1'>").arg(qsoListStyle);
    html += "<table width='100%' style='border-collapse: collapse;'>";
    html += "<tr><th align='center'>Date</th><th align='center'>Time</th><th align='center'>Band</th><th align='center'>Mode</th></tr>";
    html += QString("<tr style='color:%1;'><td align='center'>01/15/2026</td><td align='center'>14:00</td><td align='center'>20M</td><td align='center'>SSB</td></tr>").arg(m_qsoListColor.name());
    html += QString("<tr style='color:%1;'><td align='center'>03/22/2026</td><td align='center'>09:30</td><td align='center'>40M</td><td align='center'>FT8</td></tr>").arg(m_qsoListColor.name());
    html += "</table></a>";
    
    html += "</td></tr></table>";
    html += "</div>";
    
    m_uiPreview->setHtml(html);
}

#include <QInputDialog>
#include <QProcess>
#include <QProgressDialog>
#include <QMessageBox>


#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>
#include <QRegularExpression>
#include <QPrinter>
#include <QPainter>
#include <QPageSize>
#include <QDateTime>
#include <QLocale>

static QString getPrinterName(const QString &target) {
    if (target.isEmpty()) return "";
    
    // If target doesn't look like an IP or URI, it is likely a CUPS system queue.
    // Skip HTTP probing to save 5 seconds of timeouts.
    if (!target.contains(".") && !target.contains("://")) {
        return target;
    }
    
    QString urlStr = target;
    if (!target.contains("://")) {
        urlStr = QString("http://%1/").arg(target);
    } else {
        // If it's a URI, let's just extract the host for HTTP probing
        QUrl u(target);
        if (!u.host().isEmpty()) {
             urlStr = QString("http://%1/").arg(u.host());
        }
    }

    QNetworkAccessManager manager;
    QNetworkRequest request;
    request.setUrl(QUrl(urlStr));
    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QTimer timer;
    timer.setInterval(2500);
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start();
    loop.exec();
    
    QString titleText = "";
    if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
        QString html = reply->readAll();
        QRegularExpression re("<title[^>]*>(.*?)</title>", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = re.match(html);
        if (match.hasMatch()) {
            titleText = match.captured(1).trimmed();
        }
    }
    reply->deleteLater();
    
    if (titleText.isEmpty()) {
        QString hostFor631 = target;
        if (target.contains("://")) {
            hostFor631 = QUrl(target).host();
            if (hostFor631.isEmpty()) hostFor631 = target;
        }
        request.setUrl(QUrl(QString("http://%1:631/").arg(hostFor631)));
        reply = manager.get(request);
        timer.start();
        loop.exec();
        if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
            QString html = reply->readAll();
            QRegularExpression re("<title[^>]*>(.*?)</title>", QRegularExpression::CaseInsensitiveOption);
            QRegularExpressionMatch match = re.match(html);
            if (match.hasMatch()) {
                titleText = match.captured(1).trimmed();
            }
        }
        reply->deleteLater();
    }
    
    if (titleText.isEmpty() || titleText.contains("400 Bad") || titleText.contains("404 Not") || titleText.contains("Error")) {
        return target; // fallback to IP or URI
    }
    // Clean up typical fluff
    titleText.replace("Web", "", Qt::CaseInsensitive);
    titleText.replace("Server", "", Qt::CaseInsensitive);
    titleText = titleText.replace(QRegularExpression("\\s+"), " ").trimmed();
    return target + " (" + titleText + ")";
}

void ConfigDialog::createQslCardOptionsPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setSpacing(10);
    layout->setContentsMargins(15, 15, 15, 15);

    QSettings settings(QCoreApplication::organizationName(), "Qwain");

    // Help Dialog function
    auto showHelp = [this](const QString &title, const QString &text) {
        QMessageBox::information(this, title, text);
    };

    // Group 1: Power Tweaks
    QGroupBox *tweaksGroup = new QGroupBox("Power Tweaks Configuration", this);
    QVBoxLayout *tweaksLayout = new QVBoxLayout(tweaksGroup);
    
    QHBoxLayout *headerLayout = new QHBoxLayout();
    m_chkUseTweaks = new QCheckBox("Use TX power tweaks", this);
    m_chkUseTweaks->setChecked(settings.value("LogTweaker/useTweaks", false).toBool());
    
    QPushButton *helpBtn1 = new QPushButton("?", this);
    helpBtn1->setFixedSize(20, 20);
    helpBtn1->setStyleSheet("QPushButton { border-radius: 10px; border: 1px solid #a0a0a0; font-weight: bold; }");
    
    headerLayout->addWidget(m_chkUseTweaks);
    headerLayout->addWidget(helpBtn1);
    headerLayout->addStretch();
    tweaksLayout->addLayout(headerLayout);
    
    QGridLayout *grid = new QGridLayout();
    grid->setContentsMargins(15, 5, 15, 5);
    grid->addWidget(new QLabel("Min (Watts)"), 0, 1, Qt::AlignCenter);
    grid->addWidget(new QLabel("Max (Watts)"), 0, 2, Qt::AlignCenter);
    
    // FT-2
    m_chkFT2Tweaks = new QCheckBox("FT-2", this);
    m_chkFT2Tweaks->setChecked(settings.value("LogTweaker/useTweaks_FT2", true).toBool());
    m_ft2MinSpin = new QSpinBox(this);
    m_ft2MinSpin->setRange(0, 9999);
    m_ft2MinSpin->setSingleStep(5);
    m_ft2MinSpin->setValue(settings.value("LogTweaker/FT2_min", 10).toInt());
    m_ft2MaxSpin = new QSpinBox(this);
    m_ft2MaxSpin->setRange(0, 9999);
    m_ft2MaxSpin->setSingleStep(5);
    m_ft2MaxSpin->setValue(settings.value("LogTweaker/FT2_max", 100).toInt());
    
    grid->addWidget(m_chkFT2Tweaks, 1, 0);
    grid->addWidget(m_ft2MinSpin, 1, 1);
    grid->addWidget(m_ft2MaxSpin, 1, 2);
    
    // FT-4
    m_chkFT4Tweaks = new QCheckBox("FT-4", this);
    m_chkFT4Tweaks->setChecked(settings.value("LogTweaker/useTweaks_FT4", true).toBool());
    m_ft4MinSpin = new QSpinBox(this);
    m_ft4MinSpin->setRange(0, 9999);
    m_ft4MinSpin->setSingleStep(5);
    m_ft4MinSpin->setValue(settings.value("LogTweaker/FT4_min", 10).toInt());
    m_ft4MaxSpin = new QSpinBox(this);
    m_ft4MaxSpin->setRange(0, 9999);
    m_ft4MaxSpin->setSingleStep(5);
    m_ft4MaxSpin->setValue(settings.value("LogTweaker/FT4_max", 100).toInt());
    
    grid->addWidget(m_chkFT4Tweaks, 2, 0);
    grid->addWidget(m_ft4MinSpin, 2, 1);
    grid->addWidget(m_ft4MaxSpin, 2, 2);
    
    // FT-8
    m_chkFT8Tweaks = new QCheckBox("FT-8", this);
    m_chkFT8Tweaks->setChecked(settings.value("LogTweaker/useTweaks_FT8", true).toBool());
    m_ft8MinSpin = new QSpinBox(this);
    m_ft8MinSpin->setRange(0, 9999);
    m_ft8MinSpin->setSingleStep(5);
    m_ft8MinSpin->setValue(settings.value("LogTweaker/FT8_min", 10).toInt());
    m_ft8MaxSpin = new QSpinBox(this);
    m_ft8MaxSpin->setRange(0, 9999);
    m_ft8MaxSpin->setSingleStep(5);
    m_ft8MaxSpin->setValue(settings.value("LogTweaker/FT8_max", 100).toInt());
    
    grid->addWidget(m_chkFT8Tweaks, 3, 0);
    grid->addWidget(m_ft8MinSpin, 3, 1);
    grid->addWidget(m_ft8MaxSpin, 3, 2);
    
    tweaksLayout->addLayout(grid);
    layout->addWidget(tweaksGroup);

    // Help for tweaks
    connect(helpBtn1, &QPushButton::clicked, this, [showHelp]() {
        showHelp("Use TX Power Tweaks", 
                 "This category allows you to selectively override the transmit power value on your printed card.\n\n"
                 "When enabled, the card previewer will generate random power values between the defined Min and Max "
                 "limits for checked digital modes.\n\n"
                 "When disabled, the sub-checkboxes and spinner inputs are grayed out, but remain visible.");
    });

    // Gray-out state update helper
    auto updateTweaksEnabledState = [this](bool enabled) {
        m_chkFT2Tweaks->setEnabled(enabled);
        m_ft2MinSpin->setEnabled(enabled);
        m_ft2MaxSpin->setEnabled(enabled);
        
        m_chkFT4Tweaks->setEnabled(enabled);
        m_ft4MinSpin->setEnabled(enabled);
        m_ft4MaxSpin->setEnabled(enabled);
        
        m_chkFT8Tweaks->setEnabled(enabled);
        m_ft8MinSpin->setEnabled(enabled);
        m_ft8MaxSpin->setEnabled(enabled);
    };
    
    connect(m_chkUseTweaks, &QCheckBox::toggled, this, updateTweaksEnabledState);
    updateTweaksEnabledState(m_chkUseTweaks->isChecked());

    // Group 2: Emission Modes
    QGroupBox *emissionsGroup = new QGroupBox("Emission Mode Options", this);
    QVBoxLayout *emissionsLayout = new QVBoxLayout(emissionsGroup);
    
    QHBoxLayout *emissionsHeader = new QHBoxLayout();
    m_chkUseStandardEmissions = new QCheckBox("Use Standard Emission Modes", this);
    m_chkUseStandardEmissions->setChecked(settings.value("QslUpdates/UseStandardEmission", false).toBool());
    
    QPushButton *helpBtn2 = new QPushButton("?", this);
    helpBtn2->setFixedSize(20, 20);
    helpBtn2->setStyleSheet("QPushButton { border-radius: 10px; border: 1px solid #a0a0a0; font-weight: bold; }");
    
    emissionsHeader->addWidget(m_chkUseStandardEmissions);
    emissionsHeader->addWidget(helpBtn2);
    emissionsHeader->addStretch();
    emissionsLayout->addLayout(emissionsHeader);
    
    m_chkUseGenericEmissions = new QCheckBox("Use Generic Emission Modes", this);
    m_chkUseGenericEmissions->setChecked(settings.value("QslUpdates/UseGenericEmission", true).toBool());
    emissionsLayout->addWidget(m_chkUseGenericEmissions);
    
    // Connect standard & generic for mutual exclusion
    connect(m_chkUseStandardEmissions, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            m_chkUseGenericEmissions->blockSignals(true);
            m_chkUseGenericEmissions->setChecked(false);
            m_chkUseGenericEmissions->blockSignals(false);
        } else {
            m_chkUseGenericEmissions->blockSignals(true);
            m_chkUseGenericEmissions->setChecked(true);
            m_chkUseGenericEmissions->blockSignals(false);
        }
    });
    connect(m_chkUseGenericEmissions, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            m_chkUseStandardEmissions->blockSignals(true);
            m_chkUseStandardEmissions->setChecked(false);
            m_chkUseStandardEmissions->blockSignals(false);
        } else {
            m_chkUseStandardEmissions->blockSignals(true);
            m_chkUseStandardEmissions->setChecked(true);
            m_chkUseStandardEmissions->blockSignals(false);
        }
    });
    
    layout->addWidget(emissionsGroup);
    
    connect(helpBtn2, &QPushButton::clicked, this, [showHelp]() {
        showHelp("Emission Mode Settings", 
                 "Controls how QSO modes are rendered on printed QSL cards:\n\n"
                 "- Standard Emission Modes: Replaces generic modes with standard ITU emission designators:\n"
                 "  SSB -> J3E\n"
                 "  CW -> A1A\n"
                 "  AM -> A3E\n"
                 "  FM -> F3E\n"
                 "  RTTY -> F1B\n"
                 "  PSK31 -> J2B\n"
                 "  DSTAR -> F7W\n\n"
                 "- Generic Emission Modes: Prints modes exactly as they were logged in your ADIF files.");
    });

    // Group 3: QRP Options
    QGroupBox *qrpGroup = new QGroupBox("QRP Options", this);
    QVBoxLayout *qrpLayout = new QVBoxLayout(qrpGroup);
    
    QHBoxLayout *qrpHeader = new QHBoxLayout();
    m_chkUseQrpSubstitution = new QCheckBox("Use QRP Bragging", this);
    m_chkUseQrpSubstitution->setChecked(settings.value("QslUpdates/UseQrpSubstitution", false).toBool());
    
    QPushButton *helpBtn3 = new QPushButton("?", this);
    helpBtn3->setFixedSize(20, 20);
    helpBtn3->setStyleSheet("QPushButton { border-radius: 10px; border: 1px solid #a0a0a0; font-weight: bold; }");
    
    qrpHeader->addWidget(m_chkUseQrpSubstitution);
    qrpHeader->addWidget(helpBtn3);
    qrpHeader->addStretch();
    qrpLayout->addLayout(qrpHeader);

    // Submenu layout under use qrp
    QWidget *qrpSubWidget = new QWidget(this);
    QVBoxLayout *qrpSubLayout = new QVBoxLayout(qrpSubWidget);
    qrpSubLayout->setContentsMargins(20, 0, 0, 0); // Indent under Use QRP Bragging

    m_lblCurrentlySubstitutes = new QLabel(this);
    qrpSubLayout->addWidget(m_lblCurrentlySubstitutes);

    QHBoxLayout *customBragRow = new QHBoxLayout();
    m_chkCustomQrpBrag = new QCheckBox("Custom QRP brag:", this);
    m_chkCustomQrpBrag->setChecked(settings.value("QslUpdates/UseCustomQrpBrag", false).toBool());
    
    m_txtCustomQrpBragText = new QLineEdit(this);
    m_txtCustomQrpBragText->setMaxLength(4);
    m_txtCustomQrpBragText->setFixedWidth(60);
    m_txtCustomQrpBragText->setText(settings.value("QslUpdates/CustomQrpBragText", "QRP").toString());

    customBragRow->addWidget(m_chkCustomQrpBrag);
    customBragRow->addWidget(m_txtCustomQrpBragText);
    customBragRow->addStretch();
    qrpSubLayout->addLayout(customBragRow);

    qrpLayout->addWidget(qrpSubWidget);
    layout->addWidget(qrpGroup);

    auto updateCurrentlySubstitutesLabel = [this]() {
        if (!m_lblCurrentlySubstitutes) return;
        if (m_chkCustomQrpBrag && m_chkCustomQrpBrag->isChecked() && m_txtCustomQrpBragText) {
            QString customText = m_txtCustomQrpBragText->text().trimmed();
            if (customText.isEmpty()) {
                m_lblCurrentlySubstitutes->setText("Currently Substitutes :  QRP (Custom text empty)");
            } else {
                m_lblCurrentlySubstitutes->setText(QString("Currently Substitutes :  %1").arg(customText));
            }
        } else {
            m_lblCurrentlySubstitutes->setText("Currently Substitutes :  QRP");
        }
    };

    auto updateQrpSubWidgetState = [this, qrpSubWidget]() {
        qrpSubWidget->setEnabled(m_chkUseQrpSubstitution->isChecked());
    };

    auto updateCustomBragState = [this, updateCurrentlySubstitutesLabel]() {
        m_txtCustomQrpBragText->setEnabled(m_chkCustomQrpBrag->isChecked());
        updateCurrentlySubstitutesLabel();
    };

    connect(m_chkUseQrpSubstitution, &QCheckBox::toggled, this, updateQrpSubWidgetState);
    connect(m_chkCustomQrpBrag, &QCheckBox::toggled, this, updateCustomBragState);
    connect(m_txtCustomQrpBragText, &QLineEdit::textChanged, this, [updateCurrentlySubstitutesLabel](const QString &) {
        updateCurrentlySubstitutesLabel();
    });

    updateCurrentlySubstitutesLabel();
    updateQrpSubWidgetState();
    updateCustomBragState();
    
    connect(helpBtn3, &QPushButton::clicked, this, [showHelp]() {
        showHelp("QRP Bragging Settings", 
                 "When checked, if a QSO power is 6 Watts or less (<= 6W), the rendering engine will:\n\n"
                 "1. Check if the template contains a dedicated QRP cell or block.\n"
                 "2. If it does, render your active QRP/Brag string inside the QRP block.\n"
                 "3. If it does not, substitute your active QRP/Brag string inside the unused <tone> block on the QSL card.");
    });

    layout->addStretch();
    m_pagesWidget->addWidget(page);
}

void ConfigDialog::createStationPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    
    QGridLayout *grid = new QGridLayout;
    layout->addLayout(grid);
    
    // Helper to create a list block
    auto createListBlock = [this](QVBoxLayout *vbox, const QString &title, QListWidget *&listWidget, const QStringList &items, const QString &noun) {
        vbox->addWidget(new QLabel(QString("<b>%1</b>").arg(title)));
        listWidget = new QListWidget(this);
        listWidget->addItems(items);
        vbox->addWidget(listWidget);
        
        QHBoxLayout *btns = new QHBoxLayout;
        QPushButton *addBtn = new QPushButton(QString("Add %1").arg(noun));
        QPushButton *rmBtn = new QPushButton(QString("Remove %1").arg(noun));
        btns->addWidget(addBtn);
        btns->addWidget(rmBtn);
        vbox->addLayout(btns);
        
        connect(addBtn, &QPushButton::clicked, this, [this, &listWidget, noun]() {
            bool ok;
            QString text = QInputDialog::getText(this, QString("Add %1").arg(noun), QString("%1:").arg(noun), QLineEdit::Normal, "", &ok);
            if (ok && !text.isEmpty()) {
                bool duplicate = false;
                for (int i = 0; i < listWidget->count(); ++i) {
                    if (listWidget->item(i)->text().compare(text, Qt::CaseInsensitive) == 0) {
                        duplicate = true; break;
                    }
                }
                if (duplicate) {
                    QMessageBox::warning(this, "Duplicate", QString("A %1 with this value already exists.").arg(noun));
                } else {
                    listWidget->addItem(text);
                }
            }
        });
        connect(rmBtn, &QPushButton::clicked, this, [this, &listWidget, noun]() {
            QList<QListWidgetItem*> sel = listWidget->selectedItems();
            if (!sel.isEmpty()) {
                QString val = sel.first()->text().trimmed();
                if (noun == "Designer Tag") {
                    static const QStringList defaults = {
                        "<day> - Numerical (1 ... 31)",
                        "<month> - Numerical (1 ... 12)",
                        "<month> - Spelled Out (January ... December)",
                        "<month> - Abbreviated (Jan ... Dec)",
                        "<year> - Numerical",
                        "<time_utc> - UTC Time Of Contact",
                        "<band> - Toggle-able to Frequency",
                        "<mode>",
                        "<tx_pwr>",
                        "<rst_rcvd> - Received Signal Report",
                        "<rst_sent> - Sent Signal Report",
                        "<tone> - Numerical (1 - 9)",
                        "<full_name> - Full Name of Contact",
                        "<f_name> - First Name of Contact",
                        "<l_name> - Last Name of Contact",
                        "<call> - Callsign of Contact",
                        "<address> - Address of Contact",
                        "<country> - Country of Contact"
                    };
                    bool isDefault = false;
                    for (const QString &def : defaults) {
                        if (val.compare(def, Qt::CaseInsensitive) == 0) {
                            isDefault = true;
                            break;
                        }
                    }
                    if (isDefault) {
                        QMessageBox::warning(this, "Non-deletable Field", "This field is a default system field and cannot be deleted.");
                        return;
                    }
                }
                delete sel.first();
            }
        });
    };

    QVBoxLayout *radiosVbox = new QVBoxLayout;
    createListBlock(radiosVbox, "Defined Radios", m_stationRadios, settings.value("Station/Radios").toStringList(), "Radio");
    grid->addLayout(radiosVbox, 0, 0);

    QVBoxLayout *antennasVbox = new QVBoxLayout;
    createListBlock(antennasVbox, "Defined Antennas", m_stationAntennas, settings.value("Station/Antennas").toStringList(), "Antenna");
    grid->addLayout(antennasVbox, 0, 1);

    // Migration logic for Bands & Checks
    QStringList bandsList = settings.value("Station/Bands").toStringList();
    QStringList checksList = settings.value("Station/Checks").toStringList();
    if (!settings.contains("Station/Bands") && settings.contains("Station/CheckboxNames")) {
        QStringList oldNames = settings.value("Station/CheckboxNames").toStringList();
        for (const QString &n : oldNames) {
            QString ln = n.toLower();
            if (ln == "pse" || ln == "tnx") {
                checksList << n;
            } else {
                bandsList << n;
            }
        }
    }

    QVBoxLayout *bandsVbox = new QVBoxLayout;
    createListBlock(bandsVbox, "Defined Bands", m_stationBands, bandsList, "Band");
    grid->addLayout(bandsVbox, 1, 0);

    QVBoxLayout *checksVbox = new QVBoxLayout;
    createListBlock(checksVbox, "Defined Checks", m_stationChecks, checksList, "Check");
    grid->addLayout(checksVbox, 1, 1);

    QStringList adifTags = settings.value("Station/AdifTags").toStringList();
    bool hasNewField = false;
    for (const QString &tag : adifTags) {
        if (tag.contains("month") && tag.contains("Spelled Out")) {
            hasNewField = true;
            break;
        }
    }
    if (adifTags.isEmpty() || !hasNewField) {
        adifTags.clear();
        adifTags << "<day> - Numerical (1 ... 31)"
                 << "<month> - Numerical (1 ... 12)"
                 << "<month> - Spelled Out (January ... December)"
                 << "<month> - Abbreviated (Jan ... Dec)"
                 << "<year> - Numerical"
                 << "<time_utc> - UTC Time Of Contact"
                 << "<band> - Toggle-able to Frequency"
                 << "<mode>"
                 << "<tx_pwr>"
                 << "<rst_rcvd> - Received Signal Report"
                 << "<rst_sent> - Sent Signal Report"
                 << "<tone> - Numerical (1 - 9)"
                 << "<full_name> - Full Name of Contact"
                 << "<f_name> - First Name of Contact"
                 << "<l_name> - Last Name of Contact"
                 << "<call> - Callsign of Contact"
                 << "<address> - Address of Contact"
                 << "<country> - Country of Contact"
                 << "<qrp> - QSO <= 6Watts";
        settings.setValue("Station/AdifTags", adifTags);
    } else {
        bool hasQrpTag = false;
        for (const QString &tag : adifTags) {
            if (tag.contains("<qrp>")) {
                hasQrpTag = true;
                break;
            }
        }
        if (!hasQrpTag) {
            adifTags << "<qrp> - QSO <= 6Watts";
            settings.setValue("Station/AdifTags", adifTags);
        }
    }
    QVBoxLayout *adifVbox = new QVBoxLayout;
    createListBlock(adifVbox, "Defined QSLDesigner Tags", m_stationAdifTags, adifTags, "Designer Tag");
    grid->addLayout(adifVbox, 2, 0);

    m_pagesWidget->addWidget(page);
}

// Note: createDebugPage() and processDebugReport() removed per user request


static QString formatPrinterDisplay(const QString &kp) {
    QString target = kp.split(" ").first().trimmed();
    QString name = target;
    if (kp.contains("(") && kp.contains(")")) {
        int start = kp.indexOf("(") + 1;
        int end = kp.lastIndexOf(")");
        name = kp.mid(start, end - start).trimmed();
    }
    
    QString protocol = "CUPS";
    if (target.startsWith("ipp://")) protocol = "IPP";
    else if (target.startsWith("ipps://")) protocol = "IPPS";
    else if (target.startsWith("socket://") || target.startsWith("tcp://")) protocol = "TCP";
    else if (target.contains(QRegularExpression("^\\d+\\.\\d+\\.\\d+\\.\\d+"))) protocol = "Direct IP";
    else if (target.startsWith("http://") || target.startsWith("https://")) protocol = "HTTP";
    else if (!target.contains("://")) protocol = "CUPS System Printer";
    
    return QString("%1 [Connect via %2]").arg(name, protocol);
}
void ConfigDialog::updatePrintersList() {
    m_printersList->clear();
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QStringList knownPrinters = settings.value("KnownPrinters").toStringList();
    if (knownPrinters.isEmpty()) {
        QListWidgetItem *item = new QListWidgetItem("No printers defined.", m_printersList);
    } else {
        for (const QString &kp : knownPrinters) {
            QListWidgetItem *item = new QListWidgetItem(formatPrinterDisplay(kp), m_printersList);
            item->setData(Qt::UserRole, kp);
            item->setToolTip(kp);
            QPixmap pix(64, 64);
            pix.fill(Qt::transparent);
            QPainter p(&pix);
            p.setRenderHint(QPainter::Antialiasing);
            p.setPen(QPen(Qt::darkGray, 2));
            p.setBrush(Qt::lightGray);
            if (kp.toLower().contains("inkjet") || kp.toLower().contains("ink")) {
                p.drawRect(10, 20, 44, 30);
                p.setBrush(Qt::white);
                p.drawRect(20, 10, 24, 10);
                p.setBrush(Qt::cyan); p.drawEllipse(15, 30, 8, 8);
                p.setBrush(Qt::magenta); p.drawEllipse(25, 30, 8, 8);
                p.setBrush(Qt::yellow); p.drawEllipse(35, 30, 8, 8);
            } else {
                p.drawRect(10, 20, 44, 25);
                p.setBrush(Qt::white);
                p.drawRect(20, 10, 24, 10);
                p.drawRect(15, 45, 34, 15);
                p.setPen(Qt::black);
                p.drawLine(20, 50, 44, 50);
                p.drawLine(20, 55, 44, 55);
            }
            item->setIcon(QIcon(pix));
        }
    }
}

void ConfigDialog::createPrintersPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    
    layout->addWidget(new QLabel("<b>Defined WiFi Printers</b>"));
    
    m_printersList = new QListWidget(this);
    m_printersList->setViewMode(QListView::IconMode);
    m_printersList->setIconSize(QSize(64, 64));
    m_printersList->setResizeMode(QListView::Adjust);
    layout->addWidget(m_printersList);
    
    m_printersList->setContextMenuPolicy(Qt::CustomContextMenu);
    auto showProperties = [this](QListWidgetItem *item) {
        if (!item || item->text() == "No printers defined.") return;
        QString fullText = item->text();
        QString dest = fullText.split(" ").first().trimmed();
        
        QDialog propsDlg(this);
        propsDlg.setWindowTitle(QString("Printer Properties - %1").arg(dest));
        propsDlg.resize(500, 400);
        QVBoxLayout *layout = new QVBoxLayout(&propsDlg);
        
        QTabWidget *tabWidget = new QTabWidget(&propsDlg);
        layout->addWidget(tabWidget);
        
        QProcess proc;
        proc.start("lpoptions", QStringList() << "-p" << dest << "-l");
        proc.waitForFinished(3000);
        QString output = proc.readAllStandardOutput();
        if (output.trimmed().isEmpty()) {
            QLabel *lbl = new QLabel("No CUPS options found for this printer.\n\nCould be an IP/Network printer not explicitly added to CUPS.", &propsDlg);
            lbl->setAlignment(Qt::AlignCenter);
            layout->addWidget(lbl);
            
            QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Close, &propsDlg);
            connect(btnBox, &QDialogButtonBox::rejected, &propsDlg, &QDialog::reject);
            layout->addWidget(btnBox);
        } else {
            QStringList lines = output.split('\n', Qt::SkipEmptyParts);
            QMap<QString, QVBoxLayout*> tabLayouts;
            QMap<QString, QComboBox*> optionCombos;
            
            for (const QString &line : lines) {
                int colonIdx = line.indexOf(':');
                if (colonIdx == -1) continue;
                QString optDef = line.left(colonIdx).trimmed();
                QString optChoicesStr = line.mid(colonIdx + 1).trimmed();
                
                QString optKey = optDef.split('/').first();
                QString optName = optDef.contains('/') ? optDef.split('/').last() : optKey;
                
                QString tabName = "Other";
                QString kLower = optKey.toLower();
                if (kLower.contains("page") || kLower.contains("media") || kLower.contains("paper") || kLower.contains("tray") || kLower.contains("slot")) {
                    tabName = "Paper / Media";
                } else if (kLower.contains("resolution") || kLower.contains("quality") || kLower.contains("color") || kLower.contains("dpi") || kLower.contains("printoutmode")) {
                    tabName = "Quality / Color";
                } else if (kLower.contains("duplex") || kLower.contains("sides") || kLower.contains("collate") || kLower.contains("copies")) {
                    tabName = "Layout / Finishing";
                }
                
                if (!tabLayouts.contains(tabName)) {
                    QWidget *w = new QWidget;
                    QVBoxLayout *l = new QVBoxLayout(w);
                    l->setAlignment(Qt::AlignTop);
                    tabLayouts.insert(tabName, l);
                    
                    QScrollArea *sa = new QScrollArea;
                    sa->setWidgetResizable(true);
                    sa->setWidget(w);
                    tabWidget->addTab(sa, tabName);
                }
                
                QHBoxLayout *row = new QHBoxLayout;
                row->addWidget(new QLabel(optName + ":"));
                
                QComboBox *combo = new QComboBox;
                QStringList choicesList = optChoicesStr.split(" ", Qt::SkipEmptyParts);
                int currentIndex = 0;
                int idx = 0;
                for (QString choice : choicesList) {
                    if (choice.startsWith('*')) {
                        choice = choice.remove(0, 1);
                        currentIndex = idx;
                    }
                    combo->addItem(choice);
                    idx++;
                }
                combo->setCurrentIndex(currentIndex);
                
                row->addWidget(combo);
                tabLayouts[tabName]->addLayout(row);
                optionCombos.insert(optKey, combo);
            }
            
            QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &propsDlg);
            QDialog* pDlg = &propsDlg;
            connect(btnBox, &QDialogButtonBox::accepted, [optionCombos, dest, pDlg]() {
                for (auto it = optionCombos.begin(); it != optionCombos.end(); ++it) {
                    QString optKey = it.key();
                    QComboBox *c = it.value();
                    QString val = c->currentText();
                    
                    QProcess pSet;
                    pSet.start("lpoptions", QStringList() << "-p" << dest << "-o" << QString("%1=%2").arg(optKey, val));
                    pSet.waitForFinished();
                }
                pDlg->accept();
            });
            connect(btnBox, &QDialogButtonBox::rejected, &propsDlg, &QDialog::reject);
            layout->addWidget(btnBox);
        }
        
        propsDlg.exec();
    };

    connect(m_printersList, &QListWidget::itemDoubleClicked, this, showProperties);
    connect(m_printersList, &QListWidget::customContextMenuRequested, this, [this, showProperties](const QPoint &pos) {
        QListWidgetItem *item = m_printersList->itemAt(pos);
        if (!item || item->text() == "No printers defined.") return;
        QMenu menu(this);
        QAction *renameAction = menu.addAction("Rename Printer...");
        QAction *propAction = menu.addAction("Properties / Settings...");
        QAction *selected = menu.exec(m_printersList->mapToGlobal(pos));
        if (selected == propAction) {
            showProperties(item);
        } else if (selected == renameAction) {
            bool ok;
            QString newName = QInputDialog::getText(this, "Rename Printer", "Enter new name, format should be 'TARGET (Friendly Name)':", QLineEdit::Normal, item->text(), &ok);
            if (ok && !newName.isEmpty()) {
                QString oldName = item->data(Qt::UserRole).toString();
                QSettings settings(QCoreApplication::organizationName(), "Qwain");
                QStringList knownPrinters = settings.value("KnownPrinters").toStringList();
                int idx = knownPrinters.indexOf(oldName);
                if (idx != -1) {
                    knownPrinters[idx] = newName;
                    settings.setValue("KnownPrinters", knownPrinters);
                    updatePrintersList();
                }
            }
        }
    });

    updatePrintersList();
    
    QHBoxLayout *btnL = new QHBoxLayout;
    
    auto createBtnIcon = [](const QString &type) {
        QPixmap pix(48, 48);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        
        if (type == "test") {
            p.setPen(QPen(Qt::darkGray, 2)); p.setBrush(Qt::white);
            p.drawRect(12, 6, 24, 36);
            p.setPen(QPen(Qt::gray, 2));
            p.drawLine(16, 12, 32, 12);
            p.drawLine(16, 16, 32, 16);
            p.drawLine(16, 20, 32, 20);
            p.setPen(QPen(Qt::blue, 2));
            p.drawLine(16, 28, 28, 28);
            return QIcon(pix);
        }

        // Draw printer base for add/wifi/remove
        p.setPen(QPen(Qt::darkGray, 2)); p.setBrush(Qt::lightGray);
        p.drawRect(8, 15, 32, 16); // main body
        p.setBrush(Qt::white);
        p.drawRect(16, 8, 16, 7); // paper tray
        
        // Output tray
        p.drawRect(12, 31, 24, 10);
        p.setPen(QPen(Qt::black, 1));
        p.drawLine(16, 35, 32, 35);
        p.drawLine(16, 38, 32, 38);

        if (type == "add") {
            p.setPen(QPen(Qt::green, 4));
            p.drawLine(24, 18, 24, 28);
            p.drawLine(19, 23, 29, 23);
        } else if (type == "wifi") {
            p.setPen(QPen(Qt::blue, 2));
            p.drawArc(19, 18, 10, 10, 45*16, 90*16);
            p.drawArc(16, 15, 16, 16, 45*16, 90*16);
            p.drawArc(13, 12, 22, 22, 45*16, 90*16);
        } else if (type == "remove") {
            p.setPen(QPen(Qt::red, 4));
            p.drawLine(18, 23, 30, 23);
        }
        
        return QIcon(pix);
    };

    QToolButton *addBtn = new QToolButton(this);
    addBtn->setText("Add Printer");
    addBtn->setIcon(createBtnIcon("add"));
    addBtn->setIconSize(QSize(48, 48));
    addBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QToolButton *findBtn = new QToolButton(this);
    findBtn->setText("Find Printer");
    findBtn->setIcon(createBtnIcon("wifi"));
    findBtn->setIconSize(QSize(48, 48));
    findBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QToolButton *rmBtn = new QToolButton(this);
    rmBtn->setText("Remove Printer");
    rmBtn->setIcon(createBtnIcon("remove"));
    rmBtn->setIconSize(QSize(48, 48));
    rmBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QToolButton *testPrintBtn = new QToolButton(this);
    testPrintBtn->setText("Print Test Page");
    testPrintBtn->setIcon(createBtnIcon("test"));
    testPrintBtn->setIconSize(QSize(48, 48));
    testPrintBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    
    connect(addBtn, &QToolButton::clicked, this, [this]() {
        QDialog addDlg(this);
        addDlg.setWindowTitle("Add Printer");
        addDlg.setMinimumWidth(400);
        QVBoxLayout *layout = new QVBoxLayout(&addDlg);
        
        QGroupBox *directGroup = new QGroupBox("Add Direct Qwain Printer");
        QVBoxLayout *directLayout = new QVBoxLayout(directGroup);
        QLabel *directHelp = new QLabel("Enter the IP Address or URL of a network printer to print directly to it, bypassing system drivers.");
        directHelp->setWordWrap(true);
        directLayout->addWidget(directHelp);
        
        QHBoxLayout *ipLayout = new QHBoxLayout();
        ipLayout->addWidget(new QLabel("IP/URL:"));
        QLineEdit *ipEdit = new QLineEdit();
        ipEdit->setPlaceholderText("e.g. 192.168.1.50 or ipps://server/ipp/print");
        ipLayout->addWidget(ipEdit);
        directLayout->addLayout(ipLayout);
        
        QPushButton *addDirectBtn = new QPushButton("Add Direct Printer");
        addDirectBtn->setDefault(true);
        directLayout->addWidget(addDirectBtn);
        layout->addWidget(directGroup);
        
        QGroupBox *cupsGroup = new QGroupBox("Add System (CUPS) Printer");
        QVBoxLayout *cupsLayout = new QVBoxLayout(cupsGroup);
        QLabel *cupsHelp = new QLabel("Use the system printer configuration tools to install drivers and manage CUPS queues.");
        cupsHelp->setWordWrap(true);
        cupsLayout->addWidget(cupsHelp);
        
        QPushButton *btnCupsSys = new QPushButton("Launch System Printer Config");
        QPushButton *btnCupsWeb = new QPushButton("Launch CUPS Web Interface");
        cupsLayout->addWidget(btnCupsSys);
        cupsLayout->addWidget(btnCupsWeb);
        layout->addWidget(cupsGroup);
        
        QPushButton *cancelBtn = new QPushButton("Cancel");
        layout->addWidget(cancelBtn);
        layout->setAlignment(cancelBtn, Qt::AlignRight);
        
        connect(addDirectBtn, &QPushButton::clicked, [&](){
            QString ip = ipEdit->text().trimmed();
            if (ip.isEmpty()) {
                QMessageBox::warning(&addDlg, "Input Required", "Please enter an IP address or URL.");
                return;
            }
            addDlg.done(1);
        });
        
        connect(btnCupsSys, &QPushButton::clicked, [&](){
            QProcess::startDetached("system-config-printer", QStringList());
            addDlg.reject();
        });
        
        connect(btnCupsWeb, &QPushButton::clicked, [&](){
            QDesktopServices::openUrl(QUrl("http://localhost:631/admin"));
            addDlg.reject();
        });
        
        connect(cancelBtn, &QPushButton::clicked, &addDlg, &QDialog::reject);
        
        if (addDlg.exec() == 1) {
            QString ip = ipEdit->text().trimmed();
            if (!ip.isEmpty()) {
                QSettings settings(QCoreApplication::organizationName(), "Qwain");
                QStringList knownPrinters = settings.value("KnownPrinters").toStringList();
                
                bool exists = false;
                for (const QString &kp : knownPrinters) {
                    QString dest = kp.split(" ").first().trimmed();
                    if (dest == ip || dest.contains("//" + ip + ":") || dest.contains("//" + ip + "/") || dest.endsWith("//" + ip)) { 
                        exists = true; 
                        break; 
                    }
                }
                if (!exists) {
                    QString name = getPrinterName(ip);
                    knownPrinters.append(name);
                    settings.setValue("KnownPrinters", knownPrinters);
                    updatePrintersList();
                } else {
                    QMessageBox::information(this, "Printer Exists", "This printer is already in your Direct Printers list.");
                }
            }
        }
    });
    
    connect(rmBtn, &QToolButton::clicked, this, [this]() {
        QDialog rmDlg(this);
        rmDlg.setWindowTitle("Remove Printer");
        rmDlg.setMinimumWidth(400);
        QVBoxLayout *layout = new QVBoxLayout(&rmDlg);
        
        QGroupBox *directGroup = new QGroupBox("Remove Direct Qwain Printer");
        QVBoxLayout *directLayout = new QVBoxLayout(directGroup);
        QLabel *directHelp = new QLabel("Remove the currently selected Direct Printer from Qwain.");
        directHelp->setWordWrap(true);
        directLayout->addWidget(directHelp);
        
        QPushButton *rmDirectBtn = new QPushButton("Remove Selected Direct Printer");
        rmDirectBtn->setDefault(true);
        directLayout->addWidget(rmDirectBtn);
        layout->addWidget(directGroup);
        
        if (!m_printersList->currentItem() || m_printersList->currentItem()->text() == "No printers defined.") {
            rmDirectBtn->setEnabled(false);
            rmDirectBtn->setText("No Direct Printer Selected");
        }
        
        QGroupBox *cupsGroup = new QGroupBox("Remove System (CUPS) Printer");
        QVBoxLayout *cupsLayout = new QVBoxLayout(cupsGroup);
        QLabel *cupsHelp = new QLabel("Use the system printer configuration tools to remove installed CUPS printers.");
        cupsHelp->setWordWrap(true);
        cupsLayout->addWidget(cupsHelp);
        
        QPushButton *btnCupsSys = new QPushButton("Launch System Printer Config");
        QPushButton *btnCupsWeb = new QPushButton("Launch CUPS Web Interface");
        cupsLayout->addWidget(btnCupsSys);
        cupsLayout->addWidget(btnCupsWeb);
        layout->addWidget(cupsGroup);
        
        QPushButton *cancelBtn = new QPushButton("Cancel");
        layout->addWidget(cancelBtn);
        layout->setAlignment(cancelBtn, Qt::AlignRight);
        
        connect(rmDirectBtn, &QPushButton::clicked, [&](){
            rmDlg.done(1);
        });
        
        connect(btnCupsSys, &QPushButton::clicked, [&](){
            QProcess::startDetached("system-config-printer", QStringList());
            rmDlg.reject();
        });
        
        connect(btnCupsWeb, &QPushButton::clicked, [&](){
            QDesktopServices::openUrl(QUrl("http://localhost:631/printers"));
            rmDlg.reject();
        });
        
        connect(cancelBtn, &QPushButton::clicked, &rmDlg, &QDialog::reject);
        
        if (rmDlg.exec() == 1) {
            if (!m_printersList->currentItem() || m_printersList->currentItem()->text() == "No printers defined.") return;
            QString ipDesc = m_printersList->currentItem()->data(Qt::UserRole).toString();
            QString friendly = m_printersList->currentItem()->text();
            
            if (QMessageBox::question(this, "Remove Printer", "Are you sure you want to remove '" + friendly + "' from Qwain's direct printers?", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                QSettings settings(QCoreApplication::organizationName(), "Qwain");
                QStringList knownPrinters = settings.value("KnownPrinters").toStringList();
                knownPrinters.removeAll(ipDesc);
                settings.setValue("KnownPrinters", knownPrinters);
                updatePrintersList();
            }
        }
    });

    connect(testPrintBtn, &QToolButton::clicked, this, [this]() {
        if (!m_printersList->currentItem() || m_printersList->currentItem()->text() == "No printers defined.") return;
        QString ip = m_printersList->currentItem()->data(Qt::UserRole).toString();
        if (ip.contains(" ")) {
            ip = ip.split(" ").first().trimmed();
        }
        
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName("config_test_page.pdf");
        
        QPainter painter(&printer);
        painter.setFont(QFont("Arial", 16, QFont::Bold));
        painter.drawText(200, 400, "ADIF Monitor - Printer Test Page");
        
        painter.setFont(QFont("Arial", 12));
        QDateTime now = QDateTime::currentDateTime();
        int y = 600;
        int step = 150;
        painter.drawText(200, y, "Date/Time: " + QLocale().toString(now, QLocale::LongFormat)); y += step;
        painter.drawText(200, y, "Printer target: " + ip); y += step;
        
        QProcess unameInfo;
        unameInfo.start("uname", QStringList() << "-a");
        unameInfo.waitForFinished();
        QString sysInfo = unameInfo.readAllStandardOutput().trimmed();
        if (sysInfo.isEmpty()) {
            sysInfo = "Windows (or OS info not available via uname)";
        }
        painter.drawText(200, y, "System: " + sysInfo); y += step;
        painter.drawText(200, y, "Program Version: adif_gui (working version)");
        
        painter.end();
        
        // Print it via lp
        QProcess lpProc;
        if (ip.startsWith("ipp://") || ip.startsWith("ipps://") || ip.startsWith("http://") || ip.startsWith("https://") || ip.count('.') == 3) {
            QString host = ip;
            if (host.contains("://")) {
                host = QUrl(host).host();
            }
            lpProc.start("lp", QStringList() << "-h" << host + ":631" << "-o" << "raw" << "config_test_page.pdf");
        } else {
            lpProc.start("lp", QStringList() << "-d" << ip << "config_test_page.pdf");
        }
        lpProc.waitForFinished();
        int ret = lpProc.exitCode();
        if (lpProc.exitStatus() == QProcess::NormalExit && ret == 0) {
            QMessageBox::information(this, "Print", "Test Page successfully sent via lp.");
        } else {
            QString errOutput = lpProc.readAllStandardError();
            if (errOutput.isEmpty()) errOutput = "Process failed or could not be found.";
            QMessageBox::critical(this, "Error", "Error sending test page:\n" + errOutput);
        }
    });

    connect(findBtn, &QToolButton::clicked, this, [this]() {
        QProgressDialog pd("Searching for WiFi printers via mDNS and CUPS...", "Cancel", 0, 0, this);
        pd.setWindowModality(Qt::WindowModal);
        pd.setCancelButton(nullptr);
        pd.show();
        QCoreApplication::processEvents();

        QString out;
        QStringList protocols = {"_pdl-datastream._tcp", "_printer._tcp", "_ipp._tcp", "_ipps._tcp", "_http._tcp"};
        for (const QString &proto : protocols) {
            QProcess proc;
            proc.start("avahi-browse", QStringList() << "-rtp" << proto);
            proc.waitForFinished(2000);
            out += proc.readAllStandardOutput();
            out += "\n";
            QCoreApplication::processEvents();
        }

        // Also query CUPS
        QProcess lpinfo;
        lpinfo.start("lpinfo", QStringList() << "-v");
        lpinfo.waitForFinished(3000);
        QString lpinfoOut = lpinfo.readAllStandardOutput();

        QProcess lpstat;
        lpstat.start("lpstat", QStringList() << "-a");
        lpstat.waitForFinished();
        QString lpstatOut = lpstat.readAllStandardOutput();
        
        pd.close();

        QStringList ipsFound;
        QStringList lines = out.split('\n');
        for (const QString &line : lines) {
            QStringList parts = line.split(';');
            if (parts.size() >= 8 && parts[0] == "=") {
                QString ip = parts[7];
                if (!ipsFound.contains(ip)) {
                    ipsFound.append(ip);
                }
            }
        }
        
        QStringList lpLines = lpinfoOut.split('\n');
        for (const QString &line : lpLines) {
            if (line.startsWith("network ")) {
                QString uri = line.mid(8).trimmed(); // Extract URI
                if (uri.contains("://") && !ipsFound.contains(uri)) {
                    ipsFound.append(uri);
                }
            }
        }

        // Add local configured CUPS queues
        for (const QString &line : lpstatOut.split('\n')) {
            if (line.contains(" accepting requests")) {
                QString queue = line.section(" ", 0, 0);
                if (!ipsFound.contains(queue)) {
                    ipsFound.append(queue);
                }
            }
        }

        if (ipsFound.isEmpty()) {
            QMessageBox::information(this, "Find WiFi Printer", "No printers found on the local network.");
        } else {
            // resolve names first so the list is nice
            pd.setLabelText("Resolving printer names...");
            pd.show();
            QStringList displayList;
            for (const QString &ip : ipsFound) {
                QCoreApplication::processEvents();
                displayList.append(getPrinterName(ip));
            }
            pd.close();

            bool ok;
            QString choice = QInputDialog::getItem(this, "Select Printer", "Printers found:", displayList, 0, false, &ok);
            if (ok && !choice.isEmpty()) {
                QSettings settings(QCoreApplication::organizationName(), "Qwain");
                QStringList knownPrinters = settings.value("KnownPrinters").toStringList();
                QString ip = choice.split(" ").first();
                
                bool exists = false;
                for (const QString &kp : knownPrinters) {
                    QString dest = kp.split(" ").first().trimmed();
                    if (dest == ip || dest.contains("//" + ip + ":") || dest.contains("//" + ip + "/") || dest.endsWith("//" + ip)) { 
                        exists = true; 
                        break; 
                    }
                }
                if (!exists) {
                    knownPrinters.append(choice);
                    settings.setValue("KnownPrinters", knownPrinters);
                    updatePrintersList();
                    QMessageBox::information(this, "Printer Added", "Printer added to Qwain Direct Printers successfully.");
                } else {
                    QMessageBox::information(this, "Printer Exists", "This printer is already in your Direct Printers list.");
                }
            }
        }
    });
    
    btnL->addWidget(addBtn);
    btnL->addWidget(findBtn);
    btnL->addWidget(testPrintBtn);
    btnL->addWidget(rmBtn);
    btnL->addStretch();
    layout->addLayout(btnL);

    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    bool autoScanActive = settings.value("PrinterAutoScan", true).toBool();
    QPushButton *autoScanBtn = new QPushButton("New Printer Detection " + QString(autoScanActive ? "ACTIVE" : "NOT ACTIVE"));
    autoScanBtn->setStyleSheet(autoScanActive ? "background-color: lightgreen; font-weight: bold;" : "background-color: lightcoral; font-weight: bold;");
    connect(autoScanBtn, &QPushButton::clicked, this, [this, autoScanBtn]() {
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        bool active = settings.value("PrinterAutoScan", true).toBool();
        active = !active;
        settings.setValue("PrinterAutoScan", active);
        autoScanBtn->setText("New Printer Detection " + QString(active ? "ACTIVE" : "NOT ACTIVE"));
        autoScanBtn->setStyleSheet(active ? "background-color: lightgreen; font-weight: bold;" : "background-color: lightcoral; font-weight: bold;");
    });
    layout->addWidget(autoScanBtn);
    
    m_pagesWidget->addWidget(page);
}

void ConfigDialog::chooseSkinColor()
{
    QString originalColor = m_selectedColor;
    StyleDialog dlg(QFont(), QColor(m_selectedColor), this, true);

    // Real-time live update: repaints miniature console as the user interacts with the color picker
    connect(&dlg, &StyleDialog::styleChanged, this, [this](const QFont &, const QColor &c) {
        if (c.isValid()) {
            m_selectedColor = c.name();
            if (m_currentColorLabel) m_currentColorLabel->setText(m_selectedColor);
            if (m_swatchFrame) m_swatchFrame->setStyleSheet(QString("background-color: %1; border: 1px solid #4f5b66; border-radius: 4px;").arg(m_selectedColor));
            updateMiniConsolePreview();
            updatePreviewButtonStyle();
        }
    });

    if (dlg.exec() == QDialog::Accepted) {
        m_selectedColor = dlg.selectedColor().name();
    } else {
        // Revert on cancel
        m_selectedColor = originalColor;
    }
    if (m_currentColorLabel) m_currentColorLabel->setText(m_selectedColor);
    if (m_swatchFrame) m_swatchFrame->setStyleSheet(QString("background-color: %1; border: 1px solid #4f5b66; border-radius: 4px;").arg(m_selectedColor));
    updateMiniConsolePreview();
    updatePreviewButtonStyle();
    updateUIPreview();
}

void ConfigDialog::testQrzLogin()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("Credentials");
    settings.setValue("username", m_qrzUserEdit->text());
    settings.setValue("password", m_qrzPassEdit->text());
    settings.setValue("qrzApiKey", m_qrzApiKeyEdit->text());
    settings.endGroup();
    
    MainWindow *mw = qobject_cast<MainWindow*>(parentWidget());
    if (mw) mw->testQrzLogin();
}

void ConfigDialog::showSeed()
{
    MainWindow *mw = qobject_cast<MainWindow*>(parentWidget());
    if (mw) {
        QString seed = mw->getQrzSessionKey();
        QMessageBox::information(this, "QRZ Session Data", "Session Key: " + (seed.isEmpty() ? "Not connected or initialized" : seed));
    }
}

void ConfigDialog::prepNewLogfile()
{
    MainWindow *mw = qobject_cast<MainWindow*>(parentWidget());
    if (mw) mw->prepareNewAdi();
}

void ConfigDialog::forceSyncLogfile()
{
    MainWindow *mw = qobject_cast<MainWindow*>(parentWidget());
    if (mw) mw->forceSyncQrzLog();
}

void ConfigDialog::startMailCheck()
{
    MainWindow *mw = qobject_cast<MainWindow*>(parentWidget());
    if (mw) mw->startQslMailCheck();
}

void ConfigDialog::applySettings()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    
    settings.setValue("Station/Callsign", m_stationCallsign->text());
    settings.setValue("Station/Grid", m_gridEdit->text());
    QStringList radios;
    for(int i=0; i<m_stationRadios->count(); ++i) radios << m_stationRadios->item(i)->text();
    settings.setValue("Station/Radios", radios);
    QStringList antennas;
    for(int i=0; i<m_stationAntennas->count(); ++i) antennas << m_stationAntennas->item(i)->text();
    settings.setValue("Station/Antennas", antennas);
    QStringList bands;
    for(int i=0; i<m_stationBands->count(); ++i) bands << m_stationBands->item(i)->text();
    settings.setValue("Station/Bands", bands);
    QStringList checks;
    for(int i=0; i<m_stationChecks->count(); ++i) checks << m_stationChecks->item(i)->text();
    settings.setValue("Station/Checks", checks);
    QStringList adifTags;
    for (int i=0; i<m_stationAdifTags->count(); ++i) adifTags << m_stationAdifTags->item(i)->text();
    settings.setValue("Station/AdifTags", adifTags);

    settings.beginGroup("Network");
    settings.setValue("protocol", m_protocolCombo->currentText());
    settings.setValue("ip", m_ipEdit->text());
    settings.setValue("port", m_portSpin->value());
    settings.endGroup();
    
    settings.beginGroup("Credentials");
    settings.setValue("username", m_qrzUserEdit->text());
    settings.setValue("password", m_qrzPassEdit->text());
    settings.setValue("qrzApiKey", m_qrzApiKeyEdit->text());
    settings.setValue("qrzXmlSubscriber", m_qrzXmlSubCheckbox->isChecked());
    settings.setValue("arrlMember", m_arrlMemberCheckbox->isChecked());
    settings.setValue("lotwUser", m_lotwUserEdit->text());
    settings.setValue("lotwPass", m_lotwPassEdit->text());
    settings.setValue("clublogEmail", m_clublogEmailEdit->text());
    settings.setValue("clublogCall", m_clublogCallEdit->text());
    settings.setValue("clublogPass", m_clublogPassEdit->text());
    settings.setValue("clublogApiKey", m_clublogApiKeyEdit->text());
    settings.setValue("eqslUser", m_eqslUserEdit->text());
    settings.setValue("eqslPass", m_eqslPassEdit->text());
    settings.endGroup();
    
    settings.beginGroup("Feeds");
    settings.setValue("feed1Enabled", m_feed1Check->isChecked());
    settings.setValue("feed1Url", m_feed1UrlEdit->text());
    settings.setValue("feed1Refresh", m_feed1RefreshCombo->currentText());
    settings.setValue("feed2Enabled", m_feed2Check->isChecked());
    settings.setValue("feed2Url", m_feed2UrlEdit->text());
    settings.setValue("feed2Refresh", m_feed2RefreshCombo->currentText());
    settings.setValue("feed3Enabled", m_feed3Check->isChecked());
    settings.setValue("feed3Url", m_feed3UrlEdit->text());
    settings.setValue("feed3Refresh", m_feed3RefreshCombo->currentText());
    
    settings.setValue("dxClusterServer", m_dxClusterEdit->text());
    
    QStringList serversToSave;
    for(int i = 0; i < m_dxClusterListWidget->count(); ++i) {
        serversToSave << m_dxClusterListWidget->item(i)->text();
    }
    settings.setValue("dxClusterList", serversToSave);

    settings.setValue("feed1ScrollDir", m_scrollDirCombo1->currentData().toString());
    settings.setValue("feed1ScrollSpeed", m_scrollSpeedSlider1->value());
    settings.setValue("feed2ScrollDir", m_scrollDirCombo2->currentData().toString());
    settings.setValue("feed2ScrollSpeed", m_scrollSpeedSlider2->value());
    settings.setValue("feed3ScrollDir", m_scrollDirCombo3->currentData().toString());
    settings.setValue("feed3ScrollSpeed", m_scrollSpeedSlider3->value());
    settings.endGroup();
    
    settings.beginGroup("Files");
    settings.setValue("userDirectory", m_userDirectoryEdit->text());
    settings.setValue("logfile", m_logfileEdit->text());
    settings.setValue("systemLogfile", m_systemLogfileEdit->text());
    settings.setValue("debugLogfile", m_debugLogfileEdit->text());
    settings.endGroup();
    
    settings.setValue("SkinColor", m_selectedColor);
    settings.setValue("MapStyle", m_mapStyleCombo->currentIndex());
    
    settings.beginGroup("UI");
    settings.setValue("statusScrollLimit", m_statusScrollLimit->value());
    settings.setValue("alwaysEastern", m_alwaysEasternCheck->isChecked());
    settings.setValue("systemScheme", m_schemeCombo->currentText());
    settings.setValue("clockFont", m_clockFont);
    settings.setValue("clockColor", m_clockColor);
    settings.setValue("feed1Font", m_feed1Font);
    settings.setValue("feed1Color", m_feed1Color);
    settings.setValue("feed2Font", m_feed2Font);
    settings.setValue("feed2Color", m_feed2Color);
    settings.setValue("feed3Font", m_feed3Font);
    settings.setValue("feed3Color", m_feed3Color);
    settings.setValue("callInfoFont", m_callInfoFont);
    settings.setValue("callInfoColor", m_callInfoColor);
    settings.setValue("qsoListFont", m_qsoListFont);
    settings.setValue("qsoListColor", m_qsoListColor);
    settings.endGroup();

    // Save QSL Card Options
    settings.beginGroup("LogTweaker");
    settings.setValue("useTweaks", m_chkUseTweaks->isChecked());
    settings.setValue("useTweaks_FT2", m_chkFT2Tweaks->isChecked());
    settings.setValue("useTweaks_FT4", m_chkFT4Tweaks->isChecked());
    settings.setValue("useTweaks_FT8", m_chkFT8Tweaks->isChecked());
    settings.setValue("FT2_min", m_ft2MinSpin->value());
    settings.setValue("FT2_max", m_ft2MaxSpin->value());
    settings.setValue("FT4_min", m_ft4MinSpin->value());
    settings.setValue("FT4_max", m_ft4MaxSpin->value());
    settings.setValue("FT8_min", m_ft8MinSpin->value());
    settings.setValue("FT8_max", m_ft8MaxSpin->value());
    settings.endGroup();

    settings.beginGroup("QslUpdates");
    settings.setValue("UseStandardEmission", m_chkUseStandardEmissions->isChecked());
    settings.setValue("UseGenericEmission", m_chkUseGenericEmissions->isChecked());
    settings.setValue("UseQrpSubstitution", m_chkUseQrpSubstitution->isChecked());
    settings.setValue("UseCustomQrpBrag", m_chkCustomQrpBrag->isChecked());
    settings.setValue("CustomQrpBragText", m_txtCustomQrpBragText->text().trimmed());
    settings.endGroup();

    emit settingsApplied();
}

void ConfigDialog::accept() {
    applySettings();
    QDialog::accept();
}

bool ConfigDialog::eventFilter(QObject *obj, QEvent *event) {
    return QDialog::eventFilter(obj, event);
}

void ConfigDialog::filterLotwDatabase() {
    // Stub implementation to satisfy linker
}

void ConfigDialog::updatePreviewButtonStyle() {
    if (!m_schemeCombo || !m_schemeButtonPreview) return;
    QString scheme = m_schemeCombo->currentText();
    if (scheme == "None") {
        m_schemeButtonPreview->setStyleSheet("");
    } else if (scheme == "Light") {
        m_schemeButtonPreview->setStyleSheet(
            "QPushButton { "
            "   background-color: #f5f5f5; "
            "   color: #333333; "
            "   border: none; "
            "   border-radius: 4px; "
            "   padding: 6px 12px; "
            "} "
            "QPushButton:hover { "
            "   background-color: #e0e0e0; "
            "} "
            "QPushButton:pressed { "
            "   background-color: #d0d0d0; "
            "}"
        );
    } else if (scheme == "Dark") {
        m_schemeButtonPreview->setStyleSheet(
            "QPushButton { "
            "   background-color: #444444; "
            "   color: #ffffff; "
            "   border: none; "
            "   border-radius: 4px; "
            "   padding: 6px 12px; "
            "} "
            "QPushButton:hover { "
            "   background-color: #555555; "
            "} "
            "QPushButton:pressed { "
            "   background-color: #333333; "
            "}"
        );
    } else if (scheme == "Custom") {
        m_schemeButtonPreview->setStyleSheet(
            QString(
            "QPushButton { "
            "   background-color: %1; "
            "   color: #ffffff; "
            "   border: none; "
            "   border-radius: 4px; "
            "   padding: 6px 12px; "
            "} "
            "QPushButton:hover { "
            "   background-color: %1; "
            "   opacity: 0.9; "
            "}"
            ).arg(m_selectedColor)
        );
    }
}

// ============================================================================
// FUNCTION: updateMiniConsolePreview
// PURPOSE: Refreshes the real-time miniature radio console workspace preview
//          displaying staged skin background color, clock typography, callsign
//          styles, and QSO history rows.
// ============================================================================
void ConfigDialog::updateMiniConsolePreview() {
    if (!m_miniPreviewWorkspace) return;

    m_miniPreviewWorkspace->setStyleSheet(
        QString("QWidget#miniRadioWorkspace {"
                "  background-color: %1;"
                "  border: 1px solid #343e4b;"
                "  border-radius: 8px;"
                "}").arg(m_selectedColor)
    );

    if (m_miniClockLabel) {
        m_miniClockLabel->setFont(m_clockFont);
        m_miniClockLabel->setStyleSheet(QString("color: %1; background: transparent; font-weight: bold;").arg(m_clockColor.name()));
    }
    if (m_miniCallLabel) {
        m_miniCallLabel->setFont(m_callInfoFont);
        m_miniCallLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(m_callInfoColor.name()));
    }
    if (m_miniQsoLabel) {
        m_miniQsoLabel->setFont(m_qsoListFont);
        m_miniQsoLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(m_qsoListColor.name()));
    }
    if (m_swatchFrame) {
        m_swatchFrame->setStyleSheet(QString("background-color: %1; border: 1px solid #4f5b66; border-radius: 4px;").arg(m_selectedColor));
    }
    if (m_currentColorLabel) {
        m_currentColorLabel->setText(m_selectedColor);
    }
    updatePreviewButtonStyle();
}


