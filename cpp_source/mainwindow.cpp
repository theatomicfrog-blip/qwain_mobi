// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

// ============================================================================
// COMPONENT: MainWindow
// ROLE: Main Workspace controller of the Qwain ecosystem.
//       Coordinates live TCP/UDP network servers, initiates ADIF record parsing,
//       triggers local/online QRZ.com lookups, updates real-time award statistics,
//       manages layout themes/fonts, and hosts child dialog spools (designer, manager).
// INVOKED BY: main.cpp (application entry point).
// PRODUCES: Live QSO updates, leaderboard listings, and real-time GUI views.
// ============================================================================

#include "mainwindow.h"
// Optional SVG support depending on target toolchain package availability
#if __has_include(<QtSvg/QSvgRenderer>)
#include <QtSvg/QSvgRenderer>
#define HAVE_QT_SVG 1
#elif __has_include(<QSvgRenderer>)
#include <QSvgRenderer>
#define HAVE_QT_SVG 1
#endif

#include "fips_data.h"
#include "qrzcache.h"
#include "loadingsplash.h"
#include "paths.h"
#include "configdialog.h"
#include "cacheviewerdialog.h"
#include "icontoolsdialog.h"
#include "imageviewer.h"
#include "qsldesigner.h"
#include "cabrilloconverterdialog.h"
#include "receiveqsldialog.h"
#include "livestreamviewer.h"
#include "statsdialog.h"
#include <QDialog>
#include <QCheckBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTimeZone>
#include <QCloseEvent>
#include <QMenuBar>
#include <QAction>
#include <QShortcut>
#include <QProcess>
#include <QGuiApplication>
#include <QRandomGenerator>

#include <QVBoxLayout>
#include <QTextEdit>
#include <QToolButton>
#include <QTextBrowser>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QToolBar>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QHostAddress>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QCoreApplication>
#include <QProgressBar>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QDateTime>
#include "qslpreviewdialog.h"
#include "dxclusterwidget.h"
#include "graylinewidget.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include "globewidget.h"
#include <QNetworkReply>
#include <QXmlStreamReader>
#include <QInputDialog>
#include <QLineEdit>
#include <QUrl>
#include <QUrlQuery>
#include <QLabel>
#include <QImage>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSplitter>
#include <QDockWidget>
#include <QPainter>
#include <QMessageBox>
#include <QDesktopServices>
#include <QTableWidget>
#include <QHeaderView>
#include <QShortcut>
#include <QScrollBar>
#include <QColorDialog>

#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QProcess>
#include <QApplication>
#include "qslmanagerdialog.h"
#include "dxclusterwidget.h"
#include <QClipboard>
#include <QPushButton>
#include <QScreen>
#include <QRandomGenerator>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QGraphicsBlurEffect>
#include <QtMath>
#include <QTimer>
#include <QToolTip>
#include "waja_mapping.h"
#include <QKeyEvent>
#ifdef HAVE_TEXT_TO_SPEECH
#include <QTextToSpeech>
#include <QVoice>
#endif


extern QPixmap getAppLogo();

// OuterLimitsWindow removed

QString g_latestDebugCapture;

// QSLCheckers removed

class SpaceInvadersWindow : public QWidget {
    QTimer *timer;
    QRect playerRect;
    QList<QRect> bulletRects;
    QList<QRect> alienRects;
    int alienDir;
    int score;
    bool leftPressed, rightPressed;
    int ticks;
    bool gameStarted;
    bool gameOver;
    QPixmap targetLogo;
    QList<int> highScores;

    void drawPixelated(QPainter &p, const QRect &r, const QColor &c, int pattern) {
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        if (pattern == 1) { // Player
            p.drawRect(r.x() + r.width()/2 - 4, r.y(), 8, 8);
            p.drawRect(r.x() + 4, r.y() + 8, r.width() - 8, 8);
            p.drawRect(r.x(), r.y() + 16, r.width(), 8);
        }
    }

    void loadHighScores() {
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        highScores.clear();
        highScores.append(settings.value("Invaders/HighScore1", 0).toInt());
        highScores.append(settings.value("Invaders/HighScore2", 0).toInt());
        highScores.append(settings.value("Invaders/HighScore3", 0).toInt());
        while (highScores.size() < 3) highScores.append(0);
    }

    void saveHighScores() {
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        settings.setValue("Invaders/HighScore1", highScores.value(0, 0));
        settings.setValue("Invaders/HighScore2", highScores.value(1, 0));
        settings.setValue("Invaders/HighScore3", highScores.value(2, 0));
        settings.sync();
    }

    void resetHighScores() {
        highScores = {0, 0, 0};
        saveHighScores();
        update();
    }

    void recordScore(int s) {
        if (s <= 0) return;
        highScores.append(s);
        std::sort(highScores.begin(), highScores.end(), std::greater<int>());
        while (highScores.size() > 3) highScores.removeLast();
        saveHighScores();
    }

    void initAliens() {
        bulletRects.clear();
        alienRects.clear();
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 10; ++col) {
                alienRects.append(QRect(50 + col * 45, 50 + row * 40, 30, 24));
            }
        }
    }

public:
    SpaceInvadersWindow(QWidget *parent = nullptr) : QWidget(parent, Qt::Window), 
        alienDir(1), score(0), leftPressed(false), rightPressed(false), ticks(0), 
        gameStarted(false), gameOver(false) 
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setFixedSize(600, 600);
        setWindowTitle("Qwain Invaders");
        setStyleSheet("background-color: black; color: #00FF00; font-family: monospace; font-weight: bold; font-size: 24px;");
        setFocusPolicy(Qt::StrongFocus);
        setFocus();

        playerRect = QRect(275, 550, 40, 24);

        loadHighScores();

        // Load logo.png as descending target
        targetLogo.load("logo.png");
        if (targetLogo.isNull()) {
            targetLogo.load("cpp_source/logo.png");
        }
        if (targetLogo.isNull()) {
            targetLogo = getAppLogo();
        }
        if (!targetLogo.isNull()) {
            targetLogo = targetLogo.scaled(30, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        initAliens();

        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &SpaceInvadersWindow::gameLoop);
        timer->start(16); // ~60fps
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);

        // =====================================================================
        // START / TITLE SCREEN (Shown when window loads)
        // =====================================================================
        if (!gameStarted) {
            p.setPen(QColor(56, 189, 248)); // Sky Blue
            p.setFont(QFont("Courier", 22, QFont::Bold));
            p.drawText(QRect(0, 60, width(), 40), Qt::AlignCenter, "QWAIN INVADERS");

            p.setPen(Qt::yellow);
            p.setFont(QFont("Courier", 13, QFont::Bold));
            p.drawText(QRect(0, 130, width(), 25), Qt::AlignCenter, "-- TOP 3 HIGH SCORES --");

            p.setFont(QFont("Courier", 14, QFont::Bold));
            p.setPen(QColor(241, 245, 249));
            p.drawText(QRect(0, 165, width(), 25), Qt::AlignCenter, 
                QString("1ST:  %1").arg(highScores.value(0, 0), 4, 10, QChar('0')));
            p.drawText(QRect(0, 195, width(), 25), Qt::AlignCenter, 
                QString("2ND:  %1").arg(highScores.value(1, 0), 4, 10, QChar('0')));
            p.drawText(QRect(0, 225, width(), 25), Qt::AlignCenter, 
                QString("3RD:  %1").arg(highScores.value(2, 0), 4, 10, QChar('0')));

            // Prompt to begin
            p.setPen(Qt::green);
            p.setFont(QFont("Courier", 16, QFont::Bold));
            p.drawText(QRect(0, 320, width(), 35), Qt::AlignCenter, "Hit Space To Begin");

            p.setPen(QColor(148, 163, 184));
            p.setFont(QFont("Courier", 11, QFont::Bold));
            p.drawText(QRect(0, 520, width(), 25), Qt::AlignCenter, "<ESC> TO QUIT");
            p.drawText(QRect(0, 550, width(), 25), Qt::AlignCenter, "Hit R to reset High Scores");
            return;
        }
        
        drawPixelated(p, playerRect, Qt::green, 1);

        p.setBrush(Qt::white);
        p.setPen(Qt::NoPen);
        for (const QRect &b : bulletRects) p.drawRect(b);
        
        for (const QRect &a : alienRects) {
            if (!targetLogo.isNull()) {
                p.drawPixmap(a.x() + (a.width() - targetLogo.width())/2,
                             a.y() + (a.height() - targetLogo.height())/2,
                             targetLogo);
            } else {
                p.fillRect(a, Qt::red);
            }
        }

        p.setPen(Qt::green);
        p.setFont(QFont("Courier", 16, QFont::Bold));
        p.drawText(10, 30, QString("SCORE < %1 >").arg(score, 4, 10, QChar('0')));
        p.drawText(width() - 220, 30, QString("HI < %1 >").arg(highScores.value(0, 0), 4, 10, QChar('0')));
        
        // =====================================================================
        // GAME OVER / VICTORY SCREEN
        // =====================================================================
        if (alienRects.isEmpty()) {
            p.setPen(Qt::green);
            p.setFont(QFont("Courier", 20, QFont::Bold));
            p.drawText(QRect(0, 210, width(), 35), Qt::AlignCenter, "YOU WIN!");
            p.setFont(QFont("Courier", 14, QFont::Bold));
            p.setPen(Qt::white);
            p.drawText(QRect(0, 255, width(), 30), Qt::AlignCenter, QString("FINAL SCORE: %1").arg(score, 4, 10, QChar('0')));
            p.setPen(Qt::yellow);
            p.drawText(QRect(0, 310, width(), 30), Qt::AlignCenter, "PRESS SPACE TO RESTART");
            p.setPen(QColor(56, 189, 248));
            p.drawText(QRect(0, 345, width(), 30), Qt::AlignCenter, "<ESC> TO QUIT");
            p.setPen(QColor(148, 163, 184));
            p.setFont(QFont("Courier", 11, QFont::Bold));
            p.drawText(QRect(0, 550, width(), 25), Qt::AlignCenter, "Hit R to reset High Scores");
        } else if (gameOver) {
            p.setPen(Qt::red);
            p.setFont(QFont("Courier", 20, QFont::Bold));
            p.drawText(QRect(0, 210, width(), 35), Qt::AlignCenter, "GAME OVER");
            p.setFont(QFont("Courier", 14, QFont::Bold));
            p.setPen(Qt::white);
            p.drawText(QRect(0, 255, width(), 30), Qt::AlignCenter, QString("FINAL SCORE: %1").arg(score, 4, 10, QChar('0')));
            p.setPen(Qt::yellow);
            p.drawText(QRect(0, 310, width(), 30), Qt::AlignCenter, "PRESS SPACE TO RESTART");
            p.setPen(QColor(56, 189, 248));
            p.drawText(QRect(0, 345, width(), 30), Qt::AlignCenter, "<ESC> TO QUIT");
            p.setPen(QColor(148, 163, 184));
            p.setFont(QFont("Courier", 11, QFont::Bold));
            p.drawText(QRect(0, 550, width(), 25), Qt::AlignCenter, "Hit R to reset High Scores");
        }
    }

    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Escape) {
            // Offer <esc> to quit
            close();
            return;
        }
        if (e->key() == Qt::Key_R) {
            // R key resets high scores
            resetHighScores();
            return;
        }
        if (e->key() == Qt::Key_Left) leftPressed = true;
        if (e->key() == Qt::Key_Right) rightPressed = true;
        if (e->key() == Qt::Key_Space) {
            if (!gameStarted || alienRects.isEmpty() || gameOver) {
                // Space bar begins game from load screen or restarts on game over / victory
                score = 0;
                gameOver = false;
                gameStarted = true;
                ticks = 0;
                alienDir = 1;
                playerRect = QRect(275, 550, 40, 24);
                initAliens();
            } else if (bulletRects.size() < 3) {
                bulletRects.append(QRect(playerRect.center().x() - 2, playerRect.top() - 10, 4, 10));
            }
        }
    }
    void keyReleaseEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_Left) leftPressed = false;
        if (e->key() == Qt::Key_Right) rightPressed = false;
    }

    void gameLoop() {
        if (!gameStarted || alienRects.isEmpty() || gameOver) return;
        
        ticks++;
        if (leftPressed && playerRect.left() > 0) playerRect.translate(-5, 0);
        if (rightPressed && playerRect.right() < width()) playerRect.translate(5, 0);

        for (int i = 0; i < bulletRects.size(); ) {
            bulletRects[i].translate(0, -7);
            if (bulletRects[i].bottom() < 0) {
                bulletRects.removeAt(i);
            } else {
                ++i;
            }
        }

        if (ticks % (qMax(5, 30 - score/20)) == 0) {
            bool drop = false;
            for (auto &a : alienRects) {
                a.translate(alienDir * 10, 0);
                if (a.left() < 10 || a.right() > width() - 10) drop = true;
            }
            if (drop) {
                alienDir = -alienDir;
                for (auto &a : alienRects) {
                    a.translate(alienDir * 10, 20); // undo out of bounds push, and drop
                }
            }
        }

        // Check if any alien reached the player
        for (const auto &a : alienRects) {
            if (a.bottom() >= playerRect.top()) {
                gameOver = true;
                recordScore(score);
                break;
            }
        }

        for (int i = 0; i < bulletRects.size(); ) {
            bool hit = false;
            for (int j = 0; j < alienRects.size(); ++j) {
                if (bulletRects[i].intersects(alienRects[j])) {
                    alienRects.removeAt(j);
                    score += 10;
                    hit = true;
                    if (alienRects.isEmpty()) {
                        recordScore(score);
                    }
                    break;
                }
            }
            if (hit) {
                bulletRects.removeAt(i);
            } else {
                ++i;
            }
        }

        update();
    }
};

class WindowToggleFilter : public QObject {
public:
    WindowToggleFilter(QAction *action, QObject *parent = nullptr)
        : QObject(parent), m_action(action) {}
    
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::Hide) {
            m_action->setChecked(false);
        } else if (event->type() == QEvent::Show) {
            m_action->setChecked(true);
        }
        return QObject::eventFilter(obj, event);
    }
private:
    QAction *m_action;
};



