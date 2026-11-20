#include "statsdialog.h"
#include "mainwindow.h"
#include <QSet>
#include <QHeaderView>
#include <QScrollBar>
#include <QLocale>

// Helper to sanitize ADIF date strings into valid QDate objects
static QDate parseAdifDate(const QString &raw) {
    QString clean = raw.trimmed();
    clean.remove('-').remove('/').remove('.').remove(' ');
    if (clean.length() == 8) {
        return QDate::fromString(clean, "yyyyMMdd");
    }
    return QDate();
}

// ============================================================================
// WIDGET: DayBoxWidget
// Displays day number and 3 activity metric blocks:
//   - Contacts made
//   - Cards printed
//   - Cards received
// ============================================================================
DayBoxWidget::DayBoxWidget(const QDate &date, const DayStats &stats, bool isCurrentMonth, bool isToday, QWidget *parent)
    : QFrame(parent), m_date(date), m_stats(stats)
{
    setFrameShape(QFrame::StyledPanel);
    setCursor(Qt::PointingHandCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setMinimumSize(86, 92);

    // Dynamic styling based on current month, today status, and hover
    QString borderStyle;
    QString bgColor;
    if (isToday) {
        borderStyle = "2px solid #38bdf8";
        bgColor = "#1e293b";
    } else if (isCurrentMonth) {
        borderStyle = "1px solid #334155";
        bgColor = "#1e293b";
    } else {
        borderStyle = "1px solid #1e293b";
        bgColor = "#090d16";
    }

    setStyleSheet(QString(
        "DayBoxWidget {"
        "  background-color: %1;"
        "  border: %2;"
        "  border-radius: 6px;"
        "}"
        "DayBoxWidget:hover {"
        "  background-color: #243247;"
        "  border: 1px solid #60a5fa;"
        "}"
    ).arg(bgColor, borderStyle));

    QVBoxLayout *boxLayout = new QVBoxLayout(this);
    boxLayout->setContentsMargins(5, 4, 5, 4);
    boxLayout->setSpacing(2);

    // Top: Day number (R-aligned)
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->addStretch();

    QLabel *dayLabel = new QLabel(QString::number(date.day()), this);
    QString dayColor = isToday ? "#38bdf8" : (isCurrentMonth ? "#f1f5f9" : "#475569");
    dayLabel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 12px; background: transparent;").arg(dayColor));
    topLayout->addWidget(dayLabel);
    boxLayout->addLayout(topLayout);

    // Block 1: Contacts made
    QHBoxLayout *c1Layout = new QHBoxLayout();
    c1Layout->setContentsMargins(0, 0, 0, 0);
    QLabel *c1Title = new QLabel(tr("Contacts"), this);
    c1Title->setStyleSheet(QString("font-size: 9px; color: %1; background: transparent;").arg(isCurrentMonth ? "#94a3b8" : "#475569"));
    
    QLabel *c1Val = new QLabel(QString::number(stats.contactsMade), this);
    QString c1ValColor = (stats.contactsMade > 0) ? (isCurrentMonth ? "#38bdf8" : "#0284c7") : (isCurrentMonth ? "#64748b" : "#334155");
    c1Val->setStyleSheet(QString("font-size: 11px; font-weight: bold; color: %1; background: transparent;").arg(c1ValColor));
    c1Layout->addWidget(c1Title);
    c1Layout->addStretch();
    c1Layout->addWidget(c1Val);
    boxLayout->addLayout(c1Layout);

    QLabel *c1Sub = new QLabel(tr("    made"), this);
    c1Sub->setStyleSheet(QString("font-size: 8px; color: %1; background: transparent; margin-top: -3px;").arg(isCurrentMonth ? "#64748b" : "#334155"));
    boxLayout->addWidget(c1Sub);

    // Block 2: Cards Printed
    QHBoxLayout *c2Layout = new QHBoxLayout();
    c2Layout->setContentsMargins(0, 0, 0, 0);
    QLabel *c2Title = new QLabel(tr("Cards"), this);
    c2Title->setStyleSheet(QString("font-size: 9px; color: %1; background: transparent;").arg(isCurrentMonth ? "#94a3b8" : "#475569"));
    
    QLabel *c2Val = new QLabel(QString::number(stats.cardsPrinted), this);
    QString c2ValColor = (stats.cardsPrinted > 0) ? (isCurrentMonth ? "#c084fc" : "#7e22ce") : (isCurrentMonth ? "#64748b" : "#334155");
    c2Val->setStyleSheet(QString("font-size: 11px; font-weight: bold; color: %1; background: transparent;").arg(c2ValColor));
    c2Layout->addWidget(c2Title);
    c2Layout->addStretch();
    c2Layout->addWidget(c2Val);
    boxLayout->addLayout(c2Layout);

    QLabel *c2Sub = new QLabel(tr("Printed"), this);
    c2Sub->setStyleSheet(QString("font-size: 8px; color: %1; background: transparent; margin-top: -3px;").arg(isCurrentMonth ? "#64748b" : "#334155"));
    boxLayout->addWidget(c2Sub);

    // Block 3: Cards Received
    QHBoxLayout *c3Layout = new QHBoxLayout();
    c3Layout->setContentsMargins(0, 0, 0, 0);
    QLabel *c3Title = new QLabel(tr("Cards"), this);
    c3Title->setStyleSheet(QString("font-size: 9px; color: %1; background: transparent;").arg(isCurrentMonth ? "#94a3b8" : "#475569"));
    
    QLabel *c3Val = new QLabel(QString::number(stats.cardsReceived), this);
    QString c3ValColor = (stats.cardsReceived > 0) ? (isCurrentMonth ? "#34d399" : "#059669") : (isCurrentMonth ? "#64748b" : "#334155");
    c3Val->setStyleSheet(QString("font-size: 11px; font-weight: bold; color: %1; background: transparent;").arg(c3ValColor));
    c3Layout->addWidget(c3Title);
    c3Layout->addStretch();
    c3Layout->addWidget(c3Val);
    boxLayout->addLayout(c3Layout);

    QLabel *c3Sub = new QLabel(tr("Received"), this);
    c3Sub->setStyleSheet(QString("font-size: 8px; color: %1; background: transparent; margin-top: -3px;").arg(isCurrentMonth ? "#64748b" : "#334155"));
    boxLayout->addWidget(c3Sub);

    boxLayout->addStretch();
}

void DayBoxWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit dayClicked(m_date);
    }
    QFrame::mousePressEvent(event);
}

