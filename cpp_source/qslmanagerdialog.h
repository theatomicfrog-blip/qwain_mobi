// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef QSLMANAGERDIALOG_H
#define QSLMANAGERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QTextBrowser>
#include <QTableWidget>
#include <QListWidget>
#include <QLabel>
#include <QMenu>
#include <QContextMenuEvent>
#include "mainwindow.h"
#include "qsodata.h"

class QslManagerDialog : public QDialog {
    Q_OBJECT
public:
    explicit QslManagerDialog(MainWindow *mainWnd, QWidget *parent = nullptr);
    ~QslManagerDialog();

private slots:
    void doLookup();
    void numContactsSearch();
    void filterByDateClicked();
    void countryDxccSearch();
    void showAllQsos();
    void pastWeekClicked();
    void todayClicked();
    void lastXDaysClicked();
    void onResultDoubleClicked(int row, int col);
    void onResultClicked(int row, int col);
    void printQslClicked();
    void loadPrintQueue();
    void savePrintQueue();
    void selectAllResults();
    void onResultsTableContextMenu(const QPoint &pos);
    void onInfoBrowserContextMenu(const QPoint &pos);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void setupUi();
    void displayCallInfo(const QString &callsign);
    void populateResults(const QList<QsoDetail> &results, bool showCount = false, const QMap<QString, int> &counts = QMap<QString, int>(), bool sortByCount = false);

    MainWindow *m_mainWnd;

    QLineEdit *m_callsignLookup;
    QListWidget *m_searchMatchesList = nullptr;
    QCheckBox *m_chkDateRange;
    QToolButton *m_btnDateRange;
    QToolButton *m_btnCountryDxcc;
    
    QToolButton *m_btnPastWeek;
    QToolButton *m_btnToday;
    QToolButton *m_btnLastXDays;
    int m_lastXDaysCount = 30;
    
    QCheckBox *m_chkLotw;
    QCheckBox *m_chkMail;

    QTextBrowser *m_infoBrowser;
    QTableWidget *m_resultsTable;
    
    QLabel *m_lblChosenCallsign;
    QLabel *m_statusLabel;
    QPushButton *m_btnPrintQsl;

    QString m_currentChosenCallsign;
    QString m_currentDisplayedCallsign;
    QList<QsoDetail> m_currentResults;
    QColor m_qsoListColor;
    
    QListWidget *m_printQueueList;
    QString m_lastSearchMode;
    QString m_searchStartDate;
    QString m_searchEndDate;
    QDate m_userStartDate;
    QDate m_userEndDate;
    QString m_filterStartDate;
    QString m_filterEndDate;
    bool m_filterActive = false;
    QString m_searchCountryDxcc;
    void executeLastSearch();
    QString getQueueStringForCallsign(const QString &callsign);
    bool m_searchActive = false;
    bool m_isExecuting = false;
    bool m_searchPending = false;
    friend struct SearchGuard;

private slots:
    void onSyncClublogOqrs();

private:
    void processOqrsAdif(const QString &adif);
    QPushButton *m_syncClublogBtn = nullptr;
};

#endif
