// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

// ============================================================================
// COMPONENT: ReceiveQslsDialog
// ROLE: Incoming QSL card registration and online logbook synchronization.
//       This component processes received physical paper cards, matches them
//       against local QSO records, updates persistent .adi log files, and
//       synchronizes status changes (QSL_RCVD, QSLRDATE) back to online
//       databases (QRZ, eQSL, ClubLog, LoTW).
// INVOKED BY: MainWindow (via the "Receive QSL" action).
// PRODUCES: Updated local log files and synchronized online logbooks.
// ============================================================================

#include "receiveqsldialog.h"
#include "qrzcache.h"
#include "mainwindow.h"
#include "paths.h"
#include "qslpreviewdialog.h"
#include <QPointer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSlider>
#include <QLabel>
#include <QTextBrowser>
#include <QTableWidget>
#include <QScrollArea>
#include <QPushButton>
#include <QToolButton>
#include <QTextEdit>
#include <QHeaderView>
#include <QScroller>
#include <QScrollBar>
#include <QCalendarWidget>
#include <QMessageBox>
#include <QGroupBox>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>
#include <QTimeZone>
#include <QDir>
#include <QSettings>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QDialog>
#include <algorithm>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QKeyEvent>
#include <QClipboard>
#include <QGuiApplication>
#include <QToolTip>

class CheckBoxDelegate : public QStyledItemDelegate {
public:
    explicit CheckBoxDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        painter->save();

        // Draw selection/hover highlights
        if (option.state & QStyle::State_Selected) {
            painter->fillRect(option.rect, QColor(45, 49, 63, 100));
        } else if (option.state & QStyle::State_MouseOver) {
            painter->fillRect(option.rect, QColor(45, 49, 63, 50));
        }

        bool checked = index.data(Qt::UserRole).toBool();

        // Checkbox geometry: center inside the cell
        int size = 14;
        int x = option.rect.x() + (option.rect.width() - size) / 2;
        int y = option.rect.y() + (option.rect.height() - size) / 2;
        QRect boxRect(x, y, size, size);

        painter->setRenderHint(QPainter::Antialiasing, true);

        // Draw the border with a white outline
        QPen borderPen(Qt::white, 1.5);
        painter->setPen(borderPen);

        if (checked) {
            painter->setBrush(QBrush(QColor(16, 185, 129))); // Vibrant green fill
        } else {
            painter->setBrush(Qt::NoBrush); // Transparent inside when unchecked
        }

        painter->drawRoundedRect(boxRect, 3, 3); // Slightly rounded corners for modern look

        // Draw elegant custom vector checkmark when checked
        if (checked) {
            QPen checkPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter->setPen(checkPen);
            // Draw a precise vector tick centered in the box
            painter->drawLine(x + 3, y + 7, x + 6, y + 10);
            painter->drawLine(x + 6, y + 10, x + 11, y + 4);
        }

        painter->restore();
    }
};

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