MainWindow::MainWindow(LoadingSplash *splash, QWidget *parent)
    : QWidget(parent), m_port(2345), m_tcpServer(nullptr), m_udpSocket(nullptr), m_splash(splash)
{
    if (m_splash) m_splash->setProgress(10, "Initializing Database...");
    initDatabase();
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    
    applyThemeStyle();

    QShortcut *debugShortcut = new QShortcut(QKeySequence("Ctrl+D"), this);
    connect(debugShortcut, &QShortcut::activated, this, [this]() {
        QDialog *sudoDlg = new QDialog(this);
        sudoDlg->setWindowTitle("Capturing Debug Data");
        QVBoxLayout *vl = new QVBoxLayout(sudoDlg);
        QLabel *lbl = new QLabel("Gathering debug with SUDO allows RAM flag detection. It is <b><u>NOT</u></b> required, just amazingly helpful.<br>Enter sudo password below to allow, or blank to gather without the extra information.", sudoDlg);
        lbl->setWordWrap(true);
        vl->addWidget(lbl);
        QLineEdit *pwdEdit = new QLineEdit(sudoDlg);
        pwdEdit->setEchoMode(QLineEdit::Password);
        vl->addWidget(pwdEdit);
        connect(pwdEdit, &QLineEdit::returnPressed, sudoDlg, &QDialog::accept);
        sudoDlg->exec();
        
        QString sudoPwd = pwdEdit->text();
        sudoDlg->deleteLater();

        auto run_cmd = [sudoPwd](const QString& cmd, bool useSudo = false) -> QString {
            QProcess proc;
            if (useSudo && !sudoPwd.isEmpty()) {
                proc.start("sudo", QStringList() << "-S" << "bash" << "-c" << cmd);
                proc.write((sudoPwd + "\n").toUtf8());
                proc.closeWriteChannel();
            } else {
                proc.start("bash", QStringList() << "-c" << cmd);
            }
            proc.waitForFinished(5000);
            QString out = proc.readAllStandardOutput();
            return out.isEmpty() ? "[no output]\n" : out;
        };

        QString debugStr;
        debugStr += "Version: " + getAppVersion() + "\n";
        debugStr += "Date: " + QDateTime::currentDateTime().toString() + "\n";
        debugStr += "OS: " + QSysInfo::prettyProductName() + " " + QSysInfo::currentCpuArchitecture() + "\n";
        debugStr += "Kernel: " + QSysInfo::kernelType() + " " + QSysInfo::kernelVersion() + "\n";
        debugStr += "Printer: " + QSettings(QCoreApplication::organizationName(), "Qwain").value("Printers/default", "None").toString() + "\n";
        
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        QStringList templates = settings.childGroups();
        debugStr += "Templates Encoded Data:\n";
        for (const QString &group : templates) {
            if (group.startsWith("QslTemplates")) {
                settings.beginGroup(group);
                for (const QString &key : settings.allKeys()) {
                    debugStr += QString("  [%1] %2 = %3\n").arg(group, key, settings.value(key).toString());
                }
                settings.endGroup();
            }
        }
        
        debugStr += "\n=== Video Hardware ===\n" + run_cmd("lspci -nn | grep -Ei 'vga|3d|display'");
        debugStr += "\n=== Video Driver Info ===\n" + run_cmd("lspci -k | grep -B 1 -A 2 -Ei 'vga|3d|display'");
        debugStr += run_cmd("glxinfo -B 2>/dev/null | grep -Ei 'OpenGL|renderer|version'");
        debugStr += "\n=== Video Versioning / Driver Versions ===\n" + run_cmd("glxinfo -B 2>/dev/null | grep 'OpenGL version'");
        debugStr += run_cmd("glxinfo -B 2>/dev/null | grep 'OpenGL vendor'");
        debugStr += run_cmd("grep -E 'nvidia|i915|amdgpu|radeon|nouveau' /proc/modules 2>/dev/null | head -10");
        debugStr += "\n=== CPU Hardware ===\n" + run_cmd("lscpu");
        debugStr += run_cmd("grep -E 'model name|cpu MHz|cache|flags' /proc/cpuinfo | head -12");
        debugStr += "\n=== RAM Configuration ===\n" + run_cmd("free -h");
        debugStr += run_cmd("cat /proc/meminfo | grep -E 'MemTotal|MemFree|MemAvailable|Buffers|Cached|SwapTotal|SwapFree'");
        debugStr += run_cmd("numactl --hardware 2>/dev/null");
        if (!sudoPwd.isEmpty()) {
            debugStr += run_cmd("dmidecode -t memory", true);
        }
        debugStr += "\n--- Diagnostics Log ---\n" + m_textEdit->toPlainText() + "\n";
        
        extern QString g_latestDebugCapture;
        g_latestDebugCapture = debugStr;
        QMessageBox::information(nullptr, "Debug", "DEBUG Information Captured.");
    
    });
    QTimer::singleShot(2000, this, [this]() {
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        if (!settings.value("PrinterAutoScan", true).toBool()) return;
        QProcess *proc = new QProcess(this);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, proc]() {
            QString out = proc->readAllStandardOutput();
            QStringList lines = out.split('\n');
            QSettings settings(QCoreApplication::organizationName(), "Qwain");
            QStringList knownPrinters = settings.value("KnownPrinters").toStringList();
            QStringList scannedHistory = settings.value("ScannedPrintersHistory").toStringList();
            for (const QString &line : lines) {
                QStringList parts = line.split(';');
                if (parts.size() >= 8 && parts[0] == "=") {
                    QString ip = parts[7];
                    if (!scannedHistory.contains(ip)) {
                        scannedHistory.append(ip);
                        settings.setValue("ScannedPrintersHistory", scannedHistory);
                        
                        bool existsInKnown = false;
                        for (const QString &kp : knownPrinters) {
                            QString dest = kp.split(" ").first().trimmed();
                            if (dest == ip || dest.contains("//" + ip + ":") || dest.contains("//" + ip + "/") || dest.endsWith("//" + ip)) { 
                                existsInKnown = true; 
                                break; 
                            }
                        }
                        if (!existsInKnown) {
                            QMessageBox msgBox(this);
                            msgBox.setWindowTitle("New Printer Detected on Network");
                            msgBox.setText("A new printer (" + ip + ") was detected. Would you like to configure it?");
                            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                            msgBox.setDefaultButton(QMessageBox::Yes);
                            QCheckBox *cb = new QCheckBox("Never Scan Again");
                            msgBox.setCheckBox(cb);
                            if (msgBox.exec() == QMessageBox::Yes) {
                                openConfigDialog();
                            }
                            if (cb->isChecked()) {
                                settings.setValue("PrinterAutoScan", false);
                            }
                        }
                    }
                }
            }
            proc->deleteLater();
        });
        proc->start("avahi-browse", QStringList() << "-rtp" << "_ipp._tcp");
    
    });
    QMenu *configMenu = new QMenu(tr("&Configuration"), this);
    
    m_userAction = configMenu->addAction(tr("QRZ Username: Not Set"));
    m_userAction->setEnabled(false);
    m_ipAction = configMenu->addAction(tr("IP Address: "));
    m_ipAction->setEnabled(false);
    m_portAction = configMenu->addAction(tr("Port: "));
    m_portAction->setEnabled(false);
    m_logFileAction = configMenu->addAction(tr("Logfile: Not Set"));
    m_logFileAction->setEnabled(false);
    
    configMenu->addSeparator();
    QAction *settingsAction = configMenu->addAction(tr("S&ettings..."));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openConfigDialog);

    QMenu *miscMenu = configMenu->addMenu(tr("&Misc"));
    QAction *cacheViewerAction = miscMenu->addAction(tr("Cache Tools"));
    connect(cacheViewerAction, &QAction::triggered, this, &MainWindow::openCacheViewer);

    QAction *liveStreamAction = miscMenu->addAction(tr("Live Stream Viewer"));
    connect(liveStreamAction, &QAction::triggered, this, &MainWindow::openLiveStreamViewer);

    QAction *iconToolsAction = miscMenu->addAction(tr("Icon Tools"));
    connect(iconToolsAction, &QAction::triggered, this, &MainWindow::openIconTools);

    QAction *rescanAction = miscMenu->addAction(tr("Re-Scan Log File"));
    connect(rescanAction, &QAction::triggered, this, &MainWindow::loadLogfile);

    QAction *imageViewerAction = miscMenu->addAction(tr("Image Viewer/Editor"));
    connect(imageViewerAction, &QAction::triggered, this, [this]() {
        ImageViewer dlg(false, this);
        dlg.exec();
    });

    QMenu *suiteMenu = miscMenu->addMenu(tr("Suite"));
    QAction *xmlAction = suiteMenu->addAction(tr("XML QuickView"));
    connect(xmlAction, &QAction::triggered, this, &MainWindow::onXmlTestClicked);

    QAction *countyPrepAction = suiteMenu->addAction(tr("Initialize US County Cache"));
    connect(countyPrepAction, &QAction::triggered, this, &MainWindow::onCountyPrepClicked);

    QMenu *boredMenu = miscMenu->addMenu(tr("I'm Bored"));
    QAction *invadersAction = boredMenu->addAction(tr("Qwain Invaders"));
    connect(invadersAction, &QAction::triggered, this, []() {
        SpaceInvadersWindow *w = new SpaceInvadersWindow(nullptr);
        w->show();
    });

    configMenu->addSeparator();
    QAction *exitAction = configMenu->addAction(tr("E&xit"));
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    QWidget *rightMenuWidget = new QWidget(this);
    QVBoxLayout *rightMenuLayout = new QVBoxLayout(rightMenuWidget);
    rightMenuLayout->setContentsMargins(0, 0, 10, 0);
    rightMenuLayout->setSpacing(0);
    
    m_dateLabel = new QLabel("01/01/1970", this);
    m_dateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_timeLabel = new QLabel("00:00:00 UTC", this);
    m_timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QFont boldFont = m_timeLabel->font();
    boldFont.setBold(true);
    m_timeLabel->setFont(boldFont);
    
    // Time and date will be placed above logo later

    // menuBar()->setCornerWidget(rightMenuWidget, Qt::TopRightCorner);

    m_networkManager = new QNetworkAccessManager(this);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    QFont textFont = m_textEdit->font();
    // Maybe slightly smaller for logs
    textFont.setPointSize(qMax(8, textFont.pointSize() - 1));
    m_textEdit->setFont(textFont);
    m_sessionStartTime = QDateTime::currentDateTimeUtc();
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QWidget *infoWidget = new QWidget(this);
    mainLayout->addWidget(infoWidget);
    QHBoxLayout *infoLayout = new QHBoxLayout(infoWidget);

    infoLayout->setContentsMargins(0, 0, 0, 0);
    
    m_infoBrowser = new QTextBrowser(this);
    m_infoBrowser->setOpenExternalLinks(false);
    m_infoBrowser->setOpenLinks(false);
    m_infoBrowser->setMinimumHeight(150);
    m_infoBrowser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // new preference
    m_infoBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_infoBrowser->viewport()->installEventFilter(this);
    
    connect(m_infoBrowser, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        QString urlStr = url.toString();
        if (urlStr.startsWith("http://app.local/qsl_paper/")) {
            QString callsign = urlStr.mid(27);
            QslPreviewDialog dlg(callsign, m_infoBrowser->toHtml(), m_allQsos[callsign], this);
            dlg.exec();
        } else if (urlStr.startsWith("http://app.local/qsl_multi/")) {
            QString callsign = urlStr.mid(27);
            QslDesigner dlg(this);
            dlg.exec();
        } else if (urlStr.startsWith("http://app.local/toggle_map_mode")) {
            m_useHeatmapMode = !m_useHeatmapMode;
            displayCallInfo(m_lastHtmlInfoRaw, m_lastImgData, m_currentDisplayedCallsign, m_lastPota, m_lastCountry, m_lastLotw, m_lastMqsl);
        } else if (url.scheme() == "http" || url.scheme() == "https") {
            QDesktopServices::openUrl(url);
        }
    
    });
    m_newQsoLabel = new QLabel("New QSO", this);
    QFont nqFont = m_newQsoLabel->font();
    nqFont.setPointSize(16);
    nqFont.setBold(true);
    m_newQsoLabel->setFont(nqFont);
    m_newQsoLabel->setAlignment(Qt::AlignCenter);
    m_newQsoLabel->setStyleSheet("color: red;");
    m_newQsoLabel->hide();

    QGraphicsOpacityEffect *op = new QGraphicsOpacityEffect(m_newQsoLabel);
    m_newQsoLabel->setGraphicsEffect(op);
    m_newQsoAnim = new QPropertyAnimation(op, "opacity", this);
    m_newQsoAnim->setDuration(800);
    m_newQsoAnim->setStartValue(1.0);
    m_newQsoAnim->setEndValue(0.1);
    m_newQsoAnim->setEasingCurve(QEasingCurve::InOutSine);
    m_newQsoAnim->setLoopCount(-1);

    m_newCountyLabel = new QLabel("NEW COUNTY in %State%", this);
    QFont ncFont = m_newCountyLabel->font();
    ncFont.setPointSize(16);
    ncFont.setBold(true);
    m_newCountyLabel->setFont(ncFont);
    m_newCountyLabel->setAlignment(Qt::AlignCenter);
    m_newCountyLabel->setStyleSheet("color: #ffaa00;"); // bright amber/orange
    m_newCountyLabel->hide();

    QGraphicsOpacityEffect *op2 = new QGraphicsOpacityEffect(m_newCountyLabel);
    m_newCountyLabel->setGraphicsEffect(op2);
    m_newCountyAnim = new QPropertyAnimation(op2, "opacity", this);
    m_newCountyAnim->setDuration(800);
    m_newCountyAnim->setStartValue(1.0);
    m_newCountyAnim->setEndValue(0.1);
    m_newCountyAnim->setEasingCurve(QEasingCurve::InOutSine);
    m_newCountyAnim->setLoopCount(-1);

    m_newCountyTimer = new QTimer(this);
    m_newCountyTimer->setSingleShot(true);
    connect(m_newCountyTimer, &QTimer::timeout, this, [this]() {
        if (m_newCountyLabel) m_newCountyLabel->hide();
        if (m_newCountyAnim) m_newCountyAnim->stop();
    });

    m_newGridLabel = new QLabel("NEW GRID", this);
    QFont ngFont = m_newGridLabel->font();
    ngFont.setPointSize(16);
    ngFont.setBold(true);
    m_newGridLabel->setFont(ngFont);
    m_newGridLabel->setAlignment(Qt::AlignCenter);
    m_newGridLabel->setStyleSheet("color: #9b5de5;"); // beautiful electric magenta/purple
    m_newGridLabel->hide();

    QGraphicsOpacityEffect *op3 = new QGraphicsOpacityEffect(m_newGridLabel);
    m_newGridLabel->setGraphicsEffect(op3);
    m_newGridAnim = new QPropertyAnimation(op3, "opacity", this);
    m_newGridAnim->setDuration(800);
    m_newGridAnim->setStartValue(1.0);
    m_newGridAnim->setEndValue(0.1);
    m_newGridAnim->setEasingCurve(QEasingCurve::InOutSine);
    m_newGridAnim->setLoopCount(-1);

    m_newGridTimer = new QTimer(this);
    m_newGridTimer->setSingleShot(true);
    connect(m_newGridTimer, &QTimer::timeout, this, [this]() {
        if (m_newGridLabel) m_newGridLabel->hide();
        if (m_newGridAnim) m_newGridAnim->stop();
    });

    QVBoxLayout *infoLeftLayout = new QVBoxLayout();
    infoLeftLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *lookupLayout = new QHBoxLayout();
    
    m_lookupEdit = new QLineEdit(this);
    m_lookupEdit->setMaximumWidth(150);
    m_lookupEdit->setPlaceholderText("Enter Callsign...");
    m_lookupEdit->installEventFilter(this);
    lookupLayout->addWidget(m_lookupEdit);

    m_searchMatchesList = new QListWidget(this);
    m_searchMatchesList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_searchMatchesList->hide();
    m_searchMatchesList->setStyleSheet(
        "QListWidget {"
        "  background-color: #2a2a2a;"
        "  color: white;"
        "  border: 1px solid #555555;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item {"
        "  padding: 4px 8px;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #3b82f6;"
        "  color: white;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #3b82f6;"
        "  color: white;"
        "}"
    );

    connect(m_lookupEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (text.trimmed().isEmpty()) {
            m_searchMatchesList->hide();
            return;
        }
        
        QString trimmed = text.trimmed().toUpper();
        
        // Build regex pattern based on input (matching character-by-character as typed)
        QString regexPattern = "^";
        for (int i = 0; i < trimmed.length(); ++i) {
            QChar c = trimmed[i];
            if (c == '*') {
                regexPattern += ".*";
            } else if (c == '?') {
                regexPattern += ".";
            } else if (c == '.' || c == '+' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == '^' || c == '$' || c == '\\' || c == '|') {
                regexPattern += "\\" + QString(c);
            } else {
                regexPattern += QRegularExpression::escape(QString(c));
            }
        }
        
        QRegularExpression re(regexPattern, QRegularExpression::CaseInsensitiveOption);
        
        m_searchMatchesList->clear();
        
        QStringList matches;
        for (const QString &call : m_uniqueCallsigns) {
            QRegularExpressionMatch match = re.match(call);
            if (match.hasMatch()) {
                matches << call;
            }
        }
        
        matches.sort();
        
        if (matches.isEmpty()) {
            m_searchMatchesList->hide();
            return;
        }
        
        m_searchMatchesList->addItems(matches);
        
        // Adjust geometry to be right below m_lookupEdit
        QPoint pos = m_lookupEdit->mapTo(this, QPoint(0, m_lookupEdit->height()));
        int width = m_lookupEdit->width();
        int height = qMin(matches.size() * 25 + 10, 150); // limit height but scale with content
        m_searchMatchesList->setGeometry(pos.x(), pos.y(), width, height);
        m_searchMatchesList->raise();
        m_searchMatchesList->show();
    });

    connect(m_searchMatchesList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        if (!item) return;
        m_lookupEdit->setText(item->text());
        m_searchMatchesList->hide();
        emit m_lookupEdit->returnPressed();
    });
    
    QPushButton *lookupBtn = new QPushButton("QSO Search", this);
    lookupLayout->addWidget(lookupBtn);
    
    connect(lookupBtn, &QPushButton::clicked, this, [this]() {
        QString call = m_lookupEdit->text().trimmed().toUpper();
        if (call.isEmpty()) return;
        QString fakeAdif = QString("<CALL:%1>%2<QSO_DATE:8>%3<TIME_ON:6>000000<EOR>")
                            .arg(call.length()).arg(call)
                            .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMdd"));
        processAdifRecord(fakeAdif, true); // true = isManualLookup
    });
    connect(m_lookupEdit, &QLineEdit::returnPressed, lookupBtn, &QPushButton::click);

    lookupBtn->hide(); // Use Enter to accept

    lookupLayout->addStretch(1);

    infoLeftLayout->addLayout(lookupLayout);
    infoLeftLayout->addWidget(m_infoBrowser, 1);
    QHBoxLayout *indicatorLayout = new QHBoxLayout();
    indicatorLayout->setContentsMargins(0, 0, 0, 0);
    indicatorLayout->setSpacing(10);
    indicatorLayout->addWidget(m_newQsoLabel);
    indicatorLayout->addWidget(m_newCountyLabel);
    indicatorLayout->addWidget(m_newGridLabel);
    infoLeftLayout->addLayout(indicatorLayout, 0);
    
    infoLayout->addLayout(infoLeftLayout, 3);
    
    QVBoxLayout *rightPaneLayout = new QVBoxLayout();
    rightPaneLayout->setContentsMargins(0, 0, 0, 0);
    
    extern QPixmap getAppLogo();
    QPixmap lPix = getAppLogo();
    QLabel *logoLabel = new QLabel(this);
    int tH = qRound(1.2 * logicalDpiY());
    int tW = qRound((double)lPix.width() / lPix.height() * tH);
    logoLabel->setFixedSize(tW, tH);
    logoLabel->setPixmap(lPix.scaled(tW, tH, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setObjectName("logoLabel");
    logoLabel->installEventFilter(this);
    rightPaneLayout->addWidget(logoLabel, 0, Qt::AlignCenter);
    
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_dateLabel->setAlignment(Qt::AlignCenter);
    rightPaneLayout->addWidget(m_timeLabel, 0);
    rightPaneLayout->addWidget(m_dateLabel, 0);
    
    m_globeWidget = new GlobeWidget(this);
    m_globeWidget->setFixedSize(140, 140);
    m_globeWidget->setVisible(false);
    rightPaneLayout->addWidget(m_globeWidget, 0, Qt::AlignCenter);
    
    rightPaneLayout->addStretch(1);
    
    infoLayout->addLayout(rightPaneLayout, 1);
    
    DxClusterWidget *dxClusterWindow = new DxClusterWidget(nullptr);
    connect(dxClusterWindow, &DxClusterWidget::spotClicked, this, [this](const QString &callsign) {
        m_lookupEdit->setText(callsign);
        emit m_lookupEdit->returnPressed();
    });
    
    // removed infoDock code here

    QWidget *statusWidget = new QWidget(this);
    
    QHBoxLayout *statusMainLayout = new QHBoxLayout(statusWidget);

    QVBoxLayout *statusLeftLayout = new QVBoxLayout();
    
    
    QHBoxLayout *topStatusLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Status: Waiting", this);
    topStatusLayout->addWidget(m_statusLabel);
    
    // Configure textEdit to be scrollable and resizeable by allowing vertical growth
    m_textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_textEdit->setMinimumHeight(m_textEdit->fontMetrics().lineSpacing() * 2 + 10);
    
    int limit = settings.value("UI/statusScrollLimit", 50).toInt();
    m_textEdit->document()->setMaximumBlockCount(limit);
    
    topStatusLayout->addWidget(m_textEdit, 1);
    
    statusLeftLayout->addLayout(topStatusLayout);
    
    QHBoxLayout *qsoLayout = new QHBoxLayout();
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    
    m_qsoCountLabel = new QLabel("QSOs: 0", this);
    QFont boldFont2 = m_qsoCountLabel->font();
    boldFont2.setBold(true);
    m_qsoCountLabel->setFont(boldFont2);

    qsoLayout->addWidget(m_progressBar);
    qsoLayout->addWidget(m_qsoCountLabel);
    
    statusLeftLayout->addLayout(qsoLayout);
    statusMainLayout->addLayout(statusLeftLayout, 1);

    QWidget *statusWindow = new QWidget(nullptr, Qt::Window);
    statusWindow->setWindowTitle(tr("Status"));
    statusWindow->setObjectName("statusWindow");
    statusWindow->resize(400, 150);
    QVBoxLayout *statusWinLayout = new QVBoxLayout(statusWindow);
    statusWinLayout->setContentsMargins(0, 0, 0, 0);
    statusWinLayout->addWidget(statusWidget);
    
    QTimer *clockTimer = new QTimer(this);
    connect(clockTimer, &QTimer::timeout, this, &MainWindow::updateClock);
    clockTimer->start(1000);
    updateClock();
    
    // initialize dashboard and other system timers
    QTimer *qrzProbeTimer = new QTimer(this);
    connect(qrzProbeTimer, &QTimer::timeout, this, [this]() {
        testQrzLogin(true); // silent
    });
    qrzProbeTimer->start(42 * 60 * 1000); // 42 mins

    QMenu *windowsMenu = new QMenu(tr("&Viewports"), this);
    
    auto createToggleAction = [windowsMenu](const QString &title, QWidget *window) {
        QAction *act = windowsMenu->addAction(title);
        act->setCheckable(true);
        act->setChecked(false); // widgets are hidden by default when created
        QObject::connect(act, &QAction::toggled, window, &QWidget::setVisible);
        window->installEventFilter(new WindowToggleFilter(act, window));
        return act;
    };
    
    QAction *statusAct = createToggleAction(tr("Status"), statusWindow);
    QAction *dxClusterAct = createToggleAction(tr("DX Cluster Spots"), dxClusterWindow);
    // When dxClusterWindow is shown, start clustering
    connect(dxClusterAct, &QAction::toggled, dxClusterWindow, [dxClusterWindow](bool checked){
        if(checked) dxClusterWindow->startClustering();
        else dxClusterWindow->stopClustering();
    });
    
    windowsMenu->addSeparator();
    
    QAction *globeAct = windowsMenu->addAction(tr("Globe"));
    globeAct->setCheckable(true);
    globeAct->setChecked(false);
    connect(globeAct, &QAction::toggled, m_globeWidget, &QWidget::setVisible);
    
    GraylineWidget *graylineWindow = new GraylineWidget();
    graylineWindow->setWindowFlags(Qt::Window);
    QAction *graylineAct = createToggleAction(tr("Grayline"), graylineWindow);
    
    QMenu *aboutMenu = new QMenu(tr("&About"), this);
    QAction *statsAction = aboutMenu->addAction(tr("Stats"));
    connect(statsAction, &QAction::triggered, this, &MainWindow::openStatsDialog);

    QAction *copyrightAction = aboutMenu->addAction(tr("Copyright Information"));
    connect(copyrightAction, &QAction::triggered, this, [this]() {
        // Rich text Copyright Information dialog with clickable hyperlink to GNU licenses
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Copyright Information"));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setTextInteractionFlags(Qt::TextBrowserInteraction);
        msgBox.setText(
            "<p>All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.</p><br>"
            "<p>This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.</p><br>"
            "<p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.</p><br>"
            "<p>You should have received a copy of the GNU General Public License along with this program. If not, see &lt;<a href=\"https://www.gnu.org/licenses/\" style=\"color: #38bdf8;\">https://www.gnu.org/licenses/</a>&gt;.</p>"
        );
        for (QLabel *lbl : msgBox.findChildren<QLabel*>()) {
            lbl->setOpenExternalLinks(true);
            lbl->setTextInteractionFlags(Qt::TextBrowserInteraction);
        }
        msgBox.exec();
    });
    QAction *versionAction = aboutMenu->addAction(tr("Version Info"));
    connect(versionAction, &QAction::triggered, this, [this]() {
        QDialog *dlg = new QDialog(this);
        dlg->setWindowTitle("Version");
        dlg->resize(500, 400);
        dlg->setStyleSheet("background-color: black; color: white;");
        QVBoxLayout *l = new QVBoxLayout(dlg);
        
        QString ver = getAppVersion();
        
        QLabel *lbl = new QLabel("v" + ver + "\nQwain Qso System", dlg);
        lbl->setAlignment(Qt::AlignCenter);
        QFont f = lbl->font();
        f.setPointSize(20);
        f.setBold(true);
        lbl->setFont(f);
        l->addWidget(lbl);
        
        QLabel *mottoLbl = new QLabel("“In the spirit of TWAIN, Qwain embraces the same philosophy: boring name, useful function.”", dlg);
        mottoLbl->setAlignment(Qt::AlignCenter);
        mottoLbl->setWordWrap(true);
        QFont fMotto = mottoLbl->font();
        fMotto.setPointSize(14);
        fMotto.setBold(true);
        mottoLbl->setFont(fMotto);
        l->addWidget(mottoLbl);

        l->addSpacing(20);

        QLabel *quoteLbl = new QLabel("TWAIN (the scanner/camera interface standard) is indeed famously described as a backronym for “Technology Without an Interesting Name.” The name was chosen from Kipling’s line “...and never the twain shall meet...” to symbolize the once-difficult meeting of applications and imaging hardware. Once it was stylized as TWAIN, people assumed it was an acronym, and the joking expansion “Technology Without an Interesting Name” stuck.", dlg);
        quoteLbl->setAlignment(Qt::AlignCenter);
        quoteLbl->setWordWrap(true);
        QFont fQuote = quoteLbl->font();
        fQuote.setFamily("Times New Roman"); // "stoic font"
        fQuote.setPointSize(10);
        quoteLbl->setFont(fQuote);
        l->addWidget(quoteLbl);
        
        QTimer *t = new QTimer(dlg);
        int *step = new int(0);
        connect(t, &QTimer::timeout, dlg, [lbl, step, t]() {
            int maxSteps = 50;
            if (*step <= maxSteps) {
                int c = (255 * (*step)) / maxSteps;
                lbl->setStyleSheet(QString("color: rgb(0, 0, %1);").arg(c));
                (*step)++;
            } else {
                t->stop();
            }
        });
        t->start(40);
        dlg->exec();

    });
    int insertPos = 3;
    auto addMenuBtn = [this, lookupLayout, &insertPos](QMenu *menu, const QString &text) {
        QToolButton *btn = new QToolButton(this);
        btn->setText(text);
        btn->setMenu(menu);
        btn->setPopupMode(QToolButton::InstantPopup);
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setStyleSheet("QToolButton::menu-indicator { image: none; } QToolButton { border: none; padding: 4px; } QToolButton:hover { background-color: rgba(255, 255, 255, 50); }");
        lookupLayout->insertWidget(insertPos++, btn);
    };

    auto addActionBtn = [this, lookupLayout, &insertPos](const QString &text, const std::function<void()>& action) {
        QToolButton *btn = new QToolButton(this);
        btn->setText(text);
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setStyleSheet("QToolButton { border: none; padding: 4px; } QToolButton:hover { background-color: rgba(255, 255, 255, 50); }");
        QObject::connect(btn, &QToolButton::clicked, this, action);
        lookupLayout->insertWidget(insertPos++, btn);
    };
    
    addMenuBtn(configMenu, tr("Configuration"));
    
    addActionBtn(tr("Print QSLs"), [this](){
        QList<QslManagerDialog*> managers = findChildren<QslManagerDialog*>();
        if (!managers.isEmpty()) {
            managers.first()->activateWindow();
            return;
        }
        QslManagerDialog *dlg = new QslManagerDialog(this, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    });

    addActionBtn(tr("Receive QSLs"), [this](){
        QList<ReceiveQslsDialog*> rxDlgs = findChildren<ReceiveQslsDialog*>();
        if (!rxDlgs.isEmpty()) {
            rxDlgs.first()->activateWindow();
            return;
        }
        ReceiveQslsDialog *dlg = new ReceiveQslsDialog(this, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    });

    addActionBtn(tr("Design QSLs"), [this](){
        QList<QslDesigner*> designers = findChildren<QslDesigner*>();
        if (!designers.isEmpty()) {
            designers.first()->activateWindow();
            return;
        }
        QslDesigner *designer = new QslDesigner(this);
        designer->setAttribute(Qt::WA_DeleteOnClose);
        designer->show();
    });

    addMenuBtn(windowsMenu, tr("Viewports"));

    addMenuBtn(aboutMenu, tr("About"));
    
    lookupLayout->addWidget(rightMenuWidget);

    settings.beginGroup("MainWindow");
    QSize defSize(1106, 497);
    resize(defSize);
    setMinimumSize(924, 325);
    if (QScreen *screen = qApp->primaryScreen()) {
        QPoint centerPoint = screen->availableGeometry().center() - rect().center();
        move(centerPoint);
    }
    if (settings.contains("state")) {
        // State restoration removed since we are no longer a QMainWindow
    }
    settings.endGroup();

    if (m_splash) m_splash->setProgress(25, "Reading Settings...");
    readSettings();
    setWindowTitle("Qwain " + getAppVersion());
    m_textEdit->append("Qwain Ready.");

    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);
    if (m_splash) m_splash->setProgress(35, "Starting Network Servers...");
    restartServer();
    
    if (!m_logfilePath.isEmpty()) {
        if (m_splash) m_splash->setProgress(45, "Loading Logfile...");
        loadLogfile();
    }
    
    QrzCache cache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    cache.beginGroup("icons");
    int iconCount = cache.childKeys().count();
    cache.endGroup();
    
    int totalQsos = 0;
    for (const auto& list : m_allQsos) totalQsos += list.size();
    
    QString magicNumber = QString("%1%2").arg(QRandomGenerator::global()->generate(), 8, 16, QChar('0')).arg(QRandomGenerator::global()->generate(), 9, 16, QChar('0')).right(17);
    QString magicSeed = QString::number(QRandomGenerator::global()->generate64(), 8).right(11);
    
    QString systemLogMsg = QString("system logfile, begins on start\n"
                                   "QSLDesigner %1\n"
                                   "%2\n"
                                   "#icons %3, # contacts %4\n"
                                   "Magic Number: %5\n"
                                   "Magic Seed:  %6")
                           .arg(getAppVersion())
                           .arg(QDateTime::currentDateTimeUtc().toString("dd-MM-yy HH:mm:ss.zzz UTC"))
                           .arg(iconCount)
                           .arg(totalQsos)
                           .arg(magicNumber.toUpper())
                           .arg(magicSeed);
    logSystem(systemLogMsg);
    
    QTimer::singleShot(100, this, [this, statusWindow]() {
        this->raise();
        this->activateWindow();
        
        if (statusWindow->isVisible()) { statusWindow->raise(); statusWindow->activateWindow(); }
        m_lookupEdit->setFocus();
        loadQrzQueueTickFile();
    });
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    performRollingConfigBackup();
    writeSettings();
    event->accept();
    QApplication::quit();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_lookupEdit) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (m_searchMatchesList && m_searchMatchesList->isVisible()) {
                if (keyEvent->key() == Qt::Key_Down) {
                    int nextRow = m_searchMatchesList->currentRow() + 1;
                    if (nextRow >= m_searchMatchesList->count()) nextRow = 0;
                    m_searchMatchesList->setCurrentRow(nextRow);
                    return true;
                } else if (keyEvent->key() == Qt::Key_Up) {
                    int prevRow = m_searchMatchesList->currentRow() - 1;
                    if (prevRow < 0) prevRow = m_searchMatchesList->count() - 1;
                    m_searchMatchesList->setCurrentRow(prevRow);
                    return true;
                } else if (keyEvent->key() == Qt::Key_Escape) {
                    m_searchMatchesList->hide();
                    return true;
                } else if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
                    QListWidgetItem *curItem = m_searchMatchesList->currentItem();
                    if (curItem) {
                        m_lookupEdit->setText(curItem->text());
                        m_searchMatchesList->hide();
                        emit m_lookupEdit->returnPressed();
                        return true;
                    }
                }
            }
        } else if (event->type() == QEvent::FocusOut) {
            QTimer::singleShot(200, this, [this]() {
                if (m_searchMatchesList && !m_searchMatchesList->hasFocus()) {
                    m_searchMatchesList->hide();
                }
            });
        }
    }

    if (m_infoBrowser && watched == m_infoBrowser->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton && m_infoBrowser->anchorAt(me->pos()).isEmpty() && !m_currentDisplayedCallsign.isEmpty()) {
                QApplication::clipboard()->setText(m_currentDisplayedCallsign);
            }
        } else if (event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton && m_infoBrowser->anchorAt(me->pos()).isEmpty() && !m_currentDisplayedCallsign.isEmpty()) {
                QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
                qrzCache.beginGroup(m_currentDisplayedCallsign);
                
                QString name;
                if (qrzCache.contains("qrz_fname") || qrzCache.contains("qrz_name")) {
                    QString fname = qrzCache.value("qrz_fname").toString();
                    QString lname = qrzCache.value("qrz_name").toString();
                    name = fname;
                    if (!name.isEmpty() && !lname.isEmpty()) name += " ";
                    name += lname;
                    
                    QString adifName = qrzCache.value("name").toString();
                    if (fname.isEmpty() && !adifName.isEmpty() && adifName.contains(' ') && !lname.contains(' ')) {
                        name = adifName;
                    }
                }
                if (name.trimmed().isEmpty()) name = qrzCache.value("name").toString();
                
                QString addr1 = qrzCache.value("qrz_addr1").toString();
                
                QString city = qrzCache.value("qrz_city").toString();
                if (city.isEmpty()) city = qrzCache.value("qrz_addr2").toString();
                if (city.isEmpty()) city = qrzCache.value("city").toString();
                
                QString state = qrzCache.value("qrz_state").toString();
                if (state.isEmpty()) state = qrzCache.value("state").toString();
                
                QString zip = qrzCache.value("qrz_zip").toString();
                
                QString country = qrzCache.value("qrz_country").toString();
                if (country.isEmpty()) country = qrzCache.value("country").toString();
                
                qrzCache.endGroup();
                
                QStringList lines;
                lines << m_currentDisplayedCallsign;
                if (!name.trimmed().isEmpty()) lines << name.trimmed();
                if (!addr1.isEmpty()) lines << addr1;
                
                QString csstr;
                if (!city.isEmpty()) csstr += city;
                if (!state.isEmpty()) { if (!csstr.isEmpty()) csstr += ", "; csstr += state; }
                if (!zip.isEmpty()) { if (!csstr.isEmpty()) csstr += " "; csstr += zip; }
                if (!csstr.isEmpty()) lines << csstr;
                
                if (!country.isEmpty() && country.toLower() != "united states" && country.toLower() != "usa") {
                    lines << country;
                }
                
                if (!lines.isEmpty()) {
                    QApplication::clipboard()->setText(lines.join("\n"));
                }
            }
        }
    }
    
    return QWidget::eventFilter(watched, event);
}

void MainWindow::readSettings()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    
    settings.beginGroup("MainWindow");
    
    if (m_lookupEdit) {
        m_lookupEdit->clear();
    }
    
    settings.endGroup();

    settings.beginGroup("Network");
    m_protocol = settings.value("protocol", "TCP").toString();
    m_ipAddress = settings.value("ip", "127.0.0.1").toString();
    m_port = settings.value("port", 2345).toInt();
    settings.endGroup();

    settings.beginGroup("Files");
    m_logfilePath = settings.value("logfile", "").toString();
    if (m_logfilePath.isEmpty()) {
        QDir dataDir(getAppDataDir());
        if (!dataDir.exists()) dataDir.mkpath(".");
        m_logfilePath = dataDir.filePath("Qwain_Active.adi");
    }
    settings.endGroup();

    settings.beginGroup("Credentials");
    m_qrzUsername = settings.value("username", "").toString();
    m_qrzPassword = settings.value("password", "").toString();
    m_qrzXmlSubscriber = settings.value("qrzXmlSubscriber", false).toBool();
    m_qrzSessionKey = settings.value("qrzSessionKey", "").toString();
    QDateTime qrzSessionSaved = settings.value("qrzSessionSaved").toDateTime();
    if (!m_qrzSessionKey.isEmpty() && qrzSessionSaved.isValid()) {
        if (qrzSessionSaved.secsTo(QDateTime::currentDateTime()) > 20 * 3600) {
            m_qrzSessionKey.clear();
        }
    }
    settings.endGroup();

    settings.beginGroup("UI");
    if(settings.contains("clockFont")) m_timeLabel->setFont(qvariant_cast<QFont>(settings.value("clockFont")));
    if(settings.contains("clockColor")) m_timeLabel->setStyleSheet(QString("color: %1").arg(qvariant_cast<QColor>(settings.value("clockColor")).name()));
    settings.endGroup();

    updateMenuLabels();
}

void MainWindow::applyThemeStyle()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString skinColor = settings.value("SkinColor", "#181b22").toString();
    QString scheme = settings.value("UI/systemScheme", "None").toString();
    
    QString stylesheet = "";
    if (scheme == "Dark") {
        stylesheet = "QMainWindow, QDialog { background-color: #444444; }";
    } else {
        stylesheet = QString("QMainWindow { background-color: %1; }").arg(skinColor);
    }
    
    if (scheme == "Light") {
        stylesheet += "QPushButton { "
                      "  background-color: #e2e8f0; "
                      "  color: #0f172a; "
                      "  border: none; "
                      "  border-radius: 4px; "
                      "  padding: 6px 12px; "
                      "} "
                      "QPushButton:hover { "
                      "  background-color: #cbd5e1; "
                      "} "
                      "QPushButton:pressed { "
                      "  background-color: #94a3b8; "
                      "} "
                      "QToolButton { "
                      "  background-color: #f1f5f9; "
                      "  color: #0f172a; "
                      "  border: none; "
                      "  border-radius: 4px; "
                      "  padding: 4px 8px; "
                      "} "
                      "QToolButton:hover { "
                      "  background-color: #cbd5e1; "
                      "} ";
    } else if (scheme == "Dark") {
        stylesheet += "QPushButton { "
                      "  background-color: #444444; "
                      "  color: #f8fafc; "
                      "  border: none; "
                      "  border-radius: 4px; "
                      "  padding: 6px 12px; "
                      "} "
                      "QPushButton:hover { "
                      "  background-color: #555555; "
                      "} "
                      "QPushButton:pressed { "
                      "  background-color: #333333; "
                      "} "
                      "QToolButton { "
                      "  background-color: #444444; "
                      "  color: #f8fafc; "
                      "  border: none; "
                      "  border-radius: 4px; "
                      "  padding: 4px 8px; "
                      "} "
                      "QToolButton:hover { "
                      "  background-color: #555555; "
                      "} ";
    } else if (scheme == "Custom") {
        QColor col(skinColor);
        QString textColor = (col.lightness() > 150) ? "#000000" : "#ffffff";
        QString hoverColor = (col.lightness() > 150) ? col.darker(110).name() : col.lighter(110).name();
        QString pressedColor = (col.lightness() > 150) ? col.darker(120).name() : col.lighter(120).name();
        
        stylesheet += QString(
                      "QPushButton { "
                      "  background-color: %1; "
                      "  color: %2; "
                      "  border: none; "
                      "  border-radius: 4px; "
                      "  padding: 6px 12px; "
                      "} "
                      "QPushButton:hover { "
                      "  background-color: %3; "
                      "} "
                      "QPushButton:pressed { "
                      "  background-color: %4; "
                      "} "
                      "QToolButton { "
                      "  background-color: %1; "
                      "  color: %2; "
                      "  border: none; "
                      "  border-radius: 4px; "
                      "  padding: 4px 8px; "
                      "} "
                      "QToolButton:hover { "
                      "  background-color: %3; "
                      "} "
                      ).arg(skinColor).arg(textColor).arg(hoverColor).arg(pressedColor);
    }
    
    this->setStyleSheet(stylesheet);
}

void MainWindow::restartServer()
{
    if (m_tcpServer && m_tcpServer->isListening()) {
        m_tcpServer->close();
    }
    if (m_udpSocket && m_udpSocket->state() == QAbstractSocket::BoundState) {
        m_udpSocket->close();
    }
    
    QHostAddress addr(m_ipAddress);
    if (m_ipAddress.isEmpty() || m_ipAddress == "0.0.0.0") {
        addr = QHostAddress::Any;
    }

    bool startUdp = (m_protocol == "UDP" || m_protocol.contains("BOTH"));
    bool startTcp = (m_protocol == "TCP" || m_protocol.contains("BOTH"));
    
    bool udpOk = false;
    bool tcpOk = false;

    if (startUdp) {
        if (!m_udpSocket) {
            m_udpSocket = new QUdpSocket(this);
            connect(m_udpSocket, &QUdpSocket::readyRead, this, &MainWindow::onUdpReadyRead);
        }
        if (m_udpSocket->bind(addr, m_port)) {
            m_textEdit->append(QString("Listening on UDP %1:%2").arg(m_ipAddress).arg(m_port));
            udpOk = true;
        } else {
            m_textEdit->append("Failed to bind UDP: " + m_udpSocket->errorString());
        }
    }

    if (startTcp) {
        if (!m_tcpServer) {
            m_tcpServer = new QTcpServer(this);
            connect(m_tcpServer, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);
        }
        if (m_tcpServer->listen(addr, m_port)) {
            m_textEdit->append(QString("Listening on TCP %1:%2").arg(m_ipAddress).arg(m_port));
            tcpOk = true;
        } else {
            m_textEdit->append("Failed to bind TCP: " + m_tcpServer->errorString());
        }
    }

    // Set unified status label text
    if (startUdp && startTcp) {
        if (udpOk && tcpOk) {
            m_statusLabel->setText(QString("Status: Listening on BOTH (UDP & TCP) %1:%2").arg(m_ipAddress).arg(m_port));
        } else {
            m_statusLabel->setText(QString("Status: Network partially bound on %1").arg(m_port));
        }
    } else if (startUdp) {
        if (udpOk) {
            m_statusLabel->setText(QString("Status: Listening on UDP %1:%2").arg(m_ipAddress).arg(m_port));
        } else {
            m_statusLabel->setText("Status: Failed to bind");
        }
    } else if (startTcp) {
        if (tcpOk) {
            m_statusLabel->setText(QString("Status: Listening on TCP %1:%2").arg(m_ipAddress).arg(m_port));
        } else {
            m_statusLabel->setText("Status: Failed to bind");
        }
    }
}