// ============================================================================
// DIALOG: DayDetailDialog
// Detailed report for a selected day with Domestic/International breakdown
// and logged QSO list.
// ============================================================================
DayDetailDialog::DayDetailDialog(const QDate &date, const DayStats &stats, QWidget *parent)
    : QDialog(parent), m_date(date), m_stats(stats)
{
    setWindowTitle(tr("Day Activity Report — %1").arg(date.toString("MMMM d, yyyy")));
    resize(700, 540);
    setMinimumSize(580, 460);

    setStyleSheet(
        "QDialog { background-color: #0f172a; color: #f8fafc; font-family: 'Segoe UI', Arial, sans-serif; }"
        "QFrame.metricCard { background-color: #1e293b; border-radius: 8px; border: 1px solid #334155; padding: 10px; }"
        "QLabel { color: #cbd5e1; font-size: 13px; background: transparent; }"
        "QTableWidget { background-color: #020617; color: #f1f5f9; border: 1px solid #334155; border-radius: 6px; gridline-color: #1e293b; font-size: 12px; }"
        "QHeaderView::section { background-color: #1e293b; color: #38bdf8; font-weight: bold; border: 1px solid #334155; padding: 4px; }"
        "QPushButton { background-color: #3b82f6; color: #ffffff; border: none; padding: 7px 16px; border-radius: 6px; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton#CloseButton { background-color: #475569; }"
        "QPushButton#CloseButton:hover { background-color: #64748b; }"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 18, 18, 18);
    mainLayout->setSpacing(12);

    // Title banner
    QLabel *titleLabel = new QLabel(tr("Activity Report: %1").arg(date.toString("dddd, MMMM d, yyyy")), this);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #f8fafc;");
    mainLayout->addWidget(titleLabel);

    // 3 Metric Cards side-by-side
    QHBoxLayout *cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(10);

    // Card 1: Contacts Made
    QFrame *card1 = new QFrame(this);
    card1->setProperty("class", "metricCard");
    card1->setStyleSheet("background-color: #1e293b; border-radius: 8px; border: 1px solid #334155; padding: 8px;");
    QVBoxLayout *c1Lay = new QVBoxLayout(card1);
    c1Lay->setContentsMargins(8, 8, 8, 8);
    c1Lay->setSpacing(4);
    QLabel *c1Head = new QLabel(tr("Contacts Made"), card1);
    c1Head->setStyleSheet("color: #94a3b8; font-size: 12px; font-weight: bold;");
    QLabel *c1Num = new QLabel(QString::number(stats.contactsMade), card1);
    c1Num->setStyleSheet("color: #38bdf8; font-size: 24px; font-weight: bold;");
    QLabel *c1Split = new QLabel(tr("Domestic: %1  |  Intl: %2").arg(stats.contactsDomestic).arg(stats.contactsIntl), card1);
    c1Split->setStyleSheet("color: #cbd5e1; font-size: 11px;");
    c1Lay->addWidget(c1Head);
    c1Lay->addWidget(c1Num);
    c1Lay->addWidget(c1Split);
    cardsLayout->addWidget(card1);

    // Card 2: Cards Printed
    QFrame *card2 = new QFrame(this);
    card2->setProperty("class", "metricCard");
    card2->setStyleSheet("background-color: #1e293b; border-radius: 8px; border: 1px solid #334155; padding: 8px;");
    QVBoxLayout *c2Lay = new QVBoxLayout(card2);
    c2Lay->setContentsMargins(8, 8, 8, 8);
    c2Lay->setSpacing(4);
    QLabel *c2Head = new QLabel(tr("Cards Printed"), card2);
    c2Head->setStyleSheet("color: #94a3b8; font-size: 12px; font-weight: bold;");
    QLabel *c2Num = new QLabel(QString::number(stats.cardsPrinted), card2);
    c2Num->setStyleSheet("color: #c084fc; font-size: 24px; font-weight: bold;");
    QLabel *c2Split = new QLabel(tr("Domestic: %1  |  Intl: %2").arg(stats.cardsPrintedDomestic).arg(stats.cardsPrintedIntl), card2);
    c2Split->setStyleSheet("color: #cbd5e1; font-size: 11px;");
    c2Lay->addWidget(c2Head);
    c2Lay->addWidget(c2Num);
    c2Lay->addWidget(c2Split);
    cardsLayout->addWidget(card2);

    // Card 3: Cards Received
    QFrame *card3 = new QFrame(this);
    card3->setProperty("class", "metricCard");
    card3->setStyleSheet("background-color: #1e293b; border-radius: 8px; border: 1px solid #334155; padding: 8px;");
    QVBoxLayout *c3Lay = new QVBoxLayout(card3);
    c3Lay->setContentsMargins(8, 8, 8, 8);
    c3Lay->setSpacing(4);
    QLabel *c3Head = new QLabel(tr("Cards Received"), card3);
    c3Head->setStyleSheet("color: #94a3b8; font-size: 12px; font-weight: bold;");
    QLabel *c3Num = new QLabel(QString::number(stats.cardsReceived), card3);
    c3Num->setStyleSheet("color: #34d399; font-size: 24px; font-weight: bold;");
    QLabel *c3Split = new QLabel(tr("Domestic: %1  |  Intl: %2").arg(stats.cardsReceivedDomestic).arg(stats.cardsReceivedIntl), card3);
    c3Split->setStyleSheet("color: #cbd5e1; font-size: 11px;");
    c3Lay->addWidget(c3Head);
    c3Lay->addWidget(c3Num);
    c3Lay->addWidget(c3Split);
    cardsLayout->addWidget(card3);

    mainLayout->addLayout(cardsLayout);

    // Table of QSOs logged or associated with this date
    QLabel *tableHead = new QLabel(tr("Logged Activities & Contacts"), this);
    tableHead->setStyleSheet("font-size: 13px; font-weight: bold; color: #38bdf8; margin-top: 6px;");
    mainLayout->addWidget(tableHead);

    QTableWidget *table = new QTableWidget(this);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({tr("Callsign"), tr("Time (UTC)"), tr("Band"), tr("Mode"), tr("Country"), tr("Status / Action")});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);

    // Populate contacts made
    int row = 0;
    for (const QsoDetail &d : stats.qsosMade) {
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(d.call));
        table->setItem(row, 1, new QTableWidgetItem(d.time));
        table->setItem(row, 2, new QTableWidgetItem(d.band));
        table->setItem(row, 3, new QTableWidgetItem(d.mode));
        table->setItem(row, 4, new QTableWidgetItem(d.country));
        table->setItem(row, 5, new QTableWidgetItem(tr("QSO Made")));
        row++;
    }

    // Populate printed cards
    for (const QsoDetail &d : stats.qslsPrinted) {
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(d.call));
        table->setItem(row, 1, new QTableWidgetItem(d.time));
        table->setItem(row, 2, new QTableWidgetItem(d.band));
        table->setItem(row, 3, new QTableWidgetItem(d.mode));
        table->setItem(row, 4, new QTableWidgetItem(d.country));
        table->setItem(row, 5, new QTableWidgetItem(tr("Card Printed")));
        row++;
    }

    // Populate received cards
    for (const QsoDetail &d : stats.qslsReceived) {
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(d.call));
        table->setItem(row, 1, new QTableWidgetItem(d.time));
        table->setItem(row, 2, new QTableWidgetItem(d.band));
        table->setItem(row, 3, new QTableWidgetItem(d.mode));
        table->setItem(row, 4, new QTableWidgetItem(d.country));
        table->setItem(row, 5, new QTableWidgetItem(tr("Card Received")));
        row++;
    }

    m_table = table;
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &DayDetailDialog::onCellDoubleClicked);
    mainLayout->addWidget(table, 1);

    // Feedback status
    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #34d399; font-style: italic; font-size: 11px;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_statusLabel);

    // Action buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnCopy = new QPushButton(tr("Copy Day Report"), this);
    connect(btnCopy, &QPushButton::clicked, this, &DayDetailDialog::onCopyDayReport);

    QPushButton *btnClose = new QPushButton(tr("Close"), this);
    btnClose->setObjectName("CloseButton");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(btnCopy);
    btnLayout->addStretch();
    btnLayout->addWidget(btnClose);
    mainLayout->addLayout(btnLayout);
}

