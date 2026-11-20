// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

// ============================================================================
// COMPONENT: QslManagerDialog
// ROLE: QSL database manager, search engine, and print queue director.
//       This component coordinates searches across the local QSO logbook,
//       filters results by band/mode/date/DXCC, updates local printed logs,
//       pulls profiles from QrzCache, and directs the QSL print queue list.
// INVOKED BY: MainWindow (toolbar button and menu options).
// PRODUCES: Filtered QSO listings, interactive profiles, and queued print jobs.
// ============================================================================

#include "qslmanagerdialog.h"
#include "qrzcache.h"
#include "qslpreviewdialog.h"
#include "paths.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QComboBox>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QClipboard>
#include <QApplication>
#include <QMouseEvent>
#include <QDateTime>
#include <QKeyEvent>
#include <QSettings>
#include <QTimer>
#include <QPointer>
#include <QFileDialog>
#include <QToolTip>
#include <QFileInfo>
#include <QCalendarWidget>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QInputDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QProgressBar>
#include <QPointer>
#include <QMenu>
#include <QContextMenuEvent>

// --- Dialogs ---

class ProcessingStatusDialog : public QDialog {
public:
    explicit ProcessingStatusDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Processing...");
        setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
        setModal(true);
        setFixedSize(250, 90);
        
        QVBoxLayout *layout = new QVBoxLayout(this);
        QLabel *lbl = new QLabel("Processing...", this);
        lbl->setAlignment(Qt::AlignCenter);
        
        QProgressBar *bar = new QProgressBar(this);
        bar->setRange(0, 0); // Indeterminate animated bar
        bar->setTextVisible(false);
        
        layout->addWidget(lbl);
        layout->addWidget(bar);
    }
};


struct SearchGuard {
    QslManagerDialog *dlg;
    bool isRoot;
    SearchGuard(QslManagerDialog *d) : dlg(d) {
        isRoot = !dlg->m_isExecuting;
        if (isRoot) dlg->m_isExecuting = true;
    }
    ~SearchGuard() {
        if (isRoot) {
            dlg->m_isExecuting = false;
            if (dlg->m_searchPending) {
                dlg->m_searchPending = false;
                QslManagerDialog *d = dlg;
                QTimer::singleShot(0, d, [d]() { d->executeLastSearch(); });
            }
        }
    }
};

#include "schemedesigner.h"

class DateRangeDialog : public QDialog {
    Q_OBJECT
public:
    QCalendarWidget *startCal;
    QCalendarWidget *endCal;
    QLabel *statusLbl;
    int currentFontSize;
    QDialogButtonBox *bb;

    DateRangeDialog(const QDate &start, const QDate &end, QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Select Date Range");
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        
        statusLbl = new QLabel("", this);
        QFont headerFont = statusLbl->font();
        headerFont.setPointSize(12);
        statusLbl->setFont(headerFont);
        statusLbl->setAlignment(Qt::AlignCenter);
        
        QHBoxLayout *calLayout = new QHBoxLayout;
        
        QVBoxLayout *startLayout = new QVBoxLayout;
        startLayout->addWidget(new QLabel("<b>Beginning:</b>"));
        startCal = new QCalendarWidget(this);
        startCal->setGridVisible(true);
        startCal->setSelectedDate(start.isValid() ? start : QDateTime::currentDateTimeUtc().date().addYears(-1));
        startLayout->addWidget(startCal);
        calLayout->addLayout(startLayout);
        
        QVBoxLayout *endLayout = new QVBoxLayout;
        endLayout->addWidget(new QLabel("<b>Ending:</b>"));
        endCal = new QCalendarWidget(this);
        endCal->setGridVisible(true);
        endCal->setSelectedDate(end.isValid() ? end : QDateTime::currentDateTimeUtc().date());
        endLayout->addWidget(endCal);
        calLayout->addLayout(endLayout);

        mainLayout->addWidget(statusLbl);
        mainLayout->addLayout(calLayout);

        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        currentFontSize = settings.value("QslManager/CalendarFontSize", startCal->font().pointSize()).toInt();
        if (currentFontSize > 0) {
            QFont f = startCal->font();
            f.setPointSize(currentFontSize);
            startCal->setFont(f);
            endCal->setFont(f);
        }

        QHBoxLayout *bottomLayout = new QHBoxLayout;
        QPushButton *btnDec = new QPushButton("Font Size -", this);
        QPushButton *btnInc = new QPushButton("Font Size +", this);
        connect(btnDec, &QPushButton::clicked, this, [this]() {
            if (currentFontSize <= 6) return;
            currentFontSize--;
            applyFontSize();
        });
        connect(btnInc, &QPushButton::clicked, this, [this]() {
            if (currentFontSize >= 36) return;
            currentFontSize++;
            applyFontSize();
        });
        
        bottomLayout->addStretch();
        bottomLayout->addWidget(btnDec);
        bottomLayout->addWidget(btnInc);
        bottomLayout->addStretch();
        
        mainLayout->addLayout(bottomLayout);

        bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(bb, &QDialogButtonBox::accepted, this, &DateRangeDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
        
        connect(startCal, &QCalendarWidget::selectionChanged, this, &DateRangeDialog::updateStatus);
        connect(endCal, &QCalendarWidget::selectionChanged, this, &DateRangeDialog::updateStatus);

        mainLayout->addWidget(bb);
        updateStatus();
    }

    void applyFontSize() {
        QFont f = startCal->font();
        f.setPointSize(currentFontSize);
        startCal->setFont(f);
        endCal->setFont(f);
        
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        settings.setValue("QslManager/CalendarFontSize", currentFontSize);
        
        // This forces the dialog to resize to fit the new calendar sizes
        layout()->invalidate();
        adjustSize();
    }

private slots:
    void updateStatus() {
        QDate s = startCal->selectedDate();
        QDate e = endCal->selectedDate();
        if (s > e) {
            statusLbl->setText(QString("<font color='red'>Invalid Range: Beginning date is after Ending date!</font>"));
            bb->button(QDialogButtonBox::Ok)->setEnabled(false);
        } else {
            statusLbl->setText(QString("Searching for QSOs from <b style='color:#e67e22;'>%1</b> to <b style='color:#e67e22;'>%2</b>").arg(s.toString("yyyy-MM-dd"), e.toString("yyyy-MM-dd")));
            bb->button(QDialogButtonBox::Ok)->setEnabled(true);
        }
    }

    void accept() override {
        if (startCal->selectedDate() > endCal->selectedDate()) {
            return;
        }
        QDialog::accept();
    }
    
public:
    QDate startDate() const { return startCal->selectedDate(); }
    QDate endDate() const { return endCal->selectedDate(); }
};

class IconCenterDelegate : public QStyledItemDelegate {
public:
    explicit IconCenterDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        if (!icon.isNull()) {
            QStyleOptionViewItem opt = option;
            initStyleOption(&opt, index);

            // Draw background
            if (opt.state & QStyle::State_Selected) {
                painter->fillRect(opt.rect, opt.palette.highlight());
            } else {
                painter->fillRect(opt.rect, opt.palette.base());
            }

            // Draw the icon centered
            QSize iconSize = opt.decorationSize;
            if (iconSize.isEmpty()) iconSize = QSize(24, 16);
            QRect iconRect(opt.rect.center().x() - iconSize.width() / 2,
                           opt.rect.center().y() - iconSize.height() / 2,
                           iconSize.width(), iconSize.height());
            icon.paint(painter, iconRect, Qt::AlignCenter,
                       (opt.state & QStyle::State_Enabled) ? QIcon::Normal : QIcon::Disabled,
                       (opt.state & QStyle::State_Selected) ? QIcon::On : QIcon::Off);
        } else {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }
};

class CountryTableItem : public QTableWidgetItem {
public:
    CountryTableItem(const QString &country, const QIcon &icon) : QTableWidgetItem() {
        if (!icon.isNull()) {
            setIcon(icon);
            setToolTip(country);
            setData(Qt::UserRole, country); // stash country
        } else {
            setText(country);
            setData(Qt::UserRole, country);
        }
    }
    bool operator<(const QTableWidgetItem &other) const override {
        return data(Qt::UserRole).toString() < other.data(Qt::UserRole).toString();
    }
};

class CountryDxccDialog : public QDialog {
public:
    QRadioButton *rbCountry;
    QRadioButton *rbDxcc;
    QComboBox *comboCountry;
    QComboBox *comboDxcc;
    
    CountryDxccDialog(const QStringList &countries, const QStringList &dxccs, QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Country / DXCC");
        QVBoxLayout *l = new QVBoxLayout(this);
        
        rbCountry = new QRadioButton("Country", this);
        rbDxcc = new QRadioButton("DXCC", this);
        rbCountry->setChecked(true);
        
        comboCountry = new QComboBox(this);
        comboCountry->addItems(countries);
        
        comboDxcc = new QComboBox(this);
        comboDxcc->addItems(dxccs);
        comboDxcc->setEnabled(false);
        
        connect(rbCountry, &QRadioButton::toggled, this, [this](bool c){ comboCountry->setEnabled(c); });
        connect(rbDxcc, &QRadioButton::toggled, this, [this](bool c){ comboDxcc->setEnabled(c); });
        
        l->addWidget(rbCountry);
        l->addWidget(comboCountry);
        l->addWidget(rbDxcc);
        l->addWidget(comboDxcc);
        
        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
        l->addWidget(bb);
    }
    
    QString getSelection() const {
        if (rbCountry->isChecked()) return "COUNTRY:" + comboCountry->currentText();
        return "DXCC:" + comboDxcc->currentText();
    }
};

extern QPixmap getAppLogo();

QslManagerDialog::QslManagerDialog(MainWindow *mainWnd, QWidget *parent) 
    : QDialog(parent), m_mainWnd(mainWnd)
{
    setWindowTitle("Qwain Processing");
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    if (settings.contains("QslManager/Geometry")) {
        restoreGeometry(settings.value("QslManager/Geometry").toByteArray());
    } else {
        resize(1000, 600);
    }

    QString skinColor = settings.value("SkinColor", "#181b22").toString();
    this->setStyleSheet(QString("QDialog { background-color: %1; }").arg(skinColor));
    m_qsoListColor = QColor(settings.value("MainWindow/QsoListColor", "#00ff00").toString());
    
    m_userStartDate = QDate::fromString(settings.value("QslManager/StartDate", QDateTime::currentDateTimeUtc().date().toString("yyyy-MM-dd")).toString(), "yyyy-MM-dd");
    m_userEndDate = QDate::fromString(settings.value("QslManager/EndDate", QDateTime::currentDateTimeUtc().date().toString("yyyy-MM-dd")).toString(), "yyyy-MM-dd");
    m_filterActive = settings.value("QslManager/FilterActive", false).toBool();
    m_filterStartDate = m_userStartDate.toString("yyyyMMdd");
    m_filterEndDate = m_userEndDate.toString("yyyyMMdd");
    m_lastSearchMode = settings.value("QslManager/LastSearchMode", "all").toString();
    m_searchCountryDxcc = settings.value("QslManager/LastCountryDxcc", "").toString();
    
    setupUi();
    
    QTimer::singleShot(100, this, [this](){
        loadPrintQueue();
        executeLastSearch();
    });
}

QslManagerDialog::~QslManagerDialog()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.setValue("QslManager/Geometry", saveGeometry());
}

void QslManagerDialog::closeEvent(QCloseEvent *event)
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.setValue("QslManager/Geometry", saveGeometry());
    QDialog::closeEvent(event);
}

void QslManagerDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Top Row
    QVBoxLayout *topContainer = new QVBoxLayout();
    QHBoxLayout *topLayout = new QHBoxLayout();
    
    QFont techFont("Courier New", 11, QFont::Bold);
    techFont.setStyleHint(QFont::Monospace);
    QString btnStyle = "QPushButton { background-color: #2c3e50; color: white; border: 1px solid #34495e; border-radius: 4px; padding: 5px 10px; } "
                       "QPushButton:hover { background-color: #34495e; } "
                       "QPushButton:pressed { background-color: #1abc9c; }";
    
    m_callsignLookup = new QLineEdit(this);
    m_callsignLookup->setPlaceholderText("Callsign lookup");
    int charWidth = m_callsignLookup->fontMetrics().horizontalAdvance('X');
    int width10Chars = charWidth * 10 + 16;
    m_callsignLookup->setFixedWidth(width10Chars);
    m_callsignLookup->installEventFilter(this);
    connect(m_callsignLookup, &QLineEdit::returnPressed, this, &QslManagerDialog::doLookup);
    topLayout->addWidget(m_callsignLookup);

    m_searchMatchesList = new QListWidget(this);
    m_searchMatchesList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_searchMatchesList->hide();
    m_searchMatchesList->setStyleSheet(
        "QListWidget {"
        "  background-color: #2a2a2a;"
        "  color: white;"
        "  border: 1px solid #555555;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item {"
        "  padding: 4px 8px;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #3b82f6;"
        "  color: white;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #3b82f6;"
        "  color: white;"
        "}"
    );

    // Disabled matching popup suggestions/elevator list as requested by user.
    
    topLayout->addStretch();

    // Custom modern filter panel widget with Grid Layout as requested
    auto *filterPanel = new QWidget(this);
    auto *grid = new QGridLayout(filterPanel);

    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(1);

    // Column 0: label. Columns 1-3: equal-width, centered button columns.
    grid->setColumnStretch(0, 0);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 1);
    grid->setColumnStretch(3, 1);

    QSettings settings(QCoreApplication::organizationName(), "Qwain");

    m_chkMail = new QCheckBox("QSL by Mail", filterPanel);
    m_chkMail->setChecked(settings.value("QslManager/MailChecked", false).toBool());

    m_chkLotw = new QCheckBox("LoTW Member", filterPanel);
    m_chkLotw->setChecked(settings.value("QslManager/LotwChecked", false).toBool());

    m_chkDateRange = new QCheckBox(QString("Use Date Range (%1 to %2)")
                                   .arg(m_userStartDate.toString("yyyy-MM-dd"), m_userEndDate.toString("yyyy-MM-dd")), filterPanel);
    m_chkDateRange->setChecked(m_filterActive);

    QString chkStyle = "QCheckBox { font-family: 'Courier New'; font-weight: bold; font-size: 13px; } "
                       "QCheckBox::indicator { width: 16px; height: 16px; border: 2px solid #555; background: #222; border-radius: 3px; } "
                       "QCheckBox::indicator:checked { background: #2ecc71; border-color: #27ae60; }";
    m_chkLotw->setStyleSheet(chkStyle);
    m_chkMail->setStyleSheet(chkStyle);
    m_chkDateRange->setStyleSheet(chkStyle);

    connect(m_chkLotw, &QCheckBox::stateChanged, this, [this](int state) {
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        settings.setValue("QslManager/LotwChecked", state == Qt::Checked);
        executeLastSearch();
    });
    connect(m_chkMail, &QCheckBox::stateChanged, this, [this](int state) {
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        settings.setValue("QslManager/MailChecked", state == Qt::Checked);
        executeLastSearch();
    });
    connect(m_chkDateRange, &QCheckBox::stateChanged, this, [this](int state) {
        m_filterActive = (state == Qt::Checked);
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        settings.setValue("QslManager/FilterActive", m_filterActive);
        executeLastSearch();
    });

    grid->addWidget(m_chkMail,      0, 1);
    grid->addWidget(m_chkLotw,      0, 2);
    grid->addWidget(m_chkDateRange, 1, 1);

    auto *filterLabel = new QLabel(tr("Filter By:"), filterPanel);
    filterLabel->setStyleSheet("QLabel { font-family: 'Courier New'; font-weight: bold; font-size: 13px; color: #888; }");
    grid->addWidget(filterLabel, 2, 0, 1, 1, Qt::AlignRight | Qt::AlignTop);

    auto makeFilterButton = [filterPanel](const QString &text) {
        auto *button = new QToolButton(filterPanel);
        button->setText(text);
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setAutoRaise(true);
        button->setCheckable(false);
        button->setCursor(Qt::PointingHandCursor);
        button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        button->setStyleSheet("QToolButton { font-family: 'Courier New'; font-weight: bold; font-size: 12px; color: #2c87c0; padding: 2px 6px; border: none; background: transparent; } "
                              "QToolButton:hover { color: #5bc0de; background: rgba(255, 255, 255, 0.05); border-radius: 3px; } "
                              "QToolButton:pressed { color: #2ecc71; background: rgba(46, 204, 113, 0.1); border-radius: 3px; }");
        return button;
    };

    m_btnDateRange = makeFilterButton(tr("Date Range"));
    m_btnToday = makeFilterButton(tr("Today"));
    m_btnCountryDxcc = makeFilterButton(tr("Country/DXCC"));
    
    m_lastXDaysCount = settings.value("QslManager/LastXDays", 45).toInt();
    m_btnLastXDays = makeFilterButton(QString("Show Last %1 Days").arg(m_lastXDaysCount));
    m_btnPastWeek = makeFilterButton(tr("Past Week"));

    connect(m_btnDateRange, &QToolButton::clicked, this, &QslManagerDialog::filterByDateClicked);
    connect(m_btnToday, &QToolButton::clicked, this, &QslManagerDialog::todayClicked);
    connect(m_btnCountryDxcc, &QToolButton::clicked, this, &QslManagerDialog::countryDxccSearch);
    connect(m_btnLastXDays, &QToolButton::clicked, this, &QslManagerDialog::lastXDaysClicked);
    connect(m_btnPastWeek, &QToolButton::clicked, this, &QslManagerDialog::pastWeekClicked);

    grid->addWidget(m_btnDateRange,   2, 1, Qt::AlignHCenter | Qt::AlignVCenter);
    grid->addWidget(m_btnToday,       2, 2, Qt::AlignHCenter | Qt::AlignVCenter);
    grid->addWidget(m_btnCountryDxcc,  2, 3, Qt::AlignHCenter | Qt::AlignVCenter);
    grid->addWidget(m_btnLastXDays,   3, 1, Qt::AlignHCenter | Qt::AlignVCenter);
    grid->addWidget(m_btnPastWeek,    3, 2, Qt::AlignHCenter | Qt::AlignVCenter);

    topLayout->addWidget(filterPanel);
    topContainer->addLayout(topLayout);
    mainLayout->addLayout(topContainer);
    
    // Middle Area
    QHBoxLayout *midLayout = new QHBoxLayout();
    
    // Left: Callsign info
    QGroupBox *infoGroup = new QGroupBox("Callsign Info", this);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    m_infoBrowser = new QTextBrowser(this);
    m_infoBrowser->setOpenLinks(false);
    m_infoBrowser->setMouseTracking(true);
    m_infoBrowser->viewport()->setMouseTracking(true);
    m_infoBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_infoBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_infoBrowser->viewport()->installEventFilter(this);
    m_infoBrowser->setFrameShape(QFrame::NoFrame);
    m_infoBrowser->setStyleSheet("background: transparent;");
    m_infoBrowser->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_infoBrowser, &QTextBrowser::customContextMenuRequested, this, &QslManagerDialog::onInfoBrowserContextMenu);
    connect(m_infoBrowser, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        QString urlStr = url.toString();
        if (urlStr.startsWith("http://app.local/qsl_queue/")) {
            QString callsign = urlStr.mid(27);
            bool inQueue = false;
            for (int i=0; i<m_printQueueList->count(); ++i) {
                if (m_printQueueList->item(i)->text().split("|")[0].trimmed() == callsign) {
                    inQueue = true;
                    break;
                }
            }
            if (!inQueue) {
                QString queueItem = getQueueStringForCallsign(callsign);
                if (!queueItem.isEmpty()) {
                    m_printQueueList->addItem(queueItem);
                    m_btnPrintQsl->setEnabled(m_printQueueList->count() > 0);
                    savePrintQueue();
                }
            }
        } else if (urlStr.startsWith("http://app.local/print_now/")) {
            QString callsign = urlStr.mid(27);
            QList<QsoDetail> qsos = m_mainWnd->allQsos().value(callsign);
            // Guard against QslManagerDialog being closed/deleted during modal preview
            QPointer<QslManagerDialog> guard(this);
            // Parent to m_mainWnd to prevent QslManagerDialog's WA_DeleteOnClose from deleting stack-allocated dlg
            QslPreviewDialog dlg(callsign, m_infoBrowser->toHtml(), qsos, m_mainWnd);
            dlg.exec();
            
            // After returning, update UI in case QSL status was updated (if manager still exists)
            if (!guard) return;
            displayCallInfo(callsign);
            executeLastSearch();
        }
    });
    infoLayout->addWidget(m_infoBrowser);
    midLayout->addWidget(infoGroup, 2);
    
    // Center: Results
    QGroupBox *resGroup = new QGroupBox("Results from above searches", this);
    resGroup->setStyleSheet("QGroupBox { border: 1px solid gray; border-radius: 5px; margin-top: 1ex; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }");
    QVBoxLayout *resLayout = new QVBoxLayout(resGroup);
    
    QHBoxLayout *selectAllLayout = new QHBoxLayout();
    QPushButton *btnSelectAll = new QPushButton(tr("Select All"), this);
    btnSelectAll->setStyleSheet("QPushButton { border: none; background: transparent; text-decoration: underline; font-weight: bold; padding: 2px; text-align: left; color: #3b82f6; } QPushButton:hover { color: #2563eb; }");
    btnSelectAll->setCursor(Qt::PointingHandCursor);
    connect(btnSelectAll, &QPushButton::clicked, this, &QslManagerDialog::selectAllResults);
    selectAllLayout->addWidget(btnSelectAll);
    selectAllLayout->addStretch();
    resLayout->addLayout(selectAllLayout);

    m_resultsTable = new QTableWidget(this);
    m_resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultsTable->setSortingEnabled(true);
    m_resultsTable->setShowGrid(false);
    m_resultsTable->verticalHeader()->setVisible(false);
    m_resultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_resultsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_resultsTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_resultsTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_resultsTable->setFrameShape(QFrame::NoFrame);
    m_resultsTable->setStyleSheet("QTableWidget { background: transparent; } QTableView::item { border-bottom: 1px solid #e0e0e0; }");
    m_resultsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_resultsTable, &QTableWidget::customContextMenuRequested, this, &QslManagerDialog::onResultsTableContextMenu);
    connect(m_resultsTable, &QTableWidget::cellDoubleClicked, this, &QslManagerDialog::onResultDoubleClicked);
    connect(m_resultsTable, &QTableWidget::cellClicked, this, &QslManagerDialog::onResultClicked);
    resLayout->addWidget(m_resultsTable);
    midLayout->addWidget(resGroup, 3);
    
    // Right: Action
    QGroupBox *actionGroup = new QGroupBox("Print Queue", this);
    actionGroup->setStyleSheet("QGroupBox { border: 1px solid gray; border-radius: 5px; margin-top: 1ex; } QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }");
    QVBoxLayout *actionLayout = new QVBoxLayout(actionGroup);
    m_lblChosenCallsign = new QLabel("None", this);
    m_lblChosenCallsign->hide();
    
    m_btnPrintQsl = new QPushButton("Print The Queue", this);
    m_btnPrintQsl->setEnabled(false);
    connect(m_btnPrintQsl, &QPushButton::clicked, this, &QslManagerDialog::printQslClicked);
    actionLayout->addWidget(m_btnPrintQsl);

    m_syncClublogBtn = new QPushButton("☁️ Sync Club Log OQRS", this);
    m_syncClublogBtn->setStyleSheet(
        "QPushButton { background-color: #0284c7; color: white; border: 1px solid #0369a1; border-radius: 4px; padding: 6px 12px; font-weight: bold; margin-top: 6px; }"
        "QPushButton:hover { background-color: #0369a1; }"
        "QPushButton:pressed { background-color: #025680; }"
        "QPushButton:disabled { background-color: #333333; color: #777777; border-color: #444444; }"
    );
    actionLayout->addWidget(m_syncClublogBtn);
    connect(m_syncClublogBtn, &QPushButton::clicked, this, &QslManagerDialog::onSyncClublogOqrs);

    // Initial check for Club Log credentials presence to determine button availability
    {
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        bool hasNative = settings.contains("Credentials/clublogEmail") || settings.contains("Credentials/clublogCall") || settings.contains("Credentials/clublogPass");
        QSettings settingsIni(getAppDataDir() + "/settings.ini", QSettings::IniFormat);
        bool hasIni = settingsIni.contains("Credentials/clublogEmail") || settingsIni.contains("Credentials/clublogCall") || settingsIni.contains("Credentials/clublogPass");
        QSettings *activeSettings = &settings;
        if (!hasNative && hasIni) {
            activeSettings = &settingsIni;
        }

        auto cleanVal = [](const QString &val) {
            QString s = val.trimmed();
            if (s.startsWith("\"") && s.endsWith("\"")) {
                s = s.mid(1, s.length() - 2).trimmed();
            }
            return s.trimmed();
        };

        QString clEmail = cleanVal(activeSettings->value("Credentials/clublogEmail").toString());
        QString clCall = cleanVal(activeSettings->value("Credentials/clublogCall").toString());
        QString clPass = cleanVal(activeSettings->value("Credentials/clublogPass").toString());

        bool credentialsOk = !clEmail.isEmpty() && !clCall.isEmpty() && !clPass.isEmpty();
        m_syncClublogBtn->setEnabled(credentialsOk);
        if (!credentialsOk) {
            m_syncClublogBtn->setToolTip(tr("Club Log OQRS is unavailable because email, call, or password credentials are not configured."));
        }
    }

    QLabel *clearQueueLbl = new QLabel("<a href='clear' style='text-decoration:none; color:#2c87c0;'>Clear Queue</a>", this);
    QFont clearFont = clearQueueLbl->font();
    clearFont.setPointSize(6);
    clearQueueLbl->setFont(clearFont);
    clearQueueLbl->setAlignment(Qt::AlignCenter);
    connect(clearQueueLbl, &QLabel::linkActivated, this, [this](const QString &link) {
        if (link == "clear") {
            m_printQueueList->clear();
            m_btnPrintQsl->setEnabled(false);
            savePrintQueue();
        }
    });
    actionLayout->addWidget(clearQueueLbl);

    QString logPath = m_mainWnd->logfilePath();
    QString logName = logPath.isEmpty() ? "None Selected" : QFileInfo(logPath).fileName();
    QLabel *lblLogfile = new QLabel(QString("<b>Using Logfile(s):</b> <br><a href='#'>%1</a>").arg(logName), this);
    lblLogfile->setWordWrap(true);
    lblLogfile->setAlignment(Qt::AlignCenter);
    connect(lblLogfile, &QLabel::linkActivated, this, [this, lblLogfile](const QString &) {
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        settings.beginGroup("Files");
        QString defUserDir = settings.value("userDirectory", QDir::homePath()).toString();
        settings.endGroup();

        QString file = QFileDialog::getOpenFileName(this, "Select Active Logfile", defUserDir, "Log Files (*.adi *.adif *.txt *.csv *.log);;ADIF Files (*.adi *.adif);;All Files (*)");
        if (!file.isEmpty()) {
            QPointer<QslManagerDialog> guard(this);
            ProcessingStatusDialog *statusDlg = new ProcessingStatusDialog(this);
            statusDlg->show();
            QApplication::processEvents();

            if (!guard) return;

            m_mainWnd->setLogfilePath(file, false);

            if (!guard) return;

            lblLogfile->setText(QString("<b>Using Logfile(s):</b> <br><a href='#'>%1</a>").arg(QFileInfo(file).fileName()));
            executeLastSearch();

            if (!guard) return;

            statusDlg->accept();
            statusDlg->deleteLater();
        }
    });
    actionLayout->addWidget(lblLogfile);
    
    m_printQueueList = new QListWidget(this);
    m_printQueueList->setFrameShape(QFrame::NoFrame);
    m_printQueueList->setStyleSheet("background: transparent;");
    m_printQueueList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(m_printQueueList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        delete item;
        m_btnPrintQsl->setEnabled(m_printQueueList->count() > 0);
        savePrintQueue();
    });
    actionLayout->addWidget(m_printQueueList, 1);
    
    QLabel *logoLabel = new QLabel(this);
    logoLabel->setObjectName("logoLabel");
    QPixmap lPix = getAppLogo();
    logoLabel->setFixedSize(120, 120);
    logoLabel->setPixmap(lPix.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    actionLayout->addWidget(logoLabel, 0, Qt::AlignHCenter);
    
    midLayout->addWidget(actionGroup, 1);
    
    mainLayout->addLayout(midLayout, 1);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("qsl_status_label");
    m_statusLabel->setStyleSheet("QLabel { background-color: #1a1e24; color: #a0aec0; border-top: 1px solid #2d3748; padding: 6px 12px; font-family: 'Courier New'; font-size: 12px; font-weight: bold; }");
    m_statusLabel->setText("Ready.");

    // Bottom status bar layout hosting status feedback and the in-situ corner scheme toggle
    QHBoxLayout *bottomBarLayout = new QHBoxLayout();
    bottomBarLayout->setContentsMargins(0, 0, 8, 0);
    bottomBarLayout->setSpacing(6);
    bottomBarLayout->addWidget(m_statusLabel, 1);

    // In-situ appearance customizer toggle: corner checkbox placed in the lowest right corner
    // (naked checkbox without bracketed text, defaulting to unchecked false on window open)
    QCheckBox *cornerSchemeToggle = WindowSchemeManager::createCornerToggle(this, this);
    bottomBarLayout->addWidget(cornerSchemeToggle, 0, Qt::AlignRight | Qt::AlignVCenter);

    mainLayout->addLayout(bottomBarLayout);

    // Apply any previously saved custom styles from Qwain.conf [WindowStyles/QSL_Print_Manager]
    WindowSchemeManager::applySavedStyles(this);
}