void MainWindow::openConfigDialog()
{
    ConfigDialog dialog(this);
    connect(&dialog, &ConfigDialog::settingsApplied, this, &MainWindow::onSettingsApplied);
    dialog.exec();
}

void MainWindow::onSettingsApplied()
{
    QString oldIp = m_ipAddress;
    int oldPort = m_port;
    QString oldLog = m_logfilePath;
    QString oldProtocol = m_protocol;
    
    readSettings(); // Load the updated settings
    
    if (oldIp != m_ipAddress || oldPort != m_port || oldProtocol != m_protocol) {
        restartServer();
    }
    
    // Use QFileInfo to accurately determine if it's pointing to the exact same file
    QFileInfo oldFi(oldLog);
    QFileInfo newFi(m_logfilePath);
    
    bool pathChanged = true;
    if (oldFi.exists() && newFi.exists()) {
        pathChanged = (oldFi.canonicalFilePath() != newFi.canonicalFilePath());
    } else {
        QString normOld = QDir::cleanPath(oldLog).trimmed();
        QString normNew = QDir::cleanPath(m_logfilePath).trimmed();
#ifdef Q_OS_WIN
        pathChanged = (normOld.compare(normNew, Qt::CaseInsensitive) != 0);
#else
        pathChanged = (normOld.compare(normNew, Qt::CaseSensitive) != 0);
#endif

    }
    
    QDateTime currentMod = newFi.lastModified();
    bool fileContentChanged = m_logLastModified.isValid() && currentMod.isValid() && (currentMod > m_logLastModified);

    if ((pathChanged || fileContentChanged) && !m_logfilePath.isEmpty()) {
        m_textEdit->append(QString("Debug: Logfile changed or modified."));
        QMessageBox *msgBox = new QMessageBox(QMessageBox::Information, "Checking", "Performing Logfile Sanity Check...", QMessageBox::NoButton, this);
        msgBox->setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        msgBox->show();
        QCoreApplication::processEvents();
        
        loadLogfile();
        
        msgBox->accept();
        msgBox->deleteLater();
    }
    
    applyThemeStyle();
    
    updateMenuLabels();
}

// [ENTRY POINT: UDP Network Packet Callback]
// Triggered on incoming live datagrams (e.g. from WSJT-X or N1MM).
// Extracts raw bytes, appends them to m_receiveBuffer, and executes the ADIF parsing loop.
void MainWindow::onUdpReadyRead()
{
    while (m_udpSocket && m_udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        QHostAddress senderAddr;
        quint16 senderPort = 0;
        datagram.resize(m_udpSocket->pendingDatagramSize());
        m_udpSocket->readDatagram(datagram.data(), datagram.size(), &senderAddr, &senderPort);
        QString dataStr = QString::fromUtf8(datagram);
        m_receiveBuffer += dataStr;
        emit dataReceived("UDP", senderAddr.toString(), senderPort, dataStr);
    }
    
    m_statusLabel->setText("Status: Receiving UDP data...");
    
    while (true) {
        int eorIdx = m_receiveBuffer.indexOf("<EOR>", 0, Qt::CaseInsensitive);

        if (eorIdx != -1) {
            QString record = m_receiveBuffer.left(eorIdx + 5);
            
            // If the record contains a header, discard everything up to EOH
            int eohIdx = record.indexOf("<EOH>", 0, Qt::CaseInsensitive);
            if (eohIdx != -1) {
                record.remove(0, eohIdx + 5);
            }
            
            m_receiveBuffer.remove(0, eorIdx + 5);
            processAdifRecord(record, false, true, true);
        } else {
            if (m_receiveBuffer.length() > 50000) {
                m_receiveBuffer.clear();
            }
            break;
        }
    }
}

void MainWindow::onNewConnection()
{
    QTcpSocket *clientSocket = m_tcpServer->nextPendingConnection();
    m_clients.append(clientSocket);
    m_textEdit->append(QString("Client connected: %1:%2").arg(clientSocket->peerAddress().toString()).arg(clientSocket->peerPort()));

    connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &MainWindow::onClientDisconnected);
}

// [ENTRY POINT: TCP Connection Packet Callback]
// Triggered on incoming client socket data streams (e.g. from third-party loggers).
// Reads available data, segments it by <EOR> records, commits raw logs to disk, and dispatches to processAdifRecord().
void MainWindow::onReadyRead()
{
    m_statusLabel->setText("Status: Receiving data...");
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    QByteArray data = clientSocket->readAll();
    QString dataStr = QString::fromUtf8(data);
    m_receiveBuffer += dataStr;
    
    emit dataReceived("TCP", clientSocket->peerAddress().toString(), clientSocket->peerPort(), dataStr);

    while (true) {
        int eorIdx = m_receiveBuffer.indexOf("<EOR>", 0, Qt::CaseInsensitive);

        if (eorIdx != -1) {
            QString record = m_receiveBuffer.left(eorIdx + 5);
            
            // If the record contains a header, discard everything up to EOH
            int eohIdx = record.indexOf("<EOH>", 0, Qt::CaseInsensitive);
            if (eohIdx != -1) {
                record.remove(0, eohIdx + 5);
            }
            
            m_receiveBuffer.remove(0, eorIdx + 5);
            
            m_textEdit->append(QString("\n[PARSED RECORD] =>\n%1").arg(record.toHtmlEscaped()));
            processAdifRecord(record);

            // Append to configuration logfile ONLY
            QSettings settings(QCoreApplication::organizationName(), "Qwain");
            settings.beginGroup("Files");
            QString configLogPath = settings.value("logfile", "").toString();
            if (configLogPath.isEmpty()) {
                QDir dataDir(getAppDataDir());
                if (!dataDir.exists()) dataDir.mkpath(".");
                configLogPath = dataDir.filePath("Qwain_Active.adi");
            }
            settings.endGroup();

            QFile file(configLogPath);
            if (file.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream out(&file);
                out << record << "\n";
                file.close();
                m_textEdit->append(QString("-> Appended to persistent config logfile ONLY: %1").arg(configLogPath));
            } else {
                m_textEdit->append(QString("-> ERROR: Could not open config logfile %1").arg(configLogPath));
            }
        } else {
            if (m_receiveBuffer.length() > 50000) {
                m_receiveBuffer.clear();
            }
            break;
        }
    }
}

QString MainWindow::extractAdifField(const QString &record, const QString &field) {
    int idx = record.indexOf("<" + field + ":", 0, Qt::CaseInsensitive);
    if (idx != -1) {
        int endTag = record.indexOf(">", idx);
        if (endTag != -1) {
            int colon = record.indexOf(":", idx + 1);
            if (colon != -1 && colon < endTag) {
                int lenEnd = record.indexOf(":", colon + 1);
                QString lenStr;
                if (lenEnd != -1 && lenEnd < endTag) {
                    lenStr = record.mid(colon + 1, lenEnd - colon - 1);
                } else {
                    lenStr = record.mid(colon + 1, endTag - colon - 1);
                }
                int len = lenStr.toInt();
                return record.mid(endTag + 1, len).trimmed();
            }
        }
    }
    
    // Fallback: Check for <FIELD>value<... or <FIELD>value</FIELD>
    idx = record.indexOf("<" + field + ">", 0, Qt::CaseInsensitive);
    if (idx != -1) {
        int endTag = idx + field.length() + 2; // length of <FIELD>
        int nextTag = record.indexOf("<", endTag);
        if (nextTag != -1) {
            return record.mid(endTag, nextTag - endTag).trimmed();
        } else {
            return record.mid(endTag).trimmed();
        }
    }
    
    return QString();
}

void MainWindow::trackQsoStats(const QsoDetail &d_in)
{
    QsoDetail d = d_in;
    if (!d.country.isEmpty()) {
        m_dxccWorked.insert(d.country);
        if (!d.band.isEmpty() && !d.mode.isEmpty()) {
            m_dxccDetailWorked.insert(d.band + "_" + d.mode + "_" + d.country);
        }
    }
    if (!d.dxcc.isEmpty()) {
        m_dxccWorked.insert("DXCC_" + d.dxcc);
        if (!d.band.isEmpty() && !d.mode.isEmpty()) {
            m_dxccDetailWorked.insert(d.band + "_" + d.mode + "_DXCC_" + d.dxcc);
        }
    }

    if (!d.grid.isEmpty()) {
        QString g = d.grid.trimmed().toUpper();
        if (g.length() >= 4) {
            m_gridsWorked.insert(g.left(4));
        }
    }

    if (d.country == "ALASKA" || d.dxcc == "6") {
        if (d.state.isEmpty() || d.state.toUpper() == "ALASKA") d.state = "AK";
    }
    else if (d.country == "HAWAII" || d.dxcc == "110") {
        if (d.state.isEmpty() || d.state.toUpper() == "HAWAII") d.state = "HI";
    }

    if (d.country == "UNITED STATES OF AMERICA" || d.country == "UNITED STATES" || d.country == "USA" || d.dxcc == "291" || 
        d.country == "ALASKA" || d.dxcc == "6" || 
        d.country == "HAWAII" || d.dxcc == "110") {
        if (!d.state.isEmpty()) {
            if (d.band == "80M" || d.band == "40M" || d.band == "20M" || d.band == "15M" || d.band == "10M") {
                m_was5BandWorked.insert(d.band + "_" + d.state);
            }
            if (!d.band.isEmpty() && !d.mode.isEmpty()) {
                m_wasDetailWorked.insert(d.band + "_" + d.mode + "_" + d.state);
            }
        }
        if (!d.county.isEmpty()) {
            QString c = d.county.toUpper().trimmed();
            if (!c.contains(",") && !d.state.isEmpty()) {
                c = d.state.toUpper() + "," + c;
            }
            c.replace(", ", ",");
            c.remove(QRegularExpression("\\s+(COUNTY|PARISH|BOROUGH|CENSUS AREA|CITY|MUNICIPALITY)$", QRegularExpression::CaseInsensitiveOption));
            m_usCountiesWorked.insert(c.trimmed());
        }
    } else if (d.country == "JAPAN" || d.dxcc == "339") {
        QString wajaState = d.state;
        if (wajaState.isEmpty()) {
            if (d.grid.length() >= 4) {
                wajaState = getJapanPrefectureFromGrid(d.grid);
            } else if (!d.lat.isEmpty() && !d.lon.isEmpty()) {
                bool okLat, okLon;
                double latD = d.lat.toDouble(&okLat);
                double lonD = d.lon.toDouble(&okLon);
                if (okLat && okLon) {
                    wajaState = getJapanPrefectureFromLatLon(latD, lonD);
                }
            }
        }
        
        if (!wajaState.isEmpty()) {
            m_japanPrefecturesWorked.insert(wajaState);
            if (!d.band.isEmpty() && !d.mode.isEmpty()) {
                m_japanDetailWorked.insert(d.band + "_" + d.mode + "_" + wajaState);
            }
        }
        if (!d.city.isEmpty()) {
            m_japanCitiesWorked.insert(d.city);
        }
    }
    
    if (d.grid.length() >= 4) {
        m_gridQsos[d.grid.left(4)][d.call].append(d);
    }
    
    if (!d.pota.isEmpty()) {
        m_potaWorked.insert(d.pota);
        if (!d.band.isEmpty() && !d.mode.isEmpty()) {
            m_potaDetailWorked.insert(d.band + "_" + d.mode + "_" + d.pota);
        }
        m_potaQsos.append(d);
    }
}

void MainWindow::updateClock()
{
    QDateTime now = QDateTime::currentDateTimeUtc();
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    if (settings.value("UI/alwaysEastern", false).toBool()) {
        QTimeZone easternTz("America/New_York");
        if (easternTz.isValid()) {
            QDateTime easternNow = now.toTimeZone(easternTz);
            m_timeLabel->setText(easternNow.toString("HH:mm:ss") + " " + easternTz.abbreviation(easternNow));
            m_dateLabel->setText(easternNow.toString("dd/MM/yyyy"));
        } else {
            QDateTime estNow = now.toOffsetFromUtc(-5 * 3600);
            m_timeLabel->setText(estNow.toString("HH:mm:ss") + " EST");
            m_dateLabel->setText(estNow.toString("dd/MM/yyyy"));
        }
    } else {
        m_timeLabel->setText(now.toString("HH:mm:ss") + " UTC");
        m_dateLabel->setText(now.toString("dd/MM/yyyy"));
    }
}

void MainWindow::updateQsoCount()
{
    int count = m_uniqueCallsigns.size();
    m_qsoCountLabel->setText(QString("QSOs: %1").arg(count));
}

QString MainWindow::formatElapsedMs(qint64 elapsedMs)
{
    return QString("%1 ms").arg(elapsedMs, 4, 10, QChar('0'));
}

// [ENTRY POINT: Unified ADIF Record Parser & Pipeline]
// Decodes a raw ADIF string, extracts geographic and biographical fields,
// updates local real-time award statistics/leaderboards, and spools lookups.
// Inputs:
//   record - The raw bracketed ADIF string.
//   isManualLookup - True if triggered by callsign search rather than a live/imported log.
//   updateDisplay - True to immediately update GUI labels and globe views.
void MainWindow::processAdifRecord(const QString &record, bool isManualLookup, bool updateDisplay, bool isUdp) {
    qint64 startMs = QDateTime::currentMSecsSinceEpoch();
    
    QString call = extractAdifField(record, "CALL").toUpper();
    if (call.isEmpty()) {
        m_statusLabel->setText("Status: Waiting");
        return; 
    }

    bool isNew = !m_uniqueCallsigns.contains(call);
    if (!isManualLookup && isNew && qobject_cast<QTcpSocket*>(sender())) { 
        // Only blink if data is coming from the socket (live), not during loadLogfile indirectly
        m_newQsoLabel->show();
        m_newQsoAnim->start();
        QTimer::singleShot(4000, this, [this](){
            m_newQsoLabel->hide();
            m_newQsoAnim->stop();
        });
    }

    if (!isManualLookup) {
        m_uniqueCallsigns.insert(call);
        updateQsoCount();
    }

    QString grid = extractAdifField(record, "GRIDSQUARE").toUpper();
    QString comment = extractAdifField(record, "COMMENT").toUpper();
    QString potaRef = extractAdifField(record, "POTA_REF").toUpper();
    if (potaRef.isEmpty()) potaRef = extractAdifField(record, "POTA_ID").toUpper();
    if (potaRef.isEmpty()) potaRef = extractAdifField(record, "APP_QRZLOG_POTA_REF").toUpper();
    if (potaRef.isEmpty() && extractAdifField(record, "SIG").toUpper() == "POTA") {
        potaRef = extractAdifField(record, "SIG_INFO").toUpper();
    }
    
    
    QString potaFound;
    if (!potaRef.isEmpty()) potaFound = potaRef;
    else {
        QString fullRecordUpper = record.toUpper();
        QString qthUpper = extractAdifField(record, "QTH").toUpper();
        if (qthUpper.isEmpty()) qthUpper = extractAdifField(record, "APP_QRZLOG_QTH").toUpper();
        QString commentUpper = extractAdifField(record, "COMMENT").toUpper();
        
        QRegularExpression reStrict("(?:POTA|PARK)[\\s\\-:]*?([A-Z]{1,4}-?\\d{3,5})");
        QRegularExpression reLoose("\\b([A-Z]{1,4}-\\d{3,5})\\b");
        
        QRegularExpressionMatch mStrict = reStrict.match(fullRecordUpper);
        if (mStrict.hasMatch()) {
            QString matchStr = mStrict.captured(1);
            if (!matchStr.contains("-")) {
                int pos = 0;
                while (pos < matchStr.length() && !matchStr[pos].isDigit()) pos++;
                if (pos > 0 && pos < matchStr.length()) matchStr.insert(pos, "-");
            }
            potaFound = matchStr;
        } else {
            QRegularExpressionMatch mQth = reLoose.match(qthUpper);
            if (mQth.hasMatch() && (qthUpper.contains("POTA") || qthUpper.contains("PARK"))) {
                potaFound = mQth.captured(1);
            } else {
                QRegularExpressionMatch mComment = reLoose.match(commentUpper);
                if (mComment.hasMatch() && (commentUpper.contains("POTA") || commentUpper.contains("PARK"))) {
                    potaFound = mComment.captured(1);
                }
            }
        }
        
        if (!potaFound.isEmpty()) {
            QString prefix = potaFound.section('-', 0, 0).toUpper();
            if (prefix == "NA" || prefix == "SA" || prefix == "EU" || prefix == "AS" || prefix == "AF" || prefix == "OC" || prefix == "AN") {
                potaFound.clear();
            }
        }
    }

    QsoDetail d;
    d.date = extractAdifField(record, "QSO_DATE");
    d.time = extractAdifField(record, "TIME_ON");
    QString freqStr = extractAdifField(record, "FREQ");
    if (!freqStr.isEmpty()) {
        bool ok;
        double f = freqStr.toDouble(&ok);
        if (ok) d.freq = QString::number(f, 'f', 4);
        else d.freq = freqStr;
    }
    d.band = extractAdifField(record, "BAND").toUpper();
    d.mode = extractAdifField(record, "MODE").toUpper();
    {
        QString sub = extractAdifField(record, "SUBMODE").toUpper();
        if (!sub.isEmpty()) {
            d.mode = sub;
        }
    }
    d.call = call;
    d.country = extractAdifField(record, "COUNTRY").toUpper();
    d.state = extractAdifField(record, "STATE").toUpper();
    d.dxcc = extractAdifField(record, "DXCC");
    d.pota = potaFound;
    d.grid = grid;
    d.lat = extractAdifField(record, "LAT");
    d.lon = extractAdifField(record, "LON");
    if (d.lat.isEmpty()) d.lat = extractAdifField(record, "APP_QRZLOG_LAT");
    if (d.lon.isEmpty()) d.lon = extractAdifField(record, "APP_QRZLOG_LON");
    
    d.rst_rcvd = extractAdifField(record, "RST_RCVD");
    d.rst_sent = extractAdifField(record, "RST_SENT");
    d.tx_pwr = extractAdifField(record, "TX_PWR");
    d.qslsdate = extractAdifField(record, "QSLSDATE");
    d.qslrdate = extractAdifField(record, "QSLRDATE");
    d.qsl_sent = extractAdifField(record, "QSL_SENT");
    d.qsl_rcvd = extractAdifField(record, "QSL_RCVD");
    d.qslVia = extractAdifField(record, "QSL_VIA");
    d.qslMsg = extractAdifField(record, "QSLMSG");
    
    QString lotwRx = extractAdifField(record, "APP_LOTW_RXQSL");
    d.lotwRcvd = extractAdifField(record, "APP_LOTW_QSO_RCVD").toUpper() == "Y" || extractAdifField(record, "LOTW_QSL_RCVD").toUpper() == "Y" || (!lotwRx.isEmpty() && lotwRx != "00000000");
    d.mqslRcvd = extractAdifField(record, "QSL_RCVD").toUpper() == "Y";
    d.qrzRcvd = extractAdifField(record, "APP_QRZLOG_STATUS").toUpper() == "C" || extractAdifField(record, "QRZCOM_QSL_RCVD").toUpper() == "Y" || extractAdifField(record, "QRZCOM_QSL_RCVD").toUpper() == "C" || extractAdifField(record, "APP_QRZ_QSO_RCVD").toUpper() == "Y";
    d.cardRcvd = (d.qsl_rcvd.toUpper() == "Y" || !d.qslrdate.isEmpty());

    
    d.name = extractAdifField(record, "NAME");
    d.address = extractAdifField(record, "ADDRESS");
    if (d.address.isEmpty()) d.address = extractAdifField(record, "ADDR1");
    if (d.address.isEmpty()) d.address = extractAdifField(record, "ADDR2");
    d.city = extractAdifField(record, "CITY");
    d.zip = extractAdifField(record, "ZIP");
    if (d.zip.isEmpty()) d.zip = extractAdifField(record, "POSTAL_CODE");
    
    d.county = extractAdifField(record, "CNTY");

    if (!isManualLookup) {
        if (d.country.isEmpty() || d.dxcc.isEmpty()) {
            if (m_allQsos.contains(call)) {
                for (const auto &q : m_allQsos[call]) {
                    if (d.country.isEmpty() && !q.country.isEmpty()) d.country = q.country;
                    if (d.state.isEmpty() && !q.state.isEmpty()) d.state = q.state;
                    if (d.dxcc.isEmpty() && !q.dxcc.isEmpty()) d.dxcc = q.dxcc;
                    if (d.county.isEmpty() && !q.county.isEmpty()) d.county = q.county;
                    if (d.name.isEmpty() && !q.name.isEmpty()) d.name = q.name;
                    if (d.address.isEmpty() && !q.address.isEmpty()) d.address = q.address;
                    if (d.city.isEmpty() && !q.city.isEmpty()) d.city = q.city;
                    if (d.zip.isEmpty() && !q.zip.isEmpty()) d.zip = q.zip;
                }
            }
        }
        if (d.country.isEmpty() || d.dxcc.isEmpty() || d.state.isEmpty()) {
            QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
            qrzCache.beginGroup(call);
            if (d.country.isEmpty()) {
                d.country = qrzCache.value("country").toString().toUpper();
                if (d.country.isEmpty()) d.country = qrzCache.value("qrz_country").toString().toUpper();
            }
            if (d.state.isEmpty()) d.state = qrzCache.value("qrz_state").toString().toUpper();
            if (d.dxcc.isEmpty()) d.dxcc = qrzCache.value("dxcc").toString();
            if (d.county.isEmpty()) d.county = qrzCache.value("county").toString();
            if (d.name.isEmpty()) d.name = qrzCache.value("name").toString();
            if (d.address.isEmpty()) d.address = qrzCache.value("address").toString();
            if (d.city.isEmpty()) d.city = qrzCache.value("city").toString();
            if (d.zip.isEmpty()) d.zip = qrzCache.value("zip").toString();
            qrzCache.endGroup();
        }
    }

    if (!isManualLookup) {
        logSystem(QDateTime::currentDateTimeUtc().toString("dd-MM-yy HH:mm:ss.zzz UTC") + " " + call + " QSO rx'd");
        m_allQsos[call].append(d);
        trackQsoStats(d);
    }
    
    updateQsoCount();
    updateGlobe();
    if (!isManualLookup) {
        
    }
    
    QString country = extractAdifField(record, "COUNTRY");

    bool adifLotw = extractAdifField(record, "APP_LOTW_QSO_RCVD").toUpper() == "Y" || extractAdifField(record, "LOTW_QSL_RCVD").toUpper() == "Y" || (!lotwRx.isEmpty() && lotwRx != "00000000");

    bool adifMqsl = extractAdifField(record, "QSL_RCVD").toUpper() == "Y";

    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    qrzCache.beginGroup(call);
    if (qrzCache.contains("htmlInfo") && qrzCache.contains("qrz_name")) {
        m_textEdit->append(QString("-> Initializing instantly from cache for %1").arg(call));
        QString htmlInfo = qrzCache.value("htmlInfo").toString();
        QString sanitized = sanitizeHtmlInfo(htmlInfo);
        if (sanitized != htmlInfo) {
            htmlInfo = sanitized;
            qrzCache.setValue("htmlInfo", htmlInfo);
            qrzCache.sync();
        }
        // Remove legacy fields like LOTW and Mail QSL from cached htmlInfo
        htmlInfo.remove(QRegularExpression("<br><b>(?:Mail QSL|LOTW):</b>.*?((?=<br>)|$)"));
        
        QByteArray imgData = qrzCache.value("imgData").toByteArray();
        bool lotw = qrzCache.value("lotw", false).toBool() || adifLotw;
        bool mqsl = qrzCache.value("mqsl", false).toBool() || adifMqsl;
        QString cachedCountry = qrzCache.value("country").toString();
        if (cachedCountry.isEmpty()) {
            QStringList parts = htmlInfo.split("<br>");
            if (parts.size() >= 3) {
                cachedCountry = parts.last().trimmed();
                // Strip tags if any, though it shouldn't have them
                cachedCountry.remove(QRegularExpression("<[^>]*>"));
                qrzCache.setValue("country", cachedCountry); // Backfill it
                qrzCache.sync();
            }
        }

        if (country.isEmpty() && !cachedCountry.isEmpty()) {
            country = cachedCountry;
        }
        
        qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - startMs;
        QString timeInfo = QString("<br><br><small>Fetched in %1 (from local cache)</small>").arg(formatElapsedMs(elapsedMs));
        
        if (updateDisplay) {
            displayCallInfo(htmlInfo + timeInfo, imgData, call, potaFound, country, lotw, mqsl);
        }
        qrzCache.endGroup();
        return;
    }
    qrzCache.endGroup();
    
    QString name = extractAdifField(record, "NAME");
    QString addr2 = extractAdifField(record, "ADDRESS");
    if (addr2.isEmpty()) addr2 = extractAdifField(record, "ADDR2");
    QString city = extractAdifField(record, "CITY");
    QString state = d.state;
    QString zip = extractAdifField(record, "ZIP");
    if (zip.isEmpty()) zip = extractAdifField(record, "POSTAL_CODE");

    // Show provisional data immediately while fetching
    QString tempCityState = (!city.isEmpty() && !state.isEmpty()) ? QString("%1, %2").arg(city, state) : city + state;
    if (!zip.isEmpty()) tempCityState += " " + zip;
    QString tempHtmlInfo = QString("<h2>%1</h2><table border='0' cellspacing='0' cellpadding='2'>"
                                   "<tr><td align='right'><b>Name:</b></td><td align='left'>%2</td></tr>"
                                   "<tr><td align='right'><b>Address:</b></td><td align='left'>%3</td></tr>"
                                   "<tr><td align='right'><b>City:</b></td><td align='left'>%4</td></tr>"
                                   "<tr><td align='right'><b>Country:</b></td><td align='left'>%5</td></tr>"
                                   "</table>")
                           .arg(call.toHtmlEscaped(), name.toHtmlEscaped(), addr2.toHtmlEscaped(), tempCityState.toHtmlEscaped(), country.toHtmlEscaped());
    if (updateDisplay) {
        displayCallInfo(tempHtmlInfo + "<br><br><small>Fetching details...</small>", QByteArray(), call, potaFound, country, adifLotw, adifMqsl);
    }

    if (m_qrzXmlSubscriber && m_qrzSessionKey.isEmpty() && !m_qrzUsername.isEmpty() && !m_qrzPassword.isEmpty()) {
        QString urlStr = QString("https://xmldata.qrz.com/xml/current/?username=%1;password=%2;agent=Qwain")
                             .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_qrzUsername)))
                             .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_qrzPassword)));
        QNetworkRequest req((QUrl(urlStr)));
        QNetworkReply *reply = m_networkManager->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, call, name, addr2, city, state, zip, country, startMs, potaFound, adifLotw, adifMqsl, updateDisplay, isUdp]() {
            reply->deleteLater();
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray xmlData = reply->readAll();
                QXmlStreamReader xml(xmlData);
                while (!xml.atEnd() && !xml.hasError()) {
                    if (xml.readNext() == QXmlStreamReader::StartElement && xml.name().toString() == "Key") {
                        m_qrzSessionKey = xml.readElementText();
                        writeSettings();
                        break;
                    }
                }
            }
            if (!m_qrzSessionKey.isEmpty()) {
                doQrzLookup(call, name, addr2, city, state, zip, country, startMs, potaFound, adifLotw, adifMqsl, updateDisplay, isUdp);
            } else {
                doFreeLookup(call, name, addr2, city, state, zip, country, startMs, potaFound, adifLotw, adifMqsl, updateDisplay, isUdp);
            }
        });
    } else if (m_qrzXmlSubscriber && !m_qrzSessionKey.isEmpty()) {
        doQrzLookup(call, name, addr2, city, state, zip, country, startMs, potaFound, adifLotw, adifMqsl, updateDisplay, isUdp);
    } else {
        doFreeLookup(call, name, addr2, city, state, zip, country, startMs, potaFound, adifLotw, adifMqsl, updateDisplay, isUdp);
    }
    
    
}

