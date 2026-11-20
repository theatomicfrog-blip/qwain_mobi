// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef QSLSTATUSDASHBOARD_H
#define QSLSTATUSDASHBOARD_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include "mainwindow.h"

class QslStatusDashboard : public QDialog {
    Q_OBJECT
public:
    explicit QslStatusDashboard(MainWindow *mainWnd, QWidget *parent = nullptr);

private slots:
    void populateTable();

private:
    MainWindow *m_mainWnd;
    QTableWidget *m_table;
    QCheckBox *m_chkMailOnly;
    QCheckBox *m_chkDirectOnly;
    QLabel *m_statusLbl;
};

#endif // QSLSTATUSDASHBOARD_H