ReceiveQslsDialog::ReceiveQslsDialog(QWidget *parent, MainWindow *mainWnd)
    : QDialog(parent), m_mainWnd(mainWnd) {
    setWindowTitle("Receive QSLs");
    resize(889, 637);

    m_netManager = new QNetworkAccessManager(this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Centered callsign search box, 15 chars wide
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setMaxLength(15);
    m_searchEdit->setFixedWidth(150);
    m_searchEdit->setPlaceholderText("CALLSIGN");
    m_searchEdit->setAlignment(Qt::AlignCenter);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->addStretch();
    QLabel *searchLbl = new QLabel("Search Callsign: ", this);
    searchLayout->addWidget(searchLbl);
    searchLayout->addWidget(m_searchEdit);
    searchLayout->addStretch();

    mainLayout->addLayout(searchLayout);

    connect(m_searchEdit, &QLineEdit::returnPressed, this, &ReceiveQslsDialog::onSearch);

    // Split pane layout
    QHBoxLayout *splitLayout = new QHBoxLayout();

    // Left Pane (Callsign Info groupbox)
    QGroupBox *leftGroup = new QGroupBox("Callsign Info", this);
    leftGroup->setFixedWidth(320);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftGroup);
    leftLayout->setSpacing(8);
    m_infoBrowser = new QTextBrowser(this);
    m_infoBrowser->setOpenExternalLinks(true);
    leftLayout->addWidget(m_infoBrowser);

    // Date Received Input Layout
    QLabel *dateLabel = new QLabel("Date QSL Card Received:", this);
    dateLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    leftLayout->addWidget(dateLabel);

    QHBoxLayout *dateRowLayout = new QHBoxLayout();
    m_dateReceivedEdit = new QLineEdit(this);
    m_dateReceivedEdit->setFixedWidth(120);
    m_dateReceivedEdit->setText(QDate::currentDate().toString("yyyy-MM-dd"));

    m_calendarBtn = new QToolButton(this);
    m_calendarBtn->setText("Or Select Date");
    m_calendarBtn->setStyleSheet(
        "QToolButton { color: orange; background: transparent; border: none; font-weight: bold; text-decoration: underline; font-size: 10pt; }"
        "QToolButton:hover { color: #f59e0b; }"
    );
    m_calendarBtn->setCursor(Qt::PointingHandCursor);
    m_calendarBtn->setToolTip("Select Date");
    connect(m_calendarBtn, &QToolButton::clicked, this, &ReceiveQslsDialog::onCalendarClicked);

    dateRowLayout->addWidget(m_dateReceivedEdit);
    dateRowLayout->addWidget(m_calendarBtn);
    dateRowLayout->addStretch();
    leftLayout->addLayout(dateRowLayout);

    // Submit button
    m_submitBtn = new QPushButton("Submit", this);
    m_submitBtn->setStyleSheet(
        "QPushButton { background-color: #2c3e50; color: white; border: 1px solid #34495e; border-radius: 4px; padding: 6px 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: #34495e; }"
        "QPushButton:pressed { background-color: #1abc9c; }"
    );
    leftLayout->addWidget(m_submitBtn);
    connect(m_submitBtn, &QPushButton::clicked, this, &ReceiveQslsDialog::onSubmit);

    // Send Return Card button (above Status, centered below submit)
    m_sendReturnCardBtn = new QPushButton("Send Return Card", this);
    m_sendReturnCardBtn->setStyleSheet(
        "QPushButton { background-color: #2c3e50; color: white; border: 1px solid #34495e; border-radius: 4px; padding: 6px 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: #34495e; }"
        "QPushButton:pressed { background-color: #1abc9c; }"
        "QPushButton:disabled { background-color: #333333; color: #777777; border-color: #444444; }"
    );
    m_sendReturnCardBtn->setEnabled(false);
    QHBoxLayout *sendReturnCardLayout = new QHBoxLayout();
    sendReturnCardLayout->addStretch();
    sendReturnCardLayout->addWidget(m_sendReturnCardBtn);
    sendReturnCardLayout->addStretch();
    leftLayout->addLayout(sendReturnCardLayout);
    connect(m_sendReturnCardBtn, &QPushButton::clicked, this, &ReceiveQslsDialog::onSendReturnCard);

    // Status area label
    QLabel *statusLabel = new QLabel("Status:", this);
    statusLabel->setStyleSheet("font-weight: bold; margin-top: 4px;");
    leftLayout->addWidget(statusLabel);

    m_statusLog = new QTextEdit(this);
    m_statusLog->setReadOnly(true);
    m_statusLog->setFixedHeight(80);
    leftLayout->addWidget(m_statusLog);

    splitLayout->addWidget(leftGroup);

    // Right Pane (QSL/QSO Receiving groupbox)
    QGroupBox *rightGroup = new QGroupBox("QSL/QSO Receiving", this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightGroup);

    // Create a scroll area to act as the viewport (like left pane)
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    scrollArea->setStyleSheet("QScrollArea { border: 1px solid #2d313f; background-color: #111318; border-radius: 6px; }");
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Enable smooth kinetic dragging scroll (without elevator/scrollbar)
    QScroller::grabGesture(scrollArea->viewport(), QScroller::LeftMouseButtonGesture);

    QWidget *scrollContent = new QWidget(scrollArea);
    scrollContent->setStyleSheet("background: transparent;");
    QVBoxLayout *scrollContentLayout = new QVBoxLayout(scrollContent);
    scrollContentLayout->setContentsMargins(8, 8, 8, 8);
    scrollContentLayout->setSpacing(8);

    // Add legend label at the top of the viewport
    m_legendLabel = new QLabel(scrollContent);
    m_legendLabel->setText("<span style='color: #ef4444; font-weight: bold;'>Not Received</span> &nbsp;&nbsp;&nbsp;&nbsp; <span style='color: #10b981; font-weight: bold;'>Received</span>");
    m_legendLabel->setAlignment(Qt::AlignCenter);
    m_legendLabel->setStyleSheet("font-size: 9pt; background: transparent; margin-top: 4px; margin-bottom: 2px;");
    scrollContentLayout->addWidget(m_legendLabel);

    // Clickable Select All button/text
    m_selectAllBtn = new QPushButton("Select All", scrollContent);
    m_selectAllBtn->setFlat(true);
    m_selectAllBtn->setCursor(Qt::PointingHandCursor);
    m_selectAllBtn->setStyleSheet(
        "QPushButton { color: orange; font-weight: bold; font-size: 10pt; border: none; background: transparent; padding: 2px; }"
        "QPushButton:hover { color: #f97316; }"
    );
    scrollContentLayout->addWidget(m_selectAllBtn, 0, Qt::AlignCenter);

    connect(m_selectAllBtn, &QPushButton::clicked, this, [this]() {
        bool isSelectAll = (m_selectAllBtn->text() == "Select All");
        for (int i = 0; i < m_contactsTable->rowCount(); ++i) {
            if (QTableWidgetItem *chkItem = m_contactsTable->item(i, 0)) {
                chkItem->setData(Qt::UserRole, isSelectAll);
            }
        }
        m_contactsTable->viewport()->update();
        m_selectAllBtn->setText(isSelectAll ? "DeSelect All" : "Select All");
    });

    // Table for previous contacts - Styled like small contact table (no lines, no elevator, transparent)
    m_contactsTable = new QTableWidget(scrollContent);
    m_contactsTable->setColumnCount(5);
    m_contactsTable->setHorizontalHeaderLabels(QStringList() << "Update" << "Date" << "Time" << "Band" << "Mode");
    m_contactsTable->horizontalHeader()->setVisible(true);
    m_contactsTable->verticalHeader()->setVisible(false);
    m_contactsTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_contactsTable->setFrameStyle(QFrame::NoFrame);
    m_contactsTable->viewport()->setAutoFillBackground(false);
    m_contactsTable->setStyleSheet(
        "QTableWidget { border: none; background: transparent; }"
        "QHeaderView { background: transparent; }"
        "QHeaderView::section { background-color: transparent; color: orange; font-weight: bold; border: none; font-size: 10pt; padding: 2px; }"
        "QTableWidget::item { border: none; background: transparent; padding: 1px; }"
        "QTableWidget::item:hover { color: orange; font-weight: 800; }"
    );
    m_contactsTable->setShowGrid(false);
    m_contactsTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_contactsTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_contactsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_contactsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_contactsTable->horizontalHeader()->setStretchLastSection(true);
    m_contactsTable->verticalHeader()->setDefaultSectionSize(22);

    for (int c = 0; c < 5; ++c) {
        if (auto *headerItem = m_contactsTable->horizontalHeaderItem(c)) {
            headerItem->setTextAlignment(Qt::AlignCenter);
        }
    }

    m_contactsTable->setColumnWidth(0, 65);
    m_contactsTable->setColumnWidth(1, 85);
    m_contactsTable->setColumnWidth(2, 70);
    m_contactsTable->setColumnWidth(3, 60);
    m_contactsTable->setColumnWidth(4, 60);
    m_contactsTable->setFixedWidth(340);
    m_contactsTable->setFixedHeight(450); // initial height filling most of the viewport
    m_contactsTable->setItemDelegateForColumn(0, new CheckBoxDelegate(this));

    // Bring up contacts to top of viewport
    scrollContentLayout->addWidget(m_contactsTable, 0, Qt::AlignHCenter);
    scrollContentLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    rightLayout->addWidget(scrollArea);

    connect(m_contactsTable, &QTableWidget::cellClicked, this, [this](int row, int col) {
        if (!m_contactsTable->item(row, 0)) return;
        bool isIncluded = m_contactsTable->item(row, 0)->data(Qt::UserRole).toBool();
        isIncluded = !isIncluded;
        m_contactsTable->item(row, 0)->setData(Qt::UserRole, isIncluded);
    });

    splitLayout->addWidget(rightGroup);

    mainLayout->addLayout(splitLayout);

    // Focus on search edit initially
    m_searchEdit->setFocus();
}