void MainWindow::doFreeLookup(const QString &callsign, const QString &adifName, const QString &adifAddr2, const QString &adifCity, const QString &adifState, const QString &adifZip, const QString &adifCountry, qint64 startMs, const QString &pota, bool adifLotw, bool adifMqsl, bool updateDisplay, bool isUdp, const QString &originalCall) {
    QString urlStr = QString("https://api.hamdb.org/v1/%1/xml/Qwain").arg(callsign);
    QNetworkRequest req((QUrl(urlStr)));
    QNetworkReply *reply = m_networkManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callsign, adifName, adifAddr2, adifCity, adifState, adifZip, adifCountry, startMs, pota, adifLotw, adifMqsl, updateDisplay, isUdp, originalCall]() {
        reply->deleteLater();
        QString dbFname, dbLastName, dbName, dbAddr1, dbAddr2, dbState, dbZip, dbCountry, dbGrid;
        bool lotw = adifLotw;
        bool mqsl = adifMqsl;
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray xmlData = reply->readAll();
            QXmlStreamReader xml(xmlData);
            while (!xml.atEnd() && !xml.hasError()) {
                if (xml.readNext() == QXmlStreamReader::StartElement) {
                    QString tag = xml.name().toString();
                    if (tag == "name") {
                        dbName = xml.readElementText();
                    } else if (tag == "fname") {
                        dbFname = xml.readElementText();
                    } else if (tag == "addr1") {
                        dbAddr1 = xml.readElementText();
                    } else if (tag == "addr2") {
                        dbAddr2 = xml.readElementText();
                    } else if (tag == "state") {
                        dbState = xml.readElementText();
                    } else if (tag == "zip") {
                        dbZip = xml.readElementText();
                    } else if (tag == "country") {
                        dbCountry = xml.readElementText();
                    } else if (tag == "grid") {
                        dbGrid = xml.readElementText();
                    }
                }
            }
        }
        
        bool lookupFailed = (reply->error() != QNetworkReply::NoError) || (dbName.isEmpty() && dbState.isEmpty());
        bool canFallback = false;
        QString baseCall = callsign;
        if (isUdp && originalCall.isEmpty()) {
            int slashIdx = callsign.lastIndexOf('/');
            if (slashIdx != -1) {
                baseCall = callsign.left(slashIdx).trimmed();
                QString suffix = callsign.mid(slashIdx + 1).trimmed().toUpper();
                bool isInt = false;
                suffix.toInt(&isInt);
                if (isInt || suffix == "P" || suffix == "M") {
                    canFallback = true;
                }
            }
        }

        if (lookupFailed && canFallback) {
            m_textEdit->append(QString("<span style='color:orange;'>UDP lookup for '%1' failed. Retrying with base callsign '%2'...</span>").arg(callsign, baseCall));
            if (!m_qrzSessionKey.isEmpty()) {
                doQrzLookup(baseCall, adifName, adifAddr2, adifCity, adifState, adifZip, adifCountry, startMs, pota, adifLotw, adifMqsl, updateDisplay, false, callsign);
            } else {
                doFreeLookup(baseCall, adifName, adifAddr2, adifCity, adifState, adifZip, adifCountry, startMs, pota, adifLotw, adifMqsl, updateDisplay, false, callsign);
            }
            return;
        }

        QString finalFname = dbFname;
        QString finalLname = dbName;
        QString dbFullName;
        if (!finalFname.isEmpty() && !finalLname.isEmpty()) {
            dbFullName = finalFname + " " + finalLname;
        } else if (!finalLname.isEmpty()) {
            dbFullName = finalLname;
        } else if (!finalFname.isEmpty()) {
            dbFullName = finalFname;
        }

        QString finalName = !dbFullName.isEmpty() ? dbFullName : adifName;
        QString finalAddr2 = !dbAddr1.isEmpty() ? dbAddr1 : adifAddr2;
        QString finalCity = !dbAddr2.isEmpty() ? dbAddr2 : adifCity;
        QString finalState = !dbState.isEmpty() ? dbState : adifState;
        QString finalZip = !dbZip.isEmpty() ? dbZip : adifZip;
        QString finalCountry = !dbCountry.isEmpty() ? dbCountry : adifCountry;

        QString displayCall = !originalCall.isEmpty() ? originalCall : callsign;

        QString cityState = (!finalCity.isEmpty() && !finalState.isEmpty()) ? QString("%1, %2").arg(finalCity, finalState) : finalCity + finalState;
        if (!finalZip.isEmpty()) cityState += " " + finalZip;
        
        QString htmlInfo = QString("<h2>%1</h2><table border='0' cellspacing='0' cellpadding='2'>"
                                   "<tr><td align='right'><b>Name:</b></td><td align='left'>%2</td></tr>"
                                   "<tr><td align='right'><b>Address:</b></td><td align='left'>%3</td></tr>"
                                   "<tr><td align='right'><b>City:</b></td><td align='left'>%4</td></tr>"
                                   "<tr><td align='right'><b>Country:</b></td><td align='left'>%5</td></tr>"
                                   "</table>")
                               .arg(displayCall.toHtmlEscaped(), finalName.toHtmlEscaped(), finalAddr2.toHtmlEscaped(), cityState.toHtmlEscaped(), finalCountry.toHtmlEscaped());

        if (!dbGrid.isEmpty()) htmlInfo += QString("<br><b>Grid:</b> %1").arg(dbGrid);

        QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
        QStringList groupsToWrite;
        groupsToWrite << callsign;
        if (!originalCall.isEmpty() && originalCall != callsign) {
            groupsToWrite << originalCall;
        }
        for (const QString &g : groupsToWrite) {
            qrzCache.beginGroup(g);
            qrzCache.setValue("htmlInfo", htmlInfo);
            qrzCache.setValue("qrz_fname", finalFname);
            qrzCache.setValue("qrz_name", finalLname.isEmpty() ? finalName : finalLname);
            qrzCache.setValue("country", finalCountry);
            qrzCache.setValue("lotw", lotw);
            qrzCache.setValue("mqsl", mqsl);
            qrzCache.setValue("qrz_addr1", finalAddr2);
            qrzCache.setValue("qrz_addr2", dbAddr2);
            qrzCache.setValue("qrz_city", finalCity);
            qrzCache.setValue("qrz_state", finalState);
            qrzCache.setValue("qrz_zip", finalZip);
            qrzCache.setValue("qrz_country", finalCountry);
            qrzCache.setValue("grid", dbGrid);
            qrzCache.endGroup();
        }
        qrzCache.sync();

        qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - startMs;
        QString timeInfo = QString("<br><br><small>Fetched in %1 (from HamDB)</small>").arg(formatElapsedMs(elapsedMs));

        if (updateDisplay) {
            displayCallInfo(htmlInfo + timeInfo, QByteArray(), displayCall, pota, finalCountry, lotw, mqsl);
        } else {
            bool success = (reply->error() == QNetworkReply::NoError);
            if (success) {
                logQueueProgress(QString("HamDB SUCCESS for %1").arg(callsign));
                m_qrzQueueIntervalMs = qMax(50, m_qrzQueueIntervalMs - 50);
            } else {
                logQueueProgress(QString("HamDB FAILED for %1: %2").arg(callsign, reply->errorString()));
                m_qrzQueueIntervalMs = qMin(10000, m_qrzQueueIntervalMs + 100);
                m_textEdit->append(QString("<span style='color: #c0392b;'>[Queue Throttling] HamDB Error! Slowing down pacing to %1 ms.</span>").arg(m_qrzQueueIntervalMs));
            }
            if (m_qrzQueueTimer) m_qrzQueueTimer->setInterval(m_qrzQueueIntervalMs);
        }
    });
}

void MainWindow::doQrzLookup(const QString &callsign, const QString &adifName, const QString &adifAddr2, const QString &adifCity, const QString &adifState, const QString &adifZip, const QString &adifCountry, qint64 startMs, const QString &pota, bool adifLotw, bool adifMqsl, bool updateDisplay, bool isUdp, const QString &originalCall) {
    QString urlStr = QString("https://xmldata.qrz.com/xml/current/?s=%1;callsign=%2").arg(m_qrzSessionKey).arg(callsign);
    QNetworkRequest req((QUrl(urlStr)));
    QNetworkReply *reply = m_networkManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, callsign, adifName, adifAddr2, adifCity, adifState, adifZip, adifCountry, startMs, pota, adifLotw, adifMqsl, updateDisplay, isUdp, originalCall]() {
        reply->deleteLater();
        QString imgUrl;
        QString qrzFname, qrzLastName, qrzAddr1, qrzAddr2, qrzCity, qrzState, qrzZip, qrzCountry, qrzCounty, qrzGrid, qrzDxcc, qrzLat, qrzLon, qrzQslmgr;
        bool hasError = false;
        QString errorString;
        bool lotw = adifLotw;
        bool mqsl = adifMqsl;
        QByteArray xmlData;
        if (reply->error() == QNetworkReply::NoError) {
            xmlData = reply->readAll();
            QXmlStreamReader xml(xmlData);
            while (!xml.atEnd() && !xml.hasError()) {
                if (xml.readNext() == QXmlStreamReader::StartElement) {
                    QString tag = xml.name().toString();
                    if (tag == "image") {
                        imgUrl = xml.readElementText();
                    } else if (tag == "fname") {
                        qrzFname = xml.readElementText();
                    } else if (tag == "name") {
                        qrzLastName = xml.readElementText();
                    } else if (tag == "addr1") {
                        qrzAddr1 = xml.readElementText();
                    } else if (tag == "addr2") {
                        qrzAddr2 = xml.readElementText();
                    } else if (tag == "city") {
                        qrzCity = xml.readElementText();
                    } else if (tag == "state") {
                        qrzState = xml.readElementText();
                    } else if (tag == "zip") {
                        qrzZip = xml.readElementText();
                    } else if (tag == "country") {
                        qrzCountry = xml.readElementText();
                    } else if (tag == "county") {
                        qrzCounty = xml.readElementText();
                    } else if (tag == "grid") {
                        qrzGrid = xml.readElementText();
                    } else if (tag == "dxcc") {
                        qrzDxcc = xml.readElementText();
                    } else if (tag == "lat") {
                        qrzLat = xml.readElementText();
                    } else if (tag == "lon") {
                        qrzLon = xml.readElementText();
                    } else if (tag == "lotw") {
                        if (xml.readElementText() == "1") lotw = true;
                    } else if (tag == "mqsl") {
                        if (xml.readElementText() == "1") mqsl = true;
                    } else if (tag == "qslmgr") {
                        qrzQslmgr = xml.readElementText();
                    } else if (tag.compare("Error", Qt::CaseInsensitive) == 0) {
                        errorString = xml.readElementText();
                        hasError = true;
                    } else if (tag != "QRZDatabase" && tag != "Callsign" && tag != "Session") {
                        xml.skipCurrentElement();
                    }
                }
            }
        }
        
        if (reply->error() != QNetworkReply::NoError) {
            if (!updateDisplay) {
                logQueueProgress(QString("QRZ FAILED (Network error) for %1: %2").arg(callsign, reply->errorString()));
                m_qrzQueueIntervalMs = qMin(10000, m_qrzQueueIntervalMs + 100);
                m_textEdit->append(QString("<span style='color: #c0392b;'>[Queue Throttling] QRZ Network Error! Slowing down pacing to %1 ms.</span>").arg(m_qrzQueueIntervalMs));
                if (m_qrzQueueTimer) m_qrzQueueTimer->setInterval(m_qrzQueueIntervalMs);
            }
            doFreeLookup(callsign, adifName, adifAddr2, adifCity, adifState, adifZip, adifCountry, startMs, pota, adifLotw, adifMqsl, updateDisplay, isUdp, originalCall);
            return;
        }

        if (hasError) {
            bool isNotFound = errorString.contains("not found", Qt::CaseInsensitive);
            if (!updateDisplay) {
                if (isNotFound) {
                    logQueueProgress(QString("QRZ SUCCESS (Not Found) for %1: %2").arg(callsign, errorString));
                    m_qrzQueueIntervalMs = qMax(50, m_qrzQueueIntervalMs - 50);
                } else {
                    logQueueProgress(QString("QRZ FAILED (API Error) for %1: %2").arg(callsign, errorString));
                    m_qrzQueueIntervalMs = qMin(10000, m_qrzQueueIntervalMs + 100);
                    m_textEdit->append(QString("<span style='color: #c0392b;'>[Queue Throttling] QRZ API Error! Slowing down pacing to %1 ms.</span>").arg(m_qrzQueueIntervalMs));
                }
                if (m_qrzQueueTimer) m_qrzQueueTimer->setInterval(m_qrzQueueIntervalMs);
            }
            m_textEdit->append(QString("<span style='color:red;'>QRZ Error: %1. Falling back to HamQTH/Callook.</span>").arg(errorString));
            if (!isNotFound) {
                m_qrzSessionKey.clear();
                writeSettings();
            }
            doFreeLookup(callsign, adifName, adifAddr2, adifCity, adifState, adifZip, adifCountry, startMs, pota, adifLotw, adifMqsl, updateDisplay, isUdp, originalCall);
            return;
        }
        
        QString qrzName = qrzFname;
        if (!qrzName.isEmpty() && !qrzLastName.isEmpty()) qrzName += " ";
        qrzName += qrzLastName;
        
        if (reply->error() == QNetworkReply::NoError && !xmlData.isEmpty() && !hasError) {
            QString xmlStr = QString::fromUtf8(xmlData);
            QSettings mailCache(getAppDataDir() + "/qrz_mailing_cache.ini", QSettings::IniFormat);
            mailCache.beginGroup(callsign);
            auto extractTag = [&](const QString &tag) {
                QRegularExpression re("<" + tag + ">([^<]+)</" + tag + ">", QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch match = re.match(xmlStr);
                return match.hasMatch() ? match.captured(1).trimmed() : QString();
            };
            mailCache.setValue("name", extractTag("name"));
            mailCache.setValue("addr1", extractTag("addr1"));
            mailCache.setValue("addr2", extractTag("addr2"));
            mailCache.setValue("city", extractTag("city").isEmpty() ? extractTag("addr2") : extractTag("city"));
            mailCache.setValue("state", extractTag("state"));
            mailCache.setValue("zip", extractTag("zip"));
            mailCache.setValue("country", extractTag("country"));
            mailCache.endGroup();
            mailCache.sync();
        }
        
        QString finalName = !qrzName.isEmpty() ? qrzName : adifName;
        QString finalCity = !qrzCity.isEmpty() ? qrzCity : (!qrzAddr2.isEmpty() ? qrzAddr2 : adifCity);
        QString finalState = !qrzState.isEmpty() ? qrzState : adifState;
        QString finalCountry = !qrzCountry.isEmpty() ? qrzCountry : adifCountry;

        QString finalZip = !qrzZip.isEmpty() ? qrzZip : adifZip;
        QString cityState = (!finalCity.isEmpty() && !finalState.isEmpty()) ? QString("%1, %2").arg(finalCity, finalState) : finalCity + finalState;
        if (!finalZip.isEmpty()) cityState += " " + finalZip;

        QString displayCall = !originalCall.isEmpty() ? originalCall : callsign;

        QString addressLine = !qrzAddr1.isEmpty() ? qrzAddr1 : adifAddr2;
        QString htmlInfo = QString("<h2>%1</h2><table border='0' cellspacing='0' cellpadding='2'>"
                                   "<tr><td align='right'><b>Name:</b></td><td align='left'>%2</td></tr>"
                                   "<tr><td align='right'><b>Address:</b></td><td align='left'>%3</td></tr>"
                                   "<tr><td align='right'><b>City:</b></td><td align='left'>%4</td></tr>"
                                   "<tr><td align='right'><b>Country:</b></td><td align='left'>%5</td></tr>"
                                   "</table>")
                               .arg(displayCall.toHtmlEscaped(), finalName.toHtmlEscaped(), addressLine.toHtmlEscaped(), cityState.toHtmlEscaped(), finalCountry.toHtmlEscaped());

        // --- Secondary check for mail status via xcheck ---
        QSettings settings(getAppDataDir() + "/settings.ini", QSettings::IniFormat);
        QString u = settings.value("username").toString();
        QString p = settings.value("password").toString();
        if (!u.isEmpty() && !p.isEmpty()) {
            QString xurl = QString("https://xml.qrz.com/xcheck?l=%1&p=%2&call=%3").arg(QString::fromUtf8(QUrl::toPercentEncoding(u))).arg(QString::fromUtf8(QUrl::toPercentEncoding(p))).arg(QString::fromUtf8(QUrl::toPercentEncoding(callsign)));
            QNetworkRequest xreq((QUrl(xurl)));
            QNetworkReply *xrep = m_networkManager->get(xreq);
            connect(xrep, &QNetworkReply::finished, this, [this, xrep, callsign]() {
                xrep->deleteLater();
                if (xrep->error() == QNetworkReply::NoError) {
                    QString xres = xrep->readAll();
                    QrzCache qc(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
                    qc.beginGroup(callsign);
                    QString xvia;
                    QRegularExpression viaRe("<(?:qsl_via|via)>([^<]+)</(?:qsl_via|via)>", QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatch vmatch = viaRe.match(xres);
                    if (vmatch.hasMatch()) xvia = vmatch.captured(1).trimmed();
                    else xvia = xres.trimmed(); // fallback to raw string if it's plaintext
                    
                    qc.setValue("xcheck_mailable", xvia.contains("DIRECT", Qt::CaseInsensitive) || xvia.contains("MAIL", Qt::CaseInsensitive) || xvia.contains("BOX", Qt::CaseInsensitive));
                    qc.setValue("xcheck_via", xvia);
                    qc.endGroup();
                }
            });
        }
        // --- End Secondary check ---

        if (!imgUrl.isEmpty()) {
            QNetworkRequest imgReq((QUrl(imgUrl)));
            QNetworkReply *imgReply = m_networkManager->get(imgReq);
            connect(imgReply, &QNetworkReply::finished, this, [=]() {
                imgReply->deleteLater();
                QByteArray imgData;
                if (imgReply->error() == QNetworkReply::NoError) {
                    imgData = imgReply->readAll();
                }
                
                QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
                QStringList groupsToWrite;
                groupsToWrite << callsign;
                if (!originalCall.isEmpty() && originalCall != callsign) {
                    groupsToWrite << originalCall;
                }
                for (const QString &g : groupsToWrite) {
                    qrzCache.beginGroup(g);
                    qrzCache.setValue("htmlInfo", htmlInfo);
                    qrzCache.setValue("imgData", imgData);
                    qrzCache.setValue("lotw", lotw);
                    qrzCache.setValue("mqsl", mqsl);
                    qrzCache.setValue("country", finalCountry);
                    qrzCache.setValue("county", qrzCounty);
                    qrzCache.setValue("grid", qrzGrid);
                    qrzCache.setValue("dxcc", qrzDxcc);
                    qrzCache.setValue("lat", qrzLat);
                    qrzCache.setValue("lon", qrzLon);
                    qrzCache.setValue("qrz_fname", qrzFname);
                    qrzCache.setValue("qrz_name", qrzLastName);
                    qrzCache.setValue("qrz_addr1", qrzAddr1);
                    qrzCache.setValue("qrz_addr2", qrzAddr2);
                    qrzCache.setValue("qrz_city", finalCity);
                    qrzCache.setValue("qrz_state", qrzState);
                    qrzCache.setValue("qrz_zip", qrzZip);
                    qrzCache.setValue("qrz_country", qrzCountry);
                    qrzCache.setValue("qslmgr", qrzQslmgr);
                    qrzCache.endGroup();
                }
                qrzCache.sync();

                qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - startMs;
                QString timeInfo = QString("<br><br><small>Fetched in %1 (from QRZ)</small>").arg(formatElapsedMs(elapsedMs));

                if (updateDisplay) {
                    displayCallInfo(htmlInfo + timeInfo, imgData, displayCall, pota, finalCountry, lotw, mqsl);
                } else {
                    logQueueProgress(QString("QRZ SUCCESS (with image) for %1").arg(callsign));
                    m_qrzQueueIntervalMs = qMax(50, m_qrzQueueIntervalMs - 50);
                    if (m_qrzQueueTimer) m_qrzQueueTimer->setInterval(m_qrzQueueIntervalMs);
                }
            });
        } else {
            QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
            QStringList groupsToWrite;
            groupsToWrite << callsign;
            if (!originalCall.isEmpty() && originalCall != callsign) {
                groupsToWrite << originalCall;
            }
            for (const QString &g : groupsToWrite) {
                qrzCache.beginGroup(g);
                qrzCache.setValue("htmlInfo", htmlInfo);
                qrzCache.setValue("imgData", QByteArray());
                qrzCache.setValue("lotw", lotw);
                qrzCache.setValue("mqsl", mqsl);
                qrzCache.setValue("country", finalCountry);
                qrzCache.setValue("county", qrzCounty);
                qrzCache.setValue("grid", qrzGrid);
                qrzCache.setValue("dxcc", qrzDxcc);
                qrzCache.setValue("lat", qrzLat);
                qrzCache.setValue("lon", qrzLon);
                qrzCache.setValue("qrz_fname", qrzFname);
                qrzCache.setValue("qrz_name", qrzLastName);
                qrzCache.setValue("qrz_addr1", qrzAddr1);
                qrzCache.setValue("qrz_addr2", qrzAddr2);
                qrzCache.setValue("qrz_city", finalCity);
                qrzCache.setValue("qrz_state", qrzState);
                qrzCache.setValue("qrz_zip", qrzZip);
                qrzCache.setValue("qrz_country", qrzCountry);
                qrzCache.setValue("qslmgr", qrzQslmgr);
                qrzCache.endGroup();
            }
            qrzCache.sync();

            qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - startMs;
            QString timeInfo = QString("<br><br><small>Fetched in %1 (from QRZ)</small>").arg(formatElapsedMs(elapsedMs));

            if (updateDisplay) {
                displayCallInfo(htmlInfo + timeInfo, QByteArray(), displayCall, pota, finalCountry, lotw, mqsl);
            } else {
                logQueueProgress(QString("QRZ SUCCESS (no image) for %1").arg(callsign));
                m_qrzQueueIntervalMs = qMax(50, m_qrzQueueIntervalMs - 50);
                if (m_qrzQueueTimer) m_qrzQueueTimer->setInterval(m_qrzQueueIntervalMs);
            }
        }
    });
}

static const QHash<QString, QString>& getCountryDict() {
    static const QHash<QString, QString> dict = {
        {"AFGHANISTAN", "af"}, {"ALBANIA", "al"}, {"ALGERIA", "dz"}, {"AMERICAN SAMOA", "as"}, {"ANDORRA", "ad"},
        {"ANGOLA", "ao"}, {"ANGUILLA", "ai"}, {"ANTARCTICA", "aq"}, {"ANTIGUA AND BARBUDA", "ag"}, {"ARGENTINA", "ar"},
        {"ARMENIA", "am"}, {"ARUBA", "aw"}, {"AUSTRALIA", "au"}, {"AUSTRIA", "at"}, {"AZERBAIJAN", "az"},
        {"BAHAMAS (THE)", "bs"}, {"BAHAMAS", "bs"}, {"BAHRAIN", "bh"}, {"BANGLADESH", "bd"}, {"BARBADOS", "bb"},
        {"BELARUS", "by"}, {"BELGIUM", "be"}, {"BELIZE", "bz"}, {"BENIN", "bj"}, {"BERMUDA", "bm"},
        {"BHUTAN", "bt"}, {"BOLIVIA (PLURINATIONAL STATE OF)", "bo"}, {"BOLIVIA", "bo"}, {"BONAIRE, SINT EUSTATIUS AND SABA", "bq"}, {"BOSNIA AND HERZEGOVINA", "ba"},
        {"BOTSWANA", "bw"}, {"BOUVET ISLAND", "bv"}, {"BRAZIL", "br"}, {"BRITISH INDIAN OCEAN TERRITORY (THE)", "io"}, {"BRITISH INDIAN OCEAN TERRITORY", "io"},
        {"BRUNEI DARUSSALAM", "bn"}, {"BRUNEI", "bn"}, {"BULGARIA", "bg"}, {"BURKINA FASO", "bf"}, {"BURUNDI", "bi"},
        {"CABO VERDE", "cv"}, {"CAMBODIA", "kh"}, {"CAMEROON", "cm"}, {"CANADA", "ca"}, {"CAYMAN ISLANDS (THE)", "ky"},
        {"CAYMAN ISLANDS", "ky"}, {"CENTRAL AFRICAN REPUBLIC (THE)", "cf"}, {"CENTRAL AFRICAN REPUBLIC", "cf"}, {"CHAD", "td"}, {"CHILE", "cl"},
        {"CHINA", "cn"}, {"CHRISTMAS ISLAND", "cx"}, {"COCOS (KEELING) ISLANDS (THE)", "cc"}, {"COCOS ISLANDS", "cc"}, {"COLOMBIA", "co"},
        {"COMOROS (THE)", "km"}, {"COMOROS", "km"}, {"CONGO (THE DEMOCRATIC REPUBLIC OF THE)", "cd"}, {"DEMOCRATIC REPUBLIC OF THE CONGO", "cd"}, {"CONGO (THE)", "cg"},
        {"CONGO", "cg"}, {"COOK ISLANDS (THE)", "ck"}, {"COOK ISLANDS", "ck"}, {"COSTA RICA", "cr"}, {"CROATIA", "hr"},
        {"CUBA", "cu"}, {"CURAÇAO", "cw"}, {"CYPRUS", "cy"}, {"CZECHIA", "cz"}, {"CZECH REPUBLIC", "cz"},
        {"CÔTE D'IVOIRE", "ci"}, {"DENMARK", "dk"}, {"DJIBOUTI", "dj"}, {"DOMINICA", "dm"}, {"DOMINICAN REPUBLIC (THE)", "do"},
        {"DOMINICAN REPUBLIC", "do"}, {"ECUADOR", "ec"}, {"EGYPT", "eg"}, {"EL SALVADOR", "sv"}, {"EQUATORIAL GUINEA", "gq"},
        {"ERITREA", "er"}, {"ESTONIA", "ee"}, {"ESWATINI", "sz"}, {"ETHIOPIA", "et"}, {"FALKLAND ISLANDS (THE) [MALVINAS]", "fk"},
        {"FALKLAND ISLANDS", "fk"}, {"FAROE ISLANDS (THE)", "fo"}, {"FAROE ISLANDS", "fo"}, {"FIJI", "fj"}, {"FINLAND", "fi"},
        {"FRANCE", "fr"}, {"FRENCH GUIANA", "gf"}, {"FRENCH POLYNESIA", "pf"}, {"FRENCH SOUTHERN TERRITORIES (THE)", "tf"}, {"GABON", "ga"},
        {"GAMBIA (THE)", "gm"}, {"GAMBIA", "gm"}, {"GEORGIA", "ge"}, {"GERMANY", "de"}, {"GHANA", "gh"},
        {"GIBRALTAR", "gi"}, {"GREECE", "gr"}, {"GREENLAND", "gl"}, {"GRENADA", "gd"}, {"GUADELOUPE", "gp"},
        {"GUAM", "gu"}, {"GUATEMALA", "gt"}, {"GUERNSEY", "gg"}, {"GUINEA", "gn"}, {"GUINEA-BISSAU", "gw"},
        {"GUYANA", "gy"}, {"HAITI", "ht"}, {"HEARD ISLAND AND MCDONALD ISLANDS", "hm"}, {"HOLY SEE (THE)", "va"}, {"VATICAN", "va"},
        {"HONDURAS", "hn"}, {"HONG Kong", "hk"}, {"HUNGARY", "hu"}, {"ICELAND", "is"}, {"INDIA", "in"},
        {"INDONESIA", "id"}, {"IRAN (ISLAMIC REPUBLIC OF)", "ir"}, {"IRAN", "ir"}, {"IRAQ", "iq"}, {"IRELAND", "ie"},
        {"ISLE OF MAN", "im"}, {"ISRAEL", "il"}, {"ITALY", "it"}, {"JAMAICA", "jm"}, {"JAPAN", "jp"},
        {"JERSEY", "je"}, {"JORDAN", "jo"}, {"KAZAKHSTAN", "kz"}, {"KENYA", "ke"}, {"KIRIBATI", "ki"},
        {"KOREA (THE DEMOCRATIC PEOPLE'S REPUBLIC OF)", "kp"}, {"NORTH KOREA", "kp"}, {"KOREA (THE REPUBLIC OF)", "kr"}, {"SOUTH KOREA", "kr"}, {"KUWAIT", "kw"},
        {"KYRGYZSTAN", "kg"}, {"LAO PEOPLE'S DEMOCRATIC REPUBLIC (THE)", "la"}, {"LAOS", "la"}, {"LATVIA", "lv"}, {"LEBANON", "lb"},
        {"LESOTHO", "ls"}, {"LIBERIA", "lr"}, {"LIBYA", "ly"}, {"LIECHTENSTEIN", "li"}, {"LITHUANIA", "lt"},
        {"LUXEMBOURG", "lu"}, {"MACAO", "mo"}, {"MADAGASCAR", "mg"}, {"MALAWI", "mw"}, {"MALAYSIA", "my"},
        {"MALDIVES", "mv"}, {"MALI", "ml"}, {"MALTA", "mt"}, {"MARSHALL ISLANDS (THE)", "mh"}, {"MARSHALL ISLANDS", "mh"},
        {"MARTINIQUE", "mq"}, {"MAURITANIA", "mr"}, {"MAURITIUS", "mu"}, {"MAYOTTE", "yt"}, {"MEXICO", "mx"},
        {"MICRONESIA (FEDERATED STATES OF)", "fm"}, {"MICRONESIA", "fm"}, {"MOLDOVA (THE REPUBLIC OF)", "md"}, {"MOLDOVA", "md"}, {"MONACO", "mc"},
        {"MONGOLIA", "mn"}, {"MONTENEGRO", "me"}, {"MONTSERRAT", "ms"}, {"MOROCCO", "ma"}, {"MOZAMBIQUE", "mz"},
        {"MYANMAR", "mm"}, {"NAMIBIA", "na"}, {"NAURU", "nr"}, {"NEPAL", "np"}, {"NETHERLANDS (THE)", "nl"},
        {"NETHERLANDS", "nl"}, {"NEW CALEDONIA", "nc"}, {"NEW ZEALAND", "nz"}, {"NICARAGUA", "ni"}, {"NIGER (THE)", "ne"},
        {"NIGER", "ne"}, {"NIGERIA", "ng"}, {"NIUE", "nu"}, {"NORFOLK ISLAND", "nf"}, {"NORTHERN MARIANA ISLANDS (THE)", "mp"},
        {"NORTHERN MARIANA ISLANDS", "mp"}, {"NORWAY", "no"}, {"OMAN", "om"}, {"PAKISTAN", "pk"}, {"PALAU", "pw"},
        {"PALESTINE, STATE OF", "ps"}, {"PALESTINE", "ps"}, {"PANAMA", "pa"}, {"PAPUA NEW GUINEA", "pg"}, {"PARAGUAY", "py"},
        {"PERU", "pe"}, {"PHILIPPINES (THE)", "ph"}, {"PHILIPPINES", "ph"}, {"PITCAIRN", "pn"}, {"POLAND", "pl"},
        {"PORTUGAL", "pt"}, {"PUERTO RICO", "pr"}, {"QATAR", "qa"}, {"REPUBLIC OF NORTH MACEDONIA", "mk"}, {"NORTH MACEDONIA", "mk"},
        {"MACEDONIA", "mk"}, {"ROMANIA", "ro"}, {"RUSSIAN FEDERATION (THE)", "ru"}, {"RUSSIA", "ru"}, {"RWANDA", "rw"},
        {"RÉUNION", "re"}, {"REUNION", "re"}, {"SAINT BARTHÉLEMY", "bl"}, {"SAINT HELENA, ASCENSION AND TRISTAN DA CUNHA", "sh"}, {"SAINT KITTS AND NEVIS", "kn"},
        {"SAINT LUCIA", "lc"}, {"SAINT MARTIN (FRENCH PART)", "mf"}, {"SAINT PIERRE AND MIQUELON", "pm"}, {"SAINT VINCENT AND THE GRENADINES", "vc"}, {"SAMOA", "ws"},
        {"SAN MARINO", "sm"}, {"SAO TOME AND PRINCIPE", "st"}, {"SAUDI ARABIA", "sa"}, {"SENEGAL", "sn"}, {"SERBIA", "rs"},
        {"SEYCHELLES", "sc"}, {"SIERRA LEONE", "sl"}, {"SINGAPORE", "sg"}, {"SINT MAARTEN (DUTCH PART)", "sx"}, {"SLOVAKIA", "sk"},
        {"SLOVENIA", "si"}, {"SOLOMON ISLANDS", "sb"}, {"SOMALIA", "so"}, {"SOUTH AFRICA", "za"}, {"SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS", "gs"},
        {"SOUTH SUDAN", "ss"}, {"SPAIN", "es"}, {"SRI LANKA", "lk"}, {"SUDAN (THE)", "sd"}, {"SUDAN", "sd"},
        {"SURINAME", "sr"}, {"SVALBARD AND JAN MAYEN", "sj"}, {"SWEDEN", "se"}, {"SWITZERLAND", "ch"}, {"SYRIAN ARAB REPUBLIC", "sy"},
        {"SYRIA", "sy"}, {"TAIWAN (PROVINCE OF CHINA)", "tw"}, {"TAIWAN", "tw"}, {"TAJIKISTAN", "tj"}, {"TANZANIA, UNITED REPUBLIC OF", "tz"},
        {"TANZANIA", "tz"}, {"THAILAND", "th"}, {"TIMOR-LESTE", "tl"}, {"TOGO", "tg"}, {"TOKELAU", "tk"},
        {"TONGA", "to"}, {"TRINIDAD AND TOBAGO", "tt"}, {"TUNISIA", "tn"}, {"TURKEY", "tr"}, {"TURKMENISTAN", "tm"},
        {"TURKS AND CAICOS ISLANDS (THE)", "tc"}, {"TUVALU", "tv"}, {"UGANDA", "ug"}, {"UKRAINE", "ua"}, {"UNITED ARAB EMIRATES (THE)", "ae"},
        {"UNITED ARAB EMIRATES", "ae"}, {"UAE", "ae"}, {"UNITED KINGDOM OF GREAT BRITAIN AND NORTHERN IRELAND (THE)", "gb"}, {"UNITED KINGDOM", "gb"}, {"UK", "gb"},
        {"GREAT BRITAIN", "gb"}, {"UNITED STATES MINOR OUTLYING ISLANDS (THE)", "um"}, {"UNITED STATES OF AMERICA (THE)", "us"}, {"UNITED STATES OF AMERICA", "us"}, {"USA", "us"},
        {"UNITED STATES", "us"}, {"URUGUAY", "uy"}, {"UZBEKISTAN", "uz"}, {"VANUATU", "vu"}, {"VENEZUELA (BOLIVARIAN REPUBLIC OF)", "ve"},
        {"VENEZUELA", "ve"}, {"VIET NAM", "vn"}, {"VIETNAM", "vn"}, {"VIRGIN ISLANDS (BRITISH)", "vg"}, {"VIRGIN ISLANDS (U.S.)", "vi"},
        {"WALLIS AND FUTUNA", "wf"}, {"WESTERN SAHARA", "eh"}, {"YEMEN", "ye"}, {"ZAMBIA", "zm"}, {"ZIMBABWE", "zw"},
        {"ÅLAND ISLANDS", "ax"}
    };
    return dict;
}

QList<QString> MainWindow::getAllCountries() {
    return getCountryDict().keys();
}

QString MainWindow::countryToIso(QString c) {
    c = c.trimmed().toUpper();
    // Manual aliases
    if (c == "UNITED STATES OF AMERICA" || c.contains("UNITED STATES") || c == "USA" || c == "ALASKA" || c == "HAWAII" || c == "PUERTO RICO" || c == "US VIRGIN ISLANDS" || c == "GUAM") return "us";
    if (c == "JAPAN") return "jp";
    if (c == "CANADA") return "ca";
    if (c == "GERMANY" || c == "FEDERAL REPUBLIC OF GERMANY") return "de";
    if (c == "FRANCE" || c == "FRENCH POLYNESIA") return "fr";
    if (c == "ITALY" || c == "SARDINIA" || c == "SICILY") return "it";
    if (c == "GREECE" || c == "CRETE") return "gr";
    if (c == "RUSSIA" || c.contains("ASIATIC RUSSIA") || c.contains("EUROPEAN RUSSIA") || c == "KALININGRAD") return "ru";
    if (c == "AUSTRALIA" || c == "TASMANIA") return "au";
    if (c == "ENGLAND" || c == "UNITED KINGDOM" || c == "GREAT BRITAIN" || c == "UK" || c == "WALES" || c == "SCOTLAND" || c == "NORTHERN IRELAND" || c == "ISLE OF MAN") return "gb";
    if (c == "SPAIN" || c == "CANARY ISLANDS" || c == "BALEARIC ISLANDS") return "es";
    if (c == "BRAZIL") return "br";
    if (c == "ARGENTINA") return "ar";
    if (c == "MEXICO") return "mx";
    if (c == "CHINA" || c == "HONG KONG" || c == "MACAO") return "cn";
    if (c == "SOUTH KOREA" || c == "REPUBLIC OF KOREA") return "kr";
    if (c == "NEW ZEALAND" || c == "CHATHAM ISLANDS") return "nz";
    if (c == "SOUTH AFRICA") return "za";
    if (c == "NETHERLANDS") return "nl";
    if (c == "BELGIUM") return "be";
    if (c == "SWITZERLAND") return "ch";
    if (c == "SWEDEN") return "se";
    if (c == "NORWAY") return "no";
    if (c == "DENMARK" || c == "GREENLAND" || c == "FAROE ISLANDS") return "dk";
    if (c == "FINLAND" || c == "ALAND ISLANDS") return "fi";
    if (c == "POLAND") return "pl";
    if (c == "UKRAINE") return "ua";
    if (c == "ROMANIA") return "ro";
    if (c == "INDIA" || c == "ANDAMAN AND NICOBAR ISLANDS") return "in";
    if (c == "INDONESIA") return "id";
    if (c == "PHILIPPINES") return "ph";
    if (c == "IRELAND") return "ie";
    if (c == "PORTUGAL" || c == "AZORES" || c == "MADEIRA") return "pt";

    if (getCountryDict().contains(c)) return getCountryDict()[c];
    
    QRegularExpressionMatch match = QRegularExpression("\\b([A-Za-z]{2,3})\\b").match(c.toLower());
    if (match.hasMatch() && match.captured(1).length() == 2) return match.captured(1);

    return "";
}

bool MainWindow::checkLotwDatabase(const QString &callsign) {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("Files");
    QString userDir = settings.value("userDirectory", QDir::homePath()).toString();
    settings.endGroup();
    
    QString lotwPath = QDir(userDir).filePath("lotw-active-users.csv");
    if (!QFile::exists(lotwPath)) {
        lotwPath = QDir(getAppDataDir()).filePath("lotw-active-users.csv");
    }
    
    if (!QFile::exists(lotwPath)) return false;
    
    QFile file(lotwPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    
    QTextStream in(&file);
    QString cleanCall = callsign.trimmed().toUpper();
    
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith('#') || line.isEmpty()) continue;
        
        QStringList parts = line.split(',');
        if (parts.size() >= 2) {
            QString call = parts[0].trimmed().toUpper();
            if (call == cleanCall) {
                QString dateStr = parts[1].trimmed();
                QDate activeDate = QDate::fromString(dateStr, "yyyy-MM-dd");
                if (!activeDate.isValid()) {
                    activeDate = QDate::fromString(dateStr, "yyyy/MM/dd");
                }
                if (!activeDate.isValid()) {
                    activeDate = QDate::fromString(dateStr, "yyyyMMdd");
                }
                if (activeDate.isValid()) {
                    int days = activeDate.daysTo(QDate::currentDate());
                    if (days >= 0 && days <= 365) {
                        return true;
                    }
                }
                break;
            }
        }
    }
    return false;
}