// ============================================================================
// METHOD: DayDetailDialog::onCellDoubleClicked
// Purpose: Requirement 2a: when double-clicking a row of a contact whose
//          status is "QSO MADE", store the callsign and accept the dialog to
//          trigger dismissal and load into MainWindow.
// ============================================================================
void DayDetailDialog::onCellDoubleClicked(int row, int column) {
    Q_UNUSED(column);
    if (!m_table || row < 0 || row >= m_table->rowCount()) return;

    QTableWidgetItem *statusItem = m_table->item(row, 5);
    QTableWidgetItem *callItem = m_table->item(row, 0);
    if (!statusItem || !callItem) return;

    QString status = statusItem->text().trimmed();
    if (status.compare("QSO Made", Qt::CaseInsensitive) == 0) {
        m_selectedCallsign = callItem->text().trimmed().toUpper();
        if (!m_selectedCallsign.isEmpty()) {
            accept(); // Close report dialog
        }
    }
}

void DayDetailDialog::onCopyDayReport() {
    QString txt;
    txt += QString("Qwain Activity Report — %1\n").arg(m_date.toString("dddd, MMMM d, yyyy"));
    txt += "============================================================\n";
    txt += QString("Contacts Made:  %1 (Domestic: %2, International: %3)\n")
           .arg(m_stats.contactsMade).arg(m_stats.contactsDomestic).arg(m_stats.contactsIntl);
    txt += QString("Cards Printed:  %1 (Domestic: %2, International: %3)\n")
           .arg(m_stats.cardsPrinted).arg(m_stats.cardsPrintedDomestic).arg(m_stats.cardsPrintedIntl);
    txt += QString("Cards Received: %1 (Domestic: %2, International: %3)\n")
           .arg(m_stats.cardsReceived).arg(m_stats.cardsReceivedDomestic).arg(m_stats.cardsReceivedIntl);
    txt += "------------------------------------------------------------\n";
    txt += "Logged Activities:\n";
    txt += QString("%1 | %2 | %3 | %4 | %5 | %6\n")
           .arg("Callsign", -10).arg("Time UTC", -8).arg("Band", -6).arg("Mode", -6).arg("Country", -20).arg("Status", -12);
    txt += "-----------+----------+--------+--------+----------------------+-------------\n";

    for (const QsoDetail &d : m_stats.qsosMade) {
        txt += QString("%1 | %2 | %3 | %4 | %5 | %6\n")
               .arg(d.call, -10).arg(d.time, -8).arg(d.band, -6).arg(d.mode, -6).arg(d.country, -20).arg("QSO Made", -12);
    }
    for (const QsoDetail &d : m_stats.qslsPrinted) {
        txt += QString("%1 | %2 | %3 | %4 | %5 | %6\n")
               .arg(d.call, -10).arg(d.time, -8).arg(d.band, -6).arg(d.mode, -6).arg(d.country, -20).arg("Card Printed", -12);
    }
    for (const QsoDetail &d : m_stats.qslsReceived) {
        txt += QString("%1 | %2 | %3 | %4 | %5 | %6\n")
               .arg(d.call, -10).arg(d.time, -8).arg(d.band, -6).arg(d.mode, -6).arg(d.country, -20).arg("Card Received", -12);
    }

    QApplication::clipboard()->setText(txt);
    if (m_statusLabel) {
        m_statusLabel->setText(tr("Day report copied to clipboard!"));
    }
}