bool QslManagerDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_callsignLookup) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            
            if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
                if (m_searchMatchesList) {
                    m_searchMatchesList->hide();
                }
                doLookup();
                return true; // Consume Enter key so dialog's default buttons don't intercept it
            }
            
            if (m_searchMatchesList && m_searchMatchesList->isVisible()) {
                if (keyEvent->key() == Qt::Key_Down) {
                    int nextRow = m_searchMatchesList->currentRow() + 1;
                    if (nextRow >= m_searchMatchesList->count()) nextRow = 0;
                    m_searchMatchesList->setCurrentRow(nextRow);
                    return true;
                } else if (keyEvent->key() == Qt::Key_Up) {
                    int prevRow = m_searchMatchesList->currentRow() - 1;
                    if (prevRow < 0) prevRow = m_searchMatchesList->count() - 1;
                    m_searchMatchesList->setCurrentRow(prevRow);
                    return true;
                } else if (keyEvent->key() == Qt::Key_Escape) {
                    m_searchMatchesList->hide();
                    return true;
                }
            }
        } else if (event->type() == QEvent::FocusOut) {
            QTimer::singleShot(200, this, [this]() {
                if (m_searchMatchesList && !m_searchMatchesList->hasFocus()) {
                    m_searchMatchesList->hide();
                }
            });
        }
    }

    if (m_infoBrowser && watched == m_infoBrowser->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton && m_infoBrowser->anchorAt(me->pos()).isEmpty() && !m_currentDisplayedCallsign.isEmpty()) {
                QApplication::clipboard()->setText(m_currentDisplayedCallsign);
            }
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton && m_infoBrowser->anchorAt(me->pos()).isEmpty() && !m_currentDisplayedCallsign.isEmpty()) {
                QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
                qrzCache.beginGroup(m_currentDisplayedCallsign);
                
                QString name;
                if (qrzCache.contains("qrz_fname") || qrzCache.contains("qrz_name")) {
                    QString fname = qrzCache.value("qrz_fname").toString();
                    QString lname = qrzCache.value("qrz_name").toString();
                    name = fname;
                    if (!name.isEmpty() && !lname.isEmpty()) name += " ";
                    name += lname;
                    
                    QString adifName = qrzCache.value("name").toString();
                    if (fname.isEmpty() && !adifName.isEmpty() && adifName.contains(' ') && !lname.contains(' ')) {
                        name = adifName;
                    }
                }
                if (name.trimmed().isEmpty()) name = qrzCache.value("name").toString();
                
                QString addr1 = qrzCache.value("qrz_addr1").toString();
                
                QString city = qrzCache.value("qrz_city").toString();
                if (city.isEmpty()) city = qrzCache.value("qrz_addr2").toString();
                if (city.isEmpty()) city = qrzCache.value("city").toString();
                
                QString state = qrzCache.value("qrz_state").toString();
                if (state.isEmpty()) state = qrzCache.value("state").toString();
                
                QString zip = qrzCache.value("qrz_zip").toString();
                
                QString country = qrzCache.value("qrz_country").toString();
                if (country.isEmpty()) country = qrzCache.value("country").toString();
                
                qrzCache.endGroup();
                
                QStringList lines;
                lines << m_currentDisplayedCallsign;
                if (!name.trimmed().isEmpty()) lines << name.trimmed();
                if (!addr1.isEmpty()) lines << addr1;
                
                QString csstr;
                if (!city.isEmpty()) csstr += city;
                if (!state.isEmpty()) { if (!csstr.isEmpty()) csstr += ", "; csstr += state; }
                if (!zip.isEmpty()) { if (!csstr.isEmpty()) csstr += " "; csstr += zip; }
                if (!csstr.isEmpty()) lines << csstr;
                
                if (!country.isEmpty() && country.toLower() != "united states" && country.toLower() != "usa") {
                    lines << country;
                }
                
                if (!lines.isEmpty()) {
                    QApplication::clipboard()->setText(lines.join("\n"));
                }
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

void QslManagerDialog::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
}