void MainWindow::displayCallInfo(const QString &htmlInfoRaw, const QByteArray &imgData, const QString &callsign, const QString &pota, const QString &country, bool lotw, bool mqsl) {
    m_lastHtmlInfoRaw = htmlInfoRaw;
    m_lastImgData = imgData;
    m_lastPota = pota;
    m_lastCountry = country;
    m_lastLotw = lotw;
    m_lastMqsl = mqsl;

    QString htmlInfo = sanitizeHtmlInfo(htmlInfoRaw);
    lotw = checkLotwDatabase(callsign);
    m_currentDisplayedCallsign = callsign;

    QrzCache cache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    cache.beginGroup(callsign);
    QString state = cache.value("qrz_state").toString().toUpper().trimmed();
    if (state.isEmpty()) state = cache.value("state").toString().toUpper().trimmed();
    QString county = cache.value("county").toString().toUpper().trimmed();
    QString dxcc = cache.value("dxcc").toString().trimmed();
    QString cachedCountry = cache.value("country").toString().toUpper().trimmed();
    QString grid = cache.value("grid").toString().toUpper().trimmed();
    cache.endGroup();

    // Failsafe 1: If state is empty but we have local QSO logbook history for this callsign, backfill it
    if (state.isEmpty() && m_allQsos.contains(callsign) && !m_allQsos[callsign].isEmpty()) {
        const QsoDetail &q = m_allQsos[callsign].first();
        state = q.state.toUpper().trimmed();
        if (county.isEmpty()) county = q.county.toUpper().trimmed();
        if (cachedCountry.isEmpty()) cachedCountry = q.country.toUpper().trimmed();
        if (dxcc.isEmpty()) dxcc = q.dxcc.trimmed();
        if (grid.isEmpty()) grid = q.grid.toUpper().trimmed();
    }

    // Failsafe 2: Regex-parse the state abbreviation from the HTML representation (e.g. "BLOOMINGTON, MN 55437")
    if (state.isEmpty() || state.length() > 2) {
        QRegularExpression stateRe(",\\s*([A-Z]{2})\\b");
        QRegularExpressionMatch match = stateRe.match(htmlInfo);
        if (match.hasMatch()) {
            state = match.captured(1).toUpper();
        }
    }

    // Failsafe 3: Regex-parse the Maidenhead Grid from the HTML (e.g. "<b>Grid:</b> FM18dp")
    if (grid.isEmpty()) {
        QRegularExpression gridRe("Grid:</b>\\s*([A-Z0-9]{4,8})", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = gridRe.match(htmlInfo);
        if (match.hasMatch()) {
            grid = match.captured(1).toUpper();
        }
    }

    // Normalize incoming country to uppercase for robust matching
    QString upperCountry = country.toUpper().trimmed();

    bool isUs = (cachedCountry == "UNITED STATES OF AMERICA" || cachedCountry == "UNITED STATES" || cachedCountry == "USA" || cachedCountry == "ALASKA" || cachedCountry == "HAWAII" || cachedCountry == "VIRGIN ISLANDS" || cachedCountry == "U.S. VIRGIN ISLANDS" ||
                 dxcc == "291" || dxcc == "6" || dxcc == "110" || dxcc == "285" ||
                 upperCountry == "UNITED STATES OF AMERICA" || upperCountry == "UNITED STATES" || upperCountry == "USA" || upperCountry == "ALASKA" || upperCountry == "HAWAII" || upperCountry == "VIRGIN ISLANDS" || upperCountry == "U.S. VIRGIN ISLANDS");

    // Check if new US County & State
    if (m_newCountyLabel && m_newCountyAnim) {
        if (isUs && !state.isEmpty() && !county.isEmpty()) {
            QString combo = state + "," + county;
            combo.replace(", ", ",");
            if (!m_usCountiesWorked.contains(combo)) {
                m_newCountyLabel->setText(QString("NEW COUNTY in %1").arg(state));
                m_newCountyLabel->show();
                m_newCountyAnim->start();
                if (m_newCountyTimer) m_newCountyTimer->start(20000); // 5x of the 4000ms New QSO alert = 20 seconds!
            } else {
                m_newCountyLabel->hide();
                m_newCountyAnim->stop();
                if (m_newCountyTimer) m_newCountyTimer->stop();
            }
        } else {
            m_newCountyLabel->hide();
            m_newCountyAnim->stop();
            if (m_newCountyTimer) m_newCountyTimer->stop();
        }
    }

    // Check if new Grid (Maidenhead)
    if (m_newGridLabel && m_newGridAnim) {
        if (!grid.isEmpty() && grid.length() >= 4) {
            QString baseGrid = grid.left(4).toUpper();
            if (!m_gridsWorked.contains(baseGrid)) {
                m_newGridLabel->setText(QString("NEW GRID %1").arg(baseGrid));
                m_newGridLabel->show();
                m_newGridAnim->start();
                if (m_newGridTimer) m_newGridTimer->start(20000); // 20 seconds
            } else {
                m_newGridLabel->hide();
                m_newGridAnim->stop();
                if (m_newGridTimer) m_newGridTimer->stop();
            }
        } else {
            m_newGridLabel->hide();
            m_newGridAnim->stop();
            if (m_newGridTimer) m_newGridTimer->stop();
        }
    }
    QString imgTag;
    if (!imgData.isEmpty()) {
        QImage img = QImage::fromData(imgData).scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QString resName = "img_" + callsign;
        m_infoBrowser->document()->addResource(QTextDocument::ImageResource, QUrl(resName), img);
        imgTag = QString("<img src='%1'/>").arg(resName);
    } else {
        QImage dummy(150, 150, QImage::Format_RGB32);
        dummy.fill(QColor("#1e222a")); // Dark cozy shack background
        QPainter p(&dummy);
        p.setRenderHint(QPainter::Antialiasing);

        // Outer border
        p.setPen(QPen(QColor("#3e4451"), 2));
        p.drawRect(1, 1, 148, 148);

        // Desk
        p.setBrush(QColor("#2d2013"));
        p.setPen(Qt::NoPen);
        p.drawRect(0, 110, 150, 40);

        // Radio Transceiver Unit on Desk
        p.setBrush(QColor("#282c34"));
        p.setPen(QPen(QColor("#4b5263"), 1));
        p.drawRoundedRect(10, 60, 65, 50, 3, 3);

        // Glowing Frequency Meter Screen
        p.setBrush(QColor("#1b2d26"));
        p.drawRect(16, 66, 38, 20);
        p.setPen(QColor("#98c379"));
        QFont mFont("Monospace", 6, QFont::Bold);
        p.setFont(mFont);
        p.drawText(QRect(16, 66, 38, 20), Qt::AlignCenter, "CQ... 0W");

        // Knobs on Transceiver
        p.setBrush(QColor("#5c6370"));
        p.drawEllipse(18, 92, 10, 10);
        p.drawEllipse(34, 92, 8, 8);
        p.drawEllipse(46, 92, 8, 8);

        // Red Power LED
        p.setBrush(QColor("#e06c75"));
        p.drawEllipse(58, 68, 4, 4);

        // Desktop Microphone
        p.setPen(QPen(QColor("#abb2bf"), 2));
        p.drawLine(80, 110, 80, 85); // Stand stem
        p.drawLine(80, 85, 75, 78);  // Gooseneck
        p.setBrush(QColor("#5c6370"));
        p.drawEllipse(71, 72, 8, 10); // Mic element
        p.setBrush(QColor("#3e4451"));
        p.drawRect(74, 108, 12, 3);  // Base

        // Sad / Dejected Operator (Slouched Head on Arm)
        p.setPen(Qt::NoPen);
        // Body / Shirt
        p.setBrush(QColor("#3b5998"));
        p.drawRoundedRect(95, 80, 45, 35, 6, 6);
        // Arm on Desk
        p.setBrush(QColor("#2d4373"));
        p.drawRoundedRect(88, 100, 35, 12, 4, 4);
        // Head tilted down on arm
        p.setBrush(QColor("#e5c07b"));
        p.drawEllipse(102, 68, 22, 24);
        // Headphones on operator
        p.setBrush(QColor("#1e222a"));
        p.setPen(QPen(QColor("#abb2bf"), 2));
        p.drawArc(98, 62, 30, 20, 30 * 16, 120 * 16);
        p.setBrush(QColor("#3e4451"));
        p.setPen(Qt::NoPen);
        p.drawEllipse(98, 70, 6, 12); // Ear cup

        // Text Banner at Top & Bottom
        p.setPen(QColor("#e5c07b"));
        QFont labelFont("Arial", 7, QFont::Bold);
        p.setFont(labelFont);
        p.drawText(QRect(0, 8, 150, 15), Qt::AlignCenter, "NO ONE CALLING BACK");

        p.setPen(QColor("#5c6370"));
        QFont subFont("Arial", 6);
        p.setFont(subFont);
        p.drawText(QRect(0, 134, 150, 14), Qt::AlignCenter, "[ No Image Available ]");

        p.end();

        QString resName = "dummy_" + callsign;
        m_infoBrowser->document()->addResource(QTextDocument::ImageResource, QUrl(resName), dummy);
        imgTag = QString("<img src='%1'/>").arg(resName);
    }
    
    QString potaHtml = "";
    if (!pota.isEmpty()) {
        potaHtml = QString("<br><b>POTA:</b> %1").arg(pota);
    }

    QString flagHtml = "";
    QString iso = countryToIso(country);
    
    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    
    auto checkAndDownloadIcon = [&](const QString &key, const QString &url, QByteArray &outData) -> bool {
        qrzCache.beginGroup("icons");
        outData = qrzCache.value(key).toByteArray();
        qrzCache.endGroup();
        if (!outData.isEmpty()) return true; // Do NOT check startsWith("empty:") here to avoid infinite network loop
        
        // Failsafe for native icons: load instantly from resources instead of downloading!
        if (key == "arrl_icon") {
            QFile file(":/arrl_small.png");
            if (file.open(QIODevice::ReadOnly)) {
                outData = file.readAll();
                qrzCache.beginGroup("icons");
                qrzCache.setValue(key, outData);
                qrzCache.endGroup();
                return true;
            }
        } else if (key == "env_icon") {
            QFile file(":/env.png");
            if (file.open(QIODevice::ReadOnly)) {
                outData = file.readAll();
                qrzCache.beginGroup("icons");
                qrzCache.setValue(key, outData);
                qrzCache.endGroup();
                return true;
            }
        }

        QNetworkRequest req((QUrl(url)));
        req.setHeader(QNetworkRequest::UserAgentHeader, "Qwain/8.2");
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        QNetworkReply *reply = m_networkManager->get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, key, htmlInfo, imgData, callsign, pota, country, lotw, mqsl]() {
            reply->deleteLater();
            QrzCache cache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
            cache.beginGroup("icons");
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray data = reply->readAll();
                cache.setValue(key, data);
            } else {
                QString err = "empty:" + reply->errorString();
                cache.setValue(key, err.toUtf8()); // Store arbitrary string to avoid endless loops
            }
            cache.endGroup();
            cache.sync();
            displayCallInfo(htmlInfo, imgData, callsign, pota, country, lotw, mqsl);
        });
        return false;
    };
    
    QByteArray flagData, arrlData, envData;
    if (!iso.isEmpty() && !checkAndDownloadIcon("flag_" + iso, "https://flagcdn.com/w80/" + iso + ".png", flagData)) return;
    if (lotw && !checkAndDownloadIcon("arrl_icon", "https://americasboatingclubhouston.org/wp-content/uploads/2022/02/ARRL-logo-color-trans-small.png", arrlData)) return;
    if (mqsl && !checkAndDownloadIcon("env_icon", "https://icons.iconarchive.com/icons/custom-icon-design/mono-general-2/128/mail-icon.png", envData)) return;
    
    QString iconsHtml;
    
    auto makeImg = [&](const QString &cacheKey, const QByteArray &data, const QString &resNamePrefix, int defaultW, int defaultH, const QString &fallback) {
        QString imgHtml;
        QrzCache cache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
        cache.beginGroup("icons");
        int w = cache.value(cacheKey + "_W", defaultW).toInt();
        int h = cache.value(cacheKey + "_H", defaultH).toInt();
        cache.endGroup();
        if (data.isEmpty() || data.startsWith("empty")) {
            QString disp = fallback;
            if (data.startsWith("empty:")) {
                disp = "ERR";
                m_textEdit->append(QString("DEBUG [%1]: Failed to load image. Reason stored: %2").arg(cacheKey, QString::fromUtf8(data)));
            }
            if (disp.isEmpty()) return QString();
            imgHtml = QString("<div align='center' style='font-size: 10pt; font-weight: bold; color: red;'>[%1]</div>").arg(disp);
        } else {
            static int uniqueCounter = 0;
            QString uniqueResName = QString("%1_%2").arg(resNamePrefix).arg(++uniqueCounter);
            QImage img = QImage::fromData(data);
            if (w > 0 && h > 0) {
                img = img.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                imgHtml = QString("<img src='%1' width='%2' height='%3'/>").arg(uniqueResName).arg(w).arg(h);
            } else if (h > 0) {
                img = img.scaledToHeight(h, Qt::SmoothTransformation);
                imgHtml = QString("<img src='%1' height='%2'/>").arg(uniqueResName).arg(h);
            }
            m_infoBrowser->document()->addResource(QTextDocument::ImageResource, QUrl(uniqueResName), img);
        }
        return imgHtml;
    };
    
    QString flagImgHtml = !iso.isEmpty() ? makeImg("flag_" + iso, flagData, "res_flag_" + iso, -1, 24, "FLAG") : "";
    if (iso.isEmpty()) {
        m_textEdit->append(QString("DEBUG: No flag could be found for country '%1'. Is it missing in countryToIso?").arg(country));
    }
    QString envImgHtml = mqsl ? makeImg("env_icon", envData, "res_env_icon", 36, 34, "MAIL") : "";
    QString arrlImgHtml = lotw ? makeImg("arrl_icon", arrlData, "res_arrl_icon", 36, 34, "LOTW") : "";
    
    // QTextBrowser doesn't support flex, use a simple centered horizontal table or just spaces
    iconsHtml = QString("<br><table border='0' width='100%' cellspacing='0' cellpadding='0'><tr>"
                        "<td valign='middle' align='left'>%1</td>"
                        "<td valign='middle' align='right'>%2&nbsp;&nbsp;%3</td>"
                        "</tr></table>")
                    .arg(flagImgHtml, envImgHtml, arrlImgHtml);

    auto makeBtn = [](const QString& lbl, const QString& href, const QString& bg, const QString& fg) {
        return QString("<table border='1' cellspacing='0' cellpadding='8' bgcolor='%1' style='border-color:#555;'><tr><td align='center'><a href='%2' style='color:%3; text-decoration:none;'><b>%4</b></a></td></tr></table>")
            .arg(bg, href, fg, lbl);
    };

    QString paperBtnColor = "#ffffff";
    QString paperBtnBg = "#2c87c0";
    QString paperBtnHtml = makeBtn("Send Paper QSL", "http://app.local/qsl_paper/" + callsign, paperBtnBg, paperBtnColor);

    QString leftPanel = QString("<div align='center'>%1%2%3<br>%4</div>").arg(imgTag, potaHtml, iconsHtml, paperBtnHtml);
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QFont m_callInfoFont = qvariant_cast<QFont>(settings.value("UI/callInfoFont", QFont("Arial", 10)));
    QColor m_callInfoColor = qvariant_cast<QColor>(settings.value("UI/callInfoColor", QColor(Qt::white)));
    QFont m_qsoListFont = qvariant_cast<QFont>(settings.value("UI/qsoListFont", QFont("Courier", 10)));
    QColor m_qsoListColor = qvariant_cast<QColor>(settings.value("UI/qsoListColor", QColor(Qt::white)));
    
    QString callInfoStyle = QString("font-family: '%1'; font-size: %2pt; font-weight: %3; color: %4;")
                                .arg(m_callInfoFont.family()).arg(m_callInfoFont.pointSize())
                                .arg(m_callInfoFont.weight() * 8).arg(m_callInfoColor.name());
                                
    QString qsoListStyle = QString("font-family: '%1'; font-size: %2pt; font-weight: %3; color: %4;")
                                .arg(m_qsoListFont.family()).arg(m_qsoListFont.pointSize())
                                .arg(m_qsoListFont.weight() * 8).arg(m_qsoListColor.name());

    QString formattedHtmlInfo = QString("<div style=\"%1\">%2</div>").arg(callInfoStyle, htmlInfo);

    QString historyHtml;
    int qsoCount = m_allQsos.contains(callsign) ? m_allQsos[callsign].size() : 0;
    
    QColor redColor;
    redColor.setHsvF(0.0, 1.0, m_qsoListColor.valueF());
    
    QColor greenColor;
    greenColor.setHsvF(120.0 / 360.0, 1.0, m_qsoListColor.valueF());
    
    // historyHtml += "<br><hr width='100%' size='1' color='white'><br>";
    historyHtml += "<table width='100%' border='0'><tr><td valign='top' width='100%'>";
    historyHtml += "<h3 align='center' style='color:orange; margin-top:0px;'>Past Contacts</h3>";
    
    if (qsoCount > 0) {
        historyHtml += QString("<div style=\"%1\">").arg(qsoListStyle);
        historyHtml += "<table width='100%' style='border-collapse: collapse;'>";
        historyHtml += "<tr><th align='center' style='white-space:nowrap;'>Date</th><th align='center' style='white-space:nowrap;'>Time</th><th align='center' style='white-space:nowrap;'>Band</th><th align='center' style='white-space:nowrap;'>Mode</th></tr>";
        for (const QsoDetail &dRaw : m_allQsos[callsign]) {
            QsoDetail d = adjustQsoToEastern(dRaw);
            QString dStr = d.date;
            if (dStr.length() == 8) dStr = dStr.mid(6,2) + "/" + dStr.mid(4,2) + "/" + dStr.left(4);
            QString tStr = d.time;
            if (tStr.length() >= 4) tStr = tStr.left(2) + ":" + tStr.mid(2,2);
            
            QColor rowColor = d.qslsdate.isEmpty() ? redColor : greenColor;
            QString trStyle = QString("color:%1;").arg(rowColor.name());
            historyHtml += QString("<tr style=\"%5\"><td align='center' style='white-space:nowrap;'>%1</td><td align='center' style='white-space:nowrap;'>%2</td><td align='center' style='white-space:nowrap;'>%3</td><td align='center' style='white-space:nowrap;'>%4</td></tr>").arg(dStr.toHtmlEscaped(), tStr.toHtmlEscaped(), d.band.toHtmlEscaped(), d.mode.toHtmlEscaped(), trStyle);
        }
        historyHtml += "</table></div>";
        historyHtml += QString("<div align='center' style=\"font-family: '%1'; font-size: 6pt; margin-top: 10px;\">")
                            .arg(m_qsoListFont.family());
        historyHtml += QString("<span style=\"color:%1;\">Not Printed</span> &nbsp;&nbsp; ").arg(redColor.name());
        historyHtml += QString("<span style=\"color:%1;\">Printed</span></div>").arg(greenColor.name());
    } else {
        historyHtml += "<p align='center'><i>No contacts.</i></p>";
    }
    historyHtml += "</td></tr></table>";

    // State Map rendering block
    auto normalizeCounty = [](QString name) {
        name = name.toUpper().trimmed();
        name.replace(" CITY AND BOROUGH", "");
        name.replace(" CENSUS AREA", "");
        name.replace(" BOROUGH", "");
        name.replace(" MUNICIPALITY", "");
        name.replace(" COUNTY", "");
        name.replace(" PARISH", "");
        name.replace(" CITY", "");
        return name.trimmed();
    };

    if (isUs && !state.isEmpty()) {
        QFile file(":/us_counties_map.svg");
        if (file.open(QIODevice::ReadOnly)) {
            QString content = file.readAll();
            file.close();
            
            QMap<QString, QString> fMap = getFipsMap();
            // Patch missing fips from SVG map
            fMap["FL,CHARLOTTE"] = "path7617";
            fMap["FL,LEE"] = "path7615";
            fMap["FL,MONROE"] = "Monroe__FL";
            fMap["DE,NEW CASTLE"] = "New_Castle__DE";
            fMap["IN,POSEY"] = "Posey__IN";
            fMap["IN,GIBSON"] = "Gibson__IN";
            fMap["MA,NORFOLK"] = "Norfolk__MA";
            fMap["MD,SOMERSET"] = "Somerset__MD";
            fMap["ME,KNOX"] = "Knox__ME";
            fMap["ME,WALDO"] = "Waldo__ME";
            fMap["ME,HANCOCK"] = "Hancock__ME";
            fMap["MI,LEELANAU"] = "Leelanau__MI";
            fMap["MI,CHARLEVOIX"] = "Charlevoix__MI";
            fMap["MI,MACKINAC"] = "Mackinac__MI";
            fMap["MI,CHIPPEWA"] = "Chippewa__MI";
            fMap["MI,ALGER"] = "Alger__MI";
            fMap["NC,CURRITUCK"] = "Currituck__NC";
            fMap["NY,NEW YORK"] = "New_York__NY";
            fMap["NY,NASSAU"] = "Nassau__NY";
            fMap["NY,SUFFOLK"] = "Suffolk__NY";
            fMap["RI,NEWPORT"] = "Newport__RI";
            fMap["TN,TIPTON"] = "Tipton__TN";
            fMap["VA,NORTHAMPTON"] = "Northampton__VA";
            fMap["VA,ALEXANDRIA CITY"] = "Alexandria__VA";
            fMap["WA,WHATCOM"] = "Whatcom__WA";
            fMap["WA,PACIFIC"] = "Pacific__WA";
            fMap["WA,PIERCE"] = "Pierce__WA";
            fMap["WA,SAN JUAN"] = "San_Juan__WA";
            fMap["WA,KITSAP"] = "Kitsap__WA";
            fMap["WA,MASON"] = "Mason__WA";
            fMap["WA,JEFFERSON"] = "Jefferson__WA";
            fMap["WI,DOOR"] = "Door__WI";
            fMap["WI,ASHLAND"] = "Ashland__WI";
            fMap["AK,KETCHIKAN GATEWAY"] = "Ketchikan_Gateway__AK";
            fMap["AK,PETERSBURG BOROUGH"] = "path6542";
            fMap["AK,WRANGELL CITY AND BOROUGH"] = "path6556";
            fMap["AK,SKAGWAY MUNICIPALITY"] = "path6510";
            fMap["AK,HOONAH-ANGOON CENSUS AREA"] = "path6581";
            fMap["AK,PRINCE OF WALES-HYDER CENSUS AREA"] = "path6497";

            for (auto it = fMap.begin(); it != fMap.end(); ++it) {
                QString ctyStr = it.key();  // "TX,DALLAS"
                QString cssClass = it.value();
                QStringList parts = ctyStr.split(",");
                if (parts.size() < 2) continue;
                QString cState = parts[0].trimmed().toUpper();
                QString cCounty = parts[1].trimmed().toUpper();
                
                QString fillColor = "#111622"; // default for other states (extremely dark/faded)
                
                if (cState == state) {
                    bool isActive = (!county.isEmpty() && normalizeCounty(cCounty) == normalizeCounty(county));
                    if (isActive) {
                        fillColor = "#00ffff"; // vibrant cyan active county of lookup
                    } else if (!m_useHeatmapMode) {
                        bool isWorked = m_usCountiesWorked.contains(ctyStr);
                        if (!isWorked) {
                            // double check normalized
                            QString normCtyStr = cState + "," + normalizeCounty(cCounty);
                            for (const QString &w : m_usCountiesWorked) {
                                QStringList wParts = w.split(",");
                                if (wParts.size() >= 2 && wParts[0].trimmed().toUpper() == cState && normalizeCounty(wParts[1]) == normalizeCounty(cCounty)) {
                                    isWorked = true;
                                    break;
                                }
                            }
                        }
                        fillColor = isWorked ? "#ffaa00" : "#444444"; // Amber worked, dark gray unworked
                    } else {
                        // Heatmap mode: count Qso counts in state
                        int qsoCount = 0;
                        for (auto qit = m_allQsos.begin(); qit != m_allQsos.end(); ++qit) {
                            for (const QsoDetail &d : qit.value()) {
                                if (d.state.toUpper().trimmed() == state) {
                                    if (normalizeCounty(d.county) == normalizeCounty(cCounty)) {
                                        qsoCount++;
                                    }
                                }
                            }
                        }
                        if (qsoCount == 0) {
                            fillColor = "#444444";
                        } else if (qsoCount == 1) {
                            fillColor = "#48cae4"; // soft cyan
                        } else if (qsoCount <= 3) {
                            fillColor = "#0096c7"; // vibrant blue
                        } else if (qsoCount <= 7) {
                            fillColor = "#ffb703"; // vibrant gold
                        } else {
                            fillColor = "#e63946"; // fire red
                        }
                    }
                }
                
                content.replace(QString("id=\"%1\"").arg(cssClass), QString("id=\"%1\" fill=\"%2\"").arg(cssClass, fillColor));
            }

            content.replace("style=\"fill:#d0d0d0\"", "");

#if HAVE_QT_SVG
            QSvgRenderer svgRen(content.toUtf8());
            if (svgRen.isValid()) {
                QRectF stateRect;
                for (auto it = fMap.begin(); it != fMap.end(); ++it) {
                    QString ctyStr = it.key();
                    QStringList parts = ctyStr.split(",");
                    if (parts.size() >= 2 && parts[0].trimmed().toUpper() == state) {
                        QString cssClass = it.value();
                        QRectF r = svgRen.boundsOnElement(cssClass);
                        if (r.isValid() && r.width() > 0 && r.height() > 0) {
                            if (stateRect.isNull()) {
                                stateRect = r;
                            } else {
                                stateRect = stateRect.united(r);
                            }
                        }
                    }
                }

                if (!stateRect.isNull()) {
                    // Add 10% padding
                    double paddingX = stateRect.width() * 0.10;
                    double paddingY = stateRect.height() * 0.10;
                    stateRect.adjust(-paddingX, -paddingY, paddingX, paddingY);
                    
                    QImage mapImg(280, 180, QImage::Format_ARGB32_Premultiplied);
                    mapImg.fill(Qt::transparent);
                    QPainter p(&mapImg);
                    p.setRenderHint(QPainter::Antialiasing);
                    svgRen.render(&p, stateRect);
                    p.end();
                    
                    QString resName = "state_map_img";
                    m_infoBrowser->document()->addResource(QTextDocument::ImageResource, QUrl(resName), mapImg);
                    
                    QString activeModeText = m_useHeatmapMode ? "Heatmap Mode" : "Status Map";
                    QString activeModeDesc = m_useHeatmapMode ? "Color-coded by QSO count density" : "Amber: Worked, Gray: Unworked, Cyan: Active";
                    QString modeToggleLink = QString("<br><a href='http://app.local/toggle_map_mode' style='color:#ffaa00; text-decoration:none; font-size: 8pt;'><b>[ Switch to %1 ]</b></a><br><span style='font-size:7pt; color:#888888;'>%2</span>")
                                              .arg(m_useHeatmapMode ? "Status Map" : "Heatmap Mode", activeModeDesc);
                    
                    QString mapHtml = QString("<br><table border='0' cellspacing='0' cellpadding='0'><tr><td align='center'>"
                                              "<a href='http://app.local/toggle_map_mode'><img src='%1' width='280' height='180'/></a>"
                                              "</td></tr><tr><td align='center'>%2</td></tr></table>")
                                      .arg(resName, modeToggleLink);
                                      
                    // formattedHtmlInfo += mapHtml; // Retained background rendering logic; disabled UI map display as requested
                }
            }
#endif
        }
    }

    QString html = QString("<table border='0' width='100%' cellspacing='10'><tr><td valign='top' width='25%'>%1</td><td valign='top' width='45%'>%2</td><td valign='top' width='30%'>%3</td></tr></table>")
                   .arg(leftPanel, formattedHtmlInfo, historyHtml);
    m_infoBrowser->setHtml(html);
    m_statusLabel->setText("Status: Waiting");
}

