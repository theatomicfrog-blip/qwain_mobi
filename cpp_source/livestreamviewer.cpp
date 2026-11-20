#include "livestreamviewer.h"
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QScrollBar>
#include <QSettings>
#include <QCoreApplication>

LiveStreamViewer::LiveStreamViewer(MainWindow *mainWindow, QWidget *parent)
    : QDialog(parent)
    , m_mainWindow(mainWindow)
    , m_udpCount(0)
    , m_tcpCount(0)
    , m_udpBytes(0)
    , m_tcpBytes(0)
    , m_isPaused(false)
{
    setWindowTitle(tr("Network Live Stream Viewer (UDP vs TCP)"));
    resize(850, 550);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // Group Box for Live Network Statistics
    QGroupBox *statsGroup = new QGroupBox(tr("Live Stream Connection & Stats"), this);
    QHBoxLayout *statsLayout = new QHBoxLayout(statsGroup);
    statsLayout->setContentsMargins(10, 10, 10, 10);

    m_statsLabel = new QLabel(this);
    m_statsLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: #38bdf8;");
    statsLayout->addWidget(m_statsLabel);
    
    // Add active protocol status to stats
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString activeProtocol = settings.value("Network/protocol", "TCP").toString();
    int activePort = settings.value("Network/port", 2345).toInt();
    
    QLabel *statusIndicator = new QLabel(this);
    statusIndicator->setText(QString("Listening on Port: %1 | Configured Protocol: %2")
                             .arg(activePort)
                             .arg(activeProtocol));
    statusIndicator->setStyleSheet("color: #a7f3d0; font-family: monospace; font-size: 12px;");
    statsLayout->addStretch();
    statsLayout->addWidget(statusIndicator);

    mainLayout->addWidget(statsGroup);

    // The Large Scrolling Console Text Edit
    m_logEdit = new QTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setFontFamily("monospace");
    m_logEdit->setFontPointSize(10);
    // Dark terminal styling
    m_logEdit->setStyleSheet(
        "QTextEdit { "
        "  background-color: #0f172a; "
        "  color: #f8fafc; "
        "  border: 1px solid #334155; "
        "  border-radius: 6px; "
        "  font-family: 'Courier New', monospace; "
        "  padding: 8px; "
        "}"
    );
    mainLayout->addWidget(m_logEdit);

    // Horizontal controls row
    QHBoxLayout *ctrlRow = new QHBoxLayout();
    
    m_autoScrollChk = new QCheckBox(tr("Auto-scroll Console"), this);
    m_autoScrollChk->setChecked(true);
    m_autoScrollChk->setStyleSheet("font-weight: bold;");
    ctrlRow->addWidget(m_autoScrollChk);
    
    ctrlRow->addStretch();

    m_pauseBtn = new QPushButton(tr("Pause Stream"), this);
    m_pauseBtn->setStyleSheet("QPushButton { background-color: #e2e8f0; color: #0f172a; }");
    connect(m_pauseBtn, &QPushButton::clicked, this, &LiveStreamViewer::togglePause);
    ctrlRow->addWidget(m_pauseBtn);

    QPushButton *clearBtn = new QPushButton(tr("Clear Console"), this);
    connect(clearBtn, &QPushButton::clicked, this, &LiveStreamViewer::clearLog);
    ctrlRow->addWidget(clearBtn);

    QPushButton *exportBtn = new QPushButton(tr("Export Log..."), this);
    connect(exportBtn, &QPushButton::clicked, this, &LiveStreamViewer::exportLog);
    ctrlRow->addWidget(exportBtn);

    mainLayout->addLayout(ctrlRow);

    // Test Suite Box (makes it act as a test receiver & generator)
    QGroupBox *testGroup = new QGroupBox(tr("Integrated Network Test Suite (Localhost loopback)"), this);
    QHBoxLayout *testLayout = new QHBoxLayout(testGroup);
    testLayout->setSpacing(15);
    testLayout->setContentsMargins(10, 10, 10, 10);

    QLabel *testDesc = new QLabel(tr("Inject test ADIF records directly to the running Qwain port to test connections:"), this);
    testDesc->setWordWrap(true);
    testDesc->setStyleSheet("font-size: 11px; color: #64748b;");
    testLayout->addWidget(testDesc, 2);

    m_testUdpBtn = new QPushButton(tr("⚡ Send Test UDP Packet"), this);
    m_testUdpBtn->setStyleSheet("QPushButton { background-color: #059669; color: white; font-weight: bold; } QPushButton:hover { background-color: #047857; }");
    connect(m_testUdpBtn, &QPushButton::clicked, this, &LiveStreamViewer::sendTestUdp);
    testLayout->addWidget(m_testUdpBtn, 1);

    m_testTcpBtn = new QPushButton(tr("⚡ Send Test TCP Packet"), this);
    m_testTcpBtn->setStyleSheet("QPushButton { background-color: #0891b2; color: white; font-weight: bold; } QPushButton:hover { background-color: #0e7490; }");
    connect(m_testTcpBtn, &QPushButton::clicked, this, &LiveStreamViewer::sendTestTcp);
    testLayout->addWidget(m_testTcpBtn, 1);

    mainLayout->addWidget(testGroup);

    // Bottom Close row
    QHBoxLayout *closeRow = new QHBoxLayout();
    closeRow->addStretch();
    QPushButton *closeBtn = new QPushButton(tr("Close"), this);
    closeBtn->setDefault(true);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeRow->addWidget(closeBtn);
    mainLayout->addLayout(closeRow);

    // Update stats label
    clearLog();

    // Connect to MainWindow data received signal
    connect(m_mainWindow, &MainWindow::dataReceived, this, &LiveStreamViewer::handleDataReceived);
}