// [ENTRY POINT: Selection Callback / Profile Detail Assembly]
// This function displays the details for a selected callsign in the HTML info panel.
// Inputs: callsign - The amateur radio callsign to display.
// Effects: 
//   1. Retrieves biographical HTML and profile image from the local QrzCache.
//   2. If missing, programmatically triggers a background QRZ.com API lookup.
//   3. Builds and renders the interactive HTML layout (past QSO table, ARRL/QRZ confirmation icons, and Rcvd on timestamps).
void QslManagerDialog::displayCallInfo(const QString &callsign) {
    m_currentDisplayedCallsign = callsign;
    
    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    qrzCache.beginGroup(callsign);
    QString htmlInfo = qrzCache.value("htmlInfo").toString();
    
    QString sanitized = sanitizeHtmlInfo(htmlInfo);
    if (sanitized != htmlInfo) {
        htmlInfo = sanitized;
        qrzCache.setValue("htmlInfo", htmlInfo);
        qrzCache.sync();
    }
    
    QByteArray imgData = qrzCache.value("imgData").toByteArray();
    bool lotw = qrzCache.value("lotw", false).toBool();
    bool mqsl = qrzCache.value("mqsl", false).toBool();
    QString country = qrzCache.value("country").toString();
    if (country.isEmpty()) country = qrzCache.value("qrz_country").toString(); // fallback to qrz profile country if ADIF country missing
    qrzCache.endGroup();
    
    // Automatically trigger a lookup if HTML profile is missing so we have "complete information" on single click.
    if (htmlInfo.isEmpty() && m_mainWnd->allQsos().contains(callsign)) {
        // We inject the most recent QSO essentially as a dummy trigger, but to avoid duplication we just use an artificial one
        QString fakeAdif = QString("<CALL:%1>%2<QSO_DATE:8>%3<TIME_ON:6>000000<EOR>")
                            .arg(callsign.length()).arg(callsign)
                            .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMdd"));
        m_mainWnd->processAdifRecord(fakeAdif, true, false); // true = isManualLookup, doesn't add to allQsos, false = skip main window display
        
        // Let it refresh after the lookup finishes
        QTimer::singleShot(2000, this, [this, callsign](){
            if (m_currentDisplayedCallsign == callsign) { // Only update if they haven't clicked away
                displayCallInfo(callsign);
            }
        });
    }

    QString imgTag;
    if (!imgData.isEmpty()) {
        QImage img = QImage::fromData(imgData).scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QString resName = "img_" + callsign;
        m_infoBrowser->document()->addResource(QTextDocument::ImageResource, QUrl(resName), img);
        imgTag = QString("<img src='%1' width='%2' height='%3'/>").arg(resName).arg(img.width()).arg(img.height());
    }

    QString historyHtml;
    const auto &allQsos = m_mainWnd->allQsos();
    int qsoCount = allQsos.contains(callsign) ? allQsos[callsign].size() : 0;
    
    QColor redColor;
    redColor.setHsvF(0.0, 1.0, m_qsoListColor.valueF());
    
    bool hasQsl = false;

    if (qsoCount > 0) {
        for (const auto &q : allQsos[callsign]) {
            if (!q.qslsdate.isEmpty() || q.cardRcvd) {
                hasQsl = true;
            }
            if (q.lotwRcvd) lotw = true;
            if (q.mqslRcvd) mqsl = true;
            if (country.isEmpty() && !q.country.isEmpty()) country = q.country;
        }
    }
    
    auto makeImg = [&](const QString &cacheKey, const QString &fallback, int defaultW = -1, int defaultH = 24) {
        QrzCache cache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
        cache.beginGroup("icons");
        QByteArray data = cache.value(cacheKey).toByteArray();
        int w = cache.value(cacheKey + "_W", defaultW).toInt();
        int h = cache.value(cacheKey + "_H", defaultH).toInt();
        cache.endGroup();
        if (data.isEmpty() || data.startsWith("empty")) return QString();
        static int uniqueCounter = 0;
        QString uniqueResName = QString("qslmgr_%1_%2").arg(cacheKey).arg(++uniqueCounter);
        QImage img = QImage::fromData(data);
        if (w > 0 && h > 0) {
            img = img.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            m_infoBrowser->document()->addResource(QTextDocument::ImageResource, QUrl(uniqueResName), img);
            return QString("<img src='%1' width='%2' height='%3'/>").arg(uniqueResName).arg(w).arg(h);
        } else if (h > 0) {
            img = img.scaledToHeight(h, Qt::SmoothTransformation);
            m_infoBrowser->document()->addResource(QTextDocument::ImageResource, QUrl(uniqueResName), img);
            return QString("<img src='%1' height='%2'/>").arg(uniqueResName).arg(h);
        }
        m_infoBrowser->document()->addResource(QTextDocument::ImageResource, QUrl(uniqueResName), img);
        return QString("<img src='%1'/>").arg(uniqueResName);
    };

    QString iso = MainWindow::countryToIso(country);
    QString flagImgHtml = !iso.isEmpty() ? makeImg("flag_" + iso, "") : "";
    QString envImgHtml = mqsl ? makeImg("env_icon", "MAIL", 36, 34) : "";
    QString arrlImgHtml = lotw ? makeImg("arrl_icon", "LOTW", 36, 34) : "";

    QString iconsHtml = QString("<br><table border='0' width='100%' cellspacing='0' cellpadding='0'><tr>"
                        "<td valign='middle' align='left'>%1</td>"
                        "<td valign='middle' align='right'>%2&nbsp;&nbsp;%3</td>"
                        "</tr></table>")
                    .arg(flagImgHtml, envImgHtml, arrlImgHtml);
    
    QString callColorStr = hasQsl ? m_qsoListColor.name() : redColor.name();
    
    QString leftPanel = QString("<div align='center'>%1%2<br><h2 style='margin: 10px 0; color: %4;'>%3</h2></div>").arg(imgTag, iconsHtml, callsign, callColorStr);

    auto makeBtn = [](const QString& lbl, const QString& href, const QString& bg, const QString& fg) {
        return QString("<table border='1' cellspacing='0' cellpadding='8' bgcolor='%1' style='border-color:#555;'><tr><td align='center'><a href='%2' style='color:%3; text-decoration:none;'><b>%4</b></a></td></tr></table><br>")
            .arg(bg, href, fg, lbl);
    };

    QString paperBtnHtml = makeBtn("Add To Queue", "http://app.local/qsl_queue/" + callsign, "#2c87c0", "#ffffff");
    QString printNowBtnHtml = makeBtn("Print Now", "http://app.local/print_now/" + callsign, "#10b981", "#ffffff");

    bool anySentN = false;
    bool allSentY = true;
    bool anyRcvdY = false;

    if (qsoCount > 0) {
        for (const auto &q : allQsos[callsign]) {
            QString s = q.qsl_sent.trimmed().toUpper();
            QString r = q.qsl_rcvd.trimmed().toUpper();
            
            if (s.isEmpty() && !q.qslsdate.isEmpty()) {
                s = "Y";
            }
            if (s.isEmpty()) {
                s = "N";
            }
            if (s == "N") {
                anySentN = true;
            }
            if (s != "Y") {
                allSentY = false;
            }
            
            if (r.isEmpty() && (q.cardRcvd || q.mqslRcvd || q.lotwRcvd)) {
                r = "Y";
            }
            if (r.isEmpty()) {
                r = "N";
            }
            if (r == "Y") {
                anyRcvdY = true;
            }
        }
    } else {
        allSentY = false;
    }

    bool sentIsRed = anySentN || !allSentY;
    bool rcvdIsGreen = anyRcvdY;

    QString sentColor = sentIsRed ? "red" : "green";
    QString rcvdColor = rcvdIsGreen ? "green" : "red";

    QString statusIndicatorHtml = QString(
        "<div align='center' style='font-family: \"Courier New\", Courier, monospace; font-size: 10pt; margin-top: 5px; margin-bottom: 5px; color: #f1f5f9;'>"
        "QSL <span style='color:%1; font-weight:bold;'>Sent</span> / <span style='color:%2; font-weight:bold;'>Rcvd</span>"
        "</div>"
    ).arg(sentColor, rcvdColor);

    historyHtml += "<div align='center'>" + paperBtnHtml + printNowBtnHtml + "</div>";
    historyHtml += statusIndicatorHtml;
    historyHtml += "<table width='100%' border='0'><tr><td valign='top' width='100%'>";
    historyHtml += "<h3 align='center' style='color:orange; margin-top:0px;'>Past Contacts</h3>";
    
    if (qsoCount > 0) {
        historyHtml += QString("<div style=\"font-family: 'Courier New', Courier, monospace;\">");
        historyHtml += "<table border='0' width='100%' cellspacing='0' cellpadding='2'>";
        historyHtml += "<tr><th align='center'>Date</th><th align='center'>Time</th><th align='center'>Band</th><th align='center'>Mode</th><th align='center'>Conf</th></tr>";
        
        QList<QsoDetail> qlist = allQsos[callsign];
        std::sort(qlist.begin(), qlist.end(), [](const QsoDetail &a, const QsoDetail &b){
            QString at = a.date + a.time;
            QString bt = b.date + b.time;
            return at > bt;
        });
        
        for (const auto &dRaw : qlist) {
            QsoDetail d = adjustQsoToEastern(dRaw);
            QString dStr = d.date;
            if (dStr.length() == 8) dStr = dStr.left(4) + "-" + dStr.mid(4,2) + "-" + dStr.mid(6,2);
            QString tStr = d.time;
            if (tStr.length() >= 4) tStr = tStr.left(2) + ":" + tStr.mid(2,2);
            
            QColor rowColor = d.qslsdate.isEmpty() ? redColor : m_qsoListColor;
            QString trStyle = QString("color:%1;").arg(rowColor.name());
            
            QString confHtml = "&nbsp;";
            if (d.lotwRcvd) {
                confHtml = makeImg("arrl_icon", "LOTW", -1, 12);
                if (confHtml.isEmpty()) {
                    confHtml = "<b>L</b>"; // fallback if the icon is not cached
                }
            } else if (d.qrzRcvd) {
                confHtml = "<b>Q</b>";
            }
            
            historyHtml += QString("<tr style=\"%6\"><td align='center' style='white-space:nowrap;'>%1</td><td align='center' style='white-space:nowrap;'>%2</td><td align='center' style='white-space:nowrap;'>%3</td><td align='center' style='white-space:nowrap;'>%4</td><td align='center'>%5</td></tr>")
                           .arg(dStr, tStr, d.band, d.mode, confHtml, trStyle);

            if (!d.qslsdate.isEmpty()) {
                QString printedDate = d.qslsdate;
                if (printedDate.length() == 8) {
                    printedDate = printedDate.left(4) + "-" + printedDate.mid(4,2) + "-" + printedDate.mid(6,2);
                }
                historyHtml += QString("<tr><td colspan='5' align='center' style='font-size:7pt; color:%1; line-height:1; padding-bottom:2px;'>Printed: %2</td></tr>")
                               .arg(rowColor.name(), printedDate);
            }
            
            if (d.cardRcvd || !d.qslrdate.isEmpty()) {
                QString rcvdDate = d.qslrdate.trimmed();
                if (rcvdDate.isEmpty()) rcvdDate = d.date;
                if (rcvdDate.length() == 8) {
                    rcvdDate = rcvdDate.left(4) + "-" + rcvdDate.mid(4,2) + "-" + rcvdDate.mid(6,2);
                }
                historyHtml += QString("<tr><td colspan='5' align='center' style='font-size:7pt; color:%1; line-height:1; padding-bottom:4px;'>Rcvd on: %2</td></tr>")
                               .arg(rowColor.name(), rcvdDate);
            }
        }
        historyHtml += "</table></div>";
    } else {
        historyHtml += "<p align='center'><i>No contacts.</i></p>";
    }
    historyHtml += "</td></tr></table>";

    QString modifiedHtmlInfo = htmlInfo;
    
    // Upgrade old cache formats to new table format
    if (modifiedHtmlInfo.contains("<b>Name:</b> ") && !modifiedHtmlInfo.contains("<table")) {
        QString upgraded = "<table border='0' cellspacing='0' cellpadding='2'>";
        QStringList parts = modifiedHtmlInfo.split("<br>");
        for (QString part : parts) {
            if (part.startsWith("<h2>") || part.startsWith("<small>")) {
                upgraded += part;
            } else if (part.contains("<b>")) {
                int bEnd = part.indexOf("</b>") + 4;
                QString label = part.left(bEnd);
                QString value = part.mid(bEnd).trimmed();
                upgraded += QString("<tr><td align='right'>%1</td><td align='left'>%2</td></tr>").arg(label, value);
            }
        }
        upgraded += "</table>";
        modifiedHtmlInfo = upgraded;
    }

    modifiedHtmlInfo.replace(QString("<h2>%1</h2>").arg(callsign), "");
    modifiedHtmlInfo.replace("<table", "<table align='center'");

    QString html = QString("%1%2<br><br>%3")
                   .arg(leftPanel, modifiedHtmlInfo, historyHtml);
    m_infoBrowser->setHtml(html);
}

