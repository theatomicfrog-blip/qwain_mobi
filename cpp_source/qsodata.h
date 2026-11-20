// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef QSODATA_H
#define QSODATA_H

#include <QString>
#include <QMap>
#include <QList>
#include <QDateTime>
#include <QDate>
#include <QTime>
#include <QTimeZone>
#include <QSettings>
#include <QCoreApplication>

struct QsoDetail {
    QString call;
    QString date;
    QString time;
    QString band;
    QString freq;
    QString mode;
    QString myGrid;
    QString theirGrid;
    QString grid;
    bool lotwRcvd = false;
    bool mqslRcvd = false;
    bool cardRcvd = false;
    bool qrzRcvd = false;
    QString pota;
    QString name;
    QString address;
    QString addr2;
    QString city;
    QString state;
    QString zip;
    QString county;
    QString country;
    QString dxcc;
    QString lat;
    QString lon;
    QString rst_rcvd;
    QString rst_sent;
    QString tx_pwr;
    QString qslsdate;
    QString qslrdate;
    QString qsl_sent;
    QString qsl_rcvd;
    QString qslVia;
    QString qslMsg;
    QString notes;
};

inline QsoDetail adjustQsoToEastern(const QsoDetail &qso) {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    if (!settings.value("UI/alwaysEastern", false).toBool()) {
        return qso;
    }

    QTimeZone easternTz("America/New_York");
    if (!easternTz.isValid()) {
        return qso;
    }

    QString timeStr = qso.time;
    if (timeStr.length() < 6) {
        timeStr = timeStr.leftJustified(6, '0');
    }

    QDate qDate = QDate::fromString(qso.date, "yyyyMMdd");
    QTime qTime = QTime::fromString(timeStr.left(6), "hhmmss");
    if (!qDate.isValid() || !qTime.isValid()) {
        return qso;
    }

    QDateTime utcDt(qDate, qTime, Qt::UTC);
    QDateTime easternDt = utcDt.toTimeZone(easternTz);

    QsoDetail adjusted = qso;
    adjusted.date = easternDt.toString("yyyyMMdd");
    adjusted.time = easternDt.toString("hhmmss");
    return adjusted;
}

typedef QMap<QString, QMap<QString, QList<QsoDetail>>> GridQsoMap;

#endif // QSODATA_H