void MainWindow::initDatabase() {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    QDir dataDir(getAppDataDir());
    if (!dataDir.exists()) dataDir.mkpath(".");
    m_db.setDatabaseName(dataDir.filePath("qwain.sqlite"));
    
    if (!m_db.open()) {
        qDebug() << "Failed to open SQLite db:" << m_db.lastError().text();
    } else {
        QSqlQuery query;
        query.exec("CREATE TABLE IF NOT EXISTS qsos ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "call TEXT, "
                   "qso_date TEXT, "
                   "time_on TEXT, "
                   "freq TEXT, "
                   "band TEXT, "
                   "mode TEXT, "
                   "country TEXT, "
                   "state TEXT, "
                   "dxcc TEXT, "
                   "pota TEXT, "
                   "grid TEXT, "
                   "lat TEXT, "
                   "lon TEXT, "
                   "county TEXT"
                   ")");
        query.exec("CREATE INDEX IF NOT EXISTS idx_call ON qsos (call)");
    }
}

void MainWindow::importLogfileToDb(const QString &logContent) {
    if (!m_db.isOpen()) return;
    
    QSqlQuery query;
    query.exec("BEGIN TRANSACTION");
    query.exec("DELETE FROM qsos");
    
    query.prepare("INSERT INTO qsos (call, qso_date, time_on, freq, band, mode, country, state, dxcc, pota, grid, lat, lon, county) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    
    QRegularExpression eorRe("<eor>", QRegularExpression::CaseInsensitiveOption);
    QStringList records = logContent.split(eorRe);
    int recCount = 0;
    
    for (const QString &rec : records) {
        QString call = extractAdifField(rec, "CALL").toUpper();
        if (call.isEmpty()) continue;
        
        QString qsoDate = extractAdifField(rec, "QSO_DATE");
        QString timeOn = extractAdifField(rec, "TIME_ON");
        QString freq = extractAdifField(rec, "FREQ");
        QString band = extractAdifField(rec, "BAND").toUpper();
        QString mode = extractAdifField(rec, "MODE").toUpper();
        {
            QString sub = extractAdifField(rec, "SUBMODE").toUpper();
            if (!sub.isEmpty()) {
                mode = sub;
            }
        }
        QString country = extractAdifField(rec, "COUNTRY").toUpper();
        QString state = extractAdifField(rec, "STATE").toUpper();
        QString dxcc = extractAdifField(rec, "DXCC");
        QString grid = extractAdifField(rec, "GRIDSQUARE").toUpper();
        QString lat = extractAdifField(rec, "LAT");
        if (lat.isEmpty()) lat = extractAdifField(rec, "APP_QRZLOG_LAT");
        QString lon = extractAdifField(rec, "LON");
        if (lon.isEmpty()) lon = extractAdifField(rec, "APP_QRZLOG_LON");
        QString county = extractAdifField(rec, "CNTY");
        
        QString potaRef = extractAdifField(rec, "POTA_REF").toUpper();
        if (potaRef.isEmpty()) potaRef = extractAdifField(rec, "POTA_ID").toUpper();
        if (potaRef.isEmpty()) potaRef = extractAdifField(rec, "APP_QRZLOG_POTA_REF").toUpper();
        if (potaRef.isEmpty() && extractAdifField(rec, "SIG").toUpper() == "POTA") {
            potaRef = extractAdifField(rec, "SIG_INFO").toUpper();
        }
        
        
        QString potaFound;
        if (!potaRef.isEmpty()) potaFound = potaRef;
        else {
            QString fullRecordUpper = rec.toUpper();
            QString qthUpper = extractAdifField(rec, "QTH").toUpper();
            if (qthUpper.isEmpty()) qthUpper = extractAdifField(rec, "APP_QRZLOG_QTH").toUpper();
            QString commentUpper = extractAdifField(rec, "COMMENT").toUpper();
            
            QRegularExpression reStrict("(?:POTA|PARK)[\\s\\-:]*?([A-Z]{1,4}-?\\d{3,5})");
            QRegularExpression reLoose("\\b([A-Z]{1,4}-\\d{3,5})\\b");
            
            QRegularExpressionMatch mStrict = reStrict.match(fullRecordUpper);
            if (mStrict.hasMatch()) {
                QString matchStr = mStrict.captured(1);
                if (!matchStr.contains("-")) {
                    int pos = 0;
                    while (pos < matchStr.length() && !matchStr[pos].isDigit()) pos++;
                    if (pos > 0 && pos < matchStr.length()) matchStr.insert(pos, "-");
                }
                potaFound = matchStr;
            } else {
                QRegularExpressionMatch mQth = reLoose.match(qthUpper);
                if (mQth.hasMatch() && (qthUpper.contains("POTA") || qthUpper.contains("PARK"))) {
                    potaFound = mQth.captured(1);
                } else {
                    QRegularExpressionMatch mComment = reLoose.match(commentUpper);
                    if (mComment.hasMatch() && (commentUpper.contains("POTA") || commentUpper.contains("PARK"))) {
                        potaFound = mComment.captured(1);
                    }
                }
            }
            
            if (!potaFound.isEmpty()) {
                QString prefix = potaFound.section('-', 0, 0).toUpper();
                if (prefix == "NA" || prefix == "SA" || prefix == "EU" || prefix == "AS" || prefix == "AF" || prefix == "OC" || prefix == "AN") {
                    potaFound.clear();
                }
            }
        }
        
        query.addBindValue(call);
        query.addBindValue(qsoDate);
        query.addBindValue(timeOn);
        query.addBindValue(freq);
        query.addBindValue(band);
        query.addBindValue(mode);
        query.addBindValue(country);
        query.addBindValue(state);
        query.addBindValue(dxcc);
        query.addBindValue(potaFound);
        query.addBindValue(grid);
        query.addBindValue(lat);
        query.addBindValue(lon);
        query.addBindValue(county);
        query.exec();
        
        if (++recCount % 1000 == 0) QCoreApplication::processEvents();
    }
    query.exec("COMMIT");
}

void MainWindow::onClientDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    m_clients.removeOne(clientSocket);
    m_textEdit->append("Client disconnected.");
    clientSocket->deleteLater();
}

void MainWindow::loadLogfile()
{
    if (m_logfilePath.isEmpty()) return;
    
    m_logLastModified = QFileInfo(m_logfilePath).lastModified();
    QString content;
    
    QFile file(m_logfilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        content = in.readAll();
        file.close();
        
        if (content.left(100).contains("QRZLogbook", Qt::CaseInsensitive)) {
            m_textEdit->append("Loaded normal QRZ Logbook ADIF file: " + m_logfilePath);
        } else {
            m_textEdit->append("Loaded Logfile: " + m_logfilePath);
        }
    } else {
        m_textEdit->append("Could not open logfile: " + m_logfilePath);
        return; // Return if we can't open it
    }
    
    m_uniqueCallsigns.clear();
    m_gridQsos.clear();
    m_dxccWorked.clear();
    m_dxccDetailWorked.clear();
    m_was5BandWorked.clear();
    m_wasDetailWorked.clear();
    m_japanPrefecturesWorked.clear();
    m_japanDetailWorked.clear();
    m_japanCitiesWorked.clear();
    m_usCountiesWorked.clear();
    m_potaWorked.clear();
    m_potaDetailWorked.clear();
    m_potaQsos.clear();
    m_allQsos.clear();

    // Start parsing
    QRegularExpression eorRe("<eor>", QRegularExpression::CaseInsensitiveOption);
    QStringList records = content.split(eorRe);
    
    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    
    QRegularExpression re("POTA[\\s\\-:]*([A-Z0-9]{1,4}-?\\d{3,5})");
    QRegularExpression re2("\\b([A-Z0-9]{1,4}-\\d{3,5})\\b");
    int recCount = 0;
    int totalRecs = records.size();
    
    for (const QString &rec : records) {
        recCount++;
        if (recCount % 500 == 0 || recCount == totalRecs) {
            if (m_splash && totalRecs > 0) {
                int progress = 45 + (int)(45.0 * recCount / totalRecs);
                m_splash->setProgress(progress, QString("Parsing QSO %1 of %2...").arg(recCount).arg(totalRecs));
            } else {
                QCoreApplication::processEvents();
            }
        }
        QString call = extractAdifField(rec, "CALL").toUpper();
        QString grid = extractAdifField(rec, "GRIDSQUARE").toUpper();
        if (!call.isEmpty()) {
            m_uniqueCallsigns.insert(call);
            QString comment = extractAdifField(rec, "COMMENT").toUpper();
            QString potaRef = extractAdifField(rec, "POTA_REF").toUpper();
            if (potaRef.isEmpty()) potaRef = extractAdifField(rec, "POTA_ID").toUpper();
            if (potaRef.isEmpty()) potaRef = extractAdifField(rec, "APP_QRZLOG_POTA_REF").toUpper();
            if (potaRef.isEmpty() && extractAdifField(rec, "SIG").toUpper() == "POTA") {
                potaRef = extractAdifField(rec, "SIG_INFO").toUpper();
            }
            
            
            QString potaFound;
            if (!potaRef.isEmpty()) potaFound = potaRef;
            else {
                QString fullRecordUpper = rec.toUpper();
                QString qthUpper = extractAdifField(rec, "QTH").toUpper();
                if (qthUpper.isEmpty()) qthUpper = extractAdifField(rec, "APP_QRZLOG_QTH").toUpper();
                QString commentUpper = extractAdifField(rec, "COMMENT").toUpper();
                
                QRegularExpression reStrict("(?:POTA|PARK)[\\s\\-:]*?([A-Z]{1,4}-?\\d{3,5})");
                QRegularExpression reLoose("\\b([A-Z]{1,4}-\\d{3,5})\\b");
                
                QRegularExpressionMatch mStrict = reStrict.match(fullRecordUpper);
                if (mStrict.hasMatch()) {
                    QString matchStr = mStrict.captured(1);
                    if (!matchStr.contains("-")) {
                        int pos = 0;
                        while (pos < matchStr.length() && !matchStr[pos].isDigit()) pos++;
                        if (pos > 0 && pos < matchStr.length()) matchStr.insert(pos, "-");
                    }
                    potaFound = matchStr;
                } else {
                    QRegularExpressionMatch mQth = reLoose.match(qthUpper);
                    if (mQth.hasMatch() && (qthUpper.contains("POTA") || qthUpper.contains("PARK"))) {
                        potaFound = mQth.captured(1);
                    } else {
                        QRegularExpressionMatch mComment = reLoose.match(commentUpper);
                        if (mComment.hasMatch() && (commentUpper.contains("POTA") || commentUpper.contains("PARK"))) {
                            potaFound = mComment.captured(1);
                        }
                    }
                }
                
                if (!potaFound.isEmpty()) {
                    QString prefix = potaFound.section('-', 0, 0).toUpper();
                    if (prefix == "NA" || prefix == "SA" || prefix == "EU" || prefix == "AS" || prefix == "AF" || prefix == "OC" || prefix == "AN") {
                        potaFound.clear();
                    }
                }
            }
            
            QsoDetail d;
            d.date = extractAdifField(rec, "QSO_DATE");
            d.time = extractAdifField(rec, "TIME_ON");
            QString freqStr = extractAdifField(rec, "FREQ");
            if (!freqStr.isEmpty()) {
                bool ok;
                double f = freqStr.toDouble(&ok);
                if (ok) d.freq = QString::number(f, 'f', 4);
                else d.freq = freqStr;
            }
            d.band = extractAdifField(rec, "BAND").toUpper();
            d.mode = extractAdifField(rec, "MODE").toUpper();
            {
                QString sub = extractAdifField(rec, "SUBMODE").toUpper();
                if (!sub.isEmpty()) {
                    d.mode = sub;
                }
            }
            d.call = call;
            d.country = extractAdifField(rec, "COUNTRY").toUpper();
            d.state = extractAdifField(rec, "STATE").toUpper();
            d.dxcc = extractAdifField(rec, "DXCC");
            d.pota = potaFound;
            d.grid = grid;
            d.lat = extractAdifField(rec, "LAT");
            d.lon = extractAdifField(rec, "LON");
            if (d.lat.isEmpty()) d.lat = extractAdifField(rec, "APP_QRZLOG_LAT");
            if (d.lon.isEmpty()) d.lon = extractAdifField(rec, "APP_QRZLOG_LON");
            
            d.rst_rcvd = extractAdifField(rec, "RST_RCVD");
            d.rst_sent = extractAdifField(rec, "RST_SENT");
            d.tx_pwr = extractAdifField(rec, "TX_PWR");
            d.qslsdate = extractAdifField(rec, "QSLSDATE");
            d.qslrdate = extractAdifField(rec, "QSLRDATE");
            d.qsl_sent = extractAdifField(rec, "QSL_SENT");
            d.qsl_rcvd = extractAdifField(rec, "QSL_RCVD");
            d.qslVia = extractAdifField(rec, "QSL_VIA");
            d.qslMsg = extractAdifField(rec, "QSLMSG");
            
            QString lotwRx = extractAdifField(rec, "APP_LOTW_RXQSL");
            d.lotwRcvd = extractAdifField(rec, "APP_LOTW_QSO_RCVD").toUpper() == "Y" || extractAdifField(rec, "LOTW_QSL_RCVD").toUpper() == "Y" || (!lotwRx.isEmpty() && lotwRx != "00000000");
            d.mqslRcvd = extractAdifField(rec, "QSL_RCVD").toUpper() == "Y";
            d.qrzRcvd = extractAdifField(rec, "APP_QRZLOG_STATUS").toUpper() == "C" || extractAdifField(rec, "QRZCOM_QSL_RCVD").toUpper() == "Y" || extractAdifField(rec, "QRZCOM_QSL_RCVD").toUpper() == "C" || extractAdifField(rec, "APP_QRZ_QSO_RCVD").toUpper() == "Y";
            d.cardRcvd = (d.qsl_rcvd.toUpper() == "Y" || !d.qslrdate.isEmpty());

            
            d.county = extractAdifField(rec, "CNTY");
            
            qrzCache.beginGroup(call);
            if (d.country.isEmpty()) d.country = qrzCache.value("country").toString().toUpper();
            if (d.state.isEmpty()) d.state = qrzCache.value("qrz_state").toString().toUpper();
            if (d.dxcc.isEmpty()) d.dxcc = qrzCache.value("dxcc").toString();
            if (d.county.isEmpty()) d.county = qrzCache.value("county").toString();
            if (d.grid.isEmpty()) {
                d.grid = qrzCache.value("grid").toString();
                grid = d.grid;
            }
            if (d.lat.isEmpty()) d.lat = qrzCache.value("lat").toString();
            if (d.lon.isEmpty()) d.lon = qrzCache.value("lon").toString();
            qrzCache.endGroup();
            
            m_allQsos[call].append(d);
        }
    }
    // Process awards AFTER the logfile is fully loaded
    for (auto callIt = m_allQsos.constBegin(); callIt != m_allQsos.constEnd(); ++callIt) {
        for (const QsoDetail &d : callIt.value()) {
            trackQsoStats(d);
        }
    }
    
    
    updateQsoCount();
    updateGlobe();
    
    qDebug() << "loadLogfile finished. Total unique callsigns:" << m_uniqueCallsigns.size() << "Total allQsos groups:" << m_allQsos.size();
}

void MainWindow::prepareNewAdi()
{
    if (m_logfilePath.isEmpty()) {
        m_textEdit->append("No logfile selected to maintain.");
        return;
    }
    
    QFile file(m_logfilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_textEdit->append("Failed to read logfile for maintenance.");
        return;
    }
    
    QString content = file.readAll();
    file.close();
    
    int qIdx = content.indexOf('Q', 0, Qt::CaseInsensitive);
    int eohIdx = content.indexOf("<eoh>", 0, Qt::CaseInsensitive);
    
    if (qIdx != -1 && eohIdx != -1 && qIdx < eohIdx) {
        int lengthToRemove = eohIdx - qIdx + 5;
        content.remove(qIdx, lengthToRemove);
        m_textEdit->append("ADIF header stripped.");
    } else {
        m_textEdit->append("Header markers not found, proceeding with consolidation...");
    }
    
    QStringList lines = content.split('\n');
    int totalLines = lines.size();
    
    QStringList formattedRecords;
    QString currentRecord;
    int processedLines = 0;
    
    for (int i = 0; i < totalLines; ++i) {
        QString line = lines.at(i).trimmed();
        if (line.isEmpty()) continue;
        
        currentRecord += line + " ";
        
        if (line.contains("<eor>", Qt::CaseInsensitive)) {
            formattedRecords.append(currentRecord.trimmed());
            currentRecord.clear();
        }
        
        processedLines++;
        
        m_progressBar->setRange(0, totalLines);
        if (processedLines % 100 == 0) {
            m_progressBar->setValue(processedLines);
            QCoreApplication::processEvents();
        }
    }
    m_progressBar->setValue(totalLines);
    
    if (!currentRecord.trimmed().isEmpty()) {
        formattedRecords.append(currentRecord.trimmed());
    }
    
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &rec : formattedRecords) {
            out << rec << "\n";
        }
        file.close();
        m_textEdit->append("Successfully consolidated ADIF records. Reloading...");
        loadLogfile();
    } else {
        m_textEdit->append("Failed to write consolidated data back to file.");
    }
}

void MainWindow::updateLocalAdifQslStatus(const QList<QsoDetail> &selectedQsos)
{
    if (m_logfilePath.isEmpty()) return;
    
    QFile file(m_logfilePath);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) return;
    
    QString content = file.readAll();
    bool modified = false;
    QString today = QDateTime::currentDateTime().toString("yyyyMMdd");
    
    // Split full content into records robustly by <EOR> instead of by line
    QRegularExpression eorRe("<eor>", QRegularExpression::CaseInsensitiveOption);
    QStringList records = content.split(eorRe);
    
    QList<QsoDetail> remaining = selectedQsos;
    
    for (int i = 0; i < records.size(); ++i) {
        QString &rec = records[i];
        if (rec.trimmed().isEmpty()) continue;
        
        for (int j = 0; j < remaining.size(); ++j) {
            const QsoDetail &d = remaining[j];
            QString recCall = extractAdifField(rec, "CALL").trimmed().toUpper();
            QString recDate = extractAdifField(rec, "QSO_DATE").trimmed();
            
            bool isUniqueMatch = (recCall == d.call.trimmed().toUpper()) && 
                                 (recDate == d.date.trimmed());
            
            if (isUniqueMatch) {
                if (!d.band.isEmpty()) {
                    QString recBand = extractAdifField(rec, "BAND").trimmed().toUpper();
                    if (recBand != d.band.trimmed().toUpper()) isUniqueMatch = false;
                }
                if (!d.mode.isEmpty()) {
                    QString recMode = extractAdifField(rec, "MODE").trimmed().toUpper();
                    QString recSubmode = extractAdifField(rec, "SUBMODE").trimmed().toUpper();
                    if (!recSubmode.isEmpty()) {
                        recMode = recSubmode;
                    }
                    if (recMode != d.mode.trimmed().toUpper()) isUniqueMatch = false;
                }
            }
            
            if (isUniqueMatch) {
                QString dTime = d.time.trimmed();
                dTime.remove(':');
                dTime = dTime.left(4);
                
                QString recTime = extractAdifField(rec, "TIME_ON").trimmed();
                recTime.remove(':');
                recTime = recTime.left(4);
                
                if (!dTime.isEmpty() && !recTime.isEmpty()) {
                    if (recTime != dTime) {
                        isUniqueMatch = false;
                    }
                }
            }
            
            if (isUniqueMatch) {
                // Update QSL_SENT to Y
                QRegularExpression sentRe("<QSL_SENT:\\d+>[^\\s<]*", QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch sentMatch = sentRe.match(rec);
                QString newSentTag = "<QSL_SENT:1>Y";
                if (sentMatch.hasMatch()) {
                    rec.replace(sentMatch.captured(0), newSentTag);
                } else {
                    rec.append(" " + newSentTag + " ");
                }
                
                // Update QSLSDATE to today's date
                QRegularExpression dateRe("<QSLSDATE:\\d+>[^\\s<]*", QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch dateMatch = dateRe.match(rec);
                QString newDateTag = QString("<QSLSDATE:%1>%2").arg(today.length()).arg(today);
                if (dateMatch.hasMatch()) {
                    rec.replace(dateMatch.captured(0), newDateTag);
                } else {
                    rec.append(" " + newDateTag + " ");
                }
                
                // Update MODE if present
                if (!d.mode.isEmpty()) {
                    QRegularExpression modeRe("<MODE:\\d+>[^\\s<]*", QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatch modeMatch = modeRe.match(rec);
                    QString newModeTag = QString("<MODE:%1>%2").arg(d.mode.length()).arg(d.mode);
                    if (modeMatch.hasMatch()) {
                        rec.replace(modeMatch.captured(0), newModeTag);
                    } else {
                        rec.append(" " + newModeTag + " ");
                    }
                }

                // Update RST_SENT if present
                if (!d.rst_sent.isEmpty()) {
                    QRegularExpression rstSentRe("<RST_SENT:\\d+>[^\\s<]*", QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatch rstSentMatch = rstSentRe.match(rec);
                    QString newRstSentTag = QString("<RST_SENT:%1>%2").arg(d.rst_sent.length()).arg(d.rst_sent);
                    if (rstSentMatch.hasMatch()) {
                        rec.replace(rstSentMatch.captured(0), newRstSentTag);
                    } else {
                        rec.append(" " + newRstSentTag + " ");
                    }
                }

                // Update RST_RCVD if present
                if (!d.rst_rcvd.isEmpty()) {
                    QRegularExpression rstRcvdRe("<RST_RCVD:\\d+>[^\\s<]*", QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatch rstRcvdMatch = rstRcvdRe.match(rec);
                    QString newRstRcvdTag = QString("<RST_RCVD:%1>%2").arg(d.rst_rcvd.length()).arg(d.rst_rcvd);
                    if (rstRcvdMatch.hasMatch()) {
                        rec.replace(rstRcvdMatch.captured(0), newRstRcvdTag);
                    } else {
                        rec.append(" " + newRstRcvdTag + " ");
                    }
                }

                // Update NOTES if present
                if (!d.notes.isEmpty()) {
                    QRegularExpression notesRe("<NOTES:\\d+>[^<]*", QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatch notesMatch = notesRe.match(rec);
                    QString newNotesTag = QString("<NOTES:%1>%2").arg(d.notes.length()).arg(d.notes);
                    if (notesMatch.hasMatch()) {
                        rec.replace(notesMatch.captured(0), newNotesTag);
                    } else {
                        rec.append(" " + newNotesTag + " ");
                    }
                }

                // Update TX_PWR if present
                if (!d.tx_pwr.isEmpty()) {
                    QRegularExpression txPwrRe("<TX_PWR:\\d+>[^\\s<]*", QRegularExpression::CaseInsensitiveOption);
                    QRegularExpressionMatch txPwrMatch = txPwrRe.match(rec);
                    QString newTxPwrTag = QString("<TX_PWR:%1>%2").arg(d.tx_pwr.length()).arg(d.tx_pwr);
                    if (txPwrMatch.hasMatch()) {
                        rec.replace(txPwrMatch.captured(0), newTxPwrTag);
                    } else {
                        rec.append(" " + newTxPwrTag + " ");
                    }
                }
                
                modified = true;
                remaining.removeAt(j);
                break; // Stop checking this record against other selected QSOs
            }
        }
    }
    
    if (modified) {
        file.resize(0); // clear file
        file.seek(0);
        QTextStream out(&file);
        for (const QString &rec : records) {
            if (!rec.trimmed().isEmpty()) {
                out << rec << "<EOR>\n";
            }
        }
        file.close();
        m_textEdit->append("Local log file QSL status updated.");
        loadLogfile(); // refresh view
    } else {
        file.close();
    }
}

void MainWindow::setQrzUsername()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("QRZ Username"),
                                         tr("Username:"), QLineEdit::Normal,
                                         m_qrzUsername, &ok);
    if (ok && !text.isEmpty()) {
        m_qrzUsername = text;
        writeSettings(); 
    }
}

void MainWindow::setQrzPassword()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("QRZ Password"),
                                         tr("Password:"), QLineEdit::Password,
                                         m_qrzPassword, &ok);
    if (ok && !text.isEmpty()) {
        m_qrzPassword = text;
        writeSettings();
    }
}

void MainWindow::testQrzLogin(bool silent)
{
    readSettings();
    if (m_qrzUsername.isEmpty() || m_qrzPassword.isEmpty()) {
        if (!silent) m_textEdit->append("Cannot test login: QRZ Username or Password is empty.");
        return;
    }

    if (!silent) m_textEdit->append("Testing QRZ Login for user: " + m_qrzUsername + " ...");

    QString urlStr = QString("https://xmldata.qrz.com/xml/current/?username=%1;password=%2;agent=Qwain")
                         .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_qrzUsername)))
                         .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_qrzPassword)));
    
    QNetworkRequest request((QUrl(urlStr)));
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, silent]() {
        onQrzLoginReply(reply, silent);
    });
}