// ============================================================================
// DIALOG: StatsDialog
// Main statistics dialog with side-by-side Station Statistics on the left
// and interactive scrollable Activity Calendar on the right.
// ============================================================================
StatsDialog::StatsDialog(MainWindow *mainWin, QWidget *parent)
    : QDialog(parent), m_mainWin(mainWin)
{
    setWindowTitle(tr("Qwain Statistics & Activity Calendar"));
    resize(1200, 780);
    setMinimumSize(1040, 680);

    // Initialize to current year and month
    m_selectedYear = QDate::currentDate().year();
    m_selectedMonth = QDate::currentDate().month();

    // Theme-aware dark styling matching Qwain
    setStyleSheet(
        "QDialog { background-color: #0f172a; color: #f8fafc; font-family: 'Segoe UI', Arial, sans-serif; }"
        "QFrame { background-color: #1e293b; border-radius: 8px; border: 1px solid #334155; }"
        "QLabel { color: #cbd5e1; font-size: 13px; border: none; background: transparent; }"
        "QLabel#HeaderLabel { color: #f8fafc; font-size: 18px; font-weight: bold; }"
        "QLabel#SectionLabel { color: #38bdf8; font-size: 14px; font-weight: bold; border-bottom: 1px solid #334155; padding-bottom: 4px; }"
        "QLabel#ValLabel { color: #f1f5f9; font-weight: bold; font-size: 13px; }"
        "QTextEdit { background-color: #020617; color: #38bdf8; border: 1px solid #334155; border-radius: 6px; font-family: 'Consolas', 'Courier New', monospace; font-size: 12px; padding: 10px; }"
        "QComboBox, QSpinBox { background-color: #1e293b; color: #f8fafc; border: 1px solid #334155; border-radius: 6px; padding: 5px 10px; font-weight: bold; font-size: 13px; }"
        "QComboBox QAbstractItemView { background-color: #1e293b; color: #f8fafc; selection-background-color: #3b82f6; }"
        "QPushButton { background-color: #3b82f6; color: #ffffff; border: none; padding: 7px 14px; border-radius: 6px; font-weight: bold; font-size: 12px; }"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton#NavButton { background-color: #1e293b; color: #f8fafc; border: 1px solid #334155; padding: 5px 10px; font-weight: bold; font-size: 13px; }"
        "QPushButton#NavButton:hover { background-color: #334155; border-color: #60a5fa; }"
        "QPushButton#OkButton { background-color: #10b981; color: #ffffff; padding: 8px 20px; font-size: 13px; }"
        "QPushButton#OkButton:hover { background-color: #059669; }"
    );

    calculateStats(mainWin);
    m_formattedTextCache = buildStatsText();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(18, 16, 18, 16);
    mainLayout->setSpacing(12);

    // Header Title
    QLabel *titleLabel = new QLabel(tr("Qwain Statistics & Activity Calendar"), this);
    titleLabel->setObjectName("HeaderLabel");
    mainLayout->addWidget(titleLabel);

    // Side-by-side content layout: Left = Stats/Bands text pane, Right = Calendar pane
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(16);

    // ------------------------------------------------------------------------
    // LEFT PANE: Station Statistics & Band Breakdown
    // ------------------------------------------------------------------------
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(8);

    QLabel *leftTitle = new QLabel(tr("Station Statistics"), this);
    leftTitle->setObjectName("SectionLabel");
    leftLayout->addWidget(leftTitle);

    m_statsEdit = new QTextEdit(this);
    m_statsEdit->setReadOnly(true);
    m_statsEdit->setLineWrapMode(QTextEdit::NoWrap);
    m_statsEdit->setMinimumWidth(440);
    m_statsEdit->setMaximumWidth(500);

    // Enforce true system monospace font
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPointSize(10);
    m_statsEdit->setFont(monoFont);

    // Isolate mouse wheel focus to left pane
    m_statsEdit->installEventFilter(this);
    m_statsEdit->viewport()->installEventFilter(this);

    m_statsEdit->setHtml(buildStatsHtml());
    leftLayout->addWidget(m_statsEdit, 1);

    contentLayout->addLayout(leftLayout, 0);

    // ------------------------------------------------------------------------
    // RIGHT PANE: Scrollable Activity Calendar View
    // ------------------------------------------------------------------------
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(8);

    // Header navigation row: [<< Year] [< Month] [Month Combo] [Year Spin] [Month >] [Year >>] [Today]
    QHBoxLayout *navLayout = new QHBoxLayout();
    navLayout->setSpacing(6);

    QLabel *rightTitle = new QLabel(tr("Activity Calendar"), this);
    rightTitle->setObjectName("SectionLabel");
    navLayout->addWidget(rightTitle);
    navLayout->addStretch();

    QPushButton *btnPrevYear = new QPushButton(tr("<<"), this);
    btnPrevYear->setObjectName("NavButton");
    btnPrevYear->setToolTip(tr("Previous Year"));
    connect(btnPrevYear, &QPushButton::clicked, this, &StatsDialog::onPrevYear);
    navLayout->addWidget(btnPrevYear);

    QPushButton *btnPrevMonth = new QPushButton(tr("<"), this);
    btnPrevMonth->setObjectName("NavButton");
    btnPrevMonth->setToolTip(tr("Previous Month"));
    connect(btnPrevMonth, &QPushButton::clicked, this, &StatsDialog::onPrevMonth);
    navLayout->addWidget(btnPrevMonth);

    m_monthCombo = new QComboBox(this);
    for (int m = 1; m <= 12; ++m) {
        m_monthCombo->addItem(QLocale().monthName(m), m);
    }
    m_monthCombo->setCurrentIndex(m_selectedMonth - 1);
    connect(m_monthCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StatsDialog::onMonthComboChanged);
    navLayout->addWidget(m_monthCombo);

    m_yearSpin = new QSpinBox(this);
    m_yearSpin->setRange(1950, 2099);
    m_yearSpin->setValue(m_selectedYear);
    connect(m_yearSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &StatsDialog::onYearSpinChanged);
    navLayout->addWidget(m_yearSpin);

    QPushButton *btnNextMonth = new QPushButton(tr(">"), this);
    btnNextMonth->setObjectName("NavButton");
    btnNextMonth->setToolTip(tr("Next Month"));
    connect(btnNextMonth, &QPushButton::clicked, this, &StatsDialog::onNextMonth);
    navLayout->addWidget(btnNextMonth);

    QPushButton *btnNextYear = new QPushButton(tr(">>"), this);
    btnNextYear->setObjectName("NavButton");
    btnNextYear->setToolTip(tr("Next Year"));
    connect(btnNextYear, &QPushButton::clicked, this, &StatsDialog::onNextYear);
    navLayout->addWidget(btnNextYear);

    QPushButton *btnToday = new QPushButton(tr("Today"), this);
    btnToday->setObjectName("NavButton");
    btnToday->setToolTip(tr("Jump to Current Date"));
    connect(btnToday, &QPushButton::clicked, this, &StatsDialog::onTodayClicked);
    navLayout->addWidget(btnToday);

    rightLayout->addLayout(navLayout);

    // Month summary line
    m_monthSummaryLabel = new QLabel(this);
    m_monthSummaryLabel->setStyleSheet("color: #38bdf8; font-weight: bold; font-size: 13px; padding: 2px 0px;");
    rightLayout->addWidget(m_monthSummaryLabel);

    // Days of week header (Sun, Mon, Tue, Wed, Thu, Fri, Sat)
    QGridLayout *dayHeaderGrid = new QGridLayout();
    dayHeaderGrid->setSpacing(6);
    dayHeaderGrid->setContentsMargins(6, 2, 6, 2);
    const QStringList dayHeaders = {tr("Sun"), tr("Mon"), tr("Tue"), tr("Wed"), tr("Thu"), tr("Fri"), tr("Sat")};
    for (int i = 0; i < 7; ++i) {
        QLabel *lbl = new QLabel(dayHeaders[i], this);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color: #94a3b8; font-weight: bold; font-size: 12px; background-color: #1e293b; border-radius: 4px; padding: 4px; border: 1px solid #334155;");
        dayHeaderGrid->addWidget(lbl, 0, i);
    }
    rightLayout->addLayout(dayHeaderGrid);

    // Calendar grid scroll area
    m_calendarScrollArea = new QScrollArea(this);
    m_calendarScrollArea->setWidgetResizable(true);
    m_calendarScrollArea->setStyleSheet("QScrollArea { background-color: #020617; border: 1px solid #334155; border-radius: 8px; }");

    m_calendarGridContainer = new QWidget();
    m_calendarGridContainer->setStyleSheet("background-color: transparent;");
    m_calendarGrid = new QGridLayout(m_calendarGridContainer);
    m_calendarGrid->setContentsMargins(6, 6, 6, 6);
    m_calendarGrid->setSpacing(6);

    m_calendarScrollArea->setWidget(m_calendarGridContainer);
    rightLayout->addWidget(m_calendarScrollArea, 1);

    contentLayout->addLayout(rightLayout, 1);
    mainLayout->addLayout(contentLayout, 1);

    // Bottom Action Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    QPushButton *btnCopy = new QPushButton(tr("Copy to Clipboard"), this);
    connect(btnCopy, &QPushButton::clicked, this, &StatsDialog::onCopyToClipboard);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #34d399; font-style: italic; font-size: 12px;");

    QPushButton *btnOk = new QPushButton(tr("OK"), this);
    btnOk->setObjectName("OkButton");
    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);

    btnLayout->addWidget(btnCopy);
    btnLayout->addWidget(m_statusLabel);
    btnLayout->addStretch();
    btnLayout->addWidget(btnOk);

    mainLayout->addLayout(btnLayout);

    // Render initial calendar view
    updateCalendarView();
}

