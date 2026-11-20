#ifndef LIVESTREAMVIEWER_H
#define LIVESTREAMVIEWER_H

#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QHostAddress>
#include <QUdpSocket>
#include <QTcpSocket>

class MainWindow;

class LiveStreamViewer : public QDialog
{
    Q_OBJECT

public:
    explicit LiveStreamViewer(MainWindow *mainWindow, QWidget *parent = nullptr);
    ~LiveStreamViewer();

private slots:
    void handleDataReceived(const QString &protocol, const QString &senderAddress, int senderPort, const QString &data);
    void sendTestUdp();
    void sendTestTcp();
    void clearLog();
    void exportLog();
    void togglePause();

private:
    MainWindow *m_mainWindow;
    QTextEdit *m_logEdit;
    QLabel *m_statsLabel;
    QPushButton *m_pauseBtn;
    QPushButton *m_testUdpBtn;
    QPushButton *m_testTcpBtn;
    QCheckBox *m_autoScrollChk;

    int m_udpCount;
    int m_tcpCount;
    qint64 m_udpBytes;
    qint64 m_tcpBytes;
    bool m_isPaused;
};

#endif // LIVESTREAMVIEWER_H