QString QslManagerDialog::getQueueStringForCallsign(const QString &callsign) {
    const auto &allQsos = m_mainWnd->allQsos();
    if (!allQsos.contains(callsign)) return callsign + "|unprinted";
    
    QList<QsoDetail> qlist = allQsos[callsign];
    bool hasPrinted = false;
    bool hasUnprinted = false;
    for (const auto &q : qlist) {
        if (q.qslsdate.isEmpty()) {
            hasUnprinted = true;
        } else {
            hasPrinted = true;
        }
    }
    
    if (hasPrinted && hasUnprinted) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Queue QSLs");
        msgBox.setText(QString("The callsign %1 has both printed and unprinted contacts.").arg(callsign));
        msgBox.setInformativeText("Would you like to print all contacts or only the ones not yet printed?");
        QPushButton *btnAll = msgBox.addButton("All Contacts", QMessageBox::YesRole);
        QPushButton *btnUnprinted = msgBox.addButton("Only Unprinted", QMessageBox::NoRole);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.setDefaultButton(btnUnprinted);
        msgBox.exec();
        
        if (msgBox.clickedButton() == btnAll) {
            return callsign + "|all";
        } else if (msgBox.clickedButton() == btnUnprinted) {
            return callsign + "|unprinted";
        } else {
            return QString();
        }
    }
    
    if (hasPrinted && !hasUnprinted) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, 
            "Already Printed", 
            QString("%1 has already been printed. Reprint?").arg(callsign),
            QMessageBox::Yes | QMessageBox::No
        );
        if (reply == QMessageBox::Yes) {
            return callsign + "|reprint";
        } else {
            // Offer to update Club Log as when printed
            QMessageBox::StandardButton clReply = QMessageBox::question(
                this,
                "Update Club Log",
                tr("Would you like to update Club Log to mark this QSO as printed/sent?"),
                QMessageBox::Ok | QMessageBox::Cancel,
                QMessageBox::Ok
            );
            if (clReply == QMessageBox::Ok) {
                QSettings settings(QCoreApplication::organizationName(), "Qwain");
                bool hasNative = settings.contains("Credentials/clublogEmail") || settings.contains("Credentials/clublogCall") || settings.contains("Credentials/clublogPass");
                QSettings settingsIni(getAppDataDir() + "/settings.ini", QSettings::IniFormat);
                bool hasIni = settingsIni.contains("Credentials/clublogEmail") || settingsIni.contains("Credentials/clublogCall") || settingsIni.contains("Credentials/clublogPass");
                QSettings *activeSettings = &settings;
                if (!hasNative && hasIni) {
                    activeSettings = &settingsIni;
                }

                auto cleanVal = [](const QString &val) {
                    QString s = val.trimmed();
                    if (s.startsWith("\"") && s.endsWith("\"")) {
                        s = s.mid(1, s.length() - 2).trimmed();
                    }
                    return s.trimmed();
                };

                QString email = cleanVal(activeSettings->value("Credentials/clublogEmail").toString());
                QString call = cleanVal(activeSettings->value("Credentials/clublogCall").toString());
                QString password = cleanVal(activeSettings->value("Credentials/clublogPass").toString());
                QString clublogApiKey = cleanVal(activeSettings->value("Credentials/clublogApiKey").toString());

                if (email.isEmpty() || call.isEmpty() || password.isEmpty()) {
                    QMessageBox::warning(this, tr("Credentials Required"), 
                                         tr("Club Log credentials are not fully configured.\n\n"
                                            "Please go to Configuration > Settings > Credentials to configure them first."));
                } else {
                    QString adif = "ClubLog Print Update from Qwain\n<eoh>\n";
                    QString targetDate = QDateTime::currentDateTime().toString("yyyyMMdd");
                    for (const auto &d : qlist) {
                        adif += QString("<CALL:%1>%2 ").arg(d.call.length()).arg(d.call);
                        QString dateVal = d.date; dateVal.remove("-"); dateVal.remove("/");
                        adif += QString("<QSO_DATE:%1>%2 ").arg(dateVal.length()).arg(dateVal);
                        QString timeVal = d.time; timeVal.remove(":");
                        if (timeVal.length() > 4) timeVal = timeVal.left(4);
                        else if (timeVal.isEmpty()) timeVal = "0000";
                        adif += QString("<TIME_ON:%1>%2 ").arg(timeVal.length()).arg(timeVal);
                        adif += QString("<BAND:%1>%2 ").arg(d.band.length()).arg(d.band);
                        adif += QString("<MODE:%1>%2 ").arg(d.mode.length()).arg(d.mode);
                        adif += QString("<QSL_SENT:1>Y <QSLSDATE:8>%1 <EOR>\n").arg(targetDate);
                    }

                    QNetworkAccessManager *netManager = new QNetworkAccessManager(this);
                    QUrl url("https://clublog.org/putlogs.php");
                    QNetworkRequest request(url);
                    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

                    QUrlQuery postData;
                    postData.addQueryItem("email", email);
                    postData.addQueryItem("password", password);
                    postData.addQueryItem("callsign", call);
                    if (!clublogApiKey.isEmpty()) {
                        postData.addQueryItem("api", clublogApiKey);
                    }
                    postData.addQueryItem("adif", adif);
                    postData.addQueryItem("file", adif);

                    QNetworkReply *netReply = netManager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());
                    connect(netReply, &QNetworkReply::finished, this, [netReply, netManager, this]() {
                        netReply->deleteLater();
                        netManager->deleteLater();
                        m_statusLabel->setText(tr("Club Log update submitted."));
                    });
                    m_statusLabel->setText(tr("Submitting print status to Club Log..."));
                }
            }
            return QString();
        }
    }
    
    return callsign + "|unprinted";
}

// [ENTRY POINT: Centralized Search Query Gateway]
// Dispatches the appropriate search subroutine based on the active query mode.
// Uses a custom 'SearchGuard' RAII lock to avoid concurrent search collision,
// flagging 'm_searchPending' if another lookup is invoked during execution.
void QslManagerDialog::executeLastSearch() {
    if (m_isExecuting) { m_searchPending = true; return; }
    SearchGuard guard(this);
    
    if (m_lastSearchMode.isEmpty()) {
        m_lastSearchMode = "all";
    }
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.setValue("QslManager/LastSearchMode", m_lastSearchMode);
    
    m_searchActive = true;
    if (m_lastSearchMode == "numContacts") numContactsSearch();
    else if (m_lastSearchMode == "countryDxcc") countryDxccSearch();
    else if (m_lastSearchMode == "lookup") doLookup();
    else if (m_lastSearchMode == "all") showAllQsos();
    m_searchActive = false;
}

void QslManagerDialog::showAllQsos() {
    if (!m_isExecuting) {
        if (m_searchActive) { m_searchPending = true; m_lastSearchMode = "all"; return; }
        m_lastSearchMode = "all";
        executeLastSearch();
        return;
    }
    
    bool chkLotw = m_chkLotw->isChecked();
    bool chkMail = m_chkMail->isChecked();
    
    QMap<QString, int> counts;
    QList<QsoDetail> matched;
    
    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    
    const auto &allQsos = m_mainWnd->allQsos();
    for (auto it = allQsos.begin(); it != allQsos.end(); ++it) {
        QString call = it.key();
        
        bool passFilter = true;
        if (chkLotw || chkMail) {
            qrzCache.beginGroup(call);
            bool isLotw = qrzCache.value("lotw").toBool();
            bool isMqsl = qrzCache.value("mqsl").toBool();
            qrzCache.endGroup();
            
            bool adifLotw = false, adifMqsl = false;
            for (const auto &d : it.value()) {
                if(d.lotwRcvd) adifLotw = true;
                if(d.mqslRcvd) adifMqsl = true;
            }
            if (chkLotw && !(isLotw || adifLotw)) passFilter = false;
            if (chkMail && !(isMqsl || adifMqsl)) passFilter = false;
        }
        
        if (!passFilter) continue;
        
        for (const auto &d : it.value()) {
            if (m_filterActive) {
                if (d.date < m_filterStartDate || d.date > m_filterEndDate) continue;
            }
            if (!counts.contains(call)) {
                matched.append(d);
                counts[call] = 1;
            } else {
                counts[call]++;
            }
        }
    }
    
    populateResults(matched, true, counts, true);
}

// [ENTRY POINT: Wildcard Parser & Filter Matcher]
// Performs callsign-specific database searches, supporting wildcard queries (*, @, ?).
// Compiles wildcard characters into robust regular expressions to filter matching logs,
// integrates LoTW/eQSL criteria with local QrzCache status, and populates the table results.
void QslManagerDialog::doLookup() {
    if (!m_isExecuting) {
        if (m_searchActive) { m_searchPending = true; m_lastSearchMode = "lookup"; return; }
        m_lastSearchMode = "lookup";
        executeLastSearch();
        return;
    }
    
    QString call = m_callsignLookup->text().trimmed().toUpper();
    if (call.isEmpty()) {
        showAllQsos();
        return;
    }
    
    bool hasWildcard = call.contains('*') || call.contains('@') || call.contains('?');
    
    bool chkLotw = m_chkLotw->isChecked();
    bool chkMail = m_chkMail->isChecked();
    
    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    QList<QsoDetail> matched;
    
    QString regexPattern = "^";
    for (int i = 0; i < call.length(); ++i) {
        QChar c = call[i];
        if (c == '*') {
            regexPattern += ".*";
        } else if (c == '@' || c == '?') {
            regexPattern += ".";
        } else if (c == '.' || c == '+' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == '^' || c == '$' || c == '\\' || c == '|') {
            regexPattern += "\\" + QString(c);
        } else {
            regexPattern += QRegularExpression::escape(QString(c));
        }
    }
    if (hasWildcard) {
        regexPattern += "$";
    }
    
    QRegularExpression re(regexPattern, QRegularExpression::CaseInsensitiveOption);
    
    const auto &allQsos = m_mainWnd->allQsos();
    for (auto it = allQsos.begin(); it != allQsos.end(); ++it) {
        QString currentCall = it.key();
        bool callsignMatches = false;
        if (hasWildcard) {
            QRegularExpressionMatch match = re.match(currentCall);
            if (match.hasMatch()) {
                callsignMatches = true;
            }
        } else {
            if (currentCall.startsWith(call, Qt::CaseInsensitive)) {
                callsignMatches = true;
            }
        }
        
        if (!callsignMatches) continue;
        
        bool passFilter = true;
        if (chkLotw || chkMail) {
            qrzCache.beginGroup(currentCall);
            bool isLotw = qrzCache.value("lotw").toBool();
            bool isMqsl = qrzCache.value("mqsl").toBool();
            qrzCache.endGroup();
            
            bool adifLotw = false, adifMqsl = false;
            for (const auto &d : it.value()) {
                if (d.lotwRcvd) adifLotw = true;
                if (d.mqslRcvd) adifMqsl = true;
            }
            if (chkLotw && !(isLotw || adifLotw)) passFilter = false;
            if (chkMail && !(isMqsl || adifMqsl)) passFilter = false;
        }
        
        if (!passFilter) continue;
        
        for (const auto &d : it.value()) {
            if (m_filterActive) {
                if (d.date < m_filterStartDate || d.date > m_filterEndDate) continue;
            }
            matched.append(d);
        }
    }
    
    // Write detailed diagnostic file
    QString diagPath = getAppDataDir() + "/qsl_search_diagnostic.txt";
    QFile diagFile(diagPath);
    if (diagFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&diagFile);
        out << "====================================================\n";
        out << "QSL MANAGER LOOKUP DIAGNOSTIC REPORT\n";
        out << "Generated: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n";
        out << "====================================================\n\n";
        out << "Search Input: " << call << "\n";
        out << "Is Wildcard: " << (hasWildcard ? "YES" : "NO") << "\n";
        out << "Regex Pattern: " << regexPattern << "\n\n";
        out << "--- Filter Settings ---\n";
        out << "Date Range Filter Active: " << (m_filterActive ? "YES" : "NO") << "\n";
        if (m_filterActive) {
            out << "  Start Date: " << m_filterStartDate << "\n";
            out << "  End Date:   " << m_filterEndDate << "\n";
        }
        out << "LoTW Filter Active: " << (chkLotw ? "YES" : "NO") << "\n";
        out << "Mail/Paper Filter Active: " << (chkMail ? "YES" : "NO") << "\n\n";
        
        out << "--- Matching Process Details ---\n";
        
        int totalUniqueCalls = allQsos.size();
        int regexMatchCount = 0;
        int passFilterCount = 0;
        int totalQsoExamined = 0;
        int totalQsoMatched = 0;
        
        for (auto it = allQsos.begin(); it != allQsos.end(); ++it) {
            QString currentCall = it.key();
            bool callsignMatches = false;
            if (hasWildcard) {
                QRegularExpressionMatch match = re.match(currentCall);
                if (match.hasMatch()) {
                    callsignMatches = true;
                }
            } else {
                if (currentCall.startsWith(call, Qt::CaseInsensitive)) {
                    callsignMatches = true;
                }
            }
            
            if (!callsignMatches) continue;
            
            regexMatchCount++;
            out << "Callsign: " << currentCall << " (Matched Regex)\n";
            
            bool passFilter = true;
            QString skipReason = "";
            if (chkLotw || chkMail) {
                qrzCache.beginGroup(currentCall);
                bool isLotw = qrzCache.value("lotw").toBool();
                bool isMqsl = qrzCache.value("mqsl").toBool();
                qrzCache.endGroup();
                
                bool adifLotw = false, adifMqsl = false;
                for (const auto &d : it.value()) {
                    if (d.lotwRcvd) adifLotw = true;
                    if (d.mqslRcvd) adifMqsl = true;
                }
                if (chkLotw && !(isLotw || adifLotw)) {
                    passFilter = false;
                    skipReason += " [Failed LoTW confirmation check]";
                }
                if (chkMail && !(isMqsl || adifMqsl)) {
                    passFilter = false;
                    skipReason += " [Failed Mail/Paper confirmation check]";
                }
            }
            
            if (!passFilter) {
                out << "  -> SKIPPED CALLSIGN:" << skipReason << "\n\n";
                continue;
            }
            
            passFilterCount++;
            int qsoInCallCount = 0;
            int qsoInCallPassed = 0;
            
            for (const auto &d : it.value()) {
                totalQsoExamined++;
                qsoInCallCount++;
                bool qsoPassed = true;
                QString qsoSkipReason = "";
                if (m_filterActive) {
                    if (d.date < m_filterStartDate || d.date > m_filterEndDate) {
                        qsoPassed = false;
                        qsoSkipReason = QString("Date %1 is outside range %2 to %3").arg(d.date).arg(m_filterStartDate).arg(m_filterEndDate);
                    }
                }
                
                if (qsoPassed) {
                    qsoInCallPassed++;
                    totalQsoMatched++;
                    out << QString("  + QSO: Date=%1, Time=%2, Band=%3, Mode=%4 [PASSED]\n")
                           .arg(d.date).arg(d.time).arg(d.band).arg(d.mode);
                } else {
                    out << QString("  - QSO: Date=%1, Time=%2, Band=%3, Mode=%4 [SKIPPED: %5]\n")
                           .arg(d.date).arg(d.time).arg(d.band).arg(d.mode).arg(qsoSkipReason);
                }
            }
            out << QString("  Summary for %1: %2 of %3 QSOs passed\n\n").arg(currentCall).arg(qsoInCallPassed).arg(qsoInCallCount);
        }
        
        out << "====================================================\n";
        out << "FINAL SUMMARY\n";
        out << "====================================================\n";
        out << "Total unique calls in log:             " << totalUniqueCalls << "\n";
        out << "Unique calls matching pattern:         " << regexMatchCount << "\n";
        out << "Unique calls passing LoTW/Mail check:  " << passFilterCount << "\n";
        out << "Total QSOs matching call pattern:      " << totalQsoExamined << "\n";
        out << "Total QSOs fully passing all filters:  " << totalQsoMatched << "\n";
        out << "====================================================\n";
        diagFile.close();
    }

    std::sort(matched.begin(), matched.end(), [](const QsoDetail &a, const QsoDetail &b){
        QString at = a.date + a.time;
        QString bt = b.date + b.time;
        return at > bt;
    });
    
    QSet<QString> uniqueCalls;
    for (const auto &d : matched) {
        uniqueCalls.insert(d.call.toUpper());
    }
    
    if (uniqueCalls.size() == 1) {
        QString callToShow = uniqueCalls.values().first();
        m_statusLabel->setText(QString("Found %1 Matches").arg(matched.size()));
        
        QMap<QString, int> counts;
        counts[callToShow.toUpper()] = matched.size();
        populateResults(matched, true, counts, true);
        
        QString fakeAdif = QString("<CALL:%1>%2<QSO_DATE:8>%3<TIME_ON:6>000000<EOR>")
                            .arg(callToShow.length()).arg(callToShow)
                            .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMdd"));
        m_mainWnd->processAdifRecord(fakeAdif, true, false);
        
        QTimer::singleShot(500, this, [this, callToShow](){ displayCallInfo(callToShow); });
        QTimer::singleShot(2000, this, [this, callToShow](){ displayCallInfo(callToShow); });
        
        if (m_mainWnd->allQsos().contains(callToShow)) {
            displayCallInfo(callToShow);
        }
    } else if (uniqueCalls.size() > 1) {
        QMap<QString, int> counts;
        for (const auto &d : matched) {
            counts[d.call.toUpper()]++;
        }
        populateResults(matched, true, counts, true);
    } else {
        m_resultsTable->setRowCount(0);
        if (!hasWildcard) {
            if (m_mainWnd->allQsos().contains(call)) {
                m_statusLabel->setText(QString("0 matches for %1 (Filtered by date range)").arg(call));
            } else {
                m_statusLabel->setText(QString("No matches found in log for %1").arg(call));
            }
            QString fakeAdif = QString("<CALL:%1>%2<QSO_DATE:8>%3<TIME_ON:6>000000<EOR>")
                                .arg(call.length()).arg(call)
                                .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMdd"));
            m_mainWnd->processAdifRecord(fakeAdif, true, false);
            
            QTimer::singleShot(500, this, [this, call](){ displayCallInfo(call); });
            QTimer::singleShot(2000, this, [this, call](){ displayCallInfo(call); });
            
            if (m_mainWnd->allQsos().contains(call)) {
                displayCallInfo(call);
            }
        } else {
            m_statusLabel->setText(QString("No matches found for %1").arg(call));
        }
    }
}