void StatsDialog::calculateStats(MainWindow *mainWin) {
    m_version = QCoreApplication::applicationVersion();
    if (m_version.isEmpty()) {
        m_version = "1.02";
    }

    if (mainWin) {
        m_sessionUptimeSecs = mainWin->sessionStartTime().secsTo(QDateTime::currentDateTimeUtc());
    } else {
        m_sessionUptimeSecs = 0;
    }

    QSettings settings("Qwain", "Qwain");
    quint64 savedTotal = settings.value("Uptime/TotalSeconds", 0).toULongLong();
    m_totalUptimeSecs = savedTotal + m_sessionUptimeSecs;

    m_templatesPrinted = settings.value("Stats/TotalTemplatesPrinted", 0).toInt();

    QString todayUtcStr = QDateTime::currentDateTimeUtc().toString("yyyyMMdd");
    QString todayLocalStr = QDate::currentDate().toString("yyyyMMdd");

    m_dailyStats.clear();

    if (mainWin) {
        // Sets to track distinct cards sent and received (deduplicated by callsign + ADIF date).
        // Standard ADIF format (QRZ, Club Log) uses YYYYMMDD for QSLSDATE and QSLRDATE.
        // Contacts sharing the same callsign and mailing date (including overflow cards)
        // are recognized as 1 physical card.
        QSet<QString> domesticSentCards;
        QSet<QString> domesticRcvdCards;
        QSet<QString> intlSentCards;
        QSet<QString> intlRcvdCards;

        // Daily tracking sets to deduplicate card events per day
        QMap<QDate, QSet<QString>> dailySentCards;
        QMap<QDate, QSet<QString>> dailyRcvdCards;

        const auto &allQsos = mainWin->allQsos();
        for (auto it = allQsos.constBegin(); it != allQsos.constEnd(); ++it) {
            for (const QsoDetail &d : it.value()) {
                m_qsosOverall++;

                QString cleanDate = d.date;
                cleanDate = cleanDate.remove('-').remove('/').remove(' ').trimmed();
                if (cleanDate == todayUtcStr || cleanDate == todayLocalStr) {
                    m_qsosToday++;
                }

                // Band statistics
                QString bandStr = d.band.trimmed().toLower();
                if (!bandStr.isEmpty()) {
                    if (!bandStr.endsWith("m") && bandStr.toInt() > 0) {
                        bandStr += "m";
                    }
                    m_bandCounts[bandStr]++;
                }

                // Domestic vs International entity identification
                QString dxcc = d.dxcc.trimmed();
                QString country = d.country.trimmed().toUpper();

                bool isDomestic = (dxcc == "291" || dxcc == "6" || dxcc == "110" || dxcc == "285" ||
                                   country.contains("UNITED STATES") || country == "USA" ||
                                   country == "ALASKA" || country == "HAWAII" || country == "PUERTO RICO" ||
                                   country == "VIRGIN ISLANDS" || country == "U.S. VIRGIN ISLANDS" ||
                                   country == "GUAM" || country == "AMERICAN SAMOA" || country == "NORTHERN MARIANA ISLANDS");

                // Total Contacts count: every contact record increments the counter
                if (isDomestic) {
                    m_domesticContacts++;
                } else {
                    m_intlContacts++;
                }

                // Call identification for card grouping
                QString callsign = it.key().trimmed().toUpper();
                if (callsign.isEmpty()) {
                    callsign = d.call.trimmed().toUpper();
                }

                // Calendar: Log QSO made by date
                QDate contactDate = parseAdifDate(d.date);
                if (contactDate.isValid()) {
                    DayStats &day = m_dailyStats[contactDate];
                    day.contactsMade++;
                    if (isDomestic) {
                        day.contactsDomestic++;
                    } else {
                        day.contactsIntl++;
                    }
                    day.qsosMade.append(d);
                }

                // Cards Sent Calculation:
                // Match callsign with QSLSDATE (ADIF standard YYYYMMDD).
                // Multiple contacts sent on the same card or printed across overflow templates
                // share the same sent date and collapse into 1 card sent.
                bool sent = (d.qsl_sent.trimmed().toUpper() == "Y" || !d.qslsdate.trimmed().isEmpty());
                if (sent) {
                    QString sDate = d.qslsdate.trimmed();
                    QString sentKey = callsign + "|" + (sDate.isEmpty() ? QStringLiteral("SENT_UNSPECIFIED") : sDate);
                    if (isDomestic) {
                        domesticSentCards.insert(sentKey);
                    } else {
                        intlSentCards.insert(sentKey);
                    }

                    // Calendar: Record printed card on the specific date (deduplicated per day)
                    QDate cardDate = parseAdifDate(sDate);
                    if (cardDate.isValid()) {
                        QString daySentKey = callsign + "|" + sDate;
                        if (!dailySentCards[cardDate].contains(daySentKey)) {
                            dailySentCards[cardDate].insert(daySentKey);
                            DayStats &day = m_dailyStats[cardDate];
                            day.cardsPrinted++;
                            if (isDomestic) {
                                day.cardsPrintedDomestic++;
                            } else {
                                day.cardsPrintedIntl++;
                            }
                            day.qslsPrinted.append(d);
                        }
                    }
                }

                // Cards Received Calculation:
                // Match callsign with QSLRDATE (ADIF standard YYYYMMDD).
                // Multiple contacts confirmed on the same card share the same received date and collapse into 1 card received.
                bool rcvd = (d.qsl_rcvd.trimmed().toUpper() == "Y" || !d.qslrdate.trimmed().isEmpty() || d.cardRcvd);
                if (rcvd) {
                    QString rDate = d.qslrdate.trimmed();
                    QString rcvdKey = callsign + "|" + (rDate.isEmpty() ? QStringLiteral("RCVD_UNSPECIFIED") : rDate);
                    if (isDomestic) {
                        domesticRcvdCards.insert(rcvdKey);
                    } else {
                        intlRcvdCards.insert(rcvdKey);
                    }

                    // Calendar: Record received card on the specific date (deduplicated per day)
                    QDate cardDate = parseAdifDate(rDate);
                    if (cardDate.isValid()) {
                        QString dayRcvdKey = callsign + "|" + rDate;
                        if (!dailyRcvdCards[cardDate].contains(dayRcvdKey)) {
                            dailyRcvdCards[cardDate].insert(dayRcvdKey);
                            DayStats &day = m_dailyStats[cardDate];
                            day.cardsReceived++;
                            if (isDomestic) {
                                day.cardsReceivedDomestic++;
                            } else {
                                day.cardsReceivedIntl++;
                            }
                            day.qslsReceived.append(d);
                        }
                    }
                }
            }
        }

        // Store unique physical card counts
        m_domesticSent = domesticSentCards.size();
        m_domesticRcvd = domesticRcvdCards.size();
        m_intlSent = intlSentCards.size();
        m_intlRcvd = intlRcvdCards.size();
    }

    int savedPrinted = settings.value("Stats/TotalTemplatesPrinted", 0).toInt();
    if (savedPrinted > 0) {
        m_templatesPrinted = savedPrinted;
    } else {
        m_templatesPrinted = m_domesticSent + m_intlSent;
    }
}

QString StatsDialog::formatUptime(quint64 totalSecs) const {
    quint64 days = totalSecs / 86400;
    quint64 hours = (totalSecs % 86400) / 3600;
    quint64 mins = (totalSecs % 3600) / 60;
    quint64 secs = totalSecs % 60;

    if (days > 0) {
        return QString("%1d %2h %3m %4s").arg(days).arg(hours, 2, 10, QChar('0')).arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
    } else if (hours > 0) {
        return QString("%1h %2m %3s").arg(hours).arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1m %2s").arg(mins).arg(secs, 2, 10, QChar('0'));
    }
}