void MainWindow::onQrzLoginReply(QNetworkReply *reply, bool silent)
{
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        if (!silent) m_textEdit->append("QRZ Test failed with network error: " + reply->errorString());
        return;
    }

    QByteArray responseData = reply->readAll();
    QXmlStreamReader xml(responseData);
    
    QString sessionKey;
    QString errorMsg;
    QString subCount;
    
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            QString name = xml.name().toString();
            if (name == "Key") {
                sessionKey = xml.readElementText();
            } else if (name == "Error") {
                errorMsg = xml.readElementText();
            } else if (name == "SubCount") {
                subCount = xml.readElementText();
            }
        }
    }

    if (!sessionKey.isEmpty()) {
        m_qrzSessionKey = sessionKey;
        writeSettings();
        logSystem(QDateTime::currentDateTimeUtc().toString("dd-MM-yy HH:mm:ss.zzz UTC") + " QRZ Credentials SUCCESS");
        if (!silent) {
            m_textEdit->append("QRZ Test PASSED! Session Key (Seed) obtained: " + sessionKey);
            if (!subCount.isEmpty()) {
                m_textEdit->append("Remaining Daily Queries (SubCount): " + subCount);
            }
            QMessageBox::information(this, tr("QRZ Test Passed"), tr("QRZ Test PASSED! Session Key (Seed) obtained:\n\n%1\n\nRemaining Daily Queries: %2").arg(sessionKey).arg(subCount.isEmpty() ? "Unlimited or Unknown" : subCount));
        }
    } else if (!errorMsg.isEmpty()) {
        logSystem(QDateTime::currentDateTimeUtc().toString("dd-MM-yy HH:mm:ss.zzz UTC") + " QRZ Credentials FAILED: " + errorMsg);
        if (!silent) {
            m_textEdit->append("QRZ Test FAILED: " + errorMsg);
            QMessageBox::critical(this, tr("QRZ Test Failed"), tr("QRZ Test FAILED:\n\n%1").arg(errorMsg));
        }
    } else {
        logSystem(QDateTime::currentDateTimeUtc().toString("dd-MM-yy HH:mm:ss.zzz UTC") + " QRZ Credentials FAILED: Unknown response format or invalid credentials.");
        if (!silent) {
            m_textEdit->append("QRZ Test FAILED: Unknown response format or invalid credentials.");
            QMessageBox::warning(this, tr("QRZ Test Failed"), tr("QRZ Test FAILED: Unknown response format or invalid credentials."));
        }
    }
}

#include "qrzsyncdialog.h"

void MainWindow::forceSyncQrzLog()
{
    readSettings();
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString apiKey = settings.value("Credentials/qrzApiKey", "").toString();
    
    if (m_logfilePath.isEmpty()) {
        QMessageBox::warning(this, "QRZ Sync", "No active logfile configured. Set it in Settings.");
        return;
    }

    if (m_qrzUsername.isEmpty() || m_qrzPassword.isEmpty()) {
        QMessageBox::warning(this, "QRZ Sync", "No QRZ Username/Password present in credentials. Required for sync logic.");
        return;
    }
    
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, "QRZ Sync", "No QRZ API Key configured. Please enter it in Settings under QRZ Credentials.");
        return;
    }
    
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("QRZ Force Sync");
    msgBox.setText("Select Sync Type from QRZ Logbook:\n\nFull Sync: Downloads complete logbook and overwrites local logfile.\nIncremental: Downloads complete logbook but only appends NEW contacts missing locally.");
    QPushButton *btnFull = msgBox.addButton("Full Sync", QMessageBox::ActionRole);
    QPushButton *btnInc = msgBox.addButton("Incremental", QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Cancel);
    msgBox.exec();
    
    if (msgBox.clickedButton() != btnFull && msgBox.clickedButton() != btnInc) {
        return;
    }
    
    bool isFull = (msgBox.clickedButton() == btnFull);
    m_textEdit->append(QString("Starting %1 sync from QRZ Logbook...").arg(isFull ? "Full" : "Incremental"));
    
    QrzSyncDialog dialog(this, apiKey, m_logfilePath, isFull);
    dialog.setSessionKey(m_qrzSessionKey);
    dialog.exec();
    
    if (dialog.syncSuccessful()) {
        m_textEdit->append("QRZ Logbook Sync completed! Reloading logfile...");
        prepareNewAdi();
    } else {
        m_textEdit->append("QRZ Sync was canceled or failed.");
    }
}

#include "countyprepdialog.h"
#include "mailcheckdialog.h"

void MainWindow::startQslMailCheck()
{
    if (m_logfilePath.isEmpty()) {
        QMessageBox::warning(this, "Mail Check", "No active logfile configured. Set it in Settings.");
        return;
    }

    if (m_qrzSessionKey.isEmpty()) {
        QMessageBox::warning(this, "Mail Check", "No active QRZ Session. Testing QRZ login to acquire session...");
        testQrzLogin(true); // Attempt silent login
        if (m_qrzSessionKey.isEmpty()) {
            QMessageBox::warning(this, "Mail Check", "Could not obtain QRZ Session. Cannot proceed.");
            return;
        }
    }

    MailCheckDialog *dialog = new MailCheckDialog(this, m_qrzSessionKey, m_logfilePath);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MainWindow::openCacheViewer()
{
    CacheViewerDialog dialog(this);
    dialog.exec();
}

void MainWindow::openLiveStreamViewer()
{
    LiveStreamViewer dialog(this);
    dialog.exec();
}

void MainWindow::openIconTools()
{
    IconToolsDialog dialog(this);
    dialog.exec();
}

void MainWindow::setMapStyleSatellite()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.setValue("MapStyle", 0);
}

void MainWindow::setMapStyleSimplified()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.setValue("MapStyle", 1);
}

void MainWindow::performRollingConfigBackup()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString confPath = settings.fileName();
    if (confPath.isEmpty() || !QFile::exists(confPath)) {
        return;
    }

    QString bak1 = confPath + ".1.bak";
    QString bak2 = confPath + ".2.bak";
    QString bak3 = confPath + ".3.bak";
    QString bak4 = confPath + ".4.bak";
    QString bak5 = confPath + ".5.bak";

    // If .5.bak exists, archive .1.bak through .5.bak into ./config_bak/ before resetting
    if (QFile::exists(bak5)) {
        QFileInfo confInfo(confPath);
        QDir confDir = confInfo.dir();

        QDir localBakDir("./config_bak");
        if (!localBakDir.exists()) {
            localBakDir.mkpath(".");
        }
        QDir confBakDir(confDir.filePath("config_bak"));
        if (!confBakDir.exists()) {
            confBakDir.mkpath(".");
        }

        QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
        QString archiveName = QString("Config.bak_%1.gz").arg(timeStr);
        QString archivePath = localBakDir.filePath(archiveName);
        QString confArchivePath = confBakDir.filePath(archiveName);

        QStringList bakFiles;
        for (int i = 1; i <= 5; ++i) {
            QString fileI = QString("%1.%2.bak").arg(confPath).arg(i);
            if (QFile::exists(fileI)) {
                bakFiles.append(fileI);
            }
        }

        if (!bakFiles.isEmpty()) {
            QProcess tarProc;
            QStringList args;
            args << "-czf" << archivePath;
            for (const QString &f : bakFiles) {
                args << f;
            }
            tarProc.start("tar", args);
            if (tarProc.waitForFinished(5000) && tarProc.exitCode() == 0) {
                if (archivePath != confArchivePath) {
                    if (QFile::exists(confArchivePath)) QFile::remove(confArchivePath);
                    QFile::copy(archivePath, confArchivePath);
                }
            } else {
                for (const QString &f : bakFiles) {
                    QProcess gzProc;
                    gzProc.start("gzip", QStringList() << "-f" << "-k" << f);
                    gzProc.waitForFinished(2000);
                }
            }

            for (int i = 1; i <= 5; ++i) {
                QString fileI = QString("%1.%2.bak").arg(confPath).arg(i);
                if (QFile::exists(fileI)) {
                    QFile::remove(fileI);
                }
            }
        }
    }

    // Shift backups: .4 -> .5, .3 -> .4, .2 -> .3, .1 -> .2
    auto moveFile = [](const QString &from, const QString &to) {
        if (QFile::exists(from)) {
            if (QFile::exists(to)) QFile::remove(to);
            QFile::rename(from, to);
        }
    };

    moveFile(bak4, bak5);
    moveFile(bak3, bak4);
    moveFile(bak2, bak3);
    moveFile(bak1, bak2);

    // Copy defined configuration file as %defined_file%.1.bak
    if (QFile::exists(bak1)) QFile::remove(bak1);
    QFile::copy(confPath, bak1);
}

void MainWindow::writeSettings()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");

    if (m_sessionStartTime.isValid()) {
        quint64 savedSecs = settings.value("Uptime/TotalSeconds", 0).toULongLong();
        quint64 sessionSecs = m_sessionStartTime.secsTo(QDateTime::currentDateTimeUtc());
        settings.setValue("Uptime/TotalSeconds", savedSecs + sessionSecs);
        m_sessionStartTime = QDateTime::currentDateTimeUtc();
    }
    
    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    // State saving removed since we are no longer a QMainWindow
    
    // History saving removed
    
    settings.endGroup();

    settings.beginGroup("Credentials");
    settings.setValue("username", m_qrzUsername);
    settings.setValue("password", m_qrzPassword);
    settings.setValue("qrzXmlSubscriber", m_qrzXmlSubscriber);
    settings.setValue("qrzSessionKey", m_qrzSessionKey);
    settings.setValue("qrzSessionSaved", QDateTime::currentDateTime());
    settings.endGroup();
    

}

void MainWindow::updateMenuLabels()
{
    if (m_userAction) m_userAction->setText(QString("QRZ Username: %1").arg(m_qrzUsername.isEmpty() ? "Not Set" : m_qrzUsername));
    if (m_ipAction) m_ipAction->setText(QString("Network: %1 over %2").arg(m_ipAddress, m_protocol.isEmpty() ? "TCP" : m_protocol));
    if (m_portAction) m_portAction->setText(QString("Port: %1").arg(m_port));
    
    QString logName = m_logfilePath.isEmpty() ? "Not Set" : QFileInfo(m_logfilePath).fileName();
    if (m_logFileAction) m_logFileAction->setText(QString("Logfile: %1").arg(logName));
}

class BackupPreviewDialog : public QDialog {
public:
    BackupPreviewDialog(const QString &userDir, QWidget *parent = nullptr) : QDialog(parent), m_userDir(userDir) {
        setWindowTitle("Confirm Backup Contents");
        resize(600, 550);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        QLabel *infoLabel = new QLabel(QString("<b>Backup Source Directory:</b><br/>%1").arg(userDir), this);
        infoLabel->setWordWrap(true);
        mainLayout->addWidget(infoLabel);

        mainLayout->addSpacing(10);

        QLabel *treeLabel = new QLabel("<b>Files to be included in the archive:</b>", this);
        mainLayout->addWidget(treeLabel);

        m_treeEdit = new QTextEdit(this);
        m_treeEdit->setReadOnly(true);
        m_treeEdit->setFont(QFont("Monospace", 10));
        mainLayout->addWidget(m_treeEdit);

        // Generate Qt-based tree
        QString qtTree = generateTree(userDir);
        if (qtTree.isEmpty()) {
            qtTree = "(No files or directories found in the specified path via Qt API)";
        }

        // Generate Shell ground-truth
        QString shellTree = runShellFind(userDir);

        // Combine both for absolute reliability and transparency
        QString resolvedPath = QDir(userDir).absolutePath();
        m_treeText = "=== INSPECTING DIRECTORY PATH ===\n" + resolvedPath + "\n=================================\n\n"
                     "--- Qt traversal tree ---\n" + qtTree + "\n" + shellTree;
        m_treeEdit->setPlainText(m_treeText);

        QHBoxLayout *btnLayout = new QHBoxLayout();

        QPushButton *btnCopy = new QPushButton("Copy Tree", this);
        connect(btnCopy, &QPushButton::clicked, this, &BackupPreviewDialog::onCopyClicked);
        btnLayout->addWidget(btnCopy);

        btnLayout->addStretch();

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        buttonBox->button(QDialogButtonBox::Ok)->setText("Proceed with Backup");
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        btnLayout->addWidget(buttonBox);

        mainLayout->addLayout(btnLayout);
    }

    QString getTreeText() const { return m_treeText; }

private:
    void onCopyClicked() {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
        // Create a unique identifier to verify each clipboard copy event is fresh
        QString runHash = QString::number(QDateTime::currentMSecsSinceEpoch() % 1000000);
        
        QString clipboardText = m_treeText;
        clipboardText += "\n\n========================================\n";
        clipboardText += "  Clipboard Copy Verification\n";
        clipboardText += "  Timestamp: " + timestamp + "\n";
        clipboardText += "  Verification ID: " + runHash + "\n";
        clipboardText += "========================================\n";

        QGuiApplication::clipboard()->setText(clipboardText);
        QMessageBox::information(this, "Copied", QString("Directory tree copied to clipboard!\n\nVerification ID: %1").arg(runHash));
    }

    QString m_userDir;
    QString m_treeText;
    QTextEdit *m_treeEdit;

    QString generateTree(const QString &dirPath, const QString &indent = "") {
        QDir dir(dirPath);
        if (!dir.exists()) return "";

        QString result;
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDir::Name | QDir::DirsFirst);
        
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);
            bool isLast = (i == list.size() - 1);
            QString prefix = isLast ? "└── " : "├── ";
            
            result += indent + prefix + fileInfo.fileName();
            if (fileInfo.isDir()) {
                result += "/\n";
                QString nextIndent = indent + (isLast ? "    " : "│   ");
                result += generateTree(fileInfo.absoluteFilePath(), nextIndent);
            } else {
                double kb = fileInfo.size() / 1024.0;
                result += QString(" (%1 KB)\n").arg(kb, 0, 'f', 2);
            }
        }
        return result;
    }

    QString runShellFind(const QString &dirPath) {
        QProcess proc;
        QStringList args;
        args << dirPath;
        proc.start("find", args);
        if (proc.waitForFinished(3000)) {
            QString out = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
            if (!out.isEmpty()) {
                QStringList rawLines = out.split('\n');
                QString formatted = "\n--- Shell Ground-Truth (find) ---\n";
                for (const QString &line : rawLines) {
                    if (line == dirPath) continue;
                    QFileInfo fi(line);
                    QString rel = line.mid(dirPath.length());
                    if (rel.startsWith('/') || rel.startsWith('\\')) rel = rel.mid(1);
                    if (fi.isDir()) {
                        formatted += QString("└── %1/\n").arg(rel);
                    } else {
                        double kb = fi.size() / 1024.0;
                        formatted += QString("└── %1 (%2 KB)\n").arg(rel).arg(kb, 0, 'f', 2);
                    }
                }
                return formatted;
            }
        }
        return "\n--- Shell Ground-Truth (find) ---\n(Shell find command failed or returned nothing)\n";
    }
};

class BackupProgressDialog : public QDialog {
public:
    BackupProgressDialog(const QStringList &args, QWidget *parent = nullptr) 
        : QDialog(parent), m_args(args), m_hasError(false) 
    {
        setWindowTitle("Archiving Progress");
        resize(550, 380);
        setModal(true);

        m_layout = new QVBoxLayout(this);

        m_statusLabel = new QLabel("Initializing backup process...", this);
        m_statusLabel->setStyleSheet("font-weight: bold; font-size: 11pt;");
        m_layout->addWidget(m_statusLabel);

        m_progressBar = new QProgressBar(this);
        m_progressBar->setRange(0, 0); // Busy / marquee indicator
        m_progressBar->setTextVisible(false);
        m_layout->addWidget(m_progressBar);

        m_logEdit = new QTextEdit(this);
        m_logEdit->setReadOnly(true);
        m_logEdit->setFont(QFont("Monospace", 9));
        m_layout->addWidget(m_logEdit);
    }

    bool hasError() const { return m_hasError; }
    QString errorString() const { return m_errorString; }

    int exec() override {
        show();
        QCoreApplication::processEvents();

        QProcess proc;
        proc.start("tar", m_args);

        while (!proc.waitForFinished(50)) {
            QCoreApplication::processEvents();
            
            QByteArray stdOutData = proc.readAllStandardOutput();
            if (!stdOutData.isEmpty()) {
                QString text = QString::fromUtf8(stdOutData);
                QStringList lines = text.split('\n');
                for (const QString &line : lines) {
                    if (line.trimmed().isEmpty()) continue;
                    m_logEdit->append("Added: " + line);
                    m_statusLabel->setText(QString("Archiving: %1").arg(QFileInfo(line).fileName()));
                }
            }
            
            QByteArray stdErrData = proc.readAllStandardError();
            if (!stdErrData.isEmpty()) {
                m_errorString += QString::fromUtf8(stdErrData);
            }
        }

        // Read remaining data after finish
        QByteArray remainingOut = proc.readAllStandardOutput();
        if (!remainingOut.isEmpty()) {
            QString text = QString::fromUtf8(remainingOut);
            QStringList lines = text.split('\n');
            for (const QString &line : lines) {
                if (line.trimmed().isEmpty()) continue;
                m_logEdit->append("Added: " + line);
            }
        }
        QByteArray remainingErr = proc.readAllStandardError();
        if (!remainingErr.isEmpty()) {
            m_errorString += QString::fromUtf8(remainingErr);
        }

        if (proc.exitCode() != 0) {
            m_hasError = true;
            if (m_errorString.isEmpty()) {
                m_errorString = "Tar process exited with error code " + QString::number(proc.exitCode());
            }
            m_statusLabel->setText("Backup failed!");
            m_progressBar->setRange(0, 100);
            m_progressBar->setValue(0);
            
            QPushButton *btnClose = new QPushButton("Close", this);
            m_layout->addWidget(btnClose);
            connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);
            QDialog::exec();
            return QDialog::Rejected;
        } else {
            m_remainingSecs = 4;
            m_statusLabel->setText("Backup completed successfully! Auto-closing in 4s...");
            m_progressBar->setRange(0, 100);
            m_progressBar->setValue(100);
            
            QPushButton *btnOk = new QPushButton("Finish", this);
            m_layout->addWidget(btnOk);
            connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
            
            QTimer *timer = new QTimer(this);
            connect(timer, &QTimer::timeout, this, [this, timer]() {
                m_remainingSecs--;
                if (m_remainingSecs <= 0) {
                    timer->stop();
                    accept();
                } else {
                    m_statusLabel->setText(QString("Backup completed successfully! Auto-closing in %1s...").arg(m_remainingSecs));
                }
            });
            timer->start(1000);

            QDialog::exec();
            return QDialog::Accepted;
        }
    }

private:
    QStringList m_args;
    QVBoxLayout *m_layout;
    QLabel *m_statusLabel;
    QProgressBar *m_progressBar;
    QTextEdit *m_logEdit;
    bool m_hasError;
    QString m_errorString;
    int m_remainingSecs;
};

void MainWindow::backupData(const QString &userDir)
{
    QFileDialog dirDlg(this, "Select Directory to Backup", userDir.isEmpty() ? QDir::homePath() : userDir);
    dirDlg.setFileMode(QFileDialog::Directory);
    dirDlg.setOption(QFileDialog::ShowDirsOnly, true);
    dirDlg.setOption(QFileDialog::DontUseNativeDialog, true);
    dirDlg.setFilter(QDir::Dirs | QDir::Hidden | QDir::AllDirs | QDir::NoDotAndDotDot);
    
    if (dirDlg.exec() != QDialog::Accepted) {
        return;
    }
    
    QString selectedDir = dirDlg.selectedFiles().first();
    if (selectedDir.isEmpty()) {
        return;
    }

    QDir targetDir(selectedDir);
    QString cleanUserDir = targetDir.absolutePath();
    while (cleanUserDir.endsWith('/') || cleanUserDir.endsWith('\\')) {
        cleanUserDir.chop(1);
    }

    QDir cleanDir(cleanUserDir);
    if (!cleanDir.exists()) {
        if (!cleanDir.mkpath(".")) {
            QMessageBox::warning(this, "Backup Failed", "Selected Directory does not exist and could not be created.");
            return;
        }
    }

    BackupPreviewDialog previewDlg(cleanUserDir, this);
    if (previewDlg.exec() != QDialog::Accepted) {
        return;
    }

    QFileDialog fileDlg(this, "Save Full Backup", QDir::homePath() + "/qwain_backup.tar.gz", "Tar GZIP (*.tar.gz *.tgz)");
    fileDlg.setAcceptMode(QFileDialog::AcceptSave);
    fileDlg.setOption(QFileDialog::DontUseNativeDialog, true);
    fileDlg.setFilter(fileDlg.filter() | QDir::Hidden);
    if (fileDlg.exec() != QDialog::Accepted) return;
    QString dest = fileDlg.selectedFiles().first();
    if (dest.isEmpty()) return;

    QFileInfo userDirInfo(cleanUserDir);
    QString parentDir = userDirInfo.absolutePath();
    QString dirName = userDirInfo.fileName();

    if (dirName.isEmpty()) {
        dirName = ".";
        parentDir = cleanUserDir;
    }

    QStringList args;
    args << "-cvzf" << dest << "-C" << parentDir << dirName;

    BackupProgressDialog progDlg(args, this);
    if (progDlg.exec() != QDialog::Accepted || progDlg.hasError()) {
        QMessageBox::critical(this, "Backup Failed", "Error creating backup:\n" + progDlg.errorString());
        return;
    }

    // Backup completed successfully! Now calculate all metrics.
    int fileCount = 0;
    int imageCount = 0;
    int contactCount = 0;
    qint64 uncompressedSize = 0;

    QRegularExpression eorRe("<eor>", QRegularExpression::CaseInsensitiveOption);

    QDirIterator it(cleanUserDir, QDir::Files | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        fileCount++;
        QFileInfo info = it.fileInfo();
        uncompressedSize += info.size();

        QString suffix = info.suffix().toLower();
        if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || suffix == "gif" || suffix == "bmp" || suffix == "svg") {
            imageCount++;
        }

        if (suffix == "adi" || suffix == "adif") {
            QFile f(info.filePath());
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString content = QString::fromUtf8(f.readAll());
                QRegularExpressionMatchIterator rIt = eorRe.globalMatch(content);
                while (rIt.hasNext()) {
                    rIt.next();
                    contactCount++;
                }
            }
        }
    }

    qint64 compressedSize = QFileInfo(dest).size();
    double ratioPercent = 0.0;
    if (uncompressedSize > 0) {
        ratioPercent = (1.0 - (double)compressedSize / uncompressedSize) * 100.0;
    }
    double compressionRatio = 1.0;
    if (compressedSize > 0) {
        compressionRatio = (double)uncompressedSize / compressedSize;
    }

    // Generate MD5 hash of the backup file
    QString md5Hex = "-";
    QFile f(dest);
    if (f.open(QIODevice::ReadOnly)) {
        QCryptographicHash hash(QCryptographicHash::Md5);
        if (hash.addData(&f)) {
            md5Hex = hash.result().toHex().toLower();
        }
    }

    // Save metadata to QSettings
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("MostRecentBackup");
    
    QString dateStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    settings.setValue("date", dateStr);
    settings.setValue("contacts", contactCount);
    settings.setValue("images", imageCount);
    settings.setValue("files", fileCount);
    
    // Formatting size nicely
    auto formatSize = [](qint64 bytes) -> QString {
        double kb = bytes / 1024.0;
        double mb = kb / 1024.0;
        if (mb >= 1.0) return QString("%1 MB").arg(mb, 0, 'f', 2);
        return QString("%1 KB").arg(kb, 0, 'f', 2);
    };

    settings.setValue("uncompressedSize", formatSize(uncompressedSize));
    settings.setValue("compressedSize", formatSize(compressedSize));
    settings.setValue("ratio", QString("%1% (or %2:1)").arg(ratioPercent, 0, 'f', 2).arg(compressionRatio, 0, 'f', 2));
    settings.setValue("md5", md5Hex);
    settings.setValue("path", dest);
    settings.endGroup();
    settings.sync();

    QMessageBox::information(this, "Backup", "Backup completed successfully!");
}

void MainWindow::restoreData(const QString &userDir)
{
    if (userDir.isEmpty()) {
        QMessageBox::warning(this, "Restore Failed", "User Directory configuration is invalid.");
        return;
    }

    QDir targetDir(userDir);
    QString cleanUserDir = targetDir.absolutePath();
    while (cleanUserDir.endsWith('/') || cleanUserDir.endsWith('\\')) {
        cleanUserDir.chop(1);
    }

    QFileDialog fileDlg(this, "Select Backup File", QDir::homePath(), "Tar GZIP (*.tar.gz *.tgz)");
    fileDlg.setFileMode(QFileDialog::ExistingFile);
    fileDlg.setOption(QFileDialog::DontUseNativeDialog, true);
    fileDlg.setFilter(fileDlg.filter() | QDir::Hidden);
    if (fileDlg.exec() != QDialog::Accepted) return;
    QString src = fileDlg.selectedFiles().first();
    if (src.isEmpty()) return;

    if (QMessageBox::question(this, "Confirm Restore", "This will overwrite all existing configurations and files inside the User Directory. Proceed?") != QMessageBox::Yes) {
        return;
    }

    QFileInfo userDirInfo(cleanUserDir);
    QString parentPath = userDirInfo.absolutePath();

    QStringList args;
    args << "-xzf" << src << "-C" << parentPath;

    QProcess proc;
    proc.start("tar", args);
    proc.waitForFinished();
    if (proc.exitCode() == 0) {
        QMessageBox::information(this, "Restore", "Restore complete! The application will now restart to apply settings.");
        QProcess::startDetached(qApp->applicationFilePath(), qApp->arguments().mid(1));
        QApplication::quit();
    } else {
        QMessageBox::critical(this, "Restore Failed", "Error during restore:\n" + proc.readAllStandardError());
    }
}

