// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef PATHS_H
#define PATHS_H

#include <QString>
#include <QSettings>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

inline QString sanitizeHtmlInfo(const QString &html) {
    if (html.isEmpty()) return html;

    // Pattern options for regex to match across newlines and be case-insensitive
    QRegularExpression::PatternOptions options = QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption;

    // First, check if there's a structured table containing contact details
    int idx = 0;
    while ((idx = html.indexOf("<table", idx, Qt::CaseInsensitive)) != -1) {
        int tableEnd = html.indexOf("</table>", idx, Qt::CaseInsensitive);
        if (tableEnd != -1) {
            QString tableSub = html.mid(idx, tableEnd + 8 - idx);
            // Check if this table contains Name, Address, City
            if (tableSub.contains("Name:", Qt::CaseInsensitive) || 
                tableSub.contains("Address:", Qt::CaseInsensitive) || 
                tableSub.contains("City:", Qt::CaseInsensitive) ||
                tableSub.contains("Name</b>", Qt::CaseInsensitive) ||
                tableSub.contains("Address</b>", Qt::CaseInsensitive)) {
                
                // Found the core contact table!
                // Let's grab the preceding callsign header (usually h2) if it's nearby
                QString header;
                int h2Start = html.lastIndexOf("<h2", idx, Qt::CaseInsensitive);
                if (h2Start != -1 && idx - h2Start < 150) {
                    int h2End = html.indexOf("</h2>", h2Start, Qt::CaseInsensitive);
                    if (h2End != -1 && h2End < idx) {
                        header = html.mid(h2Start, h2End + 5 - h2Start) + "<br>";
                    }
                }
                
                // Let's check for any trailing Grid info right after the table
                QString trailing;
                int gridIdx = html.indexOf("<b>Grid:</b>", tableEnd + 8, Qt::CaseInsensitive);
                if (gridIdx != -1 && gridIdx - (tableEnd + 8) < 150) {
                    int nextBr = html.indexOf("<br>", gridIdx, Qt::CaseInsensitive);
                    if (nextBr != -1) {
                        trailing = html.mid(gridIdx, nextBr - gridIdx);
                    } else {
                        trailing = html.mid(gridIdx);
                    }
                    trailing = "<br>" + trailing;
                }
                
                return header + tableSub + trailing;
            }
        }
        idx += 6;
    }
    
    // Fallback: If no structured table is found, strip known bad patterns
    QString clean = html;
    clean.remove(QRegularExpression("<table[^>]*>.*?qsl_queue.*?</table>", options));
    clean.remove(QRegularExpression("<table[^>]*>.*?print_now.*?</table>", options));
    clean.remove(QRegularExpression("<table[^>]*>.*?qsl_paper.*?</table>", options));
    clean.remove(QRegularExpression("<table[^>]*>.*?qso_catchup.*?</table>", options));
    clean.remove(QRegularExpression("<h3[^>]*>Past Contacts</h3>.*$", options));
    clean.remove(QRegularExpression("<div[^>]*>.*?QSL\\s+Sent.*?</div>", options));
    
    clean = clean.trimmed();
    return clean;
}

inline QString getAppDataDir() {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString path = QFileInfo(settings.fileName()).absolutePath();
    QDir().mkpath(path);
    return path;
}

inline QString getAppVersion() {
    QString ver = "1.02";
    QFile file("package.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject() && doc.object().contains("version")) {
            ver = doc.object().value("version").toString();
        }
    }
    return ver;
}

#endif // PATHS_H
