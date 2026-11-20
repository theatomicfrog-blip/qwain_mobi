#ifndef LOADINGSPLASH_H
#define LOADINGSPLASH_H

#include <QWidget>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QScreen>
#include <QStyle>
#include <QCoreApplication>
#include <QPixmap>

extern QPixmap getAppLogo();

class LoadingSplash : public QWidget {
    Q_OBJECT
public:
    explicit LoadingSplash(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
        setAttribute(Qt::WA_TranslucentBackground);
        
        // Root widget to enable styling and rounded corners
        QWidget *root = new QWidget(this);
        root->setObjectName("rootWidget");
        root->setStyleSheet(
            "QWidget#rootWidget {"
            "  background-color: #181b22;"
            "  border: 1px solid #2d313f;"
            "  border-radius: 12px;"
            "}"
            "QLabel {"
            "  color: #e2e8f0;"
            "  font-family: 'Segoe UI', Arial, sans-serif;"
            "}"
            "QProgressBar {"
            "  border: none;"
            "  background-color: #262930;"
            "  height: 6px;"
            "  text-align: center;"
            "  color: transparent;"
            "  border-radius: 3px;"
            "}"
            "QProgressBar::chunk {"
            "  background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 #10b981, stop:1 #3b82f6);"
            "  border-radius: 3px;"
            "}"
        );
        
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->addWidget(root);
        
        QVBoxLayout *rootLayout = new QVBoxLayout(root);
        rootLayout->setContentsMargins(30, 30, 30, 30);
        rootLayout->setSpacing(15);
        
        // Logo
        QLabel *logoLabel = new QLabel(root);
        QPixmap logoPix = getAppLogo().scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        logoLabel->setPixmap(logoPix);
        logoLabel->setAlignment(Qt::AlignCenter);
        rootLayout->addWidget(logoLabel);
        
        // Subtitle
        QLabel *subLabel = new QLabel("Your Personal QSL Manager", root);
        QFont subFont = subLabel->font();
        subFont.setPointSize(10);
        subLabel->setFont(subFont);
        subLabel->setAlignment(Qt::AlignCenter);
        subLabel->setStyleSheet("color: #8b949e;");
        rootLayout->addWidget(subLabel);
        
        rootLayout->addSpacing(10);
        
        // Progress bar
        m_progressBar = new QProgressBar(root);
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
        rootLayout->addWidget(m_progressBar);
        
        // Status text
        m_statusLabel = new QLabel("Initializing...", root);
        m_statusLabel->setAlignment(Qt::AlignCenter);
        m_statusLabel->setStyleSheet("color: #8b949e; font-size: 11px;");
        rootLayout->addWidget(m_statusLabel);
        
        // Add drop shadow
        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(20);
        shadow->setColor(QColor(0, 0, 0, 180));
        shadow->setOffset(0, 5);
        root->setGraphicsEffect(shadow);
        
        // Center on primary screen
        resize(400, 380);
        if (QScreen *screen = QGuiApplication::primaryScreen()) {
            setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), screen->availableGeometry()));
        }
    }
    
    void setProgress(int value, const QString &statusText) {
        m_progressBar->setValue(value);
        m_statusLabel->setText(statusText);
        QCoreApplication::processEvents();
    }
    
private:
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
};

#endif // LOADINGSPLASH_H
