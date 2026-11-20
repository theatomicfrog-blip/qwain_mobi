#include <QtCore>
#include <QDebug>
#include <QRegularExpression>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    QString line = "DX de SZ1A-#:    18080.0  JI2XLN       CW 8 dB 19 WPM CQ              1717Z";
    QRegularExpression rx("^DX de\\s+([A-Za-z0-9\\-\\/#]+)[-\\s:]+\\s*(\\d+(?:\\.\\d+)?)\\s+([A-Za-z0-9\\-\\/]+)\\s*(.*?)\\s*(\\d{4}Z?)$");
    QRegularExpressionMatch match = rx.match(line);

    if (!match.hasMatch()) {
        // Fallback for spots without a time at the end
        rx = QRegularExpression("^DX de\\s+([A-Za-z0-9\\-\\/#]+)[-\\s:]+\\s*(\\d+(?:\\.\\d+)?)\\s+([A-Za-z0-9\\-\\/]+)\\s*(.*)");
        match = rx.match(line);
    }
    
    if (match.hasMatch()) {
        qDebug() << "MATCHED!";
        qDebug() << "Spooter:" << match.captured(1);
        qDebug() << "Freq:" << match.captured(2);
        qDebug() << "Call:" << match.captured(3);
        qDebug() << "Comment:" << match.captured(4);
        qDebug() << "Time:" << match.captured(5);
    } else {
        qDebug() << "NO MATCH";
    }

    return 0;
}