QString StatsDialog::buildStatsText() const {
    QString txt;

    txt += QString("Qwain Version: %1\n").arg(m_version);
    txt += QString("Uptime Total: %1\n").arg(formatUptime(m_totalUptimeSecs));
    txt += QString("Uptime This Session: %1\n").arg(formatUptime(m_sessionUptimeSecs));
    txt += "------------------------------\n";
    txt += QString("QSOs Made Today: %1\n").arg(m_qsosToday);
    txt += QString("QSOs Made Overall: %1\n").arg(m_qsosOverall);
    txt += "------------------------------\n";
    txt += "Bands\n\n";

    // 1. Bands Table: 2-column layout with right-justified band names and aligned stats
    const QStringList col1Bands = {"6m",  "10m", "12m", "15m", "17m", "20m"};
    const QStringList col3Bands = {"30m", "40m", "60m", "80m", "160m"};

    auto formatBandStats = [this](const QString &band) -> QString {
        int cnt = m_bandCounts.value(band, 0);
        double pct = (m_qsosOverall > 0) ? (cnt * 100.0 / m_qsosOverall) : 0.0;
        return QString("%1 (%2%)").arg(cnt).arg(pct, 0, 'f', 1);
    };

    int maxRows = qMax(col1Bands.size(), col3Bands.size());
    for (int i = 0; i < maxRows; ++i) {
        QString line;
        if (i < col1Bands.size()) {
            QString b1 = col1Bands[i];
            QString s1 = formatBandStats(b1);
            if (i < col3Bands.size()) {
                line += QString("%1 : %2").arg(b1, 5).arg(s1, -19);
            } else {
                line += QString("%1 : %2").arg(b1, 5).arg(s1);
            }
        }

        if (i < col3Bands.size()) {
            QString b2 = col3Bands[i];
            QString s2 = formatBandStats(b2);
            line += QString("%1 : %2").arg(b2, 5).arg(s2);
        }

        txt += line + "\n";
    }

    txt += "--------------------------------  \n";

    // 3. Usage Table: Left cell right-justified (width 39), right cell left-justified, no borders
    txt += "Usage\n\n";
    txt += QString("%1 %2\n\n")
           .arg("Number of Templates Printed:", 39)
           .arg(m_templatesPrinted);

    txt += QString("%1 %2\n")
           .arg("Number of Domestic Contacts:", 39)
           .arg(m_domesticContacts);
    txt += QString("%1 %2\n")
           .arg("Number of Domestic Cards Sent:", 39)
           .arg(m_domesticSent);
    txt += QString("%1 %2\n\n")
           .arg("Number of Domestic Cards Received:", 39)
           .arg(m_domesticRcvd);

    txt += QString("%1 %2\n")
           .arg("Number of International Contacts:", 39)
           .arg(m_intlContacts);
    txt += QString("%1 %2\n")
           .arg("Number of International Cards Sent:", 39)
           .arg(m_intlSent);
    txt += QString("%1 %2\n\n")
           .arg("Number of International Cards Received:", 39)
           .arg(m_intlRcvd);

    // Requirement 3: Include selected month activity in the copied text
    txt += "--------------------------------  \n";
    txt += QString("Selected Month Activity: %1 %2\n\n")
           .arg(QLocale().monthName(m_selectedMonth))
           .arg(m_selectedYear);

    int monthContacts = 0;
    int monthDomContacts = 0;
    int monthIntlContacts = 0;

    int monthPrinted = 0;
    int monthDomPrinted = 0;
    int monthIntlPrinted = 0;

    int monthRcvd = 0;
    int monthDomRcvd = 0;
    int monthIntlRcvd = 0;

    int daysInM = QDate(m_selectedYear, m_selectedMonth, 1).daysInMonth();

    for (int d = 1; d <= daysInM; ++d) {
        QDate curDate(m_selectedYear, m_selectedMonth, d);
        DayStats ds = m_dailyStats.value(curDate);

        monthContacts += ds.contactsMade;
        monthDomContacts += ds.contactsDomestic;
        monthIntlContacts += ds.contactsIntl;

        monthPrinted += ds.cardsPrinted;
        monthDomPrinted += ds.cardsPrintedDomestic;
        monthIntlPrinted += ds.cardsPrintedIntl;

        monthRcvd += ds.cardsReceived;
        monthDomRcvd += ds.cardsReceivedDomestic;
        monthIntlRcvd += ds.cardsReceivedIntl;
    }

    txt += QString("Month Contacts Made:     %1 (Domestic: %2, International: %3)\n")
           .arg(monthContacts).arg(monthDomContacts).arg(monthIntlContacts);
    txt += QString("Month Cards Printed:     %1 (Domestic: %2, International: %3)\n")
           .arg(monthPrinted).arg(monthDomPrinted).arg(monthIntlPrinted);
    txt += QString("Month Cards Received:    %1 (Domestic: %2, International: %3)\n")
           .arg(monthRcvd).arg(monthDomRcvd).arg(monthIntlRcvd);

    return txt;
}