void QslManagerDialog::numContactsSearch() {
    if (!m_isExecuting) {
        if (m_searchActive) { m_searchPending = true; m_lastSearchMode = "numContacts"; return; }
        m_lastSearchMode = "numContacts";
        executeLastSearch();
        return;
    }
    
    bool chkLotw = m_chkLotw->isChecked();
    bool chkMail = m_chkMail->isChecked();
    
    QMap<QString, int> counts;
    QList<QsoDetail> latestQsos;
    
    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    
    const auto &allQsos = m_mainWnd->allQsos();
    for (auto it = allQsos.begin(); it != allQsos.end(); ++it) {
        QString call = it.key();
        
        if (chkLotw || chkMail) {
            qrzCache.beginGroup(call);
            bool isLotw = qrzCache.value("lotw").toBool();
            bool isMqsl = qrzCache.value("mqsl").toBool();
            qrzCache.endGroup();
            
            bool adifLotw = false, adifMqsl = false;
            for (const auto &d : it.value()) {
                if(d.lotwRcvd) adifLotw = true;
                if(d.mqslRcvd) adifMqsl = true;
            }
            
            bool passFilter = true;
            if (chkLotw && !(isLotw || adifLotw)) passFilter = false;
            if (chkMail && !(isMqsl || adifMqsl)) passFilter = false;
            
            if (!passFilter) continue;
        }
        
        int validCount = 0;
        QsoDetail latestValidQso;
        for (const auto &d : it.value()) {
            if (m_filterActive) {
                if (d.date < m_filterStartDate || d.date > m_filterEndDate) continue;
            }
            validCount++;
            latestValidQso = d; // gets the last one that matches
        }
        
        if (validCount > 0) {
            counts[call] = validCount;
            latestQsos.append(latestValidQso);
        }
    }
    
    populateResults(latestQsos, true, counts, true);
}

void QslManagerDialog::filterByDateClicked() {
    DateRangeDialog dlg(m_userStartDate, m_userEndDate, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_userStartDate = dlg.startDate();
        m_userEndDate = dlg.endDate();
        
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        settings.setValue("QslManager/StartDate", m_userStartDate.toString("yyyy-MM-dd"));
        settings.setValue("QslManager/EndDate", m_userEndDate.toString("yyyy-MM-dd"));
        
        m_filterStartDate = m_userStartDate.toString("yyyyMMdd");
        m_filterEndDate = m_userEndDate.toString("yyyyMMdd");
        
        m_chkDateRange->setText(QString("Use Date Range (%1 to %2)").arg(m_userStartDate.toString("yyyy-MM-dd"), m_userEndDate.toString("yyyy-MM-dd")));
        
        bool wasChecked = m_chkDateRange->isChecked();
        m_chkDateRange->setChecked(true);
        if (wasChecked) {
            executeLastSearch();
        }
    }
}

void QslManagerDialog::lastXDaysClicked() {
    bool ok;
    int days = QInputDialog::getInt(this, "Show Last X Days", "Enter number of days (1-100):", m_lastXDaysCount, 1, 100, 1, &ok);
    if (ok) {
        m_lastXDaysCount = days;
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        settings.setValue("QslManager/LastXDays", days);
        m_btnLastXDays->setText(QString("Show Last %1 Days").arg(days));
        
        m_userEndDate = QDateTime::currentDateTimeUtc().date();
        m_userStartDate = m_userEndDate.addDays(-days);
        
        settings.setValue("QslManager/StartDate", m_userStartDate.toString("yyyy-MM-dd"));
        settings.setValue("QslManager/EndDate", m_userEndDate.toString("yyyy-MM-dd"));
        
        m_filterStartDate = m_userStartDate.toString("yyyyMMdd");
        m_filterEndDate = m_userEndDate.toString("yyyyMMdd");
        
        m_chkDateRange->setText(QString("Use Date Range (%1 to %2)").arg(m_userStartDate.toString("yyyy-MM-dd"), m_userEndDate.toString("yyyy-MM-dd")));
        m_chkDateRange->setChecked(true);
        executeLastSearch();
    }
}

void QslManagerDialog::pastWeekClicked() {
    m_userEndDate = QDateTime::currentDateTimeUtc().date();
    m_userStartDate = m_userEndDate.addDays(-7);
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.setValue("QslManager/StartDate", m_userStartDate.toString("yyyy-MM-dd"));
    settings.setValue("QslManager/EndDate", m_userEndDate.toString("yyyy-MM-dd"));
    
    m_filterStartDate = m_userStartDate.toString("yyyyMMdd");
    m_filterEndDate = m_userEndDate.toString("yyyyMMdd");
    
    m_chkDateRange->setText(QString("Use Date Range (%1 to %2)").arg(m_userStartDate.toString("yyyy-MM-dd"), m_userEndDate.toString("yyyy-MM-dd")));
    m_chkDateRange->setChecked(true);
    executeLastSearch();
}

void QslManagerDialog::todayClicked() {
    m_userEndDate = QDateTime::currentDateTimeUtc().date();
    m_userStartDate = m_userEndDate;
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.setValue("QslManager/StartDate", m_userStartDate.toString("yyyy-MM-dd"));
    settings.setValue("QslManager/EndDate", m_userEndDate.toString("yyyy-MM-dd"));
    
    m_filterStartDate = m_userStartDate.toString("yyyyMMdd");
    m_filterEndDate = m_userEndDate.toString("yyyyMMdd");
    
    m_chkDateRange->setText(QString("Use Date Range (%1 to %2)").arg(m_userStartDate.toString("yyyy-MM-dd"), m_userEndDate.toString("yyyy-MM-dd")));
    m_chkDateRange->setChecked(true);
    executeLastSearch();
}

void QslManagerDialog::countryDxccSearch() {
    if (!m_isExecuting) {
        if (m_searchActive) { m_searchPending = true; m_lastSearchMode = "countryDxcc"; return; }
        
        QSet<QString> countries;
        QMap<QString, QString> dxccMap;
        const auto &allQsos = m_mainWnd->allQsos();
        for (auto it = allQsos.begin(); it != allQsos.end(); ++it) {
            for (const auto &d : it.value()) {
                QString c = d.country.trimmed().toUpper();
                if (!c.isEmpty()) countries.insert(c);
                if (!d.dxcc.trimmed().isEmpty()) {
                    if (!dxccMap.contains(d.dxcc.trimmed()) && !c.isEmpty()) {
                        dxccMap.insert(d.dxcc.trimmed(), c);
                    } else if (!dxccMap.contains(d.dxcc.trimmed())) {
                        dxccMap.insert(d.dxcc.trimmed(), "Unknown");
                    }
                }
            }
        }
        
        QStringList cList = countries.values();
        QStringList dKeys = dxccMap.keys();
        cList.sort();
        
        // Sort dxcc keys as numbers if possible
        std::sort(dKeys.begin(), dKeys.end(), [](const QString &a, const QString &b){
            return a.toInt() < b.toInt();
        });
        
        QStringList dList;
        for (const QString &k : dKeys) {
            dList << QString("%1 (%2)").arg(k, dxccMap[k]);
        }
        
        CountryDxccDialog dlg(cList, dList, this);
        if (dlg.exec() != QDialog::Accepted) return;
        m_searchCountryDxcc = dlg.getSelection();
        m_lastSearchMode = "countryDxcc";
        
        executeLastSearch();
        return;
    }
    
    QString sel = m_searchCountryDxcc;
    
    bool chkLotw = m_chkLotw->isChecked();
    bool chkMail = m_chkMail->isChecked();
    
    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    QList<QsoDetail> matched;
    QMap<QString, int> counts;
    
    const auto &allQsos = m_mainWnd->allQsos();
    
    for (auto it = allQsos.begin(); it != allQsos.end(); ++it) {
        QString call = it.key();
        
        bool passFilter = true;
        if (chkLotw || chkMail) {
            qrzCache.beginGroup(call);
            bool isLotw = qrzCache.value("lotw").toBool();
            bool isMqsl = qrzCache.value("mqsl").toBool();
            qrzCache.endGroup();
            
            bool adifLotw = false, adifMqsl = false;
            for (const auto &d : it.value()) {
                if(d.lotwRcvd) adifLotw = true;
                if(d.mqslRcvd) adifMqsl = true;
            }
            if (chkLotw && !(isLotw || adifLotw)) passFilter = false;
            if (chkMail && !(isMqsl || adifMqsl)) passFilter = false;
        }
        
        if (!passFilter) continue;
        
        for (const auto &d : it.value()) {
            if (m_filterActive) {
                if (d.date < m_filterStartDate || d.date > m_filterEndDate) continue;
            }
            bool matches = false;
            if (sel.startsWith("COUNTRY:")) {
                if (d.country.trimmed().compare(sel.mid(8).trimmed(), Qt::CaseInsensitive) == 0) matches = true;
            } else if (sel.startsWith("DXCC:")) {
                QString targetDxcc = sel.mid(5).split(" ").first().trimmed();
                if (d.dxcc.trimmed() == targetDxcc) matches = true;
            }
            if (matches) {
                if (!counts.contains(call)) {
                    matched.append(d);
                    counts[call] = 1;
                } else {
                    counts[call]++;
                }
            }
        }
    }
    
    populateResults(matched, true, counts, true);
}

