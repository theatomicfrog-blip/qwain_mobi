// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef CACHEVIEWERDIALOG_H
#define CACHEVIEWERDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTextBrowser>

class QLineEdit;

class CacheViewerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CacheViewerDialog(QWidget *parent = nullptr);

private slots:
    void onCallsignSelected(const QString &callsign);
    void onDeleteClicked();
    void onRefreshClicked();
    void onEditClicked();
    void onAnchorClicked(const QUrl &url);
    void onRebuildClicked();
    void onRefreshDataClicked();
    void onSelectionChanged();

private:
    void populateList();
    
    QListWidget *m_listWidget;
    QTextBrowser *m_infoBrowser;
    QLineEdit *m_searchEdit;
};

#endif // CACHEVIEWERDIALOG_H