QString StatsDialog::buildStatsHtml() const {
    QString html;
    html += "<html><head><style>"
            "body { font-family: monospace, 'DejaVu Sans Mono', 'Liberation Mono', 'Courier New'; font-size: 12px; color: #38bdf8; background-color: #020617; margin: 0; padding: 4px; }"
            ".title { color: #f8fafc; font-weight: bold; font-size: 13px; }"
            ".dim { color: #94a3b8; }"
            ".val { color: #f1f5f9; font-weight: bold; }"
            ".colon { color: #64748b; padding: 0 4px; }"
            "table.bands { border-collapse: collapse; margin-top: 4px; margin-bottom: 8px; }"
            "table.bands td { padding: 2px 0; font-family: monospace; font-size: 12px; }"
            "table.usage { border-collapse: collapse; margin-top: 4px; margin-bottom: 8px; }"
            "table.usage td { padding: 2px 0; font-family: monospace; font-size: 12px; }"
            "table.daily { border-collapse: collapse; width: 100%; margin-top: 6px; font-family: monospace; font-size: 12px; }"
            "table.daily th { color: #38bdf8; border-bottom: 1px solid #334155; padding: 4px 6px; font-weight: bold; }"
            "table.daily td { padding: 2px 6px; border-bottom: 1px solid #111827; }"
            "hr { border: 0; border-top: 1px solid #334155; margin: 10px 0; }"
            "</style></head><body>";

    // Uptime & Header
    html += QString("<div>Qwain Version: <span class='val'>%1</span></div>").arg(m_version);
    html += QString("<div>Uptime Total: <span class='val'>%1</span></div>").arg(formatUptime(m_totalUptimeSecs));
    html += QString("<div>Uptime This Session: <span class='val'>%1</span></div>").arg(formatUptime(m_sessionUptimeSecs));
    html += "<hr>";
    html += QString("<div>QSOs Made Today: <span class='val'>%1</span></div>").arg(m_qsosToday);
    html += QString("<div>QSOs Made Overall: <span class='val'>%1</span></div>").arg(m_qsosOverall);
    html += "<hr>";
    html += "<div class='title'>Bands</div>";

    // 1. Bands Table (2 columns, physically locked cell widths and alignments)
    const QStringList col1Bands = {"6m",  "10m", "12m", "15m", "17m", "20m"};
    const QStringList col3Bands = {"30m", "40m", "60m", "80m", "160m"};

    auto formatBandStats = [this](const QString &band) -> QString {
        int cnt = m_bandCounts.value(band, 0);
        double pct = (m_qsosOverall > 0) ? (cnt * 100.0 / m_qsosOverall) : 0.0;
        return QString("%1 (%2%)").arg(cnt).arg(pct, 0, 'f', 1);
    };

    html += "<table class='bands' cellspacing='0' cellpadding='2' border='0'>";
    int maxRows = qMax(col1Bands.size(), col3Bands.size());
    for (int i = 0; i < maxRows; ++i) {
        html += "<tr>";
        if (i < col1Bands.size()) {
            QString b1 = col1Bands[i];
            QString s1 = formatBandStats(b1);
            html += QString("<td align='right' style='width: 45px;' class='val'>%1</td>").arg(b1);
            html += "<td align='center' class='colon'>:</td>";
            html += QString("<td align='left' style='width: 140px; padding-right: 18px;'>%1</td>").arg(s1);
        } else {
            html += "<td colspan='3'></td>";
        }

        if (i < col3Bands.size()) {
            QString b2 = col3Bands[i];
            QString s2 = formatBandStats(b2);
            html += QString("<td align='right' style='width: 45px;' class='val'>%1</td>").arg(b2);
            html += "<td align='center' class='colon'>:</td>";
            html += QString("<td align='left' style='width: 140px;'>%1</td>").arg(s2);
        } else {
            html += "<td colspan='3'></td>";
        }
        html += "</tr>";
    }
    html += "</table>";
    html += "<hr>";

    // 3. Usage Table: Left cell right-justified, right cell left-justified, no borders
    html += "<div class='title'>Usage</div>";
    html += "<table class='usage' cellspacing='0' cellpadding='2' border='0'>";
    html += QString("<tr><td align='right' class='dim' style='padding-right: 12px;'>Number of Templates Printed:</td><td align='left' class='val'>%1</td></tr>").arg(m_templatesPrinted);
    html += "<tr><td colspan='2' style='font-size: 4px;'>&nbsp;</td></tr>";
    html += QString("<tr><td align='right' class='dim' style='padding-right: 12px;'>Number of Domestic Contacts:</td><td align='left' class='val'>%1</td></tr>").arg(m_domesticContacts);
    html += QString("<tr><td align='right' class='dim' style='padding-right: 12px;'>Number of Domestic Cards Sent:</td><td align='left' class='val'>%1</td></tr>").arg(m_domesticSent);
    html += QString("<tr><td align='right' class='dim' style='padding-right: 12px;'>Number of Domestic Cards Received:</td><td align='left' class='val'>%1</td></tr>").arg(m_domesticRcvd);
    html += "<tr><td colspan='2' style='font-size: 4px;'>&nbsp;</td></tr>";
    html += QString("<tr><td align='right' class='dim' style='padding-right: 12px;'>Number of International Contacts:</td><td align='left' class='val'>%1</td></tr>").arg(m_intlContacts);
    html += QString("<tr><td align='right' class='dim' style='padding-right: 12px;'>Number of International Cards Sent:</td><td align='left' class='val'>%1</td></tr>").arg(m_intlSent);
    html += QString("<tr><td align='right' class='dim' style='padding-right: 12px;'>Number of International Cards Received:</td><td align='left' class='val'>%1</td></tr>").arg(m_intlRcvd);
    html += "</table>";
    html += "<hr>";

    // Selected Month Header & Summary
    html += QString("<div class='title'>Selected Month Activity: %1 %2</div><br>")
            .arg(QLocale().monthName(m_selectedMonth))
            .arg(m_selectedYear);

    int monthContacts = 0, monthDomContacts = 0, monthIntlContacts = 0;
    int monthPrinted = 0, monthDomPrinted = 0, monthIntlPrinted = 0;
    int monthRcvd = 0, monthDomRcvd = 0, monthIntlRcvd = 0;

    int daysInM = QDate(m_selectedYear, m_selectedMonth, 1).daysInMonth();

    for (int d = 1; d <= daysInM; ++d) {
        QDate curDate(m_selectedYear, m_selectedMonth, d);
        DayStats ds = m_dailyStats.value(curDate);

        monthContacts += ds.contactsMade;
        monthDomContacts += ds.contactsDomestic;
        monthIntlContacts += ds.contactsIntl;

        monthPrinted += ds.cardsPrinted;
        monthDomPrinted += ds.cardsPrintedDomestic;
        monthIntlPrinted += ds.cardsPrintedIntl;

        monthRcvd += ds.cardsReceived;
        monthDomRcvd += ds.cardsReceivedDomestic;
        monthIntlRcvd += ds.cardsReceivedIntl;
    }

    html += QString("<div>Month Contacts Made: <span class='val'>%1</span> <span class='dim'>(Domestic: %2, Intl: %3)</span></div>")
            .arg(monthContacts).arg(monthDomContacts).arg(monthIntlContacts);
    html += QString("<div>Month Cards Printed: <span class='val'>%1</span> <span class='dim'>(Domestic: %2, Intl: %3)</span></div>")
            .arg(monthPrinted).arg(monthDomPrinted).arg(monthIntlPrinted);
    html += QString("<div>Month Cards Received: <span class='val'>%1</span> <span class='dim'>(Domestic: %2, Intl: %3)</span></div>")
            .arg(monthRcvd).arg(monthDomRcvd).arg(monthIntlRcvd);

    html += "</body></html>";
    return html;
}