void QslManagerDialog::populateResults(const QList<QsoDetail> &results, bool showCount, const QMap<QString, int> &counts, bool sortByCount) {
    Q_UNUSED(showCount);
    Q_UNUSED(sortByCount);
    m_currentResults = results;

    if (m_lastSearchMode == "lookup" || m_lastSearchMode == "countryDxcc") {
        m_statusLabel->setText(QString("Found %1 Matches").arg(results.size()));
    } else if (m_lastSearchMode == "numContacts") {
        m_statusLabel->setText(QString("Found %1 contacts").arg(results.size()));
    } else if (m_lastSearchMode == "all") {
        m_statusLabel->setText(QString("Found %1 total QSOs").arg(results.size()));
    } else {
        m_statusLabel->setText(QString("Found %1 Matches").arg(results.size()));
    }
    
    m_resultsTable->setSortingEnabled(false);
    m_resultsTable->clearContents();
    
    int cols = 5;
    m_resultsTable->setColumnCount(cols);
    
    QStringList headers;
    headers << "Callsign" << "Country" << "DXCC" << "During" << "Total";
    
    m_resultsTable->setHorizontalHeaderLabels(headers);
    m_resultsTable->horizontalHeader()->setSortIndicatorShown(false);
    
    // Dynamic rollup to unique callsigns
    QList<QsoDetail> uniqueResults;
    QMap<QString, int> localCounts = counts;
    QSet<QString> seen;
    for (const auto &d : results) {
        QString uCall = d.call.toUpper();
        if (!seen.contains(uCall)) {
            seen.insert(uCall);
            uniqueResults.append(d);
            if (localCounts.isEmpty()) {
                localCounts[uCall] = 1;
            }
        } else {
            if (localCounts.isEmpty()) {
                localCounts[uCall]++;
            }
        }
    }
    
    m_resultsTable->setRowCount(uniqueResults.size());
    
    // Set item delegate for country flag column (column 1)
    m_resultsTable->setItemDelegateForColumn(1, new IconCenterDelegate(m_resultsTable));
    
    QColor redColor;
    redColor.setHsvF(0.0, 1.0, m_qsoListColor.valueF());
    
    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    qrzCache.beginGroup("icons");
    QHash<QString, QIcon> flagCache;

    for (int i=0; i<uniqueResults.size(); ++i) {
        const QsoDetail &d = uniqueResults[i];
        
        QString resolvedCountry = d.country;
        QString resolvedDxcc = d.dxcc;
        if (resolvedCountry.isEmpty() || resolvedDxcc.isEmpty()) {
            if (m_mainWnd->allQsos().contains(d.call)) {
                for (const auto &q : m_mainWnd->allQsos()[d.call]) {
                    if (resolvedCountry.isEmpty() && !q.country.isEmpty()) {
                        resolvedCountry = q.country;
                    }
                    if (resolvedDxcc.isEmpty() && !q.dxcc.isEmpty()) {
                        resolvedDxcc = q.dxcc;
                    }
                    if (!resolvedCountry.isEmpty() && !resolvedDxcc.isEmpty()) {
                        break;
                    }
                }
            }
        }
        if (resolvedCountry.isEmpty() || resolvedDxcc.isEmpty()) {
            QrzCache qc(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
            qc.beginGroup(d.call);
            if (resolvedCountry.isEmpty()) {
                resolvedCountry = qc.value("country").toString();
                if (resolvedCountry.isEmpty()) resolvedCountry = qc.value("qrz_country").toString();
            }
            if (resolvedDxcc.isEmpty()) {
                resolvedDxcc = qc.value("dxcc").toString();
            }
            qc.endGroup();
        }
        
        bool hasQsl = false;
        if (m_mainWnd->allQsos().contains(d.call)) {
            for (const auto &q : m_mainWnd->allQsos()[d.call]) {
                if (!q.qslsdate.isEmpty()) {
                    hasQsl = true;
                    break;
                }
            }
        }
        
        QColor rowColor = hasQsl ? m_qsoListColor : redColor;
        
        QString iso = MainWindow::countryToIso(resolvedCountry);
        QIcon flagIcon;
        if (!iso.isEmpty()) {
            if (flagCache.contains(iso)) {
                flagIcon = flagCache[iso];
            } else {
                QByteArray flagData = qrzCache.value("flag_" + iso).toByteArray();
                if (!flagData.isEmpty() && !flagData.startsWith("empty")) {
                    QImage img = QImage::fromData(flagData);
                    if (!img.isNull()) {
                        flagIcon = QIcon(QPixmap::fromImage(img));
                        flagCache[iso] = flagIcon;
                    }
                }
            }
        }
        
        QTableWidgetItem *callItem = new QTableWidgetItem(d.call);
        callItem->setForeground(rowColor);
        callItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_resultsTable->setItem(i, 0, callItem);
        
        CountryTableItem *item1 = new CountryTableItem(resolvedCountry, flagIcon);
        item1->setForeground(rowColor);
        item1->setTextAlignment(Qt::AlignCenter);
        m_resultsTable->setItem(i, 1, item1);
        
        QTableWidgetItem *item2 = new QTableWidgetItem(resolvedDxcc);
        item2->setForeground(rowColor);
        item2->setTextAlignment(Qt::AlignCenter);
        m_resultsTable->setItem(i, 2, item2);
        
        QTableWidgetItem *cntItem = new QTableWidgetItem();
        cntItem->setData(Qt::EditRole, localCounts.value(d.call.toUpper(), localCounts.value(d.call, 0)));
        cntItem->setForeground(rowColor);
        cntItem->setTextAlignment(Qt::AlignCenter);
        m_resultsTable->setItem(i, 3, cntItem);
        
        int totalCount = 0;
        if (m_mainWnd->allQsos().contains(d.call)) {
            totalCount = m_mainWnd->allQsos()[d.call].size();
        }
        
        QTableWidgetItem *totalCntItem = new QTableWidgetItem();
        totalCntItem->setData(Qt::EditRole, totalCount);
        totalCntItem->setForeground(rowColor);
        totalCntItem->setTextAlignment(Qt::AlignCenter);
        m_resultsTable->setItem(i, 4, totalCntItem);
    }
    qrzCache.endGroup();
    
    m_resultsTable->setSortingEnabled(true);
    m_resultsTable->horizontalHeader()->setSortIndicatorShown(false);
    if (sortByCount && showCount) {
        m_resultsTable->sortItems(3, Qt::DescendingOrder);
    }
}

void QslManagerDialog::onResultClicked(int row, int col) {
    Q_UNUSED(col);
    QTableWidgetItem *item = m_resultsTable->item(row, 0);
    if (!item) return;
    QString call = item->text();
    displayCallInfo(call);
}

void QslManagerDialog::onResultDoubleClicked(int row, int col) {
    Q_UNUSED(col);
    QTableWidgetItem *item = m_resultsTable->item(row, 0);
    if (!item) return;
    QString call = item->text();
    m_currentChosenCallsign = call;
    m_lblChosenCallsign->setText(call);
    
    // Check if already in queue
    bool inQueue = false;
    for (int i=0; i<m_printQueueList->count(); ++i) {
        if (m_printQueueList->item(i)->text().split("|")[0].trimmed() == call) {
            inQueue = true;
            break;
        }
    }
    
    if (!inQueue) {
        QString queueItem = getQueueStringForCallsign(call);
        if (!queueItem.isEmpty()) {
            m_printQueueList->addItem(queueItem);
            savePrintQueue();
        }
    }
    
    m_btnPrintQsl->setEnabled(m_printQueueList->count() > 0);
    
    displayCallInfo(call);
}

void QslManagerDialog::printQslClicked() {
    if (m_printQueueList->count() == 0) return;
    
    QStringList queue;
    for (int i=0; i<m_printQueueList->count(); ++i) {
        queue << m_printQueueList->item(i)->text();
    }
    
    const auto &allQsos = m_mainWnd->allQsos();
    QString firstCallWithMode = queue[0];
    QString firstCall = firstCallWithMode.split("|")[0].trimmed().toUpper();
    QString firstMode = firstCallWithMode.contains("|") ? firstCallWithMode.split("|")[1].trimmed() : "unprinted";
    if (!allQsos.contains(firstCall)) return;
    
    // To get the htmlInfo for the preview dialog:
    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    qrzCache.beginGroup(firstCall);
    QString infoHtml = qrzCache.value("htmlInfo").toString();
    qrzCache.endGroup();
    
    // Guard against QslManagerDialog being closed/deleted during modal preview
    QPointer<QslManagerDialog> guard(this);
    // Parent to m_mainWnd so QslManagerDialog deletion doesn't delete stack-allocated dlg
    QslPreviewDialog dlg(firstCall, infoHtml, allQsos[firstCall], m_mainWnd, firstMode);
    dlg.setPrintQueue(queue, m_mainWnd);
    dlg.exec();
    
    if (!guard) return;
    QStringList remaining = dlg.getRemainingQueue();
    m_printQueueList->clear();
    for (const QString &c : remaining) {
        m_printQueueList->addItem(c);
    }
    m_btnPrintQsl->setEnabled(m_printQueueList->count() > 0);
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.setValue("QslManager/PrintQueue", remaining);
    
    // After returning, update UI in case QSL status appended
    displayCallInfo(firstCall);
    executeLastSearch();
}

void QslManagerDialog::savePrintQueue() {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QStringList queue;
    for (int i=0; i<m_printQueueList->count(); ++i) {
        queue << m_printQueueList->item(i)->text();
    }
    settings.setValue("QslManager/PrintQueue", queue);
}

void QslManagerDialog::loadPrintQueue() {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QStringList queue = settings.value("QslManager/PrintQueue").toStringList();
    for (const QString &c : queue) {
        m_printQueueList->addItem(c);
    }
    m_btnPrintQsl->setEnabled(m_printQueueList->count() > 0);
}



// [ENTRY POINT: Bulk Queue Operator]
// Selects all matched QSOs from the current search results and queues them
// in the print queue list, ensuring no duplicates are introduced and updating m_btnPrintQsl state.
void QslManagerDialog::selectAllResults() {
    bool addedAny = false;
    for (int row = 0; row < m_resultsTable->rowCount(); ++row) {
        QTableWidgetItem *item = m_resultsTable->item(row, 0);
        if (!item) continue;
        QString call = item->text().trimmed().toUpper();
        if (call.isEmpty()) continue;
        
        // Check if already in queue
        bool inQueue = false;
        for (int i=0; i<m_printQueueList->count(); ++i) {
            if (m_printQueueList->item(i)->text().split("|")[0].trimmed().toUpper() == call) {
                inQueue = true;
                break;
            }
        }
        
        if (!inQueue) {
            QString queueItem = getQueueStringForCallsign(call);
            if (!queueItem.isEmpty()) {
                m_printQueueList->addItem(queueItem);
                addedAny = true;
            }
        }
    }
    if (addedAny) {
        savePrintQueue();
    }
    m_btnPrintQsl->setEnabled(m_printQueueList->count() > 0);
}

void QslManagerDialog::onResultsTableContextMenu(const QPoint &pos) {
    QTableWidgetItem *itemAtPos = m_resultsTable->itemAt(pos);
    
    // If user right-clicks on a row that isn't selected, select that row
    if (itemAtPos && !m_resultsTable->selectionModel()->isRowSelected(itemAtPos->row(), QModelIndex())) {
        m_resultsTable->clearSelection();
        m_resultsTable->selectRow(itemAtPos->row());
    }

    // Collect all selected rows
    QList<int> rows;
    for (const QModelIndex &idx : m_resultsTable->selectionModel()->selectedRows()) {
        if (!rows.contains(idx.row())) {
            rows.append(idx.row());
        }
    }
    if (rows.isEmpty() && itemAtPos) {
        rows.append(itemAtPos->row());
    }
    if (rows.isEmpty() && m_resultsTable->currentRow() >= 0) {
        rows.append(m_resultsTable->currentRow());
    }
    if (rows.isEmpty()) return;

    QMenu menu(this);
    QAction *actAdd = menu.addAction(tr("Add to Queue"));
    QAction *selected = menu.exec(m_resultsTable->viewport()->mapToGlobal(pos));
    if (selected == actAdd) {
        bool addedAny = false;
        QString lastCall;
        for (int r : rows) {
            QTableWidgetItem *callItem = m_resultsTable->item(r, 0);
            if (!callItem) continue;
            QString call = callItem->text().trimmed().toUpper();
            if (call.isEmpty()) continue;
            lastCall = call;

            bool inQueue = false;
            for (int i = 0; i < m_printQueueList->count(); ++i) {
                if (m_printQueueList->item(i)->text().split("|")[0].trimmed().toUpper() == call) {
                    inQueue = true;
                    break;
                }
            }
            if (!inQueue) {
                QString queueItem = getQueueStringForCallsign(call);
                if (!queueItem.isEmpty()) {
                    m_printQueueList->addItem(queueItem);
                    addedAny = true;
                }
            }
        }
        if (addedAny) {
            savePrintQueue();
        }
        m_btnPrintQsl->setEnabled(m_printQueueList->count() > 0);
        if (!lastCall.isEmpty() && rows.size() == 1) {
            m_currentChosenCallsign = lastCall;
            m_lblChosenCallsign->setText(lastCall);
            displayCallInfo(lastCall);
        }
    }
}

void QslManagerDialog::onInfoBrowserContextMenu(const QPoint &pos) {
    QString call = m_currentDisplayedCallsign.trimmed().toUpper();
    if (call.isEmpty()) call = m_currentChosenCallsign.trimmed().toUpper();
    if (call.isEmpty()) return;

    QMenu menu(this);
    QAction *actAdd = menu.addAction(tr("Add to Queue"));
    QAction *selected = menu.exec(m_infoBrowser->viewport()->mapToGlobal(pos));
    if (selected == actAdd) {
        bool inQueue = false;
        for (int i = 0; i < m_printQueueList->count(); ++i) {
            if (m_printQueueList->item(i)->text().split("|")[0].trimmed().toUpper() == call) {
                inQueue = true;
                break;
            }
        }
        if (!inQueue) {
            QString queueItem = getQueueStringForCallsign(call);
            if (!queueItem.isEmpty()) {
                m_printQueueList->addItem(queueItem);
                savePrintQueue();
                m_btnPrintQsl->setEnabled(m_printQueueList->count() > 0);
            }
        }
    }
}

void QslManagerDialog::contextMenuEvent(QContextMenuEvent *event) {
    QString call = m_currentDisplayedCallsign.trimmed().toUpper();
    if (call.isEmpty()) call = m_currentChosenCallsign.trimmed().toUpper();
    if (call.isEmpty() && m_resultsTable->currentRow() >= 0) {
        QTableWidgetItem *item = m_resultsTable->item(m_resultsTable->currentRow(), 0);
        if (item) call = item->text().trimmed().toUpper();
    }
    if (call.isEmpty()) {
        QDialog::contextMenuEvent(event);
        return;
    }

    QMenu menu(this);
    QAction *actAdd = menu.addAction(tr("Add to Queue"));
    QAction *selected = menu.exec(event->globalPos());
    if (selected == actAdd) {
        bool inQueue = false;
        for (int i = 0; i < m_printQueueList->count(); ++i) {
            if (m_printQueueList->item(i)->text().split("|")[0].trimmed().toUpper() == call) {
                inQueue = true;
                break;
            }
        }
        if (!inQueue) {
            QString queueItem = getQueueStringForCallsign(call);
            if (!queueItem.isEmpty()) {
                m_printQueueList->addItem(queueItem);
                savePrintQueue();
                m_btnPrintQsl->setEnabled(m_printQueueList->count() > 0);
            }
        }
    }
}

static QString extractAdifFieldLocal(const QString &record, const QString &field) {
    int idx = record.indexOf("<" + field + ":", 0, Qt::CaseInsensitive);
    if (idx != -1) {
        int endTag = record.indexOf(">", idx);
        if (endTag != -1) {
            int colon = record.indexOf(":", idx + 1);
            if (colon != -1 && colon < endTag) {
                int lenEnd = record.indexOf(":", colon + 1);
                QString lenStr;
                if (lenEnd != -1 && lenEnd < endTag) {
                    lenStr = record.mid(colon + 1, lenEnd - colon - 1);
                } else {
                    lenStr = record.mid(colon + 1, endTag - colon - 1);
                }
                int len = lenStr.toInt();
                return record.mid(endTag + 1, len).trimmed();
            }
        }
    }
    idx = record.indexOf("<" + field + ">", 0, Qt::CaseInsensitive);
    if (idx != -1) {
        int endTag = idx + field.length() + 2;
        int nextTag = record.indexOf("<", endTag);
        if (nextTag != -1) {
            return record.mid(endTag, nextTag - endTag).trimmed();
        } else {
            return record.mid(endTag).trimmed();
        }
    }
    return QString();
}

void QslManagerDialog::onSyncClublogOqrs() {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    bool hasNative = settings.contains("Credentials/clublogEmail") || settings.contains("Credentials/clublogCall");
    QSettings settingsIni(getAppDataDir() + "/settings.ini", QSettings::IniFormat);
    bool hasIni = settingsIni.contains("Credentials/clublogEmail") || settingsIni.contains("Credentials/clublogCall");
    QSettings *activeSettings = &settings;
    if (!hasNative && hasIni) {
        activeSettings = &settingsIni;
    }

    auto cleanVal = [](const QString &val) {
        QString s = val.trimmed();
        if (s.startsWith("\"") && s.endsWith("\"")) {
            s = s.mid(1, s.length() - 2).trimmed();
        }
        return s.trimmed();
    };

    QString email = cleanVal(activeSettings->value("Credentials/clublogEmail").toString());
    QString call = cleanVal(activeSettings->value("Credentials/clublogCall").toString());
    QString password = cleanVal(activeSettings->value("Credentials/clublogPass").toString());
    QString clublogApiKey = cleanVal(activeSettings->value("Credentials/clublogApiKey").toString());

    if (email.isEmpty() || call.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, tr("Credentials Required"), 
                             tr("Club Log credentials are not fully configured.\n\n"
                                "Please go to Main Menu > Configuration > Settings > Credentials "
                                "and fill in your Club Log E-mail, Callsign, and Password first."));
        return;
    }

    m_statusLabel->setText(tr("Syncing OQRS from Club Log..."));
    m_syncClublogBtn->setEnabled(false);
    m_syncClublogBtn->setText(tr("☁️ Syncing..."));

    QNetworkAccessManager *netManager = new QNetworkAccessManager(this);
    
    QUrl url("https://clublog.org/getadif.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("User-Agent", "QwainOqrsReceiver/1.0 (theatomicfrog@gmail.com)");

    QUrlQuery postData;
    postData.addQueryItem("email", email);
    postData.addQueryItem("password", password);
    postData.addQueryItem("call", call);
    if (!clublogApiKey.isEmpty()) {
        postData.addQueryItem("api", clublogApiKey);
    }
    postData.addQueryItem("type", "dxqsl");

    QNetworkReply *reply = netManager->post(request, postData.toString(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished, this, [this, reply, netManager]() {
        reply->deleteLater();
        netManager->deleteLater();
        m_syncClublogBtn->setEnabled(true);
        m_syncClublogBtn->setText(tr("☁️ Sync Club Log OQRS"));
        m_statusLabel->setText(tr("Sync complete."));

        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, tr("Sync Error"), tr("Network error during Club Log sync:\n\n%1").arg(reply->errorString()));
            return;
        }

        QByteArray data = reply->readAll();
        QString adifText = QString::fromUtf8(data);

        if (adifText.contains("Authentication failed", Qt::CaseInsensitive) || adifText.contains("error", Qt::CaseInsensitive)) {
            QMessageBox::critical(this, tr("Club Log Error"), tr("Club Log returned an error:\n\n%1").arg(adifText.left(200)));
            return;
        }

        processOqrsAdif(adifText);
    });
}

