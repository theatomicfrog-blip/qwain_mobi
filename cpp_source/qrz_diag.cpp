// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QString>
#include <QDebug>
#include <QTimer>
#include <iostream>
#include <QRegularExpression>

class QrzDiag : public QObject {
    Q_OBJECT
public:
    QrzDiag(QObject *parent = nullptr) : QObject(parent) {
        manager = new QNetworkAccessManager(this);
    }

    void start(const QString &username, const QString &password, const QStringList &calls) {
        this->calls = calls;
        QNetworkRequest req(QUrl(QString("https://xmldata.qrz.com/xml/current/?username=%1;password=%2").arg(username, password)));
        QNetworkReply *reply = manager->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                QString data = reply->readAll();
                QRegularExpression re("<Key>(.*?)<\\/Key>");
                QRegularExpressionMatch match = re.match(data);
                if (match.hasMatch()) {
                    sessionKey = match.captured(1);
                    qDebug() << "Logged in successfully. SessionKey:" << sessionKey;
                    fetchNext();
                } else {
                    qDebug() << "Failed to parse session key.";
                    qApp->quit();
                }
            } else {
                qDebug() << "Login failed:" << reply->errorString();
                qApp->quit();
            }
            reply->deleteLater();
        });
    }

private:
    void fetchNext() {
        if (calls.isEmpty()) {
            qApp->quit();
            return;
        }
        QString call = calls.takeFirst();
        QNetworkRequest req(QUrl(QString("https://xmldata.qrz.com/xml/current/?s=%1;callsign=%2").arg(sessionKey, call)));
        QNetworkReply *reply = manager->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, call]() {
            if (reply->error() == QNetworkReply::NoError) {
                QString xmlData = reply->readAll();
                qDebug() << "\n==== Record for" << call << "====";
                
                // We show regex parsing equivalent
                QRegularExpression mqslRe("<mqsl>(.*?)<\\/mqsl>");
                QRegularExpression lotwRe("<lotw>(.*?)<\\/lotw>");
                
                auto mqslMatch = mqslRe.match(xmlData);
                auto lotwMatch = lotwRe.match(xmlData);
                
                QString mqslVal = mqslMatch.hasMatch() ? mqslMatch.captured(1) : "NOT_FOUND";
                QString lotwVal = lotwMatch.hasMatch() ? lotwMatch.captured(1) : "NOT_FOUND";
                
                qDebug() << "Raw mqsl regex extract:" << mqslVal;
                qDebug() << "Raw lotw regex extract:" << lotwVal;
                
                qDebug() << "Differences:";
                if (mqslVal == "1") qDebug() << " -> Accepts Mail QSL (mqsl=1)";
                else qDebug() << " -> Does NOT accept Mail QSL (mqsl=" << mqslVal << ")";
                
                if (lotwVal == "1") qDebug() << " -> Accepts LOTW (lotw=1)";
                else qDebug() << " -> Does NOT accept LOTW (lotw=" << lotwVal << ")";
                
            } else {
                qDebug() << "Failed to fetch" << call;
            }
            reply->deleteLater();
            fetchNext();
        });
    }

    QNetworkAccessManager *manager;
    QString sessionKey;
    QStringList calls;
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    if (argc < 4) {
        qDebug() << "Usage: qrz_diag <username> <password> <call1> [call2 ...]";
        return 1;
    }
    
    QString username = argv[1];
    QString password = argv[2];
    QStringList calls;
    for (int i=3; i<argc; i++) calls.append(argv[i]);
    
    QrzDiag diag;
    diag.start(username, password, calls);
    
    return app.exec();
}

#include "qrz_diag.moc"