void StatsDialog::updateCalendarView() {
    // Clear previous cell widgets in calendar grid
    QLayoutItem *child;
    while ((child = m_calendarGrid->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    QDate firstDate(m_selectedYear, m_selectedMonth, 1);
    int daysInMonth = firstDate.daysInMonth();

    // Qt dayOfWeek: 1 = Monday ... 7 = Sunday
    // For standard Sunday = 0 calendar:
    int startCol = firstDate.dayOfWeek() % 7;

    QDate prevMonthDate = firstDate.addMonths(-1);
    int daysInPrevMonth = prevMonthDate.daysInMonth();
    QDate nextMonthDate = firstDate.addMonths(1);

    int monthTotalContacts = 0;
    int monthTotalPrinted = 0;
    int monthTotalRcvd = 0;

    // Fill 42 cells (6 weeks x 7 days)
    for (int cell = 0; cell < 42; ++cell) {
        int r = cell / 7;
        int c = cell % 7;
        QDate cellDate;
        bool isCurrentMonth = false;

        if (cell < startCol) {
            int d = daysInPrevMonth - startCol + cell + 1;
            cellDate = QDate(prevMonthDate.year(), prevMonthDate.month(), d);
            isCurrentMonth = false;
        } else if (cell < startCol + daysInMonth) {
            int d = cell - startCol + 1;
            cellDate = QDate(m_selectedYear, m_selectedMonth, d);
            isCurrentMonth = true;
        } else {
            int d = cell - (startCol + daysInMonth) + 1;
            cellDate = QDate(nextMonthDate.year(), nextMonthDate.month(), d);
            isCurrentMonth = false;
        }

        DayStats stats = m_dailyStats.value(cellDate);
        if (isCurrentMonth) {
            monthTotalContacts += stats.contactsMade;
            monthTotalPrinted += stats.cardsPrinted;
            monthTotalRcvd += stats.cardsReceived;
        }

        bool isToday = (cellDate == QDate::currentDate());
        DayBoxWidget *box = new DayBoxWidget(cellDate, stats, isCurrentMonth, isToday, m_calendarGridContainer);
        connect(box, &DayBoxWidget::dayClicked, this, &StatsDialog::openDayDetail);
        m_calendarGrid->addWidget(box, r, c);
    }

    // Update month summary label
    m_monthSummaryLabel->setText(
        QString("%1 %2:  %3 Contacts Made  |  %4 Cards Printed  |  %5 Cards Received")
        .arg(QLocale().monthName(m_selectedMonth))
        .arg(m_selectedYear)
        .arg(monthTotalContacts)
        .arg(monthTotalPrinted)
        .arg(monthTotalRcvd)
    );
}

void StatsDialog::onCopyToClipboard() {
    QApplication::clipboard()->setText(m_formattedTextCache);
    if (m_statusLabel) {
        m_statusLabel->setText(tr("Statistics & selected month copied to clipboard!"));
    }
}

void StatsDialog::onPrevMonth() {
    if (m_selectedMonth == 1) {
        m_selectedMonth = 12;
        m_selectedYear--;
    } else {
        m_selectedMonth--;
    }

    m_monthCombo->blockSignals(true);
    m_yearSpin->blockSignals(true);
    m_monthCombo->setCurrentIndex(m_selectedMonth - 1);
    m_yearSpin->setValue(m_selectedYear);
    m_monthCombo->blockSignals(false);
    m_yearSpin->blockSignals(false);

    updateCalendarView();
    m_formattedTextCache = buildStatsText();
    m_statsEdit->setHtml(buildStatsHtml());
}

void StatsDialog::onNextMonth() {
    if (m_selectedMonth == 12) {
        m_selectedMonth = 1;
        m_selectedYear++;
    } else {
        m_selectedMonth++;
    }

    m_monthCombo->blockSignals(true);
    m_yearSpin->blockSignals(true);
    m_monthCombo->setCurrentIndex(m_selectedMonth - 1);
    m_yearSpin->setValue(m_selectedYear);
    m_monthCombo->blockSignals(false);
    m_yearSpin->blockSignals(false);

    updateCalendarView();
    m_formattedTextCache = buildStatsText();
    m_statsEdit->setHtml(buildStatsHtml());
}

void StatsDialog::onPrevYear() {
    m_selectedYear--;
    m_yearSpin->blockSignals(true);
    m_yearSpin->setValue(m_selectedYear);
    m_yearSpin->blockSignals(false);

    updateCalendarView();
    m_formattedTextCache = buildStatsText();
    m_statsEdit->setHtml(buildStatsHtml());
}

void StatsDialog::onNextYear() {
    m_selectedYear++;
    m_yearSpin->blockSignals(true);
    m_yearSpin->setValue(m_selectedYear);
    m_yearSpin->blockSignals(false);

    updateCalendarView();
    m_formattedTextCache = buildStatsText();
    m_statsEdit->setHtml(buildStatsHtml());
}

void StatsDialog::onMonthComboChanged(int index) {
    m_selectedMonth = index + 1;
    updateCalendarView();
    m_formattedTextCache = buildStatsText();
    m_statsEdit->setHtml(buildStatsHtml());
}

void StatsDialog::onYearSpinChanged(int year) {
    m_selectedYear = year;
    updateCalendarView();
    m_formattedTextCache = buildStatsText();
    m_statsEdit->setHtml(buildStatsHtml());
}

void StatsDialog::onTodayClicked() {
    m_selectedYear = QDate::currentDate().year();
    m_selectedMonth = QDate::currentDate().month();

    m_monthCombo->blockSignals(true);
    m_yearSpin->blockSignals(true);
    m_monthCombo->setCurrentIndex(m_selectedMonth - 1);
    m_yearSpin->setValue(m_selectedYear);
    m_monthCombo->blockSignals(false);
    m_yearSpin->blockSignals(false);

    updateCalendarView();
    m_formattedTextCache = buildStatsText();
    m_statsEdit->setHtml(buildStatsHtml());
}

void StatsDialog::openDayDetail(const QDate &date) {
    DayStats stats = m_dailyStats.value(date);
    DayDetailDialog dlg(date, stats, this);
    if (dlg.exec() == QDialog::Accepted) {
        QString call = dlg.selectedCallsign();
        if (!call.isEmpty()) {
            // Requirement 2a: self-close report and statsdialog and open selected callsign in #mainwindow
            if (m_mainWin) {
                m_mainWin->lookupCallsign(call);
            }
            accept(); // Close StatsDialog
        }
    }
}

bool StatsDialog::eventFilter(QObject *watched, QEvent *event) {
    // Isolate left pane: when mouse wheel is rolled over m_statsEdit or its viewport,
    // scroll m_statsEdit directly and swallow the event so hitting the top/bottom limit
    // NEVER bubbles up to move the calendar!
    if ((watched == m_statsEdit || (m_statsEdit && watched == m_statsEdit->viewport())) && event->type() == QEvent::Wheel) {
        QWheelEvent *we = static_cast<QWheelEvent*>(event);
        QScrollBar *vbar = m_statsEdit->verticalScrollBar();
        if (vbar) {
            int numDegrees = we->angleDelta().y() / 8;
            int numSteps = numDegrees / 15;
            vbar->setValue(vbar->value() - (numSteps * vbar->singleStep() * 3));
        }
        return true; // Swallowed completely; never propagates to parent StatsDialog
    }
    return QDialog::eventFilter(watched, event);
}

void StatsDialog::wheelEvent(QWheelEvent *event) {
    // Isolate right pane: only cycle calendar months/years when mouse is physically inside calendar area
    if (m_calendarScrollArea) {
        QPoint ptInCalendar = m_calendarScrollArea->mapFrom(this, event->pos());
        if (m_calendarScrollArea->rect().contains(ptInCalendar)) {
            if (event->angleDelta().y() > 0) {
                if (event->modifiers() & Qt::ControlModifier) {
                    onPrevYear();
                } else {
                    onPrevMonth();
                }
            } else if (event->angleDelta().y() < 0) {
                if (event->modifiers() & Qt::ControlModifier) {
                    onNextYear();
                } else {
                    onNextMonth();
                }
            }
            event->accept();
            return;
        }
    }
    event->ignore();
}
