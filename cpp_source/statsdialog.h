#ifndef STATSDIALOG_H
#define STATSDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTextEdit>
#include <QFrame>
#include <QSettings>
#include <QCoreApplication>
#include <QDateTime>
#include <QDate>
#include <QApplication>
#include <QClipboard>
#include <QScrollArea>
#include <QComboBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QMouseEvent>
#include <QMap>
#include <QString>
#include <QList>
#include <QFontDatabase>
#include "qsodata.h"

class MainWindow;

// ============================================================================
// DATA STRUCTURE: DayStats
// Tracks daily aggregate metrics: contacts made, cards printed, cards received,
// with breakdown for domestic and international cards, plus detailed QSO lists.
// ============================================================================
struct DayStats {
    int contactsMade = 0;
    int contactsDomestic = 0;
    int contactsIntl = 0;

    int cardsPrinted = 0;
    int cardsPrintedDomestic = 0;
    int cardsPrintedIntl = 0;

    int cardsReceived = 0;
    int cardsReceivedDomestic = 0;
    int cardsReceivedIntl = 0;

    QList<QsoDetail> qsosMade;
    QList<QsoDetail> qslsPrinted;
    QList<QsoDetail> qslsReceived;
};

// ============================================================================
// WIDGET: DayBoxWidget
// Displays the requested day cell format:
//     day number
//  |---------------|
//  |Contacts  NN1  |
//  |    made       |
//  |               |
//  |Cards     NN2  |
//  |Printed        |
//  |               |
//  |Cards     NN3  |
//  |Received       |
//  |---------------|
// ============================================================================
class DayBoxWidget : public QFrame {
    Q_OBJECT
public:
    DayBoxWidget(const QDate &date, const DayStats &stats, bool isCurrentMonth, bool isToday, QWidget *parent = nullptr);

signals:
    void dayClicked(const QDate &date);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QDate m_date;
    DayStats m_stats;
};

// ============================================================================
// DIALOG: DayDetailDialog
// Displays detailed breakdown report for a selected day (Contacts made, Cards
// printed, Cards received with Domestic/International split and QSO log list).
// ============================================================================
class DayDetailDialog : public QDialog {
    Q_OBJECT
public:
    DayDetailDialog(const QDate &date, const DayStats &stats, QWidget *parent = nullptr);
    QString selectedCallsign() const { return m_selectedCallsign; }

private slots:
    void onCopyDayReport();
    void onCellDoubleClicked(int row, int column);

private:
    QDate m_date;
    DayStats m_stats;
    QLabel *m_statusLabel = nullptr;
    QTableWidget *m_table = nullptr;
    QString m_selectedCallsign;
};

// ============================================================================
// DIALOG: StatsDialog
// Full statistics dialog with side-by-side station/band stats on the left
// and interactive scrollable activity calendar on the right.
// ============================================================================
class StatsDialog : public QDialog {
    Q_OBJECT
public:
    explicit StatsDialog(MainWindow *mainWin, QWidget *parent = nullptr);

private slots:
    void onCopyToClipboard();
    void onPrevMonth();
    void onNextMonth();
    void onPrevYear();
    void onNextYear();
    void onMonthComboChanged(int index);
    void onYearSpinChanged(int year);
    void onTodayClicked();
    void openDayDetail(const QDate &date);

protected:
    void wheelEvent(QWheelEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void calculateStats(MainWindow *mainWin);
    QString buildStatsText() const;
    QString buildStatsHtml() const;
    QString formatUptime(quint64 totalSecs) const;
    void updateCalendarView();

    QString m_version;
    quint64 m_totalUptimeSecs = 0;
    quint64 m_sessionUptimeSecs = 0;

    int m_qsosToday = 0;
    int m_qsosOverall = 0;

    QMap<QString, int> m_bandCounts;

    int m_templatesPrinted = 0;

    int m_domesticContacts = 0;
    int m_domesticSent = 0;
    int m_domesticRcvd = 0;

    int m_intlContacts = 0;
    int m_intlSent = 0;
    int m_intlRcvd = 0;

    // Daily statistics cache
    QMap<QDate, DayStats> m_dailyStats;
    int m_selectedYear = 2026;
    int m_selectedMonth = 11;

    // Main window reference for looking up callsigns
    MainWindow *m_mainWin = nullptr;

    QTextEdit *m_statsEdit = nullptr;
    QLabel *m_statusLabel = nullptr;
    QString m_formattedTextCache;

    // Calendar UI widgets
    QComboBox *m_monthCombo = nullptr;
    QSpinBox *m_yearSpin = nullptr;
    QLabel *m_monthSummaryLabel = nullptr;
    QScrollArea *m_calendarScrollArea = nullptr;
    QWidget *m_calendarGridContainer = nullptr;
    QGridLayout *m_calendarGrid = nullptr;
};

#endif // STATSDIALOG_H
