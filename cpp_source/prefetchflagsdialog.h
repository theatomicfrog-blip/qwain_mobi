// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef PREFETCHFLAGSDIALOG_H
#define PREFETCHFLAGSDIALOG_H

#include <QDialog>
#include <QSet>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QNetworkAccessManager>

class PrefetchFlagsDialog : public QDialog {
    Q_OBJECT
public:
    explicit PrefetchFlagsDialog(QWidget *parent = nullptr);
    ~PrefetchFlagsDialog();

private slots:
    void getSelectedFlags();
    void onDownloadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void updateTableIcons();

private:
    void scanCountries();
    
    QTableWidget *m_table;
    QTextEdit *m_statusText;
    QPushButton *m_btnGetFlags;
    
    QStringList m_countries;
    QMap<QString, QString> m_countryToIso;
    QMap<QString, bool> m_hasCache;
    
    QNetworkAccessManager *m_netManager;
    
    // Download state
    int m_downloadIndex;
    QList<QString> m_pendingCountries;
    qint64 m_totalBytesReceived;
    qint64 m_lastPrintMs;
    qint64 m_currentDownloadStartMs;
};

#endif