void QslManagerDialog::processOqrsAdif(const QString &adif) {
    QRegularExpression eorRe("<eor>", QRegularExpression::CaseInsensitiveOption);
    QStringList records = adif.split(eorRe);

    int parsedCount = 0;
    int matchedCount = 0;
    int addedQueueCount = 0;

    auto &allQsos = m_mainWnd->allQsos(); // Fetch non-const reference to update in-memory objects!

    for (const QString &rec : records) {
        if (rec.trimmed().isEmpty()) continue;
        QString call = extractAdifFieldLocal(rec, "CALL").toUpper();
        if (call.isEmpty()) continue;
        parsedCount++;

        QString date = extractAdifFieldLocal(rec, "QSO_DATE");
        QString timeOn = extractAdifFieldLocal(rec, "TIME_ON");
        QString band = extractAdifFieldLocal(rec, "BAND").toUpper();
        QString mode = extractAdifFieldLocal(rec, "MODE").toUpper();
        QString qslSentVia = extractAdifFieldLocal(rec, "QSL_SENT_VIA").toUpper();

        if (allQsos.contains(call)) {
            bool foundMatch = false;
            // Iterate and find matching local QSO to inject qslVia routing ("B" or "D")
            for (int i = 0; i < allQsos[call].size(); ++i) {
                QsoDetail &q = allQsos[call][i];
                if (q.date == date) {
                    bool bandMatch = band.isEmpty() || q.band.isEmpty() || (q.band == band);
                    bool modeMatch = mode.isEmpty() || q.mode.isEmpty() || (q.mode == mode);
                    if (bandMatch && modeMatch) {
                        q.qslVia = (qslSentVia == "B") ? "B" : "D";
                        foundMatch = true;
                        matchedCount++;
                        break;
                    }
                }
            }
            if (!foundMatch && !allQsos[call].isEmpty()) {
                allQsos[call][0].qslVia = (qslSentVia == "B") ? "B" : "D";
                matchedCount++;
            }

            // Check if already in queue
            bool inQueue = false;
            for (int i = 0; i < m_printQueueList->count(); ++i) {
                if (m_printQueueList->item(i)->text().split("|")[0].trimmed().toUpper() == call) {
                    inQueue = true;
                    break;
                }
            }

            if (!inQueue) {
                QString queueItem = getQueueStringForCallsign(call);
                if (!queueItem.isEmpty()) {
                    m_printQueueList->addItem(queueItem);
                    addedQueueCount++;
                }
            }
        }
    }

    if (matchedCount > 0) {
        savePrintQueue();
        m_btnPrintQsl->setEnabled(m_printQueueList->count() > 0);
        QMessageBox::information(this, tr("Sync Success"), 
                                 tr("OQRS sync complete!\n\n"
                                    "Parsed %1 requests from Club Log.\n"
                                    "Successfully matched %2 QSOs inside your logbook.\n"
                                    "Added %3 new callsigns to your Print Queue!")
                                 .arg(parsedCount).arg(matchedCount).arg(addedQueueCount));
    } else {
        QMessageBox::information(this, tr("Sync Complete"), 
                                 tr("Club Log OQRS sync completed, but found 0 matches for the %1 parsed requests in your local logbook.")
                                 .arg(parsedCount));
    }
}

#include "qslmanagerdialog.moc"