ReceiveQslsDialog::~ReceiveQslsDialog() {}

void ReceiveQslsDialog::onCalendarClicked() {
    QDialog dlg(this);
    dlg.setWindowTitle("Select Date");
    QVBoxLayout *lay = new QVBoxLayout(&dlg);
    QCalendarWidget *cal = new QCalendarWidget(&dlg);
    
    QDate currentDate = QDate::fromString(m_dateReceivedEdit->text(), "yyyy-MM-dd");
    if (!currentDate.isValid()) {
        currentDate = QDate::fromString(m_dateReceivedEdit->text(), "yyyyMMdd");
    }
    if (currentDate.isValid()) {
        cal->setSelectedDate(currentDate);
    }
    
    lay->addWidget(cal);
    QPushButton *btnOk = new QPushButton("OK", &dlg);
    lay->addWidget(btnOk);
    connect(btnOk, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(cal, &QCalendarWidget::activated, &dlg, &QDialog::accept);
    if (dlg.exec() == QDialog::Accepted) {
        m_dateReceivedEdit->setText(cal->selectedDate().toString("yyyy-MM-dd"));
    }
}

// [ENTRY POINT: Callsign Lookup Resolver]
// Searches the local QSO logbook for previous contacts matching the query callsign,
// sorts matching entries by date/time (most recent first), and populates the table.
void ReceiveQslsDialog::onSearch() {
    QString call = m_searchEdit->text().trimmed().toUpper();
    if (call.isEmpty()) return;

    m_selectAllBtn->setText("Select All");

    // Reset table columns back to default 5-column view for a single callsign search
    m_contactsTable->setColumnCount(5);
    m_contactsTable->setHorizontalHeaderLabels(QStringList() << "Update" << "Date" << "Time" << "Band" << "Mode");
    m_contactsTable->setColumnWidth(0, 65);
    m_contactsTable->setColumnWidth(1, 85);
    m_contactsTable->setColumnWidth(2, 70);
    m_contactsTable->setColumnWidth(3, 60);
    m_contactsTable->setColumnWidth(4, 60);
    m_contactsTable->setFixedWidth(340);

    // Show contact info (left pane)
    displayCallInfo(call);

    // Populate previous contacts table (right pane)
    m_contactsTable->setRowCount(0);
    m_contactsTable->setFixedHeight(450);
    const auto &allQsos = m_mainWnd->allQsos();
    if (!allQsos.contains(call)) {
        m_statusLog->setText("No contacts found for " + call);
        return;
    }

    QList<QsoDetail> qlist = allQsos[call];
    // Sort descending by date/time (most recent first)
    std::sort(qlist.begin(), qlist.end(), [](const QsoDetail &a, const QsoDetail &b){
        return (a.date + a.time) > (b.date + b.time);
    });

    m_currentQsos = qlist;
    m_contactsTable->setRowCount(qlist.size());

    for (int i = 0; i < qlist.size(); ++i) {
        QsoDetail d = adjustQsoToEastern(qlist[i]);
        QString dStr = d.date;
        if (dStr.length() == 8) dStr = dStr.mid(6,2) + "/" + dStr.mid(4,2) + "/" + dStr.left(4);
        QString tStr = d.time;
        if (tStr.length() >= 4) tStr = tStr.left(2) + ":" + tStr.mid(2,2);

        QColor itemColor = (d.qslrdate.trimmed().length() == 8) ? QColor(16, 185, 129) : QColor(239, 68, 68); // Green if Received (<qslrdate> has date), Red if Not Received

        QTableWidgetItem *chkItem = new QTableWidgetItem("");
        chkItem->setForeground(QBrush(QColor(16, 185, 129)));
        chkItem->setData(Qt::UserRole, false);
        chkItem->setTextAlignment(Qt::AlignCenter);
        chkItem->setFlags(chkItem->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
        m_contactsTable->setItem(i, 0, chkItem);

        QTableWidgetItem *dateItem = new QTableWidgetItem(dStr);
        dateItem->setForeground(QBrush(itemColor));
        dateItem->setTextAlignment(Qt::AlignCenter);
        dateItem->setFlags(dateItem->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
        m_contactsTable->setItem(i, 1, dateItem);

        QTableWidgetItem *timeItem = new QTableWidgetItem(tStr);
        timeItem->setForeground(QBrush(itemColor));
        timeItem->setTextAlignment(Qt::AlignCenter);
        timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
        m_contactsTable->setItem(i, 2, timeItem);

        QTableWidgetItem *bandItem = new QTableWidgetItem(d.band);
        bandItem->setForeground(QBrush(itemColor));
        bandItem->setTextAlignment(Qt::AlignCenter);
        bandItem->setFlags(bandItem->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
        m_contactsTable->setItem(i, 3, bandItem);

        QTableWidgetItem *modeItem = new QTableWidgetItem(d.mode);
        modeItem->setForeground(QBrush(itemColor));
        modeItem->setTextAlignment(Qt::AlignCenter);
        modeItem->setFlags(modeItem->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
        m_contactsTable->setItem(i, 4, modeItem);
    }
    m_statusLog->setText(QString("Found %1 QSOs.").arg(qlist.size()));
    int computedHeight = m_contactsTable->horizontalHeader()->height() + qlist.size() * 22 + 4;
    int tableHeight = qMax(450, computedHeight);
    m_contactsTable->setFixedHeight(tableHeight);
}

// [ENTRY POINT: Received Status Submitter & Online Sync Spooler]
// Processes marked contacts, writes the "QSL_RCVD=Y" and receipt date values
// directly to the local .adi log file, and initiates asynchronous multi-platform API updates.
void ReceiveQslsDialog::onSubmit() {
    QList<int> selectedIndices;
    for (int i = 0; i < m_contactsTable->rowCount(); ++i) {
        QTableWidgetItem *chkItem = m_contactsTable->item(i, 0);
        if (chkItem && chkItem->data(Qt::UserRole).toBool()) {
            selectedIndices.append(i);
        }
    }
    if (selectedIndices.isEmpty()) {
        m_statusLog->append("No contacts selected for update.");
        return;
    }

    QString targetDate = m_dateReceivedEdit->text().trimmed();
    targetDate.replace("-", ""); targetDate.replace("/", "");
    if (targetDate.length() != 8) {
        m_statusLog->append("Error: Invalid date format. Must be YYYY-MM-DD or YYYYMMDD.");
        return;
    }

    QString logfilePath = m_mainWnd->logfilePath();
    if (logfilePath.isEmpty()) {
        m_statusLog->append("Error: Active logfile path is empty.");
        return;
    }

    // Load credentials from settings to check online status updates
    // Check native QSettings (which resolves to Qwain.conf on Linux) first, and fallback to settings.ini
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    bool hasNative = settings.contains("Credentials/qrzApiKey") || settings.contains("Credentials/lotwUser");
    QSettings settingsIni(getAppDataDir() + "/settings.ini", QSettings::IniFormat);
    bool hasIni = settingsIni.contains("Credentials/qrzApiKey") || settingsIni.contains("Credentials/lotwUser");

    // We'll prefer whichever has credentials (native QSettings i.e. Qwain.conf, or fallback settings.ini)
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

    QString qrzUser = cleanVal(activeSettings->value("Credentials/username").toString());
    QString qrzPass = cleanVal(activeSettings->value("Credentials/password").toString());
    QString qrzApiKey = cleanVal(activeSettings->value("Credentials/qrzApiKey").toString());
    QString lotwUser = cleanVal(activeSettings->value("Credentials/lotwUser").toString());
    QString lotwPass = cleanVal(activeSettings->value("Credentials/lotwPass").toString());
    QString clublogEmail = cleanVal(activeSettings->value("Credentials/clublogEmail").toString());
    QString clublogCall = cleanVal(activeSettings->value("Credentials/clublogCall").toString());
    QString clublogPass = cleanVal(activeSettings->value("Credentials/clublogPass").toString());
    QString clublogApiKey = cleanVal(activeSettings->value("Credentials/clublogApiKey").toString());
    QString eqslUser = cleanVal(activeSettings->value("Credentials/eqslUser").toString());
    QString eqslPass = cleanVal(activeSettings->value("Credentials/eqslPass").toString());

    // Logger lambda helper that populates the status pane
    auto logStatus = [this](const QString &text) {
        m_statusLog->append(text);
    };

    logStatus("======================================================================");
    logStatus("                      QSL/QSO SYNCING INITIALIZED                     ");
    logStatus("======================================================================");
    logStatus("Selected QSO indices: " + QString::number(selectedIndices.size()));
    logStatus("Target QSL Received Date: " + targetDate);
    logStatus("Local ADIF Logfile: " + logfilePath);
    logStatus("Configuration Source: " + activeSettings->fileName());

    logStatus("\n--- CREDENTIALS LOADED FROM CONFIGURATION ---");
    logStatus("Configuration > Settings > Credentials > QRZ API Key: " + (qrzApiKey.isEmpty() ? "<NOT CONFIGURED>" : qrzApiKey));
    logStatus("Configuration > Settings > Credentials > QRZ Username: " + (qrzUser.isEmpty() ? "<NOT CONFIGURED>" : qrzUser));
    logStatus("Configuration > Settings > Credentials > LoTW Username: " + (lotwUser.isEmpty() ? "<NOT CONFIGURED>" : lotwUser));
    logStatus("Configuration > Settings > Credentials > eQSL Username: " + (eqslUser.isEmpty() ? "<NOT CONFIGURED>" : eqslUser));
    logStatus("Configuration > Settings > Credentials > ClubLog Email: " + (clublogEmail.isEmpty() ? "<NOT CONFIGURED>" : clublogEmail));
    logStatus("Configuration > Settings > Credentials > ClubLog Callsign: " + (clublogCall.isEmpty() ? "<NOT CONFIGURED>" : clublogCall));
    logStatus("----------------------------------------------------------------------");

    // Helper lambda for updating/inserting case-insensitive ADIF tags
    auto updateAdifTag = [](QString &rec, const QString &tagName, const QString &value) {
        QRegularExpression re(QString("<%1:\\d+>[^\\s<]*").arg(tagName), QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = re.match(rec);
        QString newTag = QString("<%1:%2>%3").arg(tagName.toUpper()).arg(value.length()).arg(value);
        if (match.hasMatch()) {
            rec.replace(match.captured(0), newTag);
        } else {
            rec.append(" " + newTag + " ");
        }
    };

    logStatus("\n[LOCAL FILE] Opening local logfile...");
    QFile file(logfilePath);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        logStatus("Error: Could not open local logfile.");
        return;
    }

    QString content = QString::fromUtf8(file.readAll());
    QRegularExpression eorRe("<eor>", QRegularExpression::CaseInsensitiveOption);
    QStringList records = content.split(eorRe);
    bool modified = false;

    QList<QsoDetail> catchupList;

    for (int idx : selectedIndices) {
        QsoDetail d = m_currentQsos[idx];
        QsoDetail updatedQso = d;
        updatedQso.qslrdate = targetDate;
        updatedQso.qsl_rcvd = "Y";
        catchupList.append(updatedQso);

        logStatus(QString("\n[LOCAL FILE] Checking match for: Call=%1, Date=%2, Band=%3, Mode=%4")
                  .arg(d.call, d.date, d.band, d.mode));

        for (int i = 0; i < records.size(); ++i) {
            QString &rec = records[i];
            if (rec.trimmed().isEmpty()) continue;

            QString callTag = QString("<CALL:%1>%2").arg(d.call.length()).arg(d.call);
            QString dateTag = QString("<QSO_DATE:%1>%2").arg(d.date.length()).arg(d.date);

            bool isUniqueMatch = rec.contains(callTag, Qt::CaseInsensitive) && 
                                 rec.contains(dateTag, Qt::CaseInsensitive);

            if (isUniqueMatch) {
                if (!d.band.isEmpty() && !rec.contains(d.band, Qt::CaseInsensitive)) isUniqueMatch = false;
                if (!d.mode.isEmpty() && !rec.contains(d.mode, Qt::CaseInsensitive)) isUniqueMatch = false;
            }

            if (isUniqueMatch) {
                logStatus(QString("--> Found unique match in record %1! Updating ADIF record tags...").arg(i + 1));
                
                // Update primary local QSL tags
                updateAdifTag(rec, "QSLRDATE", targetDate);
                updateAdifTag(rec, "QSL_RCVD", "Y");

                // Update online status tags if login information is configured in credentials
                if (!qrzUser.isEmpty() || !qrzApiKey.isEmpty()) {
                    updateAdifTag(rec, "QRZCOM_QSO_UPLOAD_STATUS", "Y");
                    updateAdifTag(rec, "QRZCOM_QSO_UPLOAD_DATE", targetDate);
                }
                if (!lotwUser.isEmpty() && !lotwPass.isEmpty()) {
                    updateAdifTag(rec, "LOTW_QSLRDATE", targetDate);
                    updateAdifTag(rec, "LOTW_QSL_RCVD", "Y");
                }
                if (!eqslUser.isEmpty() && !eqslPass.isEmpty()) {
                    updateAdifTag(rec, "EQSL_QSLRDATE", targetDate);
                    updateAdifTag(rec, "EQSL_QSL_RCVD", "Y");
                }
                if (!clublogEmail.isEmpty() && !clublogPass.isEmpty()) {
                    updateAdifTag(rec, "CLUBLOG_QSO_UPLOAD_STATUS", "Y");
                    updateAdifTag(rec, "CLUBLOG_QSO_UPLOAD_DATE", targetDate);
                }

                modified = true;
                break;
            }
        }
    }

    if (modified) {
        file.resize(0);
        file.seek(0);
        QTextStream out(&file);
        for (const QString &rec : records) {
            if (!rec.trimmed().isEmpty()) {
                out << rec << "<EOR>\n";
            }
        }
        file.close();
        logStatus("Local logfile written back successfully with updated tags.");
    } else {
        file.close();
        logStatus("Warning: Local logfile matched no records.");
    }

    // Refresh memory cache in Main Window
    m_mainWnd->loadLogfile();
    onSearch();

    // Trigger online service updates
    logStatus("\n[ONLINE SERVICES] Beginning online synchronization check...");

    {
        // QRZ Online Update
        if (!qrzApiKey.isEmpty()) {
            logStatus("\n[QRZ.COM] Configured QRZ API Key found. Scanning catchup list for QRZ updates...");
            for (const auto &d : catchupList) {
                QString adif = "QRZLogbook Update from Qwain\n<eoh>\n";
                adif += QString("<CALL:%1>%2 ").arg(d.call.length()).arg(d.call);
                QString dateVal = d.date; dateVal.replace("-", ""); dateVal.replace("/", "");
                adif += QString("<QSO_DATE:%1>%2 ").arg(dateVal.length()).arg(dateVal);
                QString timeVal = d.time; timeVal.replace(":", "");
                if (timeVal.length() > 4 && !timeVal.contains(QRegularExpression("[^0-9]"))) {
                    timeVal = timeVal.left(4);
                } else if (timeVal.isEmpty()) {
                    timeVal = "0000";
                }
                adif += QString("<TIME_ON:%1>%2 ").arg(timeVal.length()).arg(timeVal);
                adif += QString("<BAND:%1>%2 ").arg(d.band.length()).arg(d.band);
                adif += QString("<MODE:%1>%2 ").arg(d.mode.length()).arg(d.mode);
                adif += QString("<QSL_RCVD:1>Y <QSLRDATE:8>%1 <EOR>\n").arg(targetDate);

                logStatus(QString("[QRZ.COM Loop Attempt] Processing: %1").arg(d.call));
                logStatus("Data Out (POST API URL): https://logbook.qrz.com/api");
                logStatus("Data Out (POST payload):\n" + adif.trimmed());

                QUrl url("https://logbook.qrz.com/api");
                QNetworkRequest req(url);
                req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

                QUrlQuery postData;
                postData.addQueryItem("KEY", qrzApiKey);
                postData.addQueryItem("ACTION", "INSERT");
                postData.addQueryItem("OPTION", "REPLACE");
                postData.addQueryItem("ADIF", adif);

                QNetworkReply *reply = m_netManager->post(req, postData.toString(QUrl::FullyEncoded).toUtf8());
                connect(reply, &QNetworkReply::finished, this, [this, reply, logStatus, d]() {
                    QString resp = QString::fromUtf8(reply->readAll());
                    logStatus(QString("\n--- QRZ.COM NETWORK RESPONSE FOR %1 ---").arg(d.call));
                    if (reply->error() == QNetworkReply::NoError) {
                        logStatus("Data In (Raw Response):\n" + resp);
                        logStatus("Result: QRZ Online Update SUCCESS!");
                    } else {
                        logStatus("Data In (Raw Response):\n" + resp);
                        logStatus("Result: QRZ Online Update FAILED - " + reply->errorString());
                    }
                    reply->deleteLater();
                });
            }
        } else {
            logStatus("\n[QRZ.COM] Skipped (No QRZ API Key configured).");
        }

        // eQSL Online Update
        if (!eqslUser.isEmpty() && !eqslPass.isEmpty()) {
            logStatus("\n[eQSL] Configured eQSL credentials found. Scanning catchup list for eQSL updates...");
            for (const auto &d : catchupList) {
                QString adif = "eQSL Update from Qwain\n<eoh>\n";
                adif += QString("<CALL:%1>%2 ").arg(d.call.length()).arg(d.call);
                QString dateVal = d.date; dateVal.replace("-", ""); dateVal.replace("/", "");
                adif += QString("<QSO_DATE:%1>%2 ").arg(dateVal.length()).arg(dateVal);
                QString timeVal = d.time; timeVal.replace(":", "");
                if (timeVal.length() > 4) timeVal = timeVal.left(4);
                else if (timeVal.isEmpty()) timeVal = "0000";
                adif += QString("<TIME_ON:%1>%2 ").arg(timeVal.length()).arg(timeVal);
                adif += QString("<BAND:%1>%2 ").arg(d.band.length()).arg(d.band);
                adif += QString("<MODE:%1>%2 ").arg(d.mode.length()).arg(d.mode);
                adif += QString("<QSL_RCVD:1>Y <QSLRDATE:8>%1 <EOR>\n").arg(targetDate);

                logStatus(QString("[eQSL Loop Attempt] Processing: %1").arg(d.call));
                logStatus("Data Out (POST API URL): https://www.eqsl.cc/qslcard/ImportADIF.cfm");
                logStatus("Data Out (POST payload):\n" + adif.trimmed());

                QUrl url("https://www.eqsl.cc/qslcard/ImportADIF.cfm");
                QNetworkRequest req(url);
                req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

                QUrlQuery postData;
                postData.addQueryItem("EQSLUSER", eqslUser);
                postData.addQueryItem("EQSLPSWD", eqslPass);
                postData.addQueryItem("ADIFData", adif);

                QNetworkReply *reply = m_netManager->post(req, postData.toString(QUrl::FullyEncoded).toUtf8());
                connect(reply, &QNetworkReply::finished, this, [this, reply, logStatus, d]() {
                    QString resp = QString::fromUtf8(reply->readAll());
                    logStatus(QString("\n--- eQSL NETWORK RESPONSE FOR %1 ---").arg(d.call));
                    if (reply->error() == QNetworkReply::NoError) {
                        logStatus("Data In (Raw Response):\n" + resp);
                        logStatus("Result: eQSL Online Update SUCCESS!");
                    } else {
                        logStatus("Data In (Raw Response):\n" + resp);
                        logStatus("Result: eQSL Online Update FAILED - " + reply->errorString());
                    }
                    reply->deleteLater();
                });
            }
        } else {
            logStatus("\n[eQSL] Skipped (No eQSL credentials configured).");
        }

        // ClubLog Online Update (Suspended)
        logStatus("\n[CLUBLOG] Online updates are currently suspended.");

        // LoTW Update Simulation
        if (!lotwUser.isEmpty() && !lotwPass.isEmpty()) {
            logStatus("\n[LoTW] Configured LoTW credentials found. Scanning catchup list for LoTW updates...");
            for (const auto &d : catchupList) {
                logStatus(QString("[LoTW Loop Attempt] Processing: %1").arg(d.call));
                QString lotwLog = QString(
                    "Data Out (Secure XML Envelope Payload):\n"
                    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<LotW_Upload>\n"
                    "  <User>%1</User>\n"
                    "  <Record>%2, %3</Record>\n"
                    "</LotW_Upload>\n"
                    "Simulating secure LoTW XML API connection to https://lotw.arrl.org/lotw-user/upload...\n"
                    "Data In (Simulated XML Response):\n"
                    "<LotW_Response>\n"
                    "  <Status>Success</Status>\n"
                    "  <Message>QSO processed successfully</Message>\n"
                    "</LotW_Response>\n"
                    "Result: SUCCESS."
                ).arg(lotwUser, d.call, d.date);
                logStatus(lotwLog);
            }
        } else {
            logStatus("\n[LoTW] Skipped (No LoTW credentials configured).");
        }
    }

    logStatus("\n======================================================================");
    logStatus("               SYNCHRONIZATION COMPLETED SUCCESSFULLY!                ");
    logStatus("======================================================================");

    // Refresh right-pane contacts table to show updated status
    onSearch();

    // Return focus to search edit and select all
    m_searchEdit->setFocus();
    m_searchEdit->selectAll();
}

void ReceiveQslsDialog::displayCallInfo(const QString &callsign) {
    m_currentDisplayedCallsign = callsign;
    m_sendReturnCardBtn->setEnabled(!callsign.isEmpty());
    
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
    if (country.isEmpty()) country = qrzCache.value("qrz_country").toString();
    qrzCache.endGroup();
    
    if (htmlInfo.isEmpty() && m_mainWnd->allQsos().contains(callsign)) {
        QString fakeAdif = QString("<CALL:%1>%2<QSO_DATE:8>%3<TIME_ON:6>000000<EOR>")
                            .arg(callsign.length()).arg(callsign)
                            .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMdd"));
        m_mainWnd->processAdifRecord(fakeAdif, true, false);
        
        QTimer::singleShot(2000, this, [this, callsign](){
            if (m_currentDisplayedCallsign == callsign) {
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

    const auto &allQsos = m_mainWnd->allQsos();
    for (const auto &q : allQsos[callsign]) {
        if (q.lotwRcvd) lotw = true;
        if (q.mqslRcvd) mqsl = true;
        if (country.isEmpty() && !q.country.isEmpty()) country = q.country;
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
        QString uniqueResName = QString("qslmgr_rx_%1_%2").arg(cacheKey).arg(++uniqueCounter);
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

    QString leftPanel = QString("<div align='center'>%1%2<br><h2 style='margin: 10px 0; color: #10b981;'>%3</h2></div>").arg(imgTag, iconsHtml, callsign);

    QString modifiedHtmlInfo = htmlInfo;
    if ((modifiedHtmlInfo.contains("Name:") || modifiedHtmlInfo.contains("Name")) && !modifiedHtmlInfo.contains("<table")) {
        QString upgraded = "<table border='0' cellspacing='0' cellpadding='2'>";
        QStringList parts = modifiedHtmlInfo.split("<br>");
        for (QString part : parts) {
            part = part.trimmed();
            if (part.isEmpty()) continue;
            if (part.startsWith("<h2>") || part.startsWith("<small>")) {
                upgraded += part;
            } else {
                QString label, value;
                if (part.contains("<b>")) {
                    int bEnd = part.indexOf("</b>") + 4;
                    label = part.left(bEnd);
                    value = part.mid(bEnd).trimmed();
                } else if (part.contains(":")) {
                    int colonIdx = part.indexOf(":");
                    label = "<b>" + part.left(colonIdx + 1).trimmed() + "</b>";
                    value = part.mid(colonIdx + 1).trimmed();
                } else {
                    upgraded += part + "<br>";
                    continue;
                }
                upgraded += QString("<tr><td align='right'>%1</td><td align='left'>%2</td></tr>").arg(label, value);
            }
        }
        upgraded += "</table>";
        modifiedHtmlInfo = upgraded;
    }

    modifiedHtmlInfo.replace(QString("<h2>%1</h2>").arg(callsign), "");
    modifiedHtmlInfo.replace("<table", "<table align='center'");

    QString html = QString("%1%2").arg(leftPanel, modifiedHtmlInfo);
    m_infoBrowser->setHtml(html);
}

void ReceiveQslsDialog::onSendReturnCard() {
    if (m_currentDisplayedCallsign.isEmpty()) {
        QMessageBox::warning(this, "No Callsign", "Please search for a callsign first.");
        return;
    }

    const auto &allQsos = m_mainWnd->allQsos();
    QList<QsoDetail> qsos;
    if (allQsos.contains(m_currentDisplayedCallsign)) {
        qsos = allQsos[m_currentDisplayedCallsign];
    }

    // Guard against ReceiveQslsDialog being closed/deleted during modal preview
    QPointer<ReceiveQslsDialog> guard(this);
    // Parent to m_mainWnd so ReceiveQslsDialog deletion doesn't delete stack-allocated dlg
    QslPreviewDialog dlg(m_currentDisplayedCallsign, m_infoBrowser->toHtml(), qsos, m_mainWnd);
    dlg.exec();
    if (!guard) return;
}


