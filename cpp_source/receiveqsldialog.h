// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

#ifndef RECEIVEQSLDIALOG_H
#define RECEIVEQSLDIALOG_H

#include <QDialog>
#include <QString>
#include <QList>
#include <QDate>
#include <QNetworkAccessManager>
#include "qsodata.h"

class QLineEdit;
class QSlider;
class QLabel;
class QTextBrowser;
class QTableWidget;
class QPushButton;
class QToolButton;
class QCheckBox;
class QTextEdit;
class MainWindow;

class ReceiveQslsDialog : public QDialog {
    Q_OBJECT
public:
    explicit ReceiveQslsDialog(QWidget *parent, MainWindow *mainWnd);
    ~ReceiveQslsDialog();

private slots:
    void onSearch();
    void onCalendarClicked();
    void onSubmit();
    void onSendReturnCard();

private:
    void displayCallInfo(const QString &callsign);

    MainWindow *m_mainWnd;

    QLineEdit *m_searchEdit;

    QTextBrowser *m_infoBrowser;
    
    QTableWidget *m_contactsTable;
    QLabel *m_legendLabel;
    QPushButton *m_selectAllBtn;
    QLineEdit *m_dateReceivedEdit;
    QToolButton *m_calendarBtn;
    QPushButton *m_submitBtn;
    QPushButton *m_sendReturnCardBtn;
    QTextEdit *m_statusLog;

    QString m_currentDisplayedCallsign;
    QList<QsoDetail> m_currentQsos;
    QNetworkAccessManager *m_netManager;
};

#endif // RECEIVEQSLDIALOG_H