// persona methods removed

void MainWindow::updateGlobe() {
    if (!m_globeWidget) return;
    QList<GlobeMarker> markers;
    
    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);

    for (auto it = m_allQsos.constBegin(); it != m_allQsos.constEnd(); ++it) {
        if (it.value().isEmpty()) continue;
        const auto &qList = it.value();
        
        bool confirmed = false;
        for (const auto &q : qList) {
            if (q.lotwRcvd || q.cardRcvd || q.mqslRcvd || !q.qslsdate.isEmpty()) {
                confirmed = true;
                break;
            }
        }
        
        double lat = 0.0, lon = 0.0;
        bool hasLoc = false;
        
        auto parseCoord = [](QString s) -> double {
            s = s.toUpper().trimmed();
            if (s.isEmpty()) return 0.0;
            
            bool ok;
            double val = s.toDouble(&ok);
            if (ok) return val;
            
            int sign = 1;
            if (s.contains('S') || s.contains('W')) sign = -1;
            s.replace(QRegularExpression("[NWES]"), "");
            s = s.trimmed();
            
            int spaceIdx = s.indexOf(' ');
            if (spaceIdx != -1) {
                double deg = s.left(spaceIdx).toDouble();
                double min = s.mid(spaceIdx + 1).toDouble();
                return sign * (deg + min / 60.0);
            } else {
                return sign * s.toDouble();
            }
        };

        for (const auto &q : qList) {
            if (!q.lat.isEmpty() && !q.lon.isEmpty()) {
                lat = parseCoord(q.lat);
                lon = parseCoord(q.lon);
                hasLoc = true;
                break;
            } else if (!q.grid.isEmpty() && q.grid.length() >= 4) {
                QString g = q.grid.toUpper();
                int lonDeg = (g[0].unicode() - 'A') * 20;
                int latDeg = (g[1].unicode() - 'A') * 10;
                int lonMin = (g[2].unicode() - '0') * 2;
                int latMin = (g[3].unicode() - '0') * 1;
                
                lon = -180.0 + lonDeg + lonMin + 1.0; 
                lat = -90.0 + latDeg + latMin + 0.5;
                hasLoc = true;
                break;
            }
        }
        
        if (!hasLoc) {
            qrzCache.beginGroup(it.key());
            if (qrzCache.contains("qrz_lat") && qrzCache.contains("qrz_lon")) {
                QString tempLat = qrzCache.value("qrz_lat").toString();
                QString tempLon = qrzCache.value("qrz_lon").toString();
                if (!tempLat.isEmpty() && !tempLon.isEmpty()) {
                    lat = tempLat.toDouble();
                    lon = tempLon.toDouble();
                    hasLoc = true;
                }
            } else if (qrzCache.contains("lat") && qrzCache.contains("lon")) {
                QString tempLat = qrzCache.value("lat").toString();
                QString tempLon = qrzCache.value("lon").toString();
                if (!tempLat.isEmpty() && !tempLon.isEmpty()) {
                    lat = parseCoord(tempLat);
                    lon = parseCoord(tempLon);
                    hasLoc = true;
                }
            }
            qrzCache.endGroup();
        }
        
        if (hasLoc) {
            QColor color = confirmed ? Qt::green : Qt::red;
            markers.append({lat, lon, it.key(), color});
        }
    }
    
    qDebug() << "updateGlobe formed" << markers.size() << "markers.";
    m_globeWidget->setMarkers(markers);
}

void MainWindow::logSystem(const QString& msg) {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("Files");
    QString sysLogPath = settings.value("systemLogfile", "").toString();
    settings.endGroup();
    
    if (sysLogPath.isEmpty()) return;
    
    QFile file(sysLogPath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << msg << "\n";
    }
}

void MainWindow::logDebug(const QString& msg) {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("Files");
    QString debugLogPath = settings.value("debugLogfile", "").toString();
    settings.endGroup();
    
    if (debugLogPath.isEmpty()) return;
    
    QFile file(debugLogPath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << msg << "\n";
    }
}

void MainWindow::openAwardMap(const QString &awStr) {
    // Stub implementation to satisfy linker
}

void MainWindow::updateLeaderboard() {
    // Stub implementation to satisfy linker
}

void MainWindow::onXmlTestClicked() {
    readSettings();
    if (m_qrzUsername.isEmpty() || m_qrzPassword.isEmpty()) {
        QMessageBox::warning(this, tr("QRZ XML Test"), tr("QRZ Username or Password is empty. Please configure them first in Settings > Misc > QRZ Credentials."));
        return;
    }

    bool ok = false;
    QString callsign = QInputDialog::getText(this, tr("QRZ XML Test Lookup"),
                                             tr("Enter Callsign to check:"),
                                             QLineEdit::Normal, "", &ok);
    if (!ok || callsign.trimmed().isEmpty()) {
        return;
    }
    callsign = callsign.trimmed().toUpper();

    m_textEdit->append(QString("XML Test: Authenticating user %1 for lookup of %2...").arg(m_qrzUsername, callsign));

    // Step 1: Login request to get a session key
    QString urlStr = QString("https://xmldata.qrz.com/xml/current/?username=%1;password=%2;agent=Qwain")
                         .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_qrzUsername)))
                         .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_qrzPassword)));
    
    QNetworkRequest request((QUrl(urlStr)));
    QNetworkReply *reply = m_networkManager->get(request);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, callsign]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QString errMsg = tr("Network error obtaining session key: %1").arg(reply->errorString());
            m_textEdit->append("XML Test: " + errMsg);
            QMessageBox::critical(this, tr("QRZ XML Test Error"), errMsg);
            return;
        }

        QByteArray responseData = reply->readAll();
        QXmlStreamReader xml(responseData);
        QString sessionKey;
        QString errorMsg;
        QString subCount;

        while (!xml.atEnd() && !xml.hasError()) {
            QXmlStreamReader::TokenType token = xml.readNext();
            if (token == QXmlStreamReader::StartElement) {
                QString name = xml.name().toString();
                if (name == "Key") {
                    sessionKey = xml.readElementText();
                } else if (name == "Error") {
                    errorMsg = xml.readElementText();
                } else if (name == "SubCount") {
                    subCount = xml.readElementText();
                }
            }
        }

        if (sessionKey.isEmpty()) {
            QString err = errorMsg.isEmpty() ? tr("Unknown login response/credentials failure.") : errorMsg;
            m_textEdit->append("XML Test failed: " + err);
            
            QDialog *errDlg = new QDialog(this);
            errDlg->setWindowTitle(tr("QRZ XML Error Response"));
            QVBoxLayout *layout = new QVBoxLayout(errDlg);
            QTextEdit *pe = new QTextEdit(errDlg);
            pe->setReadOnly(true);
            pe->setPlainText(QString::fromUtf8(responseData));
            pe->setFont(QFont("Courier New", 11));
            layout->addWidget(pe);
            QPushButton *closeBtn = new QPushButton(tr("Close"), errDlg);
            connect(closeBtn, &QPushButton::clicked, errDlg, &QDialog::accept);
            layout->addWidget(closeBtn);
            errDlg->resize(500, 350);
            errDlg->exec();
            errDlg->deleteLater();
            return;
        }

        m_qrzSessionKey = sessionKey;
        writeSettings();
        m_textEdit->append(QString("XML Test: Login Success. Session Key: %1").arg(sessionKey));
        if (!subCount.isEmpty()) {
            m_textEdit->append(QString("XML Test: Remaining Daily Queries (SubCount): %1").arg(subCount));
        }
        m_textEdit->append(QString("XML Test: Fetching raw record for %1...").arg(callsign));

        // Step 2: Fetch the callsign record
        QString lookupUrl = QString("https://xmldata.qrz.com/xml/current/?s=%1;callsign=%2")
                                .arg(sessionKey)
                                .arg(callsign);
        
        QNetworkRequest lookupReq((QUrl(lookupUrl)));
        QNetworkReply *lookupReply = m_networkManager->get(lookupReq);
        
        connect(lookupReply, &QNetworkReply::finished, this, [this, lookupReply, callsign]() {
            lookupReply->deleteLater();
            if (lookupReply->error() != QNetworkReply::NoError) {
                QString errMsg = tr("Network error fetching callsign record: %1").arg(lookupReply->errorString());
                m_textEdit->append("XML Test: " + errMsg);
                QMessageBox::critical(this, tr("QRZ XML Test Error"), errMsg);
                return;
            }

            QByteArray lookupData = lookupReply->readAll();
            m_textEdit->append(QString("XML Test: Finished fetching %1 data successfully.").arg(callsign));

            // Display in raw XML response dialog
            QDialog *xmlDlg = new QDialog(this);
            xmlDlg->setWindowTitle(tr("QRZ XML Raw Response for %1").arg(callsign));
            
            QVBoxLayout *layout = new QVBoxLayout(xmlDlg);
            
            QLabel *infoLbl = new QLabel(tr("<b>Raw XML Response:</b>"), xmlDlg);
            layout->addWidget(infoLbl);

            QTextEdit *pe = new QTextEdit(xmlDlg);
            pe->setReadOnly(true);
            pe->setPlainText(QString::fromUtf8(lookupData));
            
            QFont monoFont("Courier New");
            monoFont.setStyleHint(QFont::Monospace);
            monoFont.setPointSize(11);
            pe->setFont(monoFont);
            layout->addWidget(pe);

            QHBoxLayout *btnLayout = new QHBoxLayout();
            QPushButton *copyBtn = new QPushButton(tr("Copy to Clipboard"), xmlDlg);
            connect(copyBtn, &QPushButton::clicked, this, [pe]() {
                QApplication::clipboard()->setText(pe->toPlainText());
            });
            btnLayout->addWidget(copyBtn);

            QPushButton *closeBtn = new QPushButton(tr("Close"), xmlDlg);
            connect(closeBtn, &QPushButton::clicked, xmlDlg, &QDialog::accept);
            btnLayout->addWidget(closeBtn);

            layout->addLayout(btnLayout);
            xmlDlg->resize(650, 500);
            xmlDlg->exec();
        });
    });
}

void MainWindow::onCountyPrepClicked() {
    if (m_logfilePath.isEmpty()) {
        QMessageBox::warning(this, tr("Initialize US County Cache"), tr("No active logfile configured. Set it in Settings first."));
        return;
    }

    readSettings();
    if (m_qrzUsername.isEmpty() || m_qrzPassword.isEmpty()) {
        QMessageBox::warning(this, tr("Initialize US County Cache"), tr("QRZ credentials are empty. Please configure them first in Settings > Misc > QRZ Credentials."));
        return;
    }

    auto launchPrepDialog = [this]() {
        // Collect all unique callsigns in m_allQsos matching DXCC 291, 6, 110, 285
        QStringList callsToQuery;
        for (auto it = m_allQsos.constBegin(); it != m_allQsos.constEnd(); ++it) {
            QString call = it.key().toUpper();
            bool match = false;
            for (const QsoDetail &d : it.value()) {
                QString dxcc = d.dxcc.trimmed();
                QString country = d.country.toUpper().trimmed();
                if (dxcc == "291" || dxcc == "6" || dxcc == "110" || dxcc == "285" ||
                    country == "UNITED STATES OF AMERICA" || country == "UNITED STATES" || country == "USA" ||
                    country == "ALASKA" || country == "HAWAII" || country == "VIRGIN ISLANDS" || country == "U.S. VIRGIN ISLANDS") {
                    match = true;
                    break;
                }
            }
            if (match) {
                callsToQuery.append(call);
            }
        }

        if (callsToQuery.isEmpty()) {
            QMessageBox::information(this, tr("Initialize US County Cache"), tr("No US-based (DXCC 291, 6, 110, 285) callsigns found in your active logfile."));
            return;
        }

        CountyPrepDialog *dlg = new CountyPrepDialog(this, m_qrzSessionKey, getAppDataDir(), callsToQuery, &m_usCountiesWorked);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    };

    if (m_qrzSessionKey.isEmpty()) {
        m_textEdit->append("County Prep: Active session key missing. Logging in to QRZ XML service first...");
        
        QString urlStr = QString("https://xmldata.qrz.com/xml/current/?username=%1;password=%2;agent=Qwain")
                             .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_qrzUsername)))
                             .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_qrzPassword)));
        
        QNetworkRequest request((QUrl(urlStr)));
        QNetworkReply *reply = m_networkManager->get(request);
        
        connect(reply, &QNetworkReply::finished, this, [this, reply, launchPrepDialog]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                QMessageBox::critical(this, tr("QRZ Login Error"), tr("Network error obtaining session key for bulk query: %1").arg(reply->errorString()));
                return;
            }

            QByteArray responseData = reply->readAll();
            QXmlStreamReader xml(responseData);
            QString sessionKey;
            QString errorMsg;

            while (!xml.atEnd() && !xml.hasError()) {
                if (xml.readNext() == QXmlStreamReader::StartElement) {
                    if (xml.name().toString() == "Key") {
                        sessionKey = xml.readElementText();
                    } else if (xml.name().toString() == "Error") {
                        errorMsg = xml.readElementText();
                    }
                }
            }

            if (sessionKey.isEmpty()) {
                QString err = errorMsg.isEmpty() ? tr("Unknown QRZ credentials / subscription login failure.") : errorMsg;
                QMessageBox::critical(this, tr("QRZ Login Error"), tr("Failed to obtain QRZ Session Key: %1").arg(err));
                return;
            }

            m_qrzSessionKey = sessionKey;
            writeSettings();
            m_textEdit->append("County Prep: Successfully logged in. Session key acquired.");
            launchPrepDialog();
        });
    } else {
        launchPrepDialog();
    }
}

void MainWindow::setLogfilePath(const QString &path, bool persist) {
    m_logfilePath = path;
    if (persist) {
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        settings.beginGroup("Files");
        settings.setValue("logfile", path);
        settings.endGroup();
    }
    
    // 1. Parse and format as a consolidated logfile (calls loadLogfile() internally)
    prepareNewAdi();
    
    // 2. Local-only parsing of the new logfile for standard mail and direct indicators
    QFile file(m_logfilePath);
    int detectedMailCalls = 0;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = file.readAll();
        file.close();
        
        struct LocalMailInfo {
            QString xcheckVia;
            QString country;
            QString state;
        };
        QMap<QString, LocalMailInfo> pendingUpdates;
        
        // Split by <EOR> or <eor>
        QRegularExpression eorRe("<eor>", QRegularExpression::CaseInsensitiveOption);
        QStringList records = content.split(eorRe);
        
        int recordCount = 0;
        for (const QString &rec : records) {
            recordCount++;
            if (recordCount % 100 == 0) {
                QApplication::processEvents();
            }
            
            QString call = extractAdifField(rec, "CALL").trimmed().toUpper();
            if (call.isEmpty()) continue;
            
            // Extract standard indicators
            QString qslVia = extractAdifField(rec, "QSL_VIA").trimmed();
            QString qslMsg = extractAdifField(rec, "QSLMSG").trimmed();
            QString comment = extractAdifField(rec, "COMMENT").trimmed();
            QString qslRcvdVia = extractAdifField(rec, "QSL_RCVD_VIA").trimmed().toUpper();
            QString qslSentVia = extractAdifField(rec, "QSL_SENT_VIA").trimmed().toUpper();
            bool qslRcvdY = extractAdifField(rec, "QSL_RCVD").trimmed().toUpper() == "Y";
            
            // Check for direct / mail indicators
            bool isDirect = qslVia.contains("DIRECT", Qt::CaseInsensitive) || 
                            qslVia.contains("MAIL", Qt::CaseInsensitive) ||
                            qslVia.contains("PO BOX", Qt::CaseInsensitive) ||
                            qslVia.contains("P.O. BOX", Qt::CaseInsensitive) ||
                            qslVia.contains("DIR", Qt::CaseInsensitive) ||
                            qslVia.contains("HOME", Qt::CaseInsensitive) ||
                            qslMsg.contains("DIRECT", Qt::CaseInsensitive) || 
                            qslMsg.contains("MAIL", Qt::CaseInsensitive) ||
                            comment.contains("DIRECT", Qt::CaseInsensitive) || 
                            comment.contains("MAIL", Qt::CaseInsensitive) ||
                            qslRcvdVia == "D" || qslRcvdVia == "DIRECT" || qslRcvdVia == "M" || qslRcvdVia == "MAIL" ||
                            qslSentVia == "D" || qslSentVia == "DIRECT" || qslSentVia == "M" || qslSentVia == "MAIL" ||
                            qslRcvdY;
                            
            if (isDirect) {
                LocalMailInfo info;
                if (!qslVia.isEmpty()) {
                    info.xcheckVia = qslVia;
                } else if (!qslMsg.isEmpty()) {
                    info.xcheckVia = qslMsg;
                } else {
                    info.xcheckVia = "DIRECT";
                }
                info.country = extractAdifField(rec, "COUNTRY").trimmed().toUpper();
                info.state = extractAdifField(rec, "STATE").trimmed().toUpper();
                
                pendingUpdates[call] = info;
                detectedMailCalls++;
            }
        }
        
        if (!pendingUpdates.isEmpty()) {
            QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
            QSettings mailCache(getAppDataDir() + "/qrz_mailing_cache.ini", QSettings::IniFormat);
            
            for (auto it = pendingUpdates.constBegin(); it != pendingUpdates.constEnd(); ++it) {
                const QString &call = it.key();
                const LocalMailInfo &info = it.value();
                
                qrzCache.beginGroup(call);
                qrzCache.setValue("mqsl", true);
                qrzCache.setValue("xcheck_mailable", true);
                qrzCache.setValue("xcheck_via", info.xcheckVia);
                qrzCache.endGroup();
                
                mailCache.beginGroup(call);
                mailCache.setValue("country", info.country);
                mailCache.setValue("state", info.state);
                mailCache.endGroup();
            }
            qrzCache.sync();
            mailCache.sync();
        }
    }
    
    m_textEdit->append(QString("Parsed new logfile locally for standard indicators. Flagged %1 callsign(s) as mail/direct QSL mailable.").arg(detectedMailCalls));
}

void MainWindow::startQrzQueue(const QStringList &callsigns) {
    m_qrzQueue = callsigns;
    m_qrzQueueTotal = callsigns.size();
    m_qrzQueueSuccessCount = 0;
    m_qrzQueueLastCall = "";
    m_qrzQueueIntervalMs = 1500; // Reset pacing on fresh start

    logSystem(QString("[Cache Refresh Queue] Background refresh started. Total unique callsigns in queue: %1").arg(m_qrzQueueTotal));
    m_textEdit->append(QString("<span style='color: #2980b9; font-weight: bold;'>[Queue] Background refresh started. Total: %1</span>").arg(m_qrzQueueTotal));

    if (!m_qrzQueueTimer) {
        m_qrzQueueTimer = new QTimer(this);
        connect(m_qrzQueueTimer, &QTimer::timeout, this, &MainWindow::processNextQrzQueueItem);
    }
    m_qrzQueueTimer->start(m_qrzQueueIntervalMs); // polite adaptive pacing
    saveQrzQueueTickFile("ACTIVE");
}

void MainWindow::stopQrzQueueWorker() {
    if (m_qrzQueueTimer && m_qrzQueueTimer->isActive()) {
        m_qrzQueueTimer->stop();
        logSystem(QString("[Cache Refresh Queue] Background refresh paused/stopped. Processed successfully: %1. Last: %2. Remaining: %3.")
            .arg(m_qrzQueueSuccessCount).arg(m_qrzQueueLastCall).arg(m_qrzQueue.size()));
        m_textEdit->append(QString("<span style='color: #e67e22; font-weight: bold;'>[Queue] Background refresh paused. Processed: %1. Remaining: %2.</span>")
            .arg(m_qrzQueueSuccessCount).arg(m_qrzQueue.size()));
        saveQrzQueueTickFile("PAUSED");
    }
}

void MainWindow::resumeQrzQueue() {
    if (m_qrzQueue.isEmpty()) return;

    logSystem(QString("[Cache Refresh Queue] Background refresh resumed. Processed so far: %1. Remaining: %2.")
        .arg(m_qrzQueueSuccessCount).arg(m_qrzQueue.size()));
    m_textEdit->append(QString("<span style='color: #2980b9; font-weight: bold;'>[Queue] Resuming background refresh from where you left off. Processed: %1. Remaining: %2. Current interval: %3 ms.</span>")
        .arg(m_qrzQueueSuccessCount).arg(m_qrzQueue.size()).arg(m_qrzQueueIntervalMs));

    if (!m_qrzQueueTimer) {
        m_qrzQueueTimer = new QTimer(this);
        connect(m_qrzQueueTimer, &QTimer::timeout, this, &MainWindow::processNextQrzQueueItem);
    }
    m_qrzQueueTimer->start(m_qrzQueueIntervalMs);
    saveQrzQueueTickFile("ACTIVE");
}

void MainWindow::logQueueProgress(const QString &msg) {
    QString logPath = getAppDataDir() + "/qrz_background_refresh.log";
    QFile file(logPath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss UTC") << " - " << msg << " (Interval: " << m_qrzQueueIntervalMs << "ms)\n";
    }
}

void MainWindow::processNextQrzQueueItem() {
    if (m_qrzQueue.isEmpty()) {
        if (m_qrzQueueTimer) m_qrzQueueTimer->stop();
        logSystem(QString("[Cache Refresh Queue] Completed! Successfully updated details for %1 contacts.").arg(m_qrzQueueSuccessCount));
        m_textEdit->append(QString("<span style='color: #27ae60; font-weight: bold;'>[Queue] Background refresh completed! Updated %1 contacts.</span>").arg(m_qrzQueueSuccessCount));
        saveQrzQueueTickFile("COMPLETED");
        return;
    }

    QString callsign = m_qrzQueue.takeFirst().trimmed().toUpper();
    if (callsign.isEmpty()) {
        // Skip empty callsigns
        QMetaObject::invokeMethod(this, "processNextQrzQueueItem", Qt::QueuedConnection);
        return;
    }

    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    bool hasQrzXml = settings.value("Credentials/qrzXmlSubscriber", false).toBool() && !settings.value("Credentials/username").toString().isEmpty();

    // Call with updateDisplay = false to run silently in the background
    if (hasQrzXml) {
        if (!m_qrzSessionKey.isEmpty()) {
            doQrzLookup(callsign, "", "", "", "", "", "", QDateTime::currentMSecsSinceEpoch(), "", false, false, false);
        } else {
            // Initiate silent login first to get/refresh the session key, then execute doQrzLookup
            QString u = settings.value("Credentials/username", "").toString();
            QString p = settings.value("Credentials/password", "").toString();
            QString urlStr = QString("https://xmldata.qrz.com/xml/current/?username=%1;password=%2;agent=Qwain")
                                 .arg(QString::fromUtf8(QUrl::toPercentEncoding(u)))
                                 .arg(QString::fromUtf8(QUrl::toPercentEncoding(p)));
            QNetworkRequest req((QUrl(urlStr)));
            QNetworkReply *reply = m_networkManager->get(req);
            connect(reply, &QNetworkReply::finished, this, [this, reply, callsign]() {
                reply->deleteLater();
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray xmlData = reply->readAll();
                    QXmlStreamReader xml(xmlData);
                    while (!xml.atEnd() && !xml.hasError()) {
                        if (xml.readNext() == QXmlStreamReader::StartElement && xml.name().toString() == "Key") {
                            m_qrzSessionKey = xml.readElementText();
                            writeSettings();
                            break;
                        }
                    }
                }
                if (!m_qrzSessionKey.isEmpty()) {
                    doQrzLookup(callsign, "", "", "", "", "", "", QDateTime::currentMSecsSinceEpoch(), "", false, false, false);
                } else {
                    doFreeLookup(callsign, "", "", "", "", "", "", QDateTime::currentMSecsSinceEpoch(), "", false, false, false);
                }
            });
        }
    } else {
        doFreeLookup(callsign, "", "", "", "", "", "", QDateTime::currentMSecsSinceEpoch(), "", false, false, false);
    }

    m_qrzQueueSuccessCount++;
    m_qrzQueueLastCall = callsign;
    
    m_textEdit->append(QString("<span style='color: #7f8c8d;'>[Queue] Refreshing %1 (%2 / %3) [%4 ms pacing]...</span>")
        .arg(callsign)
        .arg(m_qrzQueueSuccessCount)
        .arg(m_qrzQueueTotal)
        .arg(m_qrzQueueIntervalMs));

    saveQrzQueueTickFile("ACTIVE");
}

void MainWindow::saveQrzQueueTickFile(const QString &state) {
    QString tickPath = getAppDataDir() + "/qrz_refresh_queue.tick";
    QFile file(tickPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "STATE=" << state << "\n";
        out << "LAST_SUCCESSFUL_CALLSIGN=" << m_qrzQueueLastCall << "\n";
        out << "SUCCESS_COUNT=" << m_qrzQueueSuccessCount << "\n";
        out << "TOTAL_COUNT=" << m_qrzQueueTotal << "\n";
        out << "PACING_INTERVAL=" << m_qrzQueueIntervalMs << "\n";
        out << "QUEUE=" << m_qrzQueue.join(",") << "\n";
        file.close();
    }
}

void MainWindow::loadQrzQueueTickFile() {
    QString tickPath = getAppDataDir() + "/qrz_refresh_queue.tick";
    if (!QFile::exists(tickPath)) return;

    QFile file(tickPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    QString state = "";
    QString lastCall = "";
    int successCount = 0;
    int totalCount = 0;
    int pacingInterval = 1500;
    QStringList queue;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith("STATE=")) state = line.mid(6);
        else if (line.startsWith("LAST_SUCCESSFUL_CALLSIGN=")) lastCall = line.mid(25);
        else if (line.startsWith("SUCCESS_COUNT=")) successCount = line.mid(14).toInt();
        else if (line.startsWith("TOTAL_COUNT=")) totalCount = line.mid(12).toInt();
        else if (line.startsWith("PACING_INTERVAL=")) pacingInterval = line.mid(16).toInt();
        else if (line.startsWith("QUEUE=")) {
            QString qStr = line.mid(6);
            if (!qStr.isEmpty()) {
                queue = qStr.split(',', Qt::SkipEmptyParts);
            }
        }
    }
    file.close();

    if ((state == "ACTIVE" || state == "PAUSED") && !queue.isEmpty()) {
        m_qrzQueue = queue;
        m_qrzQueueTotal = totalCount;
        m_qrzQueueSuccessCount = successCount;
        m_qrzQueueLastCall = lastCall;
        m_qrzQueueIntervalMs = pacingInterval;

        if (state == "ACTIVE") {
            logSystem(QString("[Cache Refresh Queue] Interrupted process detected on next launch! Resuming background queue with %1 remaining callsigns.").arg(m_qrzQueue.size()));
            m_textEdit->append(QString("<span style='color: #2980b9; font-weight: bold;'>[Queue] Resuming background refresh. Remaining: %1. Interval: %2 ms.</span>").arg(m_qrzQueue.size()).arg(m_qrzQueueIntervalMs));

            if (!m_qrzQueueTimer) {
                m_qrzQueueTimer = new QTimer(this);
                connect(m_qrzQueueTimer, &QTimer::timeout, this, &MainWindow::processNextQrzQueueItem);
            }
            m_qrzQueueTimer->start(m_qrzQueueIntervalMs); // Resume pacing
        } else {
            logSystem(QString("[Cache Refresh Queue] Paused queue loaded on launch. Remaining: %1.").arg(m_qrzQueue.size()));
            m_textEdit->append(QString("<span style='color: #e67e22; font-weight: bold;'>[Queue] Paused background refresh queue loaded. Remaining: %1. Click Refresh Data in Cache Viewer to resume.</span>").arg(m_qrzQueue.size()));
        }
    }
}

void MainWindow::openStatsDialog() {
    StatsDialog dlg(this, this);
    dlg.exec();
}

void MainWindow::incrementTemplatesPrinted(int count) {
    QSettings settings("Qwain", "Qwain");
    int cur = settings.value("Stats/TotalTemplatesPrinted", 0).toInt();
    settings.setValue("Stats/TotalTemplatesPrinted", cur + count);
}

// ============================================================================
// METHOD: MainWindow::lookupCallsign
// Programmatically loads a callsign from statsdialog into MainWindow.
// Dispatches the QSO record processing to display the contact in MainWindow
// without populating the text search box (m_lookupEdit).
// ============================================================================
void MainWindow::lookupCallsign(const QString &callsign) {
    QString call = callsign.trimmed().toUpper();
    if (call.isEmpty()) return;
    if (m_searchMatchesList) {
        m_searchMatchesList->hide();
    }
    // Note: Do not populate m_lookupEdit (text search box) per user specification.
    // Construct lookup record and process directly.
    QString fakeAdif = QString("<CALL:%1>%2<QSO_DATE:8>%3<TIME_ON:6>000000<EOR>")
                        .arg(call.length()).arg(call)
                        .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMdd"));
    processAdifRecord(fakeAdif, true); // true = isManualLookup

    raise();
    activateWindow();
}