LiveStreamViewer::~LiveStreamViewer()
{
}

void LiveStreamViewer::handleDataReceived(const QString &protocol, const QString &senderAddress, int senderPort, const QString &data)
{
    if (m_isPaused) return;

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    int byteCount = data.toUtf8().size();

    // Update Stats counters
    if (protocol == "UDP") {
        m_udpCount++;
        m_udpBytes += byteCount;
    } else {
        m_tcpCount++;
        m_tcpBytes += byteCount;
    }

    // Refresh dynamic stats banner
    m_statsLabel->setText(QString("TCP Packets: %1 (%2 KB)   |   UDP Packets: %3 (%4 KB)   |   Total: %5 Packets")
                          .arg(m_tcpCount)
                          .arg(QString::number(m_tcpBytes / 1024.0, 'f', 2))
                          .arg(m_udpCount)
                          .arg(QString::number(m_udpBytes / 1024.0, 'f', 2))
                          .arg(m_tcpCount + m_udpCount));

    // Construct color-coded HTML text representation
    // UDP in emerald green (#10B981)
    // TCP in bright cyan (#06B6D4)
    // Metadata in slate (#94A3B8)
    // Content in white (#F8FAFC)
    QString color = (protocol == "UDP") ? "#10b981" : "#06b6d4";
    QString htmlLine = QString(
        "<span style='color: #64748b;'>[%1]</span> "
        "<span style='color: %2; font-weight: bold;'>[%3]</span> "
        "<span style='color: #94a3b8; font-style: italic;'>from %4:%5 (%6 bytes)</span> => "
        "<span style='color: #f8fafc;'>%7</span>"
    ).arg(timestamp)
     .arg(color)
     .arg(protocol)
     .arg(senderAddress)
     .arg(senderPort)
     .arg(byteCount)
     .arg(data.toHtmlEscaped().replace("\n", "<br/>"));

    m_logEdit->append(htmlLine);

    // Force automatic scroll down if checked
    if (m_autoScrollChk->isChecked()) {
        QScrollBar *sb = m_logEdit->verticalScrollBar();
        sb->setValue(sb->maximum());
    }
}

void LiveStreamViewer::sendTestUdp()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    int port = settings.value("Network/port", 2345).toInt();
    
    QUdpSocket socket;
    QByteArray record = "<CALL:9>W4DJF/QRP <BAND:3>20M <MODE:3>SSB <RST_SENT:2>59 <RST_RCVD:2>59 <TX_PWR:2>5W <COMMENT:14>Test UDP Stream <EOR>\n";
    
    socket.writeDatagram(record, QHostAddress::LocalHost, port);
}

void LiveStreamViewer::sendTestTcp()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    int port = settings.value("Network/port", 2345).toInt();

    QTcpSocket *socket = new QTcpSocket(this);
    socket->connectToHost(QHostAddress::LocalHost, port);
    
    connect(socket, &QTcpSocket::connected, this, [socket]() {
        socket->write("<CALL:8>CO2DSE/M <BAND:3>40M <MODE:2>CW <RST_SENT:3>599 <RST_RCVD:3>599 <TX_PWR:4>100W <COMMENT:14>Test TCP Stream <EOR>\n");
        socket->disconnectFromHost();
    });
    
    connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
}

void LiveStreamViewer::clearLog()
{
    m_logEdit->clear();
    m_udpCount = 0;
    m_tcpCount = 0;
    m_udpBytes = 0;
    m_tcpBytes = 0;
    
    m_statsLabel->setText(tr("TCP Packets: 0 (0.00 KB)   |   UDP Packets: 0 (0.00 KB)   |   Total: 0 Packets"));
    m_logEdit->append(tr("<span style='color: #64748b;'>Console cleared. Realtime packet listener initialized and active...</span>"));
}

void LiveStreamViewer::exportLog()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Console Log"), "", tr("Text Files (*.txt);;All Files (*)"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Failed"), tr("Could not write to file %1").arg(fileName));
        return;
    }

    QTextStream out(&file);
    out << m_logEdit->toPlainText();
    file.close();
}

void LiveStreamViewer::togglePause()
{
    m_isPaused = !m_isPaused;
    if (m_isPaused) {
        m_pauseBtn->setText(tr("Resume Stream"));
        m_pauseBtn->setStyleSheet("QPushButton { background-color: #ef4444; color: white; font-weight: bold; }");
        m_logEdit->append(tr("<span style='color: #ef4444; font-weight: bold;'>[SYSTEM] Stream viewer paused. Sockets are still listening, but viewer logging is frozen.</span>"));
    } else {
        m_pauseBtn->setText(tr("Pause Stream"));
        m_pauseBtn->setStyleSheet("QPushButton { background-color: #e2e8f0; color: #0f172a; }");
        m_logEdit->append(tr("<span style='color: #10b981; font-weight: bold;'>[SYSTEM] Stream viewer resumed. Live traffic feeds restored.</span>"));
    }
}
