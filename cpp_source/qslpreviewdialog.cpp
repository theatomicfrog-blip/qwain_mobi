#include "addressrenderinspectorwindow.h"
#include "qsltemplateinspector.h"
#include "qrcodegen.hpp"
// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

// ============================================================================
// COMPONENT: QslPreviewDialog
// ROLE: High-fidelity print preview layout engine and batch spool controller.
//       This component processes selected QSO records, formats them according
//       to custom template tags (including standard emission modes and QRP
//       bragging auto-sizing text), paints them onto a virtual graphics scene
//       for real-time preview, and drives spooled physical/PDF batch printing.
// INVOKED BY: QslManagerDialog ("Print Now") or MainWindow.
// PRODUCES: On-screen preview renders, spooled card/envelope print runs.
// ============================================================================

#include <QCoreApplication>
#include "qrzcache.h"
#include <QClipboard>
#include <QGuiApplication>
#include <QDateTime>
#include <QProcess>
#include <QProgressBar>
#include <QStackedWidget>
#include <QEventLoop>
#include "paths.h"
#include "mainwindow.h"
#include "qslpreviewdialog.h"
#include <QTextBrowser>

void QslPreviewDialog::logDiag(const QString &msg) {
    if (m_diagLogBrowser) {
        m_diagLogBrowser->append(QString("[%1] %2").arg(QTime::currentTime().toString("hh:mm:ss.zzz"), msg));
    }
}
#include "qsldesigner.h"
#include "schemedesigner.h"
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <algorithm>
#include <QRegularExpression>
#include <QHeaderView>
#include <QMenu>
#include <QPainter>
#include <QMessageBox>
#include <QFile>
#include <QGraphicsProxyWidget>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextBlockFormat>
#include <QFontDialog>
#include "styledialog.h"
#include <QColorDialog>
#include <QWheelEvent>
#include <QPrinter>
#include <QPrintDialog>
#include <QPageSize>
#include <QPageLayout>
#include <QTextEdit>
#include <QInputDialog>
#include <QFontComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>


#include <QApplication>
#include <QShortcut>
#include <QKeySequence>
#include <QRandomGenerator>
#include <QTimer>
#include <QtMath>
#include <QPrinterInfo>
#include <QFileDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QUrl>
#include <QSet>

static QString formatNameOrCity(const QString &input) {
    if (input.isEmpty()) return input;
    
    bool hasLetters = false;
    bool allUpper = true;
    bool allLower = true;
    for (int i = 0; i < input.length(); ++i) {
        QChar c = input[i];
        if (c.isLetter()) {
            hasLetters = true;
            if (c.isUpper()) {
                allLower = false;
            } else if (c.isLower()) {
                allUpper = false;
            }
        }
    }
    
    if (!hasLetters || (!allUpper && !allLower)) {
        return input;
    }
    
    QString result = input;
    bool nextIsCapital = true;
    for (int i = 0; i < result.length(); ++i) {
        QChar c = result[i];
        if (c.isLetter()) {
            if (nextIsCapital) {
                result[i] = c.toUpper();
                nextIsCapital = false;
            } else {
                result[i] = c.toLower();
            }
        } else if (c == ' ' || c == '-' || c == '/' || c == '(' || c == ')' || c == '.' || c == ',') {
            nextIsCapital = true;
        }
    }
    return result;
}

static QString formatCity(const QString &cityStr) {
    if (cityStr.isEmpty()) return cityStr;
    
    bool hasLetters = false;
    bool allUpper = true;
    bool allLower = true;
    for (int i = 0; i < cityStr.length(); ++i) {
        QChar c = cityStr[i];
        if (c.isLetter()) {
            hasLetters = true;
            if (c.isUpper()) {
                allLower = false;
            } else if (c.isLower()) {
                allUpper = false;
            }
        }
    }
    
    if (!hasLetters || (!allUpper && !allLower)) {
        return cityStr;
    }
    
    QStringList parts = cityStr.split(',');
    for (int p = 0; p < parts.size(); ++p) {
        QString &part = parts[p];
        QStringList words = part.split(' ');
        for (int w = 0; w < words.size(); ++w) {
            QString &word = words[w];
            if (word.isEmpty()) continue;
            
            bool isAllLetter = true;
            int letterCount = 0;
            for (int i = 0; i < word.length(); ++i) {
                if (word[i].isLetter()) {
                    letterCount++;
                } else {
                    isAllLetter = false;
                }
            }
            if (isAllLetter && letterCount == 2) {
                word = word.toUpper();
                continue;
            }
            
            bool nextIsCapital = true;
            for (int i = 0; i < word.length(); ++i) {
                QChar c = word[i];
                if (c.isLetter()) {
                    if (nextIsCapital) {
                        word[i] = c.toUpper();
                        nextIsCapital = false;
                    } else {
                        word[i] = c.toLower();
                    }
                } else {
                    nextIsCapital = true;
                }
            }
        }
        part = words.join(' ');
    }
    return parts.join(',');
}

static QString formatAddress(const QString &addrStr) {
    if (addrStr.isEmpty()) return addrStr;
    
    bool hasLetters = false;
    bool allUpper = true;
    bool allLower = true;
    for (int i = 0; i < addrStr.length(); ++i) {
        QChar c = addrStr[i];
        if (c.isLetter()) {
            hasLetters = true;
            if (c.isUpper()) {
                allLower = false;
            } else if (c.isLower()) {
                allUpper = false;
            }
        }
    }
    
    if (!hasLetters || (!allUpper && !allLower)) {
        return addrStr;
    }
    
    QStringList words = addrStr.split(' ');
    
    for (int w = 0; w < words.size(); ++w) {
        QString &word = words[w];
        if (word.isEmpty()) continue;
        
        bool isAllLetter = true;
        int letterCount = 0;
        for (int i = 0; i < word.length(); ++i) {
            if (word[i].isLetter()) {
                letterCount++;
            } else {
                isAllLetter = false;
            }
        }
        if (isAllLetter && letterCount == 2) {
            word = word.toUpper();
            continue;
        }
        
        bool nextIsCapital = true;
        for (int i = 0; i < word.length(); ++i) {
            QChar c = word[i];
            if (c.isLetter()) {
                if (nextIsCapital) {
                    word[i] = c.toUpper();
                    nextIsCapital = false;
                } else {
                    word[i] = c.toLower();
                }
            } else {
                nextIsCapital = true;
            }
        }
    }
    
    return words.join(' ');
}

static QString formatCountry(const QString &country) {
    QString c = country.trimmed().toUpper();
    if (c == "ASIATIC RUSSIA") {
        return "RUSSIA";
    }
    return c;
}

static void addTextOnPathToScene(QGraphicsScene *scene, QGraphicsItem *parent, const QString &text, const QRectF &cell, const QFont &font, const QColor &color, int overallAlign = Qt::AlignCenter, bool autoShrink = false, bool onlyShrink = false, double *outShrunkPercent = nullptr) {
    if (text.isEmpty() || cell.width() <= 0 || cell.height() <= 0) return;
    
    // Divide into lines
    QStringList lines = text.split(QRegularExpression("<br/?>|\\n"), Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        lines << text;
    }
    
    QFont activeFont = font;
    if (autoShrink) {
        double cw = cell.width();
        double ch = cell.height();
        double size = activeFont.pointSizeF() > 0 ? activeFont.pointSizeF() : (activeFont.pixelSize() > 0 ? activeFont.pixelSize() * 0.75 : 10.0);
        if (size <= 0) size = 10.0;
        
        double maxAllowedSize = onlyShrink ? size : qMax(48.0, size * 2.5);
        double bestSize = 3.0;
        double testSize = 3.0;
        
        while (testSize <= maxAllowedSize) {
            QFont testFont = activeFont;
            testFont.setPointSizeF(testSize);
            QFontMetricsF fm(testFont);
            double fontHeight = fm.height();
            double totalHeight = lines.size() * fontHeight + (lines.size() - 1) * 2.0;
            
            bool fits = onlyShrink ? true : (totalHeight <= ch);
            if (fits) {
                for (const QString &line : lines) {
                    QString cleanLine = line;
                    cleanLine.remove("<COUNTRY_ALIGN_RIGHT>", Qt::CaseInsensitive);
                    cleanLine.remove(QRegularExpression("<[^>]*>"));
                    cleanLine = cleanLine.trimmed();
                    if (fm.horizontalAdvance(cleanLine) > cw) {
                        fits = false;
                        break;
                    }
                }
            }
            if (fits) {
                bestSize = testSize;
            } else {
                break;
            }
            testSize += 0.5;
        }
        activeFont.setPointSizeF(bestSize);
        if (outShrunkPercent) {
            *outShrunkPercent = bestSize / size;
        }
    } else {
        if (outShrunkPercent) {
            *outShrunkPercent = 1.0;
        }
    }
    
    QFontMetricsF fm(activeFont);
    double fontHeight = fm.height();
    double cw = cell.width();
    double ch = cell.height();
    
    double totalHeight = lines.size() * fontHeight + (lines.size() - 1) * 2.0;
    double startY = cell.top() + (ch - totalHeight) / 2.0;
    
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        
        int lineAlign = overallAlign;
        if (line.contains("align='right'", Qt::CaseInsensitive) || 
            line.contains("align=\"right\"", Qt::CaseInsensitive) ||
            line.contains("<COUNTRY_ALIGN_RIGHT>", Qt::CaseInsensitive)) {
            lineAlign = Qt::AlignRight;
        } else if (line.contains("align='center'", Qt::CaseInsensitive) || 
                   line.contains("align=\"center\"", Qt::CaseInsensitive)) {
            lineAlign = Qt::AlignCenter;
        } else if (line.contains("align='left'", Qt::CaseInsensitive) || 
                   line.contains("align=\"left\"", Qt::CaseInsensitive)) {
            lineAlign = Qt::AlignLeft;
        }
        
        line.remove("<COUNTRY_ALIGN_RIGHT>", Qt::CaseInsensitive);
        line.remove(QRegularExpression("<[^>]*>"));
        line = line.trimmed();
        if (line.isEmpty()) continue;
        
        QGraphicsSimpleTextItem *lineItem = new QGraphicsSimpleTextItem(line, parent);
        lineItem->setFont(activeFont);
        lineItem->setBrush(QBrush(color));
        
        QRectF lBr = lineItem->boundingRect();
        double x = cell.left();
        if (lineAlign == Qt::AlignRight) {
            x = cell.right() - lBr.width();
        } else if (lineAlign == Qt::AlignCenter) {
            x = cell.left() + (cw - lBr.width()) / 2.0;
        }
        double y = startY + i * (fontHeight + 2.0);
        lineItem->setPos(x, y);
    }
}

class CommentEditorDialog : public QDialog {
public:
    QTextEdit *m_textEdit;
    QComboBox *m_alignCombo;
    
    QRadioButton *m_slotRadios[4];
    QTextEdit *m_slotEdits[4];
    QString m_slotAligns[4];
    int m_selectedSlotIndex;
    
    QString m_initialText;
    QString m_initialAlign;

    void applyAlignmentToEdit(QTextEdit *edit, const QString &alignStr) {
        Qt::Alignment flag = Qt::AlignCenter;
        if (alignStr == "L Justify") flag = Qt::AlignLeft;
        else if (alignStr == "R Justify") flag = Qt::AlignRight;
        
        QTextCursor cursor = edit->textCursor();
        edit->selectAll();
        edit->setAlignment(flag);
        edit->setTextCursor(cursor);
    }

    CommentEditorDialog(const QString &text, const QString &align, QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Comment Chooser/Editor");
        resize(750, 430);
        m_selectedSlotIndex = 0;
        m_initialText = text;
        m_initialAlign = align;

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        
        // Horizontal layout for the main body (Left Pane | Arrows | Right Pane)
        QHBoxLayout *bodyLayout = new QHBoxLayout();
        
        // Left Pane (Main Editor)
        QVBoxLayout *leftLayout = new QVBoxLayout();
        leftLayout->addWidget(new QLabel("<b>Current Comment Editor:</b>", this));
        m_textEdit = new QTextEdit(this);
        m_textEdit->setPlainText(text);
        leftLayout->addWidget(m_textEdit);
        
        // Apply initial alignment to left text edit
        QTimer::singleShot(50, this, [this, align]() {
            applyAlignmentToEdit(m_textEdit, align.isEmpty() ? "Center" : align);
        });
        
        QPushButton *btnUndo = new QPushButton("Undo", this);
        btnUndo->setToolTip("Restore to initial comment and alignment");
        btnUndo->setStyleSheet("QPushButton { font-weight: bold; background-color: #3b82f6; color: white; border: 1px solid #2563eb; border-radius: 4px; padding: 6px; } QPushButton:hover { background-color: #2563eb; }");
        leftLayout->addWidget(btnUndo);
        
        bodyLayout->addLayout(leftLayout, 5);
        
        // Middle Pane (Stacked USE / SAVE buttons)
        QVBoxLayout *middleLayout = new QVBoxLayout();
        middleLayout->addStretch();
        
        QPushButton *btnToLeft = new QPushButton("USE ◀", this);
        btnToLeft->setToolTip("Bring highlighted slot to current editor");
        btnToLeft->setFixedSize(70, 32);
        btnToLeft->setStyleSheet("QPushButton { font-weight: bold; font-size: 11px; border: 1px solid #94a3b8; border-radius: 4px; background-color: #f1f5f9; color: #334155; } QPushButton:hover { background-color: #e2e8f0; }");
        
        QPushButton *btnToRight = new QPushButton("SAVE ▶", this);
        btnToRight->setToolTip("Save current editor to highlighted slot");
        btnToRight->setFixedSize(70, 32);
        btnToRight->setStyleSheet("QPushButton { font-weight: bold; font-size: 11px; border: 1px solid #94a3b8; border-radius: 4px; background-color: #f1f5f9; color: #334155; } QPushButton:hover { background-color: #e2e8f0; }");
        
        middleLayout->addWidget(btnToRight);
        middleLayout->addWidget(btnToLeft);
        middleLayout->addStretch();
        
        bodyLayout->addLayout(middleLayout, 2);
        
        // Right Pane (Stored Comments)
        QVBoxLayout *rightLayout = new QVBoxLayout();
        rightLayout->addWidget(new QLabel("<b>Stored Comments</b>", this));
        
        QLabel *subtitleLbl = new QLabel("Enter Comment or Select Stored Comment", this);
        subtitleLbl->setStyleSheet("color: #64748b; font-style: italic; font-size: 11px; margin-bottom: 4px;");
        rightLayout->addWidget(subtitleLbl);
        
        QButtonGroup *group = new QButtonGroup(this);
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        
        for (int i = 0; i < 4; ++i) {
            QHBoxLayout *slotRow = new QHBoxLayout();
            
            m_slotRadios[i] = new QRadioButton(QString::number(i + 1), this);
            m_slotRadios[i]->setStyleSheet("QRadioButton { font-weight: bold; }");
            if (i == 0) {
                m_slotRadios[i]->setChecked(true);
            }
            group->addButton(m_slotRadios[i], i);
            slotRow->addWidget(m_slotRadios[i]);
            
            m_slotEdits[i] = new QTextEdit(this);
            m_slotEdits[i]->setFixedHeight(48); // Exactly 2 rows of text high
            m_slotEdits[i]->setPlaceholderText(QString("Empty Slot %1").arg(i + 1));
            
            // Load persistent slot data
            QString savedText = settings.value(QString("PrintManager/SlotText_%1").arg(i), "").toString();
            m_slotEdits[i]->setPlainText(savedText);
            m_slotAligns[i] = settings.value(QString("PrintManager/SlotAlign_%1").arg(i), "Center").toString();
            
            // Apply loaded alignment to slot text edits
            QTimer::singleShot(50, this, [this, i]() {
                applyAlignmentToEdit(m_slotEdits[i], m_slotAligns[i]);
            });
            
            slotRow->addWidget(m_slotEdits[i]);
            rightLayout->addLayout(slotRow);
            
            // Allow clicking/focusing inside text area or clicking radio button to highlight slot
            m_slotEdits[i]->installEventFilter(this);
            
            connect(m_slotRadios[i], &QRadioButton::toggled, this, [this, i](bool checked) {
                if (checked) {
                    m_selectedSlotIndex = i;
                }
            });
        }
        
        bodyLayout->addLayout(rightLayout, 5);
        mainLayout->addLayout(bodyLayout);
        
        // Bottom Row (Alignment Combo | Spacer | OK & Cancel buttons)
        QHBoxLayout *bottomRow = new QHBoxLayout();
        
        bottomRow->addWidget(new QLabel("Alignment:", this));
        m_alignCombo = new QComboBox(this);
        m_alignCombo->addItems({"L Justify", "Center", "R Justify"});
        m_alignCombo->setCurrentText(align.isEmpty() ? "Center" : align);
        bottomRow->addWidget(m_alignCombo);
        
        bottomRow->addStretch();
        
        QPushButton *btnOk = new QPushButton("OK", this);
        btnOk->setStyleSheet("QPushButton { font-weight: bold; padding: 6px 16px; }");
        connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
        
        QPushButton *btnCancel = new QPushButton("Cancel", this);
        btnCancel->setStyleSheet("QPushButton { padding: 6px 16px; }");
        connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
        
        bottomRow->addWidget(btnOk);
        bottomRow->addWidget(btnCancel);
        
        mainLayout->addLayout(bottomRow);

        // When user changes alignment in combo box, instantly update m_textEdit alignment layout on the fly!
        connect(m_alignCombo, &QComboBox::currentTextChanged, this, [this](const QString &newAlign) {
            applyAlignmentToEdit(m_textEdit, newAlign);
        });

        // Undo action
        connect(btnUndo, &QPushButton::clicked, this, [this]() {
            m_textEdit->setPlainText(m_initialText);
            m_alignCombo->setCurrentText(m_initialAlign.isEmpty() ? "Center" : m_initialAlign);
            applyAlignmentToEdit(m_textEdit, m_initialAlign.isEmpty() ? "Center" : m_initialAlign);
        });

        // Connect arrow button triggers
        connect(btnToLeft, &QPushButton::clicked, this, [this]() {
            int slot = m_selectedSlotIndex;
            if (slot >= 0 && slot < 4) {
                QString txt = m_slotEdits[slot]->toPlainText();
                QString alignVal = m_slotAligns[slot];
                m_textEdit->setPlainText(txt);
                m_alignCombo->setCurrentText(alignVal.isEmpty() ? "Center" : alignVal);
                applyAlignmentToEdit(m_textEdit, alignVal.isEmpty() ? "Center" : alignVal);
            }
        });
        
        connect(btnToRight, &QPushButton::clicked, this, [this]() {
            int slot = m_selectedSlotIndex;
            if (slot >= 0 && slot < 4) {
                QString txt = m_textEdit->toPlainText();
                QString alignVal = m_alignCombo->currentText();
                m_slotEdits[slot]->setPlainText(txt);
                m_slotAligns[slot] = alignVal;
                applyAlignmentToEdit(m_slotEdits[slot], alignVal);
                
                // Persist instantly to QSettings
                QSettings settings(QCoreApplication::organizationName(), "Qwain");
                settings.setValue(QString("PrintManager/SlotText_%1").arg(slot), txt);
                settings.setValue(QString("PrintManager/SlotAlign_%1").arg(slot), alignVal);
            }
        });
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        for (int i = 0; i < 4; ++i) {
            if (watched == m_slotEdits[i]) {
                if (event->type() == QEvent::FocusIn || event->type() == QEvent::MouseButtonPress) {
                    m_slotRadios[i]->setChecked(true);
                    m_selectedSlotIndex = i;
                }
            }
        }
        return QDialog::eventFilter(watched, event);
    }
};


static QString formatPrinterDisplay(const QString &kp) {
    QString target = kp.split(" ").first().trimmed();
    QString name = target;
    if (kp.contains("(") && kp.contains(")")) {
        int start = kp.indexOf("(") + 1;
        int end = kp.lastIndexOf(")");
        name = kp.mid(start, end - start).trimmed();
    }
    return name;
}

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPixmapItem>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <functional>

class FinalTweaksDialog : public QDialog {
public:
    FinalTweaksDialog(double top, double left, bool useTweaks, QslPreviewDialog *parent) 
        : QDialog(parent), m_previewDlg(parent) 
    {
        setWindowTitle("Final Tweak");
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        
        QFormLayout *formLayout = new QFormLayout();
        
        m_topSpin = new QDoubleSpinBox(this);
        m_topSpin->setRange(-5.0, 5.0);
        m_topSpin->setSingleStep(0.01);
        m_topSpin->setSuffix(" in");
        m_topSpin->setValue(top);
        
        QHBoxLayout *topLayout = new QHBoxLayout();
        topLayout->addWidget(m_topSpin);
        topLayout->addWidget(new QLabel("(- = ↑)", this));
        
        m_leftSpin = new QDoubleSpinBox(this);
        m_leftSpin->setRange(-5.0, 5.0);
        m_leftSpin->setSingleStep(0.01);
        m_leftSpin->setSuffix(" in");
        m_leftSpin->setValue(left);
        
        QHBoxLayout *leftLayout = new QHBoxLayout();
        leftLayout->addWidget(m_leftSpin);
        leftLayout->addWidget(new QLabel("(- = ←)", this));
        
        m_chkUseTweaks = new QCheckBox("Use Tweaks", this);
        m_chkUseTweaks->setChecked(useTweaks);
        
        QPushButton *btnVisualAdjust = new QPushButton("Visual Adjust...", this);
        
        formLayout->addRow("Top Margin Shift:", topLayout);
        formLayout->addRow("Left Margin Shift:", leftLayout);
        formLayout->addRow("", m_chkUseTweaks);
        formLayout->addRow("", btnVisualAdjust);
        
        mainLayout->addLayout(formLayout);
        
        connect(btnVisualAdjust, &QPushButton::clicked, this, &FinalTweaksDialog::onVisualAdjustClicked);
        
        QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        mainLayout->addWidget(btnBox);
        
        resize(320, 200);
    }
    
    double topShift() const { return m_topSpin->value(); }
    double leftShift() const { return m_leftSpin->value(); }
    bool useTweaks() const { return m_chkUseTweaks->isChecked(); }
    
private:
    void onVisualAdjustClicked();
    
    QDoubleSpinBox *m_topSpin;
    QDoubleSpinBox *m_leftSpin;
    QCheckBox *m_chkUseTweaks;
    QslPreviewDialog *m_previewDlg;
};

class ZoomableGraphicsView : public QGraphicsView {
public:
    ZoomableGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr) : QGraphicsView(scene, parent) {
        setRenderHint(QPainter::Antialiasing);
        setRenderHint(QPainter::SmoothPixmapTransform);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    }

protected:
    void wheelEvent(QWheelEvent *event) override {
        double factor = 1.15;
        if (event->angleDelta().y() < 0) {
            factor = 1.0 / factor;
        }
        scale(factor, factor);
    }
};

class DraggablePixmapItem : public QGraphicsPixmapItem {
public:
    DraggablePixmapItem(const QPixmap &pixmap) : QGraphicsPixmapItem(pixmap) {
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
        setCursor(Qt::OpenHandCursor);
    }

    void setOnMoveCallback(std::function<void(const QPointF&)> cb) {
        m_moveCallback = cb;
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override {
        setCursor(Qt::ClosedHandCursor);
        QGraphicsPixmapItem::mousePressEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override {
        setCursor(Qt::OpenHandCursor);
        QGraphicsPixmapItem::mouseReleaseEvent(event);
    }

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override {
        if (change == ItemPositionChange && m_moveCallback) {
            m_moveCallback(value.toPointF());
        }
        return QGraphicsPixmapItem::itemChange(change, value);
    }

private:
    std::function<void(const QPointF&)> m_moveCallback;
};

class QslTweakAdjustDialog : public QDialog {
public:
    QslTweakAdjustDialog(double initLeftShift, double initTopShift,
                         double printOffsetX, double printOffsetY,
                         double paperW, double paperH,
                         const QPixmap &templatePixmap,
                         const QString &templateName,
                         const QString &printerName,
                         int printCorrection,
                         const QString &bgImagePath,
                         const QList<QVariantMap> &templateItems,
                         QWidget *parent = nullptr)
        : QDialog(parent),
          m_printOffsetX(printOffsetX),
          m_printOffsetY(printOffsetY),
          m_paperW(paperW),
          m_paperH(paperH),
          m_isUpdating(false)
    {
        setWindowTitle("Visual Print Tweak Adjustment");
        resize(1150, 750);

        // Respect print direction for showing template
        double finalRotation = 180.0 + printCorrection;
        while (finalRotation >= 360.0) finalRotation -= 360.0;
        while (finalRotation <= -360.0) finalRotation += 360.0;
        if (finalRotation < 0.0) finalRotation += 360.0;

        double rad = qDegreesToRadians(finalRotation);
        m_cosA = qCos(rad);
        m_sinA = qSin(rad);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        
        // Horizontal bar for inputs and instructions
        QHBoxLayout *topBar = new QHBoxLayout();
        
        topBar->addWidget(new QLabel("Visual Horiz Offset (L/R):", this));
        m_xSpin = new QDoubleSpinBox(this);
        m_xSpin->setRange(-5.0, 5.0);
        m_xSpin->setSingleStep(0.01);
        m_xSpin->setDecimals(2);
        m_xSpin->setSuffix(" in");
        
        // Forward transform from physical to visual offsets to initialize visual spinboxes
        double initVisX = initLeftShift * m_cosA + initTopShift * m_sinA;
        m_xSpin->setValue(initVisX);
        topBar->addWidget(m_xSpin);
        
        topBar->addWidget(new QLabel("Visual Vert Offset (U/D):", this));
        m_ySpin = new QDoubleSpinBox(this);
        m_ySpin->setRange(-5.0, 5.0);
        m_ySpin->setSingleStep(0.01);
        m_ySpin->setDecimals(2);
        m_ySpin->setSuffix(" in");

        double initVisY = -initLeftShift * m_sinA + initTopShift * m_cosA;
        m_ySpin->setValue(initVisY);
        topBar->addWidget(m_ySpin);
        
        QLabel *helpLabel = new QLabel("Drag template layer to reposition (respects print rotation). Wheel to zoom.", this);
        helpLabel->setStyleSheet("color: #555555; font-style: italic;");
        topBar->addSpacing(10);
        topBar->addWidget(helpLabel);
        
        topBar->addStretch();
        mainLayout->addLayout(topBar);
        
        // Horizontal split layout: rendering on left, detailed browser on right
        QHBoxLayout *contentLayout = new QHBoxLayout();
        
        // Build Graphics Scene and View
        m_scene = new QGraphicsScene(this);
        m_view = new ZoomableGraphicsView(m_scene, this);
        m_view->setStyleSheet("background-color: #d0d0d0; border: 1px solid #bbbbbb;");
        contentLayout->addWidget(m_view, 3);

        // Rotate the view to respect visual print direction orientation
        if (finalRotation != 0.0) {
            m_view->rotate(finalRotation);
        }
        
        // Build Info Browser
        QTextBrowser *infoBrowser = new QTextBrowser(this);
        infoBrowser->setMinimumWidth(380);
        infoBrowser->setStyleSheet(
            "QTextBrowser {"
            "   background-color: #ffffff;"
            "   border: 1px solid #cbd5e1;"
            "   border-radius: 4px;"
            "   font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;"
            "   font-size: 13px;"
            "   color: #1e293b;"
            "   padding: 10px;"
            "}"
        );
        
        // Generate beautiful HTML content for the template info
        QString html = "<html><head><style>"
                       "body { font-family: sans-serif; color: #1e293b; line-height: 1.4; }"
                       "h3 { color: #0f172a; margin-top: 0; margin-bottom: 8px; border-bottom: 2px solid #e2e8f0; padding-bottom: 4px; font-size: 16px; }"
                       "h4 { color: #334155; margin-top: 16px; margin-bottom: 8px; border-bottom: 1px solid #e2e8f0; padding-bottom: 2px; font-size: 14px; }"
                       "table { width: 100%; border-collapse: collapse; margin-bottom: 12px; }"
                       "td { padding: 4px 2px; vertical-align: top; font-size: 13px; }"
                       "td.label { font-weight: bold; color: #475569; width: 40%; }"
                       ".card { border: 1px solid #e2e8f0; border-radius: 4px; background-color: #f8fafc; padding: 8px; margin-bottom: 8px; }"
                       ".card-header { font-weight: bold; color: #0f172a; border-bottom: 1px solid #f1f5f9; padding-bottom: 3px; margin-bottom: 5px; }"
                       ".badge { background-color: #e2e8f0; color: #475569; border-radius: 3px; padding: 1px 4px; font-size: 11px; font-weight: bold; }"
                       "</style></head><body>";
        
        html += QString("<h3>Template: %1</h3>").arg(templateName.isEmpty() ? "<i>Untitled Template</i>" : templateName);
        
        html += "<table>";
        html += QString("<tr><td class='label'>Paper Size:</td><td>%1 &times; %2 in</td></tr>")
                .arg(paperW / 96.0, 0, 'f', 3)
                .arg(paperH / 96.0, 0, 'f', 3);
        html += QString("<tr><td class='label'>Default Printer:</td><td>%1</td></tr>")
                .arg(printerName.isEmpty() ? "<i>None (System Default)</i>" : printerName);
        html += QString("<tr><td class='label'>Print Correction:</td><td>%1&deg;</td></tr>").arg(printCorrection);
        html += QString("<tr><td class='label'>Base Offset X:</td><td>%1 in</td></tr>").arg(printOffsetX);
        html += QString("<tr><td class='label'>Base Offset Y:</td><td>%1 in</td></tr>").arg(printOffsetY);
        html += QString("<tr><td class='label'>Background Image:</td><td>%1</td></tr>")
                .arg(bgImagePath.isEmpty() ? "<i>None / Transparent</i>" : bgImagePath);
        html += "</table>";
        
        html += QString("<h4>Layout Elements (%1)</h4>").arg(templateItems.size());
        
        for (int i = 0; i < templateItems.size(); ++i) {
            const QVariantMap &item = templateItems[i];
            QString idStr = item.value("id").toString();
            
            html += "<div class='card'>";
            html += QString("<div class='card-header'>#%1 &nbsp; <span class='badge'>%2</span></div>").arg(i + 1).arg(idStr);
            
            html += "<table>";
            
            double x = item.value("x").toDouble();
            double y = item.value("y").toDouble();
            double w = item.value("w").toDouble();
            double h = item.value("h").toDouble();
            html += QString("<tr><td class='label'>Bounds (X, Y):</td><td>(%1, %2)</td></tr>").arg(x).arg(y);
            html += QString("<tr><td class='label'>Size (W &times; H):</td><td>%1 &times; %2 px</td></tr>").arg(w).arg(h);
            
            if (item.contains("text") && !item.value("text").toString().isEmpty()) {
                html += QString("<tr><td class='label'>Text Content:</td><td>\"%1\"</td></tr>").arg(item.value("text").toString());
            }
            if (item.contains("font") && !item.value("font").toString().isEmpty()) {
                html += QString("<tr><td class='label'>Font Family:</td><td>%1</td></tr>").arg(item.value("font").toString());
            }
            if (item.contains("size") && item.value("size").toDouble() > 0) {
                html += QString("<tr><td class='label'>Font Size:</td><td>%1 pt</td></tr>").arg(item.value("size").toDouble());
            }
            if (item.contains("color") && !item.value("color").toString().isEmpty()) {
                html += QString("<tr><td class='label'>Color:</td><td><font color='%1'><b>%1</b></font></td></tr>").arg(item.value("color").toString());
            }
            if (item.contains("rows") && item.value("rows").toInt() > 0) {
                html += QString("<tr><td class='label'>Grid Rows:</td><td>%1</td></tr>").arg(item.value("rows").toInt());
            }
            if (item.contains("cols") && item.value("cols").toInt() > 0) {
                html += QString("<tr><td class='label'>Grid Cols:</td><td>%1</td></tr>").arg(item.value("cols").toInt());
            }
            if (item.contains("circleLineWidth") && item.value("circleLineWidth").toDouble() > 0) {
                html += QString("<tr><td class='label'>Circle Width:</td><td>%1 px</td></tr>").arg(item.value("circleLineWidth").toDouble());
            }
            if (item.contains("imagePath") && !item.value("imagePath").toString().isEmpty()) {
                html += QString("<tr><td class='label'>Image Path:</td><td>%1</td></tr>").arg(item.value("imagePath").toString());
            }
            if (item.contains("cellData")) {
                QStringList cd = item.value("cellData").toStringList();
                if (!cd.isEmpty()) {
                    html += QString("<tr><td class='label'>Cell Mappings:</td><td>%1</td></tr>").arg(cd.join(", "));
                }
            }
            
            html += "</table>";
            html += "</div>";
        }
        
        html += "</body></html>";
        infoBrowser->setHtml(html);
        contentLayout->addWidget(infoBrowser, 1);
        
        mainLayout->addLayout(contentLayout);
        
        // Paper background representation
        QGraphicsRectItem *paperItem = m_scene->addRect(0, 0, m_paperW, m_paperH, QPen(Qt::black, 2), Qt::white);
        paperItem->setZValue(-10);
        
        // Also draw simple grid lines or dashed lines inside the paper to show bounds clearly
        QGraphicsRectItem *dashedBorder = m_scene->addRect(0, 0, m_paperW, m_paperH, QPen(Qt::darkGray, 1, Qt::DashLine));
        dashedBorder->setZValue(-9);

        // Add the template rendering as a layer on top
        m_layerItem = new DraggablePixmapItem(templatePixmap);
        m_scene->addItem(m_layerItem);
        m_layerItem->setZValue(1);
        
        // Calculate initial pixel position of the template layer in unrotated coordinates
        double pxX = (m_printOffsetX + initLeftShift) * 96.0;
        double pxY = (m_printOffsetY + initTopShift) * 96.0;
        m_layerItem->setPos(pxX, pxY);
        
        // Connect changes from dragging to spinboxes
        m_layerItem->setOnMoveCallback([this](const QPointF &newPos) {
            if (m_isUpdating) return;
            m_isUpdating = true;
            
            // Get unrotated physical offsets
            double leftShift = (newPos.x() / 96.0) - m_printOffsetX;
            double topShift = (newPos.y() / 96.0) - m_printOffsetY;
            
            // Rotate physical offsets to visual ones
            double visX = leftShift * m_cosA + topShift * m_sinA;
            double visY = -leftShift * m_sinA + topShift * m_cosA;
            
            m_xSpin->setValue(visX);
            m_ySpin->setValue(visY);
            m_isUpdating = false;
        });
        
        // Connect changes from spinboxes to layer position
        auto updateLayerFromSpin = [this]() {
            if (m_isUpdating) return;
            m_isUpdating = true;
            
            double visX = m_xSpin->value();
            double visY = m_ySpin->value();
            
            // Transform visual offsets back to physical ones
            double leftShift = visX * m_cosA - visY * m_sinA;
            double topShift = visX * m_sinA + visY * m_cosA;
            
            double pxX = (m_printOffsetX + leftShift) * 96.0;
            double pxY = (m_printOffsetY + topShift) * 96.0;
            m_layerItem->setPos(pxX, pxY);
            m_isUpdating = false;
        };
        
        connect(m_xSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [updateLayerFromSpin]() {
            updateLayerFromSpin();
        });
        connect(m_ySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [updateLayerFromSpin]() {
            updateLayerFromSpin();
        });
        
        // Bottom ButtonBox
        QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        mainLayout->addWidget(btnBox);
        
        // Center view on scene initially, but let's fit paper in view with a small margin
        m_scene->setSceneRect(-200, -200, m_paperW + 400, m_paperH + 400);
        m_view->fitInView(0, 0, m_paperW, m_paperH, Qt::KeepAspectRatio);
    }
    
    double leftShift() const {
        double visX = m_xSpin->value();
        double visY = m_ySpin->value();
        return visX * m_cosA - visY * m_sinA;
    }

    double topShift() const {
        double visX = m_xSpin->value();
        double visY = m_ySpin->value();
        return visX * m_sinA + visY * m_cosA;
    }

private:
    double m_printOffsetX;
    double m_printOffsetY;
    double m_paperW;
    double m_paperH;
    double m_cosA;
    double m_sinA;
    
    QDoubleSpinBox *m_xSpin;
    QDoubleSpinBox *m_ySpin;
    
    QGraphicsScene *m_scene;
    ZoomableGraphicsView *m_view;
    DraggablePixmapItem *m_layerItem;
    bool m_isUpdating;
};

void FinalTweaksDialog::onVisualAdjustClicked() {
    if (!m_previewDlg || !m_previewDlg->m_scene) return;
    
    QPixmap currentPixmap(m_previewDlg->m_paperW, m_previewDlg->m_paperH);
    currentPixmap.fill(Qt::white);
    QPainter painter(&currentPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    m_previewDlg->m_scene->render(&painter, QRectF(0, 0, m_previewDlg->m_paperW, m_previewDlg->m_paperH), QRectF(0, 0, m_previewDlg->m_paperW, m_previewDlg->m_paperH));
    painter.end();
    
    QslTweakAdjustDialog dlg(m_leftSpin->value(), m_topSpin->value(), 
                             m_previewDlg->m_printOffsetX, m_previewDlg->m_printOffsetY, m_previewDlg->m_paperW, m_previewDlg->m_paperH, 
                             currentPixmap, m_previewDlg->m_currentTemplate, m_previewDlg->m_printerName,
                             m_previewDlg->m_printCorrection, m_previewDlg->m_bgImagePath, m_previewDlg->m_templateItems, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_leftSpin->setValue(dlg.leftShift());
        m_topSpin->setValue(dlg.topShift());
        m_chkUseTweaks->setChecked(true);
    }
}

QslPreviewDialog::QslPreviewDialog(const QString &callsign, const QString &infoHtml, const QList<QsoDetail> &originalQsos, QWidget *parent, const QString &printMode) 
    : QDialog(parent), m_callsign(callsign), m_infoHtml(infoHtml), m_qsos(originalQsos), m_paperW(6.5 * 96), m_paperH(3.625 * 96), m_printCorrection(0), m_printOffsetX(0.0), m_printOffsetY(0.0)
{
    // Try to load QSOs from user-defined logfiles if available
    QList<QsoDetail> fileQsos = loadQsosFromDefinedLogfiles(callsign);
    if (!fileQsos.isEmpty()) {
        m_qsos = fileQsos;
    }
    const QList<QsoDetail> &qsos = m_qsos;

    bool printAll = true;
    if (printMode == "all" || printMode == "reprint") {
        printAll = true;
    } else if (printMode == "unprinted") {
        printAll = false;
    } else {
        bool hasPrinted = false;
        bool hasUnprinted = false;
        for (const auto &qso : qsos) {
            if (qso.qslsdate.isEmpty()) {
                hasUnprinted = true;
            } else {
                hasPrinted = true;
            }
        }
        
        if (hasPrinted && hasUnprinted) {
            QMessageBox msgBox(parent ? qobject_cast<QWidget*>(parent) : nullptr);
            msgBox.setWindowTitle("Print QSLs");
            msgBox.setText(QString("The callsign %1 has both printed and unprinted contacts.").arg(callsign));
            msgBox.setInformativeText("Would you like to print all contacts or only the ones not yet printed?");
            QPushButton *btnAll = msgBox.addButton("All Contacts", QMessageBox::YesRole);
            QPushButton *btnUnprinted = msgBox.addButton("Only Unprinted", QMessageBox::NoRole);
            msgBox.addButton(QMessageBox::Cancel);
            msgBox.setDefaultButton(btnUnprinted);
            msgBox.exec();
            
            if (msgBox.clickedButton() == btnAll) {
                printAll = true;
            } else if (msgBox.clickedButton() == btnUnprinted) {
                printAll = false;
            } else {
                QTimer::singleShot(0, this, &QDialog::reject);
                printAll = false;
            }
        }
    }

    setWindowTitle(QString("Print Manager - %1").arg(callsign));
    setMinimumSize(939, 747);
    setMaximumSize(939, 747);
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    if (settings.contains("PrintManager/Geometry")) {
        restoreGeometry(settings.value("PrintManager/Geometry").toByteArray());
    } else {
        resize(939, 747);
    }

    m_manualFieldData["<comment>"] = settings.value("PrintManager/PersistentComment", "").toString();
    m_manualFieldData["<comment_align>"] = settings.value("PrintManager/PersistentCommentAlign", "Center").toString();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    refreshAddressData(callsign);
    
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(splitter, 1);
    
    // LEFT SIDE: Preview (Top) and Contacts (Bottom)
    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    m_scene = new QGraphicsScene(this);
    m_view = new QGraphicsView(m_scene);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setFixedSize(709, 332);
    leftLayout->addWidget(new QLabel("<b>Paper QSL Layout Preview:</b>"), 0);
    leftLayout->addWidget(m_view, 3);
    
    // Bottom: Contacts Tables
    QHBoxLayout *contactsBtnLayout = new QHBoxLayout();
    QLabel *lblSelectQsl = new QLabel("<b>Select QSL data:</b>");
    lblSelectQsl->setWordWrap(true);
    contactsBtnLayout->addWidget(lblSelectQsl);
    QPushButton *btnTestDiag = new QPushButton("TEST (Address Diag)", this);
    btnTestDiag->setStyleSheet("background-color: #8b5cf6; color: white; font-weight: bold; padding: 4px 10px; border-radius: 4px;");
    connect(btnTestDiag, &QPushButton::clicked, this, &QslPreviewDialog::openAddressTestDialog);
    contactsBtnLayout->addWidget(btnTestDiag);

    m_btnUndo = new QPushButton("Undo", this);
    m_btnReset = new QPushButton("Reset", this);
    m_btnUndo->setEnabled(false);
    
    // allow variable sizing buttons to maintain layout
    QSizePolicy sp(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_btnUndo->setSizePolicy(sp);
    m_btnReset->setSizePolicy(sp);
    
    m_btnFreqBandSwitch = new QPushButton("Freq/Band", this);
    m_btnFreqBandSwitch->setSizePolicy(sp);
    m_btnFreqBandSwitch->setEnabled(false); // disabled by default until template logic checks
    
    contactsBtnLayout->addWidget(m_btnFreqBandSwitch);
    
    m_btnAddressFont = new QPushButton("Address Font", this);
    m_btnAddressFont->setSizePolicy(sp);
    m_btnAddressFont->hide();
    contactsBtnLayout->addWidget(m_btnAddressFont);
    
    m_btnCommentFont = new QPushButton("Comment Font", this);
    m_btnCommentFont->setSizePolicy(sp);
    m_btnCommentFont->hide();
    contactsBtnLayout->addWidget(m_btnCommentFont);

    m_btnCallFont = new QPushButton("Callsign Font", this);
    m_btnCallFont->setSizePolicy(sp);
    m_btnCallFont->hide();
    contactsBtnLayout->addWidget(m_btnCallFont);

    
    contactsBtnLayout->addWidget(m_btnUndo);
    contactsBtnLayout->addWidget(m_btnReset);
    
    contactsBtnLayout->addStretch(1);
    
    connect(m_btnAddressFont, &QPushButton::clicked, this, &QslPreviewDialog::changeAddressFont);
    connect(m_btnCommentFont, &QPushButton::clicked, this, &QslPreviewDialog::changeCommentFont);
    connect(m_btnCallFont, &QPushButton::clicked, this, &QslPreviewDialog::changeCallFont);
    connect(m_btnFreqBandSwitch, &QPushButton::clicked, this, [this]() {
        m_showFreq = !m_showFreq;
        updatePreview();
    });
    connect(m_btnUndo, &QPushButton::clicked, this, &QslPreviewDialog::undo);
    connect(m_btnReset, &QPushButton::clicked, this, &QslPreviewDialog::resetToDefault);
    
    QVBoxLayout *sliderVLayout = new QVBoxLayout();
    QLabel *sliderTitle = new QLabel("Check Mark Size", this);
    sliderTitle->setAlignment(Qt::AlignCenter);
    sliderVLayout->addWidget(sliderTitle);
    
    QHBoxLayout *sliderHLayout = new QHBoxLayout();
    QLabel *sliderMinus = new QLabel("-", this);
    sliderHLayout->addWidget(sliderMinus);
    
    m_chkSizeSlider = new QSlider(Qt::Horizontal, this);
    m_chkSizeSlider->setRange(10, 64);
    m_chkSizeSlider->setValue(16);
    m_chkSizeSlider->setToolTip("Adjust Checkmark Size");
    m_chkSizeSlider->setFixedWidth(80);
    connect(m_chkSizeSlider, &QSlider::valueChanged, this, &QslPreviewDialog::updatePreview);
    sliderHLayout->addWidget(m_chkSizeSlider);
    
    QLabel *sliderPlus = new QLabel("+", this);
    sliderHLayout->addWidget(sliderPlus);
    
    sliderVLayout->addLayout(sliderHLayout);
    
    m_chkTypeCombo = new QComboBox(this);
    m_chkTypeCombo->addItem("Standard ✓");
    m_chkTypeCombo->addItem("Comical ✗");
    m_chkTypeCombo->addItem("Whimsical ✔");
    connect(m_chkTypeCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &QslPreviewDialog::updatePreview);
    connect(m_chkTypeCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &QslPreviewDialog::savePrintingPreferences);
    sliderVLayout->addWidget(m_chkTypeCombo);
    
    contactsBtnLayout->addLayout(sliderVLayout);

    leftLayout->addLayout(contactsBtnLayout, 0);
    
    // Checkmark Options
    m_chkOptionsGroup = new QGroupBox("", this);
    m_chkOptionsGroup->hide(); // hidden until template logic shows it
    QVBoxLayout *chkLayout = new QVBoxLayout(m_chkOptionsGroup);
    
    QHBoxLayout *bottomRowLayout = new QHBoxLayout();
    bottomRowLayout->addStretch(1);
    m_lblCardsUsed = new QLabel("Using 1 Card(s)", m_chkOptionsGroup);
    bottomRowLayout->addWidget(m_lblCardsUsed);
    chkLayout->addLayout(bottomRowLayout);
    
    leftLayout->addWidget(m_chkOptionsGroup, 0);
    
    // Blank Fields / Comment Editor
    m_blankFieldsWidget = new QWidget(this);
    QHBoxLayout *bfOuterLayout = new QHBoxLayout(m_blankFieldsWidget);
    bfOuterLayout->setContentsMargins(0, 0, 0, 0);
    
    m_chkComment = new QCheckBox("Show Comment?", this);
    m_chkComment->setChecked(false); // default is unselected
    m_customChkBoxStates["_comment_"] = false;
    bfOuterLayout->addWidget(m_chkComment);
    
    bfOuterLayout->addSpacing(8);
    
    m_btnCommentEditor = new QPushButton("Comment Chooser/Editor", this);
    m_btnCommentEditor->setFlat(true);
    m_btnCommentEditor->setStyleSheet("font-weight: bold; color: palette(windowText); text-align: left; background: transparent; border: none; padding: 0px;");
    m_btnCommentEditor->setCursor(Qt::PointingHandCursor);
    bfOuterLayout->addWidget(m_btnCommentEditor);
    
    connect(m_chkComment, &QCheckBox::toggled, this, [this](bool checked) {
        m_customChkBoxStates["_comment_"] = checked;
        updatePreview();
    });
    
    connect(m_btnCommentEditor, &QPushButton::clicked, this, [this]() {
        CommentEditorDialog dlg(m_manualFieldData.value("<comment>"), m_manualFieldData.value("<comment_align>"), this);
        if (dlg.exec() == QDialog::Accepted) {
            QString txt = dlg.m_textEdit->toPlainText();
            QString align = dlg.m_alignCombo->currentText();
            m_manualFieldData["<comment>"] = txt;
            m_manualFieldData["<comment_align>"] = align;
            
            QSettings settings(QCoreApplication::organizationName(), "Qwain");
            settings.setValue("PrintManager/PersistentComment", txt);
            settings.setValue("PrintManager/PersistentCommentAlign", align);
            
            updatePreview(); // redraw
        }
    });

    m_blankFieldsLayout = new QHBoxLayout();
    bfOuterLayout->addLayout(m_blankFieldsLayout);
    bfOuterLayout->addSpacing(10);
    
    m_btnEnterMissingData = new QPushButton("Enter Missing Addr Data", this);
    m_btnEnterMissingData->setFlat(true);
    m_btnEnterMissingData->setStyleSheet("font-weight: bold; color: palette(windowText); text-align: left; background: transparent; border: none; padding: 0px;");
    m_btnEnterMissingData->setCursor(Qt::PointingHandCursor);
    m_btnEnterMissingData->hide(); // Hide initially
    bfOuterLayout->addWidget(m_btnEnterMissingData);
    
    connect(m_btnEnterMissingData, &QPushButton::clicked, this, &QslPreviewDialog::promptMissingAddressData);

    bfOuterLayout->addStretch();
    // Always show it if you want, but for now we follow old logic or just show it if <comment> exists. Let's not hide it.
    leftLayout->addWidget(m_blankFieldsWidget, 0);
    
    QHBoxLayout *qsoLabelLayout = new QHBoxLayout();
    QLabel *lblSelectQso = new QLabel("<b>Select Qso:</b>");
    lblSelectQso->setWordWrap(true);
    qsoLabelLayout->addWidget(lblSelectQso);
    QPushButton *btnSelectAll = new QPushButton("Select All", this);
    QPushButton *btnClearAll = new QPushButton("Clear All", this);
    qsoLabelLayout->addWidget(btnSelectAll);
    qsoLabelLayout->addWidget(btnClearAll);
    qsoLabelLayout->addStretch(1);
    
    QString defLog = settings.value("Files/logfile").toString();
    QStringList logPaths = defLog.split(";", Qt::SkipEmptyParts);
    QString logName;
    if (logPaths.isEmpty()) {
        logName = "Not Set";
    } else if (logPaths.size() == 1) {
        logName = QFileInfo(logPaths.first()).fileName();
    } else {
        logName = QString("%1 and %2 others").arg(QFileInfo(logPaths.first()).fileName()).arg(logPaths.size() - 1);
    }

    m_lblLogfile = new QLabel(QString("<b>Using Logfile(s):</b> <a href='#'>%1</a>").arg(logName));
    m_lblLogfile->setWordWrap(true);
    m_lblLogfile->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    connect(m_lblLogfile, &QLabel::linkActivated, this, [this](const QString& link) {
        onLogfileClicked(link);
    });
    qsoLabelLayout->addWidget(m_lblLogfile);
    
    leftLayout->addLayout(qsoLabelLayout, 0);
    
    m_contactsTable = new QTableWidget(qsos.size(), 5, this);
    m_contactsTable->horizontalHeader()->setVisible(false);
    m_contactsTable->verticalHeader()->setVisible(false);
    m_contactsTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_contactsTable->setFrameStyle(QFrame::NoFrame);
    m_contactsTable->viewport()->setAutoFillBackground(false);
    m_contactsTable->setStyleSheet("QTableWidget { border: 1px solid white; background: transparent; } QTableWidget::item { border: none; background: transparent; } QTableWidget::item:hover { color: orange; font-weight: 800; }");
    m_contactsTable->setShowGrid(false);
    m_contactsTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_contactsTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_contactsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_contactsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_contactsTable->setColumnWidth(0, 30);
    m_contactsTable->setMaximumHeight(200);
    int unprintedCount = 0;
    for (const auto &qso : qsos) {
        if (qso.qslsdate.isEmpty()) {
            unprintedCount++;
        }
    }
    for (int i=0; i<qsos.size(); ++i) {
        QsoDetail adjustedQso = adjustQsoToEastern(qsos[i]);
        bool checked = printAll || qsos[i].qslsdate.isEmpty() || (unprintedCount == 0);
        QTableWidgetItem *chkItem = new QTableWidgetItem(checked ? "✔" : "");
        chkItem->setForeground(QBrush(Qt::green));
        chkItem->setData(Qt::UserRole, checked);
        chkItem->setTextAlignment(Qt::AlignCenter);
        m_contactsTable->setItem(i, 0, chkItem);
        
        m_contactsTable->setItem(i, 1, new QTableWidgetItem(QDate::fromString(adjustedQso.date, "yyyyMMdd").toString("dd-MM-yyyy")));
        QString formattedTime = adjustedQso.time.left(4); // "1430"
        formattedTime.insert(2, ":"); // "14:30"
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        QString tzAbbr = settings.value("UI/alwaysEastern", false).toBool() ? "et" : "utc";
        m_contactsTable->setItem(i, 2, new QTableWidgetItem(formattedTime + " " + tzAbbr));
        m_contactsTable->setItem(i, 3, new QTableWidgetItem(adjustedQso.band));
        m_contactsTable->setItem(i, 4, new QTableWidgetItem(adjustedQso.mode));
    }
    
    connect(btnSelectAll, &QPushButton::clicked, this, [this]() {
        for (int i=0; i < m_contactsTable->rowCount(); ++i) {
            m_contactsTable->item(i, 0)->setData(Qt::UserRole, true);
            m_contactsTable->item(i, 0)->setText("✔");
        }
        updatePreview();
    });
    connect(btnClearAll, &QPushButton::clicked, this, [this]() {
        for (int i=0; i < m_contactsTable->rowCount(); ++i) {
            m_contactsTable->item(i, 0)->setData(Qt::UserRole, false);
            m_contactsTable->item(i, 0)->setText("");
        }
        updatePreview();
    });
    
    connect(m_contactsTable, &QTableWidget::cellClicked, this, [this](int row, int col) {
        Q_UNUSED(col);
        if (!m_contactsTable->item(row, 0)) return;
        bool isIncluded = m_contactsTable->item(row, 0)->data(Qt::UserRole).toBool();
        isIncluded = !isIncluded;
        m_contactsTable->item(row, 0)->setData(Qt::UserRole, isIncluded);
        m_contactsTable->item(row, 0)->setText(isIncluded ? "✔" : "");
        updatePreview();
    });
    
    leftLayout->addWidget(m_contactsTable, 1);
    
    splitter->addWidget(leftWidget);
    
    // RIGHT SIDE: Templates list
    QWidget *rightWidget = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    
    rightLayout->addWidget(new QLabel("<b>Select Qso Card To Use:</b>"));
    m_templateList = new QListWidget(this);
    rightLayout->addWidget(m_templateList, 1);

    rightLayout->addWidget(new QLabel("<b>Envelopes:</b>"));
    m_envTemplateList = new QListWidget(this);
    rightLayout->addWidget(m_envTemplateList, 1);

    QSettings tSettings(QCoreApplication::organizationName(), "Qwain");
    tSettings.beginGroup("QslTemplates");
    for (const QString &name : tSettings.childGroups()) {
        tSettings.beginGroup(name);
        bool isEnv = tSettings.value("isEnvelope", false).toBool();
        tSettings.endGroup();
        if (isEnv) m_envTemplateList->addItem(name);
        else m_templateList->addItem(name);
    }
    tSettings.endGroup();
    
    // Load last-used card/envelope selections
    QSettings mainSettings(QCoreApplication::organizationName(), "Qwain");
    QString lastCardTemp = mainSettings.value("QslUpdates/LastCardTemplate", "").toString();
    QString lastEnvTemp = mainSettings.value("QslUpdates/LastEnvTemplate", "").toString();

    bool cardSelected = false;
    if (!lastCardTemp.isEmpty() && m_templateList->count() > 0) {
        for (int i = 0; i < m_templateList->count(); ++i) {
            if (m_templateList->item(i)->text() == lastCardTemp) {
                m_templateList->setCurrentRow(i);
                cardSelected = true;
                break;
            }
        }
    }
    if (!cardSelected && m_templateList->count() > 0) {
        m_templateList->setCurrentRow(0);
    }

    bool envSelected = false;
    if (!lastEnvTemp.isEmpty() && m_envTemplateList->count() > 0) {
        for (int i = 0; i < m_envTemplateList->count(); ++i) {
            if (m_envTemplateList->item(i)->text() == lastEnvTemp) {
                m_envTemplateList->setCurrentRow(i);
                envSelected = true;
                break;
            }
        }
    }
    if (!envSelected && m_envTemplateList->count() > 0) {
        m_envTemplateList->setCurrentRow(0);
    }

    connect(m_templateList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        m_previewingEnvelope = false;
        updatePreview();
    });
    connect(m_templateList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        m_previewingEnvelope = false;
        onTemplateChosen(item->text());
    });
    connect(m_templateList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current, QListWidgetItem *previous) {
        if (current) {
            m_previewingEnvelope = false;
            updateThumbnail(current->text());
            updatePreview();
            
            if (!m_isProgrammaticTemplateChange) {
                QSettings mainSettings(QCoreApplication::organizationName(), "Qwain");
                mainSettings.setValue("QslUpdates/LastCardTemplate", current->text());
            }
        }
    });

    connect(m_envTemplateList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        m_previewingEnvelope = true;
        updatePreview();
    });
    connect(m_envTemplateList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        m_previewingEnvelope = true;
        onTemplateChosen(item->text());
    });
    connect(m_envTemplateList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current, QListWidgetItem *previous) {
        if (current) {
            m_previewingEnvelope = true;
            updateThumbnail(current->text());
            updatePreview();
            
            QSettings mainSettings(QCoreApplication::organizationName(), "Qwain");
            mainSettings.setValue("QslUpdates/LastEnvTemplate", current->text());
        }
    });

    // Dedicated preview window
    m_previewWindow = new QWidget(nullptr, Qt::Window);
    m_previewWindow->setWindowTitle("Preview Window");
    m_previewWindow->resize(300, 200);
    // do not force resize when content changes, let user resize it
    m_previewWindow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *previewLayout = new QVBoxLayout(m_previewWindow);
    previewLayout->addWidget(new QLabel("<b>Preview:</b>"));
    m_thumbnailScene = new QGraphicsScene(this);
    m_thumbnailView = new QGraphicsView(m_thumbnailScene, m_previewWindow);
    m_thumbnailView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_thumbnailView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_thumbnailView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    previewLayout->addWidget(m_thumbnailView);
    // show as a normal window
    m_previewWindow->setWindowFlags(Qt::Window);

    
    
    
    // Hidden/virtual control widgets to support template-unique margin shift values without changing legacy painting code
    m_topMarginSpin = new QDoubleSpinBox(this);
    m_topMarginSpin->hide();
    m_topMarginSpin->setRange(-5.0, 5.0);
    m_topMarginSpin->setSingleStep(0.01);
    m_topMarginSpin->setSuffix(" in");
    m_leftMarginSpin = new QDoubleSpinBox(this);
    m_leftMarginSpin->hide();
    m_leftMarginSpin->setRange(-5.0, 5.0);
    m_leftMarginSpin->setSingleStep(0.01);
    m_leftMarginSpin->setSuffix(" in");
    m_chkUsePrintTweaks = new QCheckBox("Use?", this);
    m_chkUsePrintTweaks->hide();
    
    m_topMarginSpin->setValue(mainSettings.value("PrintTweaks/topMargin", -0.17).toDouble());
    m_leftMarginSpin->setValue(mainSettings.value("PrintTweaks/leftMargin", 0.0).toDouble());
    m_chkUsePrintTweaks->setChecked(false);
    
    connect(m_topMarginSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
        saveUniqueTweaks();
    });
    connect(m_leftMarginSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
        saveUniqueTweaks();
    });
    connect(m_chkUsePrintTweaks, &QCheckBox::toggled, this, [this](bool checked) {
        saveUniqueTweaks();
    });

    m_printingPrefsGroup = new QGroupBox("Printing Preferences", this);
    m_printingPrefsGroup->installEventFilter(this);
    m_btnHelp = new QToolButton(m_printingPrefsGroup);
    m_btnHelp->setText("?");
    m_btnHelp->setToolTip("Printing Preferences Help");
    m_btnHelp->setStyleSheet(
        "QToolButton {"
        "  font-weight: bold;"
        "  color: #0066cc;"
        "  background: #e6f2ff;"
        "  border: 1px solid #0066cc;"
        "  border-radius: 8px;"
        "  min-width: 16px;"
        "  max-width: 16px;"
        "  min-height: 16px;"
        "  max-height: 16px;"
        "  font-size: 10px;"
        "  padding: 0px;"
        "}"
        "QToolButton:hover {"
        "  background: #cce6ff;"
        "}"
        "QToolButton:pressed {"
        "  background: #99ccff;"
        "}"
    );
    m_btnHelp->setCursor(Qt::PointingHandCursor);

    connect(m_btnHelp, &QToolButton::clicked, this, [this]() {
        QDialog helpDlg(this);
        helpDlg.setWindowTitle("Printing Preferences Help");
        helpDlg.setMinimumSize(450, 520);
        QVBoxLayout *layout = new QVBoxLayout(&helpDlg);
        
        QTextBrowser *browser = new QTextBrowser(&helpDlg);
        browser->setHtml(R"html(
<!DOCTYPE html>
<html>
<head>
<style>
body { font-family: sans-serif; font-size: 11pt; line-height: 1.4; color: #ffffff; background-color: #202020; margin: 15px; }
h1 { font-size: 14pt; font-weight: bold; text-align: center; margin-bottom: 20px; color: #ffffff; }
h2 { font-size: 12pt; font-weight: bold; margin-top: 15px; margin-bottom: 10px; border-bottom: 1px solid #555555; padding-bottom: 3px; color: #ffffff; }
h3 { font-size: 11pt; font-weight: bold; margin-top: 10px; margin-bottom: 5px; margin-left: 15px; color: #ffffff; }
p { margin-left: 25px; margin-top: 0; margin-bottom: 12px; color: #ffffff; font-weight: normal; }
</style>
</head>
<body>
<h1>Printing Preferences Help</h1>

<h2>Queue Printing Options</h2>
<p>There are two options to printing the queue</p>

<h3>Card, then Envelope.</h3>
<p>This option can be used for single or multiple card printing tasks, the card is printed then the card, so they can be kept together.</p>

<h3>All Cards, then Envelopes</h3>
<p>This option sends all the cards as one big one after the other printing, then pauses, awaiting the envelopes. This is particularly helpful when doing a batch jobs. Cards and Envelopes are printed in the same order as the queue dictates.</p>

<h2>Print Pacing Options</h2>

<h3>One at a Time with Pausing</h3>
<p>This option while in batch printing inserts a OK to continue pause. This can be helpful in test printing or for whatever reason you may choose. The pausing affects envelope printing as well.</p>

<h3>Continuous Print</h3>
<p>This option allows cards and envelopes to be printed as soon as the printer allows, giving you time to insert another card/envelope and just continue without delay.</p>

<h2>Print Targets</h2>

<h3>Print Card / Print Envelope</h3>
<p>This allows you to selectively print the card or envelop or not.<br>This is helpful if you are looking to reprint just the card or just the envelope.</p>

<h3>Print Queue in Reverse</h3>
<p>Simply prints the queue in reverse order.</p>

<h3>Printing To</h3>
<p>One of the added features of Qwain is the ability to print to different printers, either installed locally or on your network. You can define which printer to handle your cards and envelopes. Print to just one printer or split them up. It is up to you.</p>

<h3>Final Tweak Settings</h3>
<p>ALL printers are not made equally. Some printers have default offset printing settings that may or may not be allowed to be changed by the print job. This setting allows you to skew the X and Y of the print job for the ultimate in precision centering. ANY settings in which you determine is the fit for your needs are associated with the card or envelope chosen.<br>Default is 0.00x by 0.00y</p>

<h3>Use? Checkbox</h3>
<p>Allows you to selectively overide the use of, or non-use of any tweaks in the settings.</p>

</body>
</html>
        )html");
        
        layout->addWidget(browser);
        
        QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Close, &helpDlg);
        connect(btnBox, &QDialogButtonBox::rejected, &helpDlg, &QDialog::reject);
        layout->addWidget(btnBox);
        
        helpDlg.exec();
    });

    QVBoxLayout *prefsMainLayout = new QVBoxLayout(m_printingPrefsGroup);
    prefsMainLayout->setSpacing(4);
    prefsMainLayout->setContentsMargins(10, 0, 10, 6);
    
    // 1. Queue Printing Options
    prefsMainLayout->addWidget(new QLabel("<b>Queue Printing Options</b>", this));
    m_chkCardThenEnv = new QCheckBox("Card, then Envelope", this);
    m_chkAllCardsThenEnvs = new QCheckBox("All Cards, then Envelopes", this);
    prefsMainLayout->addWidget(m_chkCardThenEnv);
    prefsMainLayout->addWidget(m_chkAllCardsThenEnvs);
    
    QButtonGroup *queueGroup = new QButtonGroup(this);
    queueGroup->setExclusive(true);
    queueGroup->addButton(m_chkCardThenEnv);
    queueGroup->addButton(m_chkAllCardsThenEnvs);
    
    prefsMainLayout->addSpacing(2);
    
    // 2. Print Pacing Options
    prefsMainLayout->addWidget(new QLabel("<b>Print Pacing Options</b>", this));
    m_chkPacingOneAtATime = new QCheckBox("One at a Time with pausing", this);
    m_chkPacingContinuous = new QCheckBox("Continuous Print", this);
    prefsMainLayout->addWidget(m_chkPacingOneAtATime);
    prefsMainLayout->addWidget(m_chkPacingContinuous);
    
    QHBoxLayout *pacingDelayLayout = new QHBoxLayout();
    pacingDelayLayout->setContentsMargins(15, 0, 0, 0);
    pacingDelayLayout->addWidget(new QLabel("Delay:", this));
    m_spinPacingDelay = new QSpinBox(this);
    m_spinPacingDelay->setRange(0, 60);
    m_spinPacingDelay->setValue(4);
    m_spinPacingDelay->setSuffix(" sec");
    pacingDelayLayout->addWidget(m_spinPacingDelay);
    pacingDelayLayout->addStretch(1);
    prefsMainLayout->addLayout(pacingDelayLayout);
    
    connect(m_chkPacingContinuous, &QCheckBox::toggled, m_spinPacingDelay, &QSpinBox::setEnabled);
    m_spinPacingDelay->setEnabled(m_chkPacingContinuous->isChecked());
    
    QButtonGroup *pacingGroup = new QButtonGroup(this);
    pacingGroup->setExclusive(true);
    pacingGroup->addButton(m_chkPacingOneAtATime);
    pacingGroup->addButton(m_chkPacingContinuous);
    
    prefsMainLayout->addSpacing(2);
    
    // 3. Print Target Options
    prefsMainLayout->addWidget(new QLabel("<b>Print Targets</b>", this));
    m_chkPrintCard = new QCheckBox("Print Card", this);
    m_chkPrintCard->setChecked(mainSettings.value("QslUpdates/PrintCard", true).toBool());
    m_chkPrintEnvelope = new QCheckBox("Print Envelope", this);
    m_chkPrintEnvelope->setChecked(mainSettings.value("QslUpdates/PrintEnvelope", false).toBool());
    m_chkPrintReverse = new QCheckBox("Print Queue in Reverse", this);
    m_chkPrintReverse->setChecked(mainSettings.value("QslUpdates/PrintReverse", false).toBool());
    
    prefsMainLayout->addWidget(m_chkPrintCard);
    prefsMainLayout->addWidget(m_chkPrintEnvelope);
    prefsMainLayout->addWidget(m_chkPrintReverse);
    
    connect(m_chkPrintCard, &QCheckBox::toggled, this, &QslPreviewDialog::updatePreview);
    connect(m_chkPrintEnvelope, &QCheckBox::toggled, this, &QslPreviewDialog::updatePreview);
    
    connect(m_chkPrintReverse, &QCheckBox::toggled, this, &QslPreviewDialog::updatePreview);
    
    prefsMainLayout->addSpacing(2);
    
    // 3. Printers Block
    m_comboCardPrinter = new QComboBox(this);
    m_comboCardPrinter->hide();
    m_comboEnvPrinter = new QComboBox(this);
    m_comboEnvPrinter->hide();
    
    QStringList knownPrinters = mainSettings.value("KnownPrinters").toStringList();
    for (const QString &kp : knownPrinters) {
        QString dest = kp.split(" ").first().trimmed();
        QString display = formatPrinterDisplay(kp);
        m_comboCardPrinter->addItem(display, dest);
        m_comboEnvPrinter->addItem(display, dest);
    }
    
    if (m_comboCardPrinter->findData("Print to PDF") < 0) {
        m_comboCardPrinter->addItem("Print to PDF", "Print to PDF");
    }
    if (m_comboEnvPrinter->findData("Print to PDF") < 0) {
        m_comboEnvPrinter->addItem("Print to PDF", "Print to PDF");
    }
    
    QPushButton *btnPrintingTo = new QPushButton("Printing to", this);
    QPushButton *btnFinalTweaksSettings = new QPushButton("Final Tweak", this);
    
    QHBoxLayout *tweaksBtnLayout = new QHBoxLayout();
    tweaksBtnLayout->addWidget(btnPrintingTo);
    tweaksBtnLayout->addWidget(btnFinalTweaksSettings);
    tweaksBtnLayout->addStretch();
    prefsMainLayout->addLayout(tweaksBtnLayout);
    
    // Save to settings when user interacts
    connect(m_chkCardThenEnv, &QCheckBox::toggled, this, &QslPreviewDialog::savePrintingPreferences);
    connect(m_chkAllCardsThenEnvs, &QCheckBox::toggled, this, &QslPreviewDialog::savePrintingPreferences);
    connect(m_chkPacingOneAtATime, &QCheckBox::toggled, this, &QslPreviewDialog::savePrintingPreferences);
    connect(m_chkPacingContinuous, &QCheckBox::toggled, this, &QslPreviewDialog::savePrintingPreferences);
    connect(m_comboCardPrinter, &QComboBox::currentTextChanged, this, &QslPreviewDialog::savePrintingPreferences);
    connect(m_comboEnvPrinter, &QComboBox::currentTextChanged, this, &QslPreviewDialog::savePrintingPreferences);
    connect(m_spinPacingDelay, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &QslPreviewDialog::savePrintingPreferences);
    
    connect(btnPrintingTo, &QPushButton::clicked, this, [this]() {
        m_comboCardPrinter->setParent(nullptr);
        m_comboEnvPrinter->setParent(nullptr);
        m_comboCardPrinter->show();
        m_comboEnvPrinter->show();
        
        QDialog dlg(this);
        dlg.setWindowTitle("Printing To");
        dlg.setMinimumWidth(380);
        QVBoxLayout *dlgLayout = new QVBoxLayout(&dlg);
        
        QFormLayout *form = new QFormLayout();
        form->addRow("Print Cards On:", m_comboCardPrinter);
        form->addRow("Print Envelopes On:", m_comboEnvPrinter);
        dlgLayout->addLayout(form);
        
        QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        dlgLayout->addWidget(btnBox);
        
        int prevCardIdx = m_comboCardPrinter->currentIndex();
        int prevEnvIdx = m_comboEnvPrinter->currentIndex();
        
        if (dlg.exec() == QDialog::Accepted) {
            savePrintingPreferences();
        } else {
            m_comboCardPrinter->setCurrentIndex(prevCardIdx);
            m_comboEnvPrinter->setCurrentIndex(prevEnvIdx);
        }
        
        m_comboCardPrinter->setParent(this);
        m_comboEnvPrinter->setParent(this);
        m_comboCardPrinter->hide();
        m_comboEnvPrinter->hide();
    });
    
    connect(btnFinalTweaksSettings, &QPushButton::clicked, this, [this]() {
        FinalTweaksDialog dlg(m_topMarginSpin->value(), m_leftMarginSpin->value(), m_chkUsePrintTweaks->isChecked(), this);
        if (dlg.exec() == QDialog::Accepted) {
            m_topMarginSpin->setValue(dlg.topShift());
            m_leftMarginSpin->setValue(dlg.leftShift());
            m_chkUsePrintTweaks->setChecked(dlg.useTweaks());
            saveUniqueTweaks();
        }
    });
    
    loadPrintingPreferences();
    rightLayout->addWidget(m_printingPrefsGroup, 0);

    QGroupBox *updateQslGroup = new QGroupBox("Log Updates", this);
    QVBoxLayout *updateQslLayout = new QVBoxLayout(updateQslGroup);
    
    QHBoxLayout *logsLayout = new QHBoxLayout;
    m_chkQRZ = new QCheckBox("QRZ", this);
    m_chkEqsl = new QCheckBox("eQSL", this);

    m_chkQRZ->setChecked(mainSettings.value("QslUpdates/QRZ", false).toBool());
    m_chkEqsl->setChecked(mainSettings.value("QslUpdates/eQSL", false).toBool());
    m_chkQRZ->setEnabled(true);
    m_chkEqsl->setEnabled(true);
    logsLayout->addWidget(m_chkQRZ);
    logsLayout->addWidget(m_chkEqsl);
    updateQslLayout->addLayout(logsLayout);
    rightLayout->addWidget(updateQslGroup, 0);



    m_btnPrint = new QPushButton("Send To Printer", this);
    connect(m_btnPrint, &QPushButton::clicked, this, &QslPreviewDialog::onSendToPrinterClicked);
    rightLayout->addWidget(m_btnPrint, 0);


    
    m_netManager = new QNetworkAccessManager(this);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 7); // give more space to the left
    splitter->setStretchFactor(1, 3); // 30% to the right (a 60% shrink from 50/50? Right gets smaller)

    for (QPushButton *btn : findChildren<QPushButton*>()) {
        QFont f = btn->font();
        f.setPointSize(8);
        btn->setFont(f);
    }

    if (m_printQueue.isEmpty()) {
        m_printQueue << (m_callsign + "|" + (printAll ? "all" : "unprinted"));
        m_printQueueIndex = 0;
    }

    updatePreview();
}



void QslPreviewDialog::promptMissingAddressData() {
    QDialog dlg(this);
    dlg.setWindowTitle("Enter Missing Data");
    dlg.setStyleSheet("QDialog { background-color: #f0f0f0; }");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);
    
    QFrame *envelopeFrame = new QFrame(&dlg);
    envelopeFrame->setObjectName("envelopeFrame");
    envelopeFrame->setStyleSheet("#envelopeFrame { background-color: #ffffff; border: 1px solid #cccccc; border-radius: 4px; }");
    envelopeFrame->setFixedSize(500, 250);
    
    QVBoxLayout *envLayout = new QVBoxLayout(envelopeFrame);
    envLayout->setContentsMargins(15, 15, 15, 15);
    
    QHBoxLayout *topRow = new QHBoxLayout();
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString returnName = settings.value("Station/Callsign").toString();
    QString returnAddr = "";
    QString returnCity = "";
    
    QString returnText = "<b>" + returnName + "</b><br>" + returnAddr + "<br>" + returnCity;
    QLabel *returnLabel = new QLabel(returnText, envelopeFrame);
    returnLabel->setStyleSheet("font-size: 10pt; color: #555;");
    returnLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    topRow->addWidget(returnLabel);
    
    topRow->addStretch();
    
    QLabel *stampLabel = new QLabel("STAMP", envelopeFrame);
    stampLabel->setFixedSize(50, 60);
    stampLabel->setStyleSheet("border: 1px dashed #999; color: #999; font-size: 8pt; font-weight: bold;");
    stampLabel->setAlignment(Qt::AlignCenter);
    topRow->addWidget(stampLabel);
    
    envLayout->addLayout(topRow);
    
    envLayout->addStretch();
    
    QHBoxLayout *addrRow = new QHBoxLayout();
    addrRow->addStretch(1);
    
    QFormLayout *fl = new QFormLayout();
    fl->setLabelAlignment(Qt::AlignRight);
    
    QLineEdit *nameEdit = new QLineEdit(envelopeFrame);
    nameEdit->setPlaceholderText("Name / Callsign");
    nameEdit->setText(m_addrName);
    nameEdit->setStyleSheet("background: #fdfdfd; color: black; border: 1px solid #ddd; padding: 3px; min-width: 200px;");
    
    QLineEdit *streetEdit = new QLineEdit(envelopeFrame);
    streetEdit->setPlaceholderText("Street Address");
    streetEdit->setText(m_addrStreet);
    streetEdit->setStyleSheet("background: #fdfdfd; color: black; border: 1px solid #ddd; padding: 3px; min-width: 200px;");
    
    QLineEdit *cityEdit = new QLineEdit(envelopeFrame);
    cityEdit->setPlaceholderText("City, State Zip");
    cityEdit->setText(m_addrCity);
    cityEdit->setStyleSheet("background: #fdfdfd; color: black; border: 1px solid #ddd; padding: 3px; min-width: 200px;");
    
    QLineEdit *countryEdit = new QLineEdit(envelopeFrame);
    countryEdit->setPlaceholderText("Country");
    countryEdit->setText(m_addrCountry);
    countryEdit->setStyleSheet("background: #fdfdfd; color: black; border: 1px solid #ddd; padding: 3px; min-width: 200px;");
    
    fl->addRow("To:", nameEdit);
    fl->addRow("", streetEdit);
    fl->addRow("", cityEdit);
    fl->addRow("", countryEdit);
    
    addrRow->addLayout(fl, 2);
    
    envLayout->addLayout(addrRow);
    envLayout->addStretch();
    
    mainLayout->addWidget(envelopeFrame);
    
    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    mainLayout->addWidget(btnBox);
    
    if (dlg.exec() == QDialog::Accepted) {
        m_addrName = formatNameOrCity(nameEdit->text());
        m_addrStreet = formatAddress(streetEdit->text());
        m_addrCity = formatCity(cityEdit->text());
        m_addrCountry = formatCountry(countryEdit->text());
        m_addressManuallyAccepted = true;
        updatePreview();
    }
}
void QslPreviewDialog::showPreviewWindowOnly() {
    updatePreview();
    updateThumbnail(m_currentTemplate);
    if (m_previewWindow) {
        m_previewWindow->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_previewWindow, &QWidget::destroyed, this, &QObject::deleteLater);
        m_previewWindow->show();
        m_previewWindow->raise();
        m_previewWindow->activateWindow();
    }
}

void QslPreviewDialog::setPreviewState(const QVariantMap &state) {
    m_templateList->setEnabled(false); // Can't switch templates when previewing unsaved state
    m_paperW = state["paperW"].toDouble();
    m_paperH = state["paperH"].toDouble();
    m_printerName = state.value("printerName").toString();
    m_printCorrection = state.value("printCorrection", 0).toInt();
    m_printOffsetX = state.value("printOffsetX", 0.0).toDouble();
    m_printOffsetY = state.value("printOffsetY", 0.0).toDouble();
    m_bgImagePath = state["bgImagePath"].toString();
    m_templateItems.clear();
    QVariantList items = state["items"].toList();
    for (const QVariant &v : items) {
        m_templateItems.append(v.toMap());
    }
    updateCustomCheckmarks(); // Inject here
    updatePreview();
}

void QslPreviewDialog::updateCustomCheckmarks() {
    // Safely and asynchronously delete all direct child widgets of the group box
    // to prevent crashes if a checkbox is deleted from within its own clicked/toggled signal handler.
    for (QWidget *w : m_chkOptionsGroup->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
        w->hide();
        w->setParent(nullptr);
        w->deleteLater();
    }
    if (m_chkOptionsGroup->layout()) {
        delete m_chkOptionsGroup->layout();
    }
    
    m_customChkBoxes.clear();
    m_lblCardsUsed = nullptr;

    QVBoxLayout *mainVLayout = new QVBoxLayout(m_chkOptionsGroup);
    mainVLayout->setContentsMargins(10, 6, 10, 6);
    mainVLayout->setSpacing(4);
    
    QLabel *lblCheckmarksTitle = new QLabel("Checkmarks", m_chkOptionsGroup);
    lblCheckmarksTitle->setStyleSheet("font-weight: bold; color: palette(windowText); margin-right: 6px;");
    lblCheckmarksTitle->setAlignment(Qt::AlignLeft);
    
    QList<QCheckBox*> checkmarkCheckBoxes;
    QSet<QString> parsedCheckNames;
    
    for (const QVariantMap &m : m_templateItems) {
        QString idStr = m["id"].toString();
        if (idStr.startsWith("CHK_")) {
            // avoid duplicates
            if (parsedCheckNames.contains(idStr)) continue;
            parsedCheckNames.insert(idStr);
            
            QString name = idStr.mid(4);
            if (name.contains(":")) {
                name = name.split(":").last().trimmed();
            }
            
            QCheckBox *cb = new QCheckBox(name, m_chkOptionsGroup);
            
            bool shouldCheck = false;
            if (m_customChkBoxStates.contains(idStr)) {
                shouldCheck = m_customChkBoxStates.value(idStr);
            } else {
                if (idStr.contains("QSL") && !idStr.contains("PSE") && !idStr.contains("TNX")) {
                    shouldCheck = true;
                }
                
                for (const QsoDetail &q : m_qsos) {
                    if (q.band.trimmed().compare(name.trimmed(), Qt::CaseInsensitive) == 0) {
                        shouldCheck = true;
                        break;
                    }
                }
                m_customChkBoxStates[idStr] = shouldCheck;
            }
            if (shouldCheck) {
                cb->setChecked(true);
            }
            
            connect(cb, &QCheckBox::toggled, this, [this, cb, name, idStr](bool checked) {
                m_customChkBoxStates[idStr] = checked;
                if (checked) {
                    if (name.compare("TNX", Qt::CaseInsensitive) == 0) {
                        for (auto it = m_customChkBoxes.begin(); it != m_customChkBoxes.end(); ++it) {
                            QCheckBox *other = it.value();
                            if (other != cb && other->text().compare("PSE", Qt::CaseInsensitive) == 0) {
                                m_customChkBoxStates[it.key()] = false;
                                other->blockSignals(true);
                                other->setChecked(false);
                                other->blockSignals(false);
                            }
                        }
                    } else if (name.compare("PSE", Qt::CaseInsensitive) == 0) {
                        for (auto it = m_customChkBoxes.begin(); it != m_customChkBoxes.end(); ++it) {
                            QCheckBox *other = it.value();
                            if (other != cb && other->text().compare("TNX", Qt::CaseInsensitive) == 0) {
                                m_customChkBoxStates[it.key()] = false;
                                other->blockSignals(true);
                                other->setChecked(false);
                                other->blockSignals(false);
                            }
                        }
                    }
                }
                updatePreview();
            });
            
            m_customChkBoxes[idStr] = cb;
            checkmarkCheckBoxes.append(cb);
        }
    }

    // Check if the current template contains a comment box field
    bool templateHasComment = false;
    for (const QVariantMap &m : m_templateItems) {
        QString idStr = m["id"].toString();
        QString textStr = m["cellData"].toStringList().isEmpty() ? m["text"].toString() : m["cellData"].toStringList().first();
        QString lowerId = idStr.toLower();
        QString lowerTextStr = textStr.toLower();
        auto isKeyword = [&](const QStringList& keywords) {
            return keywords.contains(lowerId) || keywords.contains(lowerTextStr);
        };
        bool isCommentBox = isKeyword({"comment", "<comment>", "commentbox", "comment box", "newblock", "qslmsg", "<qslmsg>", "notes", "<notes>", "message", "<message>", "msg", "<msg>"}) ||
                            lowerId.contains("comment") || lowerTextStr.contains("comment") ||
                            lowerId.contains("commentbox") || lowerTextStr.contains("commentbox") ||
                            lowerId.contains("newblock") || lowerTextStr.contains("newblock") ||
                            lowerId.contains("qslmsg") || lowerTextStr.contains("qslmsg") ||
                            lowerId.contains("notes") || lowerTextStr.contains("notes") ||
                            lowerId.contains("message") || lowerTextStr.contains("message") ||
                            lowerId.contains("msg") || lowerTextStr.contains("msg");
        if (isCommentBox) {
            templateHasComment = true;
            break;
        }
    }

    if (m_chkComment) {
        m_chkComment->setVisible(templateHasComment);
        m_chkComment->setChecked(m_customChkBoxStates.value("_comment_", false));
    }
    if (m_btnCommentEditor) {
        m_btnCommentEditor->setVisible(templateHasComment);
    }

    // Lay out checkmarkCheckBoxes in rows of up to 10 before wrapping
    QHBoxLayout *currentRowLayout = nullptr;
    for (int i = 0; i < checkmarkCheckBoxes.size(); ++i) {
        if (i % 10 == 0) {
            currentRowLayout = new QHBoxLayout();
            currentRowLayout->setContentsMargins(0, 0, 0, 0);
            currentRowLayout->setSpacing(6);
            if (i == 0) {
                currentRowLayout->addWidget(lblCheckmarksTitle);
            }
            mainVLayout->addLayout(currentRowLayout);
        }
        currentRowLayout->addWidget(checkmarkCheckBoxes[i]);
    }

    // Left-align checkboxes in the last row
    if (currentRowLayout) {
        currentRowLayout->addStretch(1);
    }

    // Now detect custom circles
    QList<QCheckBox*> circleCheckBoxes;
    QSet<QString> parsedCircleNames;
    
    for (const QVariantMap &m : m_templateItems) {
        QString idStr = m["id"].toString();
        if (idStr.startsWith("CIRC_")) {
            // avoid duplicates
            if (parsedCircleNames.contains(idStr)) continue;
            parsedCircleNames.insert(idStr);
            
            QString name = idStr.mid(5);
            if (name.contains(":")) {
                name = name.split(":").last().trimmed();
            }
            
            QCheckBox *cb = new QCheckBox(name, m_chkOptionsGroup);
            
            bool shouldCheck = false;
            if (m_customChkBoxStates.contains(idStr)) {
                shouldCheck = m_customChkBoxStates.value(idStr);
            } else {
                if (idStr.contains("QSL") && !idStr.contains("PSE") && !idStr.contains("TNX")) {
                    shouldCheck = true;
                }
                
                for (const QsoDetail &q : m_qsos) {
                    if (q.band.trimmed().compare(name.trimmed(), Qt::CaseInsensitive) == 0) {
                        shouldCheck = true;
                        break;
                    }
                }
                m_customChkBoxStates[idStr] = shouldCheck;
            }
            if (shouldCheck) {
                cb->setChecked(true);
            }
            
            connect(cb, &QCheckBox::toggled, this, [this, idStr](bool checked) {
                m_customChkBoxStates[idStr] = checked;
                updatePreview();
            });
            
            m_customChkBoxes[idStr] = cb;
            circleCheckBoxes.append(cb);
        }
    }

    // If there are circles, add them in a dedicated section with a left-justified title label "Circles"
    if (!circleCheckBoxes.isEmpty()) {
        QLabel *lblCirclesTitle = new QLabel("Circles", m_chkOptionsGroup);
        lblCirclesTitle->setStyleSheet("font-weight: bold; color: palette(windowText); margin-top: 6px; margin-right: 6px;");
        lblCirclesTitle->setAlignment(Qt::AlignLeft);
        
        QHBoxLayout *currentCircleRowLayout = nullptr;
        for (int i = 0; i < circleCheckBoxes.size(); ++i) {
            if (i % 10 == 0) {
                currentCircleRowLayout = new QHBoxLayout();
                currentCircleRowLayout->setContentsMargins(0, 0, 0, 0);
                currentCircleRowLayout->setSpacing(6);
                if (i == 0) {
                    currentCircleRowLayout->addWidget(lblCirclesTitle);
                }
                mainVLayout->addLayout(currentCircleRowLayout);
            }
            currentCircleRowLayout->addWidget(circleCheckBoxes[i]);
        }
        if (currentCircleRowLayout) {
            currentCircleRowLayout->addStretch(1);
        }
    }

    // Add card count label row at bottom
    QHBoxLayout *bottomRowLayout = new QHBoxLayout();
    bottomRowLayout->setContentsMargins(0, 0, 0, 0);
    bottomRowLayout->addStretch(1);
    
    int maxContactsPerCard = 4;
    for (const QVariantMap &m : m_templateItems) {
        int rows = m["rows"].toInt();
        int cols = m["cols"].toInt();
        if (cols >= 3 && rows > 1) {
            QStringList cellData = m["cellData"].toStringList();
            bool firstRowHasTags = false;
            for (int c = 0; c < cols; ++c) {
                if (c < cellData.size()) {
                    QString def = cellData[c].toLower().trimmed();
                    if (def.startsWith("<") && def.endsWith(">")) {
                        firstRowHasTags = true;
                        break;
                    }
                }
            }
            maxContactsPerCard = firstRowHasTags ? rows : (rows - 1);
        }
    }

    int checkedCount = 0;
    if (m_contactsTable) {
        for (int i = 0; i < m_contactsTable->rowCount(); ++i) {
            if (m_contactsTable->item(i, 0) && m_contactsTable->item(i, 0)->data(Qt::UserRole).toBool()) {
                checkedCount++;
            }
        }
        // Keep checkedCount as 0 if all are explicitly unchecked
    } else {
        checkedCount = m_qsos.size();
    }

    QString labelText;
    if (m_previewingEnvelope) {
        int totalEnvelopes = (checkedCount == 0) ? 0 : 1;
        labelText = QString("Using %1 Envelope(s)").arg(totalEnvelopes);
    } else {
        int totalCards = (checkedCount == 0) ? 0 : qMax(1, (checkedCount + maxContactsPerCard - 1) / maxContactsPerCard);
        labelText = QString("Using %1 Card(s)").arg(totalCards);
    }

    m_lblCardsUsed = new QLabel(labelText, m_chkOptionsGroup);
    bottomRowLayout->addWidget(m_lblCardsUsed);
    mainVLayout->addLayout(bottomRowLayout);
}

void QslPreviewDialog::setDataOnly(bool dataOnly) {
    m_dataOnly = dataOnly;
    renderTemplate();
}

void QslPreviewDialog::loadTemplate(const QString& rawName) {
    logDiag(QString("Loading template: %1").arg(rawName));
    if (rawName.isEmpty()) return;
    QString name = rawName;
    if (name.startsWith("🏷️ ")) name = name.mid(3).trimmed();
    m_currentTemplate = name;
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");
    settings.beginGroup(name);
    
    m_paperW = settings.value("paperW", 6.5*96).toDouble();
    m_paperH = settings.value("paperH", 3.625*96).toDouble();
    m_printerName = settings.value("printerName").toString();
    m_printCorrection = settings.value("printCorrection", 0).toInt();
    m_printOffsetX = settings.value("printOffsetX", 0.0).toDouble();
    m_printOffsetY = settings.value("printOffsetY", 0.0).toDouble();
    m_bgImagePath = settings.value("bgImagePath").toString();
    

    
    m_templateItems.clear();
    int count = settings.value("count", 0).toInt();
    for (int i=0; i<count; ++i) {
        settings.beginGroup(QString("Area_%1").arg(i));
        QVariantMap map;
        for (const auto &k : settings.childKeys()) {
            map[k] = settings.value(k);
        }
        
        int cdCount = settings.beginReadArray("CellData");
        QStringList cellData;
        for (int j=0; j<cdCount; ++j) {
            settings.setArrayIndex(j);
            cellData.append(settings.value("data").toString());
        }
        settings.endArray();
        map["cellData"] = cellData;
        
        m_templateItems.append(map);
        double itemZ = map.contains("zValue") ? map["zValue"].toDouble() : 0.0;
        logDiag(QString("  [Item #%1] ID='%2', Pos=(%3, %4), Size=(%5 x %6), ZValue=%7, Rows=%8, Cols=%9, IsImage=%10, CellData Count=%11")
            .arg(i + 1)
            .arg(map["id"].toString())
            .arg(map["x"].toDouble(), 0, 'f', 1)
            .arg(map["y"].toDouble(), 0, 'f', 1)
            .arg(map["w"].toDouble(), 0, 'f', 1)
            .arg(map["h"].toDouble(), 0, 'f', 1)
            .arg(itemZ, 0, 'f', 2)
            .arg(map["rows"].toInt())
            .arg(map["cols"].toInt())
            .arg(map["isImage"].toBool() ? "TRUE" : "FALSE")
            .arg(cellData.size()));
        settings.endGroup();
    }
    logDiag(QString("Loaded %1 template item(s) from group 'QslTemplates/%2'").arg(count).arg(m_currentTemplate));
    
    if (m_topMarginSpin && m_leftMarginSpin && m_chkUsePrintTweaks) {
        m_topMarginSpin->blockSignals(true);
        m_leftMarginSpin->blockSignals(true);
        m_chkUsePrintTweaks->blockSignals(true);
        
        m_chkUsePrintTweaks->setChecked(settings.value("useTweaksUnique", false).toBool());
        
        QSettings mainSettings(QCoreApplication::organizationName(), "Qwain");
        double oldTop = mainSettings.value("PrintTweaks/topMargin", -0.17).toDouble();
        double oldLeft = mainSettings.value("PrintTweaks/leftMargin", 0.0).toDouble();
        
        m_topMarginSpin->setValue(settings.value("topMarginUnique", oldTop).toDouble());
        m_leftMarginSpin->setValue(settings.value("leftMarginUnique", oldLeft).toDouble());
        
        m_topMarginSpin->blockSignals(false);
        m_leftMarginSpin->blockSignals(false);
        m_chkUsePrintTweaks->blockSignals(false);
    }
    
    settings.endGroup();
    settings.endGroup();
    
    updateCustomCheckmarks();
}

void QslPreviewDialog::saveUniqueTweaks() {
    if (m_currentTemplate.isEmpty()) return;
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");
    settings.beginGroup(m_currentTemplate);
    settings.setValue("useTweaksUnique", m_chkUsePrintTweaks ? m_chkUsePrintTweaks->isChecked() : false);
    settings.setValue("topMarginUnique", m_topMarginSpin ? m_topMarginSpin->value() : -0.17);
    settings.setValue("leftMarginUnique", m_leftMarginSpin ? m_leftMarginSpin->value() : 0.0);
    settings.endGroup();
    settings.endGroup();
    
    // Also save legacy/global print tweaks as backup/fallback
    settings.beginGroup("PrintTweaks");
    settings.setValue("topMargin", m_topMarginSpin ? m_topMarginSpin->value() : -0.17);
    settings.setValue("leftMargin", m_leftMarginSpin ? m_leftMarginSpin->value() : 0.0);
    settings.endGroup();

    updatePreview();
}

void QslPreviewDialog::loadPrintingPreferences() {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("PrintingPreferences");
    
    int queueOption = settings.value("QueueOption", 0).toInt();
    int pacingOption = settings.value("PacingOption", 0).toInt();
    int checkmarkType = settings.value("CheckmarkType", 0).toInt();
    
    QString cardPrinter = settings.value("CardPrinter", "").toString();
    QString envPrinter = settings.value("EnvelopePrinter", "").toString();
    int pacingDelay = settings.value("PacingDelay", 4).toInt();
    
    bool multiQslEnabled = settings.value("MultiQslEnabled", false).toBool();
    QString multiQslTemplate = settings.value("MultiQslTemplate", "").toString();
    
    settings.endGroup();

    if (m_spinPacingDelay) {
        m_spinPacingDelay->blockSignals(true);
        m_spinPacingDelay->setValue(pacingDelay);
        m_spinPacingDelay->setEnabled(pacingOption == 1);
        m_spinPacingDelay->blockSignals(false);
    }

    if (m_chkTypeCombo) {
        m_chkTypeCombo->blockSignals(true);
        m_chkTypeCombo->setCurrentIndex(checkmarkType);
        m_chkTypeCombo->blockSignals(false);
    }
    
    if (m_chkCardThenEnv && m_chkAllCardsThenEnvs) {
        m_chkCardThenEnv->blockSignals(true);
        m_chkAllCardsThenEnvs->blockSignals(true);
        m_chkCardThenEnv->setChecked(queueOption == 0);
        m_chkAllCardsThenEnvs->setChecked(queueOption == 1);
        m_chkCardThenEnv->blockSignals(false);
        m_chkAllCardsThenEnvs->blockSignals(false);
    }
    if (m_chkPacingOneAtATime && m_chkPacingContinuous) {
        m_chkPacingOneAtATime->blockSignals(true);
        m_chkPacingContinuous->blockSignals(true);
        m_chkPacingOneAtATime->setChecked(pacingOption == 0);
        m_chkPacingContinuous->setChecked(pacingOption == 1);
        m_chkPacingOneAtATime->blockSignals(false);
        m_chkPacingContinuous->blockSignals(false);
    }
    

    
    if (m_comboCardPrinter && !cardPrinter.isEmpty()) {
        m_comboCardPrinter->blockSignals(true);
        int idx = m_comboCardPrinter->findData(cardPrinter);
        if (idx >= 0) {
            m_comboCardPrinter->setCurrentIndex(idx);
        } else {
            int textIdx = m_comboCardPrinter->findText(cardPrinter);
            if (textIdx >= 0) m_comboCardPrinter->setCurrentIndex(textIdx);
        }
        m_comboCardPrinter->blockSignals(false);
    }
    if (m_comboEnvPrinter && !envPrinter.isEmpty()) {
        m_comboEnvPrinter->blockSignals(true);
        int idx = m_comboEnvPrinter->findData(envPrinter);
        if (idx >= 0) {
            m_comboEnvPrinter->setCurrentIndex(idx);
        } else {
            int textIdx = m_comboEnvPrinter->findText(envPrinter);
            if (textIdx >= 0) m_comboEnvPrinter->setCurrentIndex(textIdx);
        }
        m_comboEnvPrinter->blockSignals(false);
    }
}

void QslPreviewDialog::savePrintingPreferences() {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("PrintingPreferences");
    
    int queueOption = 0;
    if (m_chkAllCardsThenEnvs && m_chkAllCardsThenEnvs->isChecked()) {
        queueOption = 1;
    }
    
    int pacingOption = 0;
    if (m_chkPacingContinuous && m_chkPacingContinuous->isChecked()) {
        pacingOption = 1;
    }
    
    settings.setValue("QueueOption", queueOption);
    settings.setValue("PacingOption", pacingOption);
    if (m_chkTypeCombo) {
        settings.setValue("CheckmarkType", m_chkTypeCombo->currentIndex());
    }
    
    if (m_comboCardPrinter) {
        settings.setValue("CardPrinter", m_comboCardPrinter->currentData().toString().isEmpty() ? m_comboCardPrinter->currentText() : m_comboCardPrinter->currentData().toString());
    }
    if (m_comboEnvPrinter) {
        settings.setValue("EnvelopePrinter", m_comboEnvPrinter->currentData().toString().isEmpty() ? m_comboEnvPrinter->currentText() : m_comboEnvPrinter->currentData().toString());
    }
    if (m_spinPacingDelay) {
        settings.setValue("PacingDelay", m_spinPacingDelay->value());
    }
    

    
    settings.endGroup();
}

void QslPreviewDialog::onTemplateChosen(const QString& name) {
    loadTemplate(name);
    updatePreview();
}


void QslPreviewDialog::updateThumbnail(const QString& rawName) {
    if (rawName.isEmpty() || !m_thumbnailScene || !m_thumbnailView) return;
    QString name = rawName;
    if (name.startsWith("🏷️ ")) name = name.mid(3).trimmed();
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");
    if (!settings.childGroups().contains(name)) {
        settings.endGroup();
        return;
    }
    settings.beginGroup(name);
    
    double paperW = settings.value("paperW", 6.25*96).toDouble();
    double paperH = settings.value("paperH", 3.5*96).toDouble();
    QString bgImagePath = settings.value("bgImagePath").toString();
    
    settings.endGroup();
    settings.endGroup();

    m_thumbnailScene->clear();
    
    QGraphicsRectItem *paper = m_thumbnailScene->addRect(0, 0, paperW, paperH, Qt::NoPen, Qt::white);
    paper->setZValue(-2);
    
    if (!bgImagePath.isEmpty() && QFile::exists(bgImagePath)) {
        QPixmap pm(bgImagePath);
        QGraphicsPixmapItem *bg = m_thumbnailScene->addPixmap(pm.scaled(paperW, paperH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        bg->setPos(0, 0);
        bg->setZValue(-1);
    }
    
    m_thumbnailView->fitInView(m_thumbnailScene->sceneRect(), Qt::KeepAspectRatio);
    // No auto show
}

void QslPreviewDialog::saveStateForUndo() {
    if (m_currentTemplate.isEmpty()) return;
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString prefix = QString("QslTemplateOverrides/%1").arg(m_currentTemplate);
    settings.beginGroup(prefix);
    QMap<QString, QVariant> state;
    for (const QString &k : settings.allKeys()) {
        state[k] = settings.value(k);
    }
    settings.endGroup();
    
    m_undoStack.append(state);
    if (m_undoStack.size() > 3) { // 3 levels undo
        m_undoStack.removeFirst();
    }
    m_btnUndo->setEnabled(true);
}


void QslPreviewDialog::undo() {
    if (m_undoStack.isEmpty()) return;
    QMap<QString, QVariant> state = m_undoStack.takeLast();
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString prefix = QString("QslTemplateOverrides/%1").arg(m_currentTemplate);
    settings.remove(prefix); // clear all current overrides
    settings.beginGroup(prefix);
    for (auto it = state.constBegin(); it != state.constEnd(); ++it) {
        settings.setValue(it.key(), it.value());
    }
    settings.endGroup();
    m_btnUndo->setEnabled(!m_undoStack.isEmpty());
    renderTemplate();
}

void QslPreviewDialog::resetToDefault() {
    auto reply = QMessageBox::question(this, "Reset Customization",
                                       "Are you sure? This will remove all customization to the template",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }
    saveStateForUndo();
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString prefix = QString("QslTemplateOverrides/%1").arg(m_currentTemplate);
    settings.remove(prefix);
    renderTemplate();
}

// [ENTRY POINT: Preview Invalidation Slot]
// Clears layout buffers, recalculates table dimensions, and re-triggers renderTemplate().
void QslPreviewDialog::updatePreview() {
    QString mainName = m_previewingEnvelope ? getSelectedEnvTemplate() : getSelectedCardTemplate();
    
    if (mainName.isEmpty()) return;
    
    bool hasEnvelope = !getSelectedEnvTemplate().isEmpty();
    
    // Save current state
    QString savedTemplate = m_currentTemplate;
    
    // Render MAIN
    loadTemplate(mainName);
    renderTemplate(); // Renders to m_scene
    
    // Restore state
    loadTemplate(mainName);
    
    QTimer::singleShot(10, this, [this]() {
        if (m_scene && m_view && !m_scene->sceneRect().isEmpty()) {
            m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
        }
    });

    updateSendToPrinterButtonText();
}

// [ENTRY POINT: Core Layout Composition and Tag Substitution Engine]
// Loads selected card/envelope templates, processes each element (labels, shapes, text boxes),
// resolves custom ADIF/QSL tags (like <qrp> with auto-scaling text on path), and compiles the scene.

// Clean, dedicated Address Box formatter binding ADIF fields directly


void QslPreviewDialog::renderTemplate() {
    logDiag(QString("Rendering template '%1' with %2 item(s)...").arg(m_currentTemplate).arg(m_templateItems.size()));
    m_detectedClippingFields.clear();
    m_clippingFieldTexts.clear();
    m_clippingFieldFonts.clear();
    QSettings mainSettings(QCoreApplication::organizationName(), "Qwain");
    bool useStandardEmission = mainSettings.value("QslUpdates/UseStandardEmission", false).toBool();
    bool useQrpSubstitution = mainSettings.value("QslUpdates/UseQrpSubstitution", false).toBool();
    bool useCustomQrpBrag = mainSettings.value("QslUpdates/UseCustomQrpBrag", false).toBool();
    QString customQrpBragText = mainSettings.value("QslUpdates/CustomQrpBragText", "QRP").toString().trimmed();
    if (customQrpBragText.isEmpty()) customQrpBragText = "QRP";
    QString qrpSubstitutionValue = useCustomQrpBrag ? customQrpBragText : "QRP";

    auto formatMode = [&](const QString &m) -> QString {
        if (!useStandardEmission) return m;
        QString mu = m.toUpper().trimmed();
        if (mu == "SSB") return "J3E";
        if (mu == "CW") return "A1A";
        if (mu == "AM") return "A3E";
        if (mu == "FM") return "F3E";
        if (mu == "RTTY") return "F1B";
        if (mu == "PSK31") return "J2B";
        if (mu == "DSTAR") return "F7W";
        return m;
    };

    mainSettings.beginGroup("LogTweaker");
    bool useTweaks = mainSettings.value("useTweaks", false).toBool();
    bool useTweaks_FT2 = mainSettings.value("useTweaks_FT2", true).toBool();
    bool useTweaks_FT4 = mainSettings.value("useTweaks_FT4", true).toBool();
    bool useTweaks_FT8 = mainSettings.value("useTweaks_FT8", true).toBool();
    QMap<QString, QPair<int, int>> tweaksMinMax;
    if (useTweaks) {
        QStringList modes = {"FT2", "FT4", "FT8", "SSB"};
        for (const QString &mode : modes) {
            tweaksMinMax[mode] = qMakePair(mainSettings.value(mode + "_min", 10).toInt(), mainSettings.value(mode + "_max", 100).toInt());
        }
    }
    mainSettings.endGroup();

    bool templateHasQrpBox = false;
    for (const QVariantMap &item : m_templateItems) {
        QStringList cellData = item["cellData"].toStringList();
        for (const QString &cell : cellData) {
            QString cl = cell.toLower().trimmed();
            if (cl == "<qrp>" || cl == "qrp" || cl.startsWith("<qrp>")) {
                templateHasQrpBox = true;
                break;
            }
        }
        if (templateHasQrpBox) break;
        
        QString text = item["text"].toString().toLower().trimmed();
        QString id = item["id"].toString().toLower().trimmed();
        if (text == "<qrp>" || text == "qrp" || text.startsWith("<qrp>") ||
            id == "<qrp>" || id == "qrp" || id.startsWith("<qrp>")) {
            templateHasQrpBox = true;
            break;
        }
    }

    auto getTweakPwr = [&](const QsoDetail &q) -> QString {
        if (!q.tx_pwr.isEmpty()) return q.tx_pwr;
        if (!useTweaks) return "";
        QString modeKey = q.mode.toUpper();
        if (modeKey == "FT2" && !useTweaks_FT2) return "";
        if (modeKey == "FT4" && !useTweaks_FT4) return "";
        if (modeKey == "FT8" && !useTweaks_FT8) return "";
        if (modeKey == "J3E" || modeKey == "LSB" || modeKey == "USB" || modeKey == "SSB") modeKey = "SSB";
        if (tweaksMinMax.contains(modeKey)) {
            int minPwr = tweaksMinMax[modeKey].first;
            int maxPwr = tweaksMinMax[modeKey].second;
            if (minPwr > maxPwr) qSwap(minPwr, maxPwr);
            uint seed = qHash(q.date + q.time + q.band + q.mode);
            QRandomGenerator rand(seed);
            int steps = (maxPwr - minPwr) / 5;
            if (steps < 0) steps = 0;
            int val = minPwr + (rand.bounded(steps + 1) * 5);
            return QString::number(val);
        }
        return "";
    };

    auto isQrpContact = [&](const QsoDetail &q) -> bool {
        QString pwrStr = getTweakPwr(q).trimmed();
        if (pwrStr.isEmpty()) return false;
        QString cleanPwr;
        for (QChar c : pwrStr) {
            if (c.isDigit() || c == '.') cleanPwr.append(c);
            else break;
        }
        bool ok = false;
        double pwrVal = cleanPwr.toDouble(&ok);
        return (ok && pwrVal > 0.0 && pwrVal <= 6.0);
    };

    m_scene->clear();
    
    QRectF envelopeRect(0, 0, m_paperW, m_paperH);
    QGraphicsRectItem *paper = m_scene->addRect(envelopeRect, Qt::NoPen, Qt::white);
    paper->setZValue(-2);
    
    if (!m_bgImagePath.isEmpty() && QFile::exists(m_bgImagePath)) {
        QPixmap pm(m_bgImagePath);
        if (!pm.isNull()) {
            QSettings settings(QCoreApplication::organizationName(), "Qwain_EXIF");
            QString calibDims = settings.value(m_bgImagePath + "_CalibDims", "").toString();
            
            if (!calibDims.isEmpty()) {
                QString d = calibDims.toLower();
                bool isCm = d.contains("cm");
                d = d.remove("in").remove("ches").remove("cm").remove(" ");
                QStringList parts = d.split("x");
                if (parts.size() == 2) {
                    double wIn = parts[0].toDouble();
                    double hIn = parts[1].toDouble();
                    if (isCm) { wIn /= 2.54; hIn /= 2.54; } // convert to inches
                    
                    if (wIn > 0 && hIn > 0) {
                        double targetW = wIn * 96.0;
                        double targetH = hIn * 96.0;
                        QGraphicsPixmapItem *bg = m_scene->addPixmap(pm.scaled(targetW, targetH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                        bg->setPos(m_paperW/2.0 - targetW/2.0, m_paperH/2.0 - targetH/2.0);
                        bg->setZValue(-1);
                    }
                }
            } else {
                QGraphicsPixmapItem *bg = m_scene->addPixmap(pm.scaled(m_paperW, m_paperH, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                bg->setPos(m_paperW/2.0 - bg->pixmap().width()/2.0, m_paperH/2.0 - bg->pixmap().height()/2.0);
                bg->setZValue(-1);
            }
        }
    }

    // Get Selected Contacts
    QList<QsoDetail> selected;
    for (int i=0; i<m_contactsTable->rowCount(); ++i) {
        if (m_contactsTable->item(i, 0)->data(Qt::UserRole).toBool()) {
            selected.append(m_qsos[i]);
        }
    }
    // Do not auto-recheck when selected is empty to allow explicit clear all.
    if (m_batchPrintingOverflowPhase && !m_savedOriginalTemplate.isEmpty() && !m_previewingEnvelope) {
        int normalMax = calculateMaxContactsForCardTemplate(m_savedOriginalTemplate);
        if (selected.size() > normalMax) {
            selected = selected.mid(normalMax);
        } else {
            selected.clear();
        }
    }
    
    if (m_scene && m_scene->property("emptyPreview").toBool()) {
        selected.clear();
    }
    
    // Fallback sample QSO data when no log contacts are checked so QSO blocks and tags always render visibly
    if (selected.isEmpty()) {
        if (!m_qsos.isEmpty()) {
            selected.append(m_qsos.first());
        } else {
            QsoDetail dummy;
            dummy.call = m_callsign.isEmpty() ? "W4DJF" : m_callsign;
            dummy.date = QDateTime::currentDateTimeUtc().toString("yyyyMMdd");
            dummy.time = QDateTime::currentDateTimeUtc().toString("hhmm");
            dummy.freq = "14.250";
            dummy.band = "20M";
            dummy.mode = "FT8";
            dummy.rst_sent = "-05";
            dummy.rst_rcvd = "-08";
            dummy.name = "John Doe";
            dummy.address = "123 Main St";
            dummy.city = "Anytown, US";
            dummy.zip = "12345";
            dummy.country = "UNITED STATES";
            selected.append(dummy);
        }
    }
    
    for (int i = 0; i < selected.size(); ++i) {
        selected[i] = adjustQsoToEastern(selected[i]);
    }
    
    int maxContactsPerCard = 0;
    for (const QVariantMap &m : m_templateItems) {
        int rows = m["rows"].toInt();
        int cols = m["cols"].toInt();
        if (cols >= 3 && rows > 1) {
            QStringList cellData = m["cellData"].toStringList();
            bool firstRowHasTags = false;
            for (int c = 0; c < cols; ++c) {
                if (c < cellData.size()) {
                    QString def = cellData[c].toLower().trimmed();
                    if (def.startsWith("<") && def.endsWith(">")) {
                        firstRowHasTags = true;
                        break;
                    }
                }
            }
            int tableCapacity = firstRowHasTags ? rows : (rows - 1);
            maxContactsPerCard += tableCapacity;
        }
    }
    if (maxContactsPerCard < 1) maxContactsPerCard = 1;
    
    int totalCards = selected.isEmpty() ? 0 : qMax(1, (selected.size() + maxContactsPerCard - 1) / maxContactsPerCard);
    int totalEnvelopes = selected.isEmpty() ? 0 : 1;
    
    if (!m_scene->property("isPrinting").toBool()) {
        m_currentCardIndex = 0; // Only preview one card
    }
    
    if (m_lblCardsUsed) {
        if (m_previewingEnvelope) {
            m_lblCardsUsed->setText(QString("Using %1 Envelope(s)").arg(totalEnvelopes));
        } else {
            m_lblCardsUsed->setText(QString("Using %1 Card(s)").arg(totalCards));
        }
    }
    
    int startIndex = m_currentCardIndex * maxContactsPerCard;
    int count = qMin(maxContactsPerCard, selected.size() - startIndex);
    if (count > 0) {
        selected = selected.mid(startIndex, count);
    } else {
        selected.clear();
    }
    
    int numContacts = selected.size();
    
    bool hasEnvelopeAddress = false;
    bool hasCommentBox = false;
    bool hasCallsignField = false;
    if (m_btnEnterMissingData) m_btnEnterMissingData->hide();
    // Address font button visibility will be handled at the end, after checking all fields
    m_chkOptionsGroup->hide();
    QStringList blankFields;
    bool potaRendered = false;

    QString srSub = "D"; // default
    if (!selected.isEmpty()) {
        QString contactDxcc = selected[0].dxcc;
        if (contactDxcc.isEmpty()) {
            QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
            qrzCache.beginGroup(selected[0].call);
            contactDxcc = qrzCache.value("dxcc").toString();
            if (contactDxcc.isEmpty()) {
                contactDxcc = qrzCache.value("qrz_dxcc").toString();
            }
            qrzCache.endGroup();
        }
        contactDxcc = contactDxcc.trimmed();
        bool placeI = (contactDxcc != "291" && contactDxcc != "202");
        srSub = placeI ? "I" : "D";

        // Bureau OQRS check with ARRL Member restriction!
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        bool isArrlMember = settings.value("Credentials/arrlMember", false).toBool();

        bool isBureau = (selected[0].qslVia.trimmed().toUpper() == "B" || selected[0].qslVia.trimmed().toUpper() == "BUREAU");
        if (isBureau) {
            if (isArrlMember) {
                srSub = "B"; // Render "B" inside stamp box for Bureau!
            } else {
                // If not an ARRL member, fallback to International stamp reminder
                srSub = "I";
            }
        }
    }

    int globalContactIdx = 0;
    for (const QVariantMap &m : m_templateItems) {
        QString idStr = m["id"].toString();
        
        if (idStr.startsWith("REF_POINT_")) continue; // Don't show ref points in preview
        
        if (idStr == "STAMP_REMINDER") {
            if (m_previewingEnvelope) {
                double txtX = m["x"].toDouble();
                double txtY = m["y"].toDouble();
                double txtW = m["w"].toDouble();
                if (txtW <= 0) txtW = 40.0;
                
                QFont f("Arial", 6, QFont::Bold);
                QFontMetricsF fm(f);
                double fontHeight = fm.height();
                
                // If STAMP_REMINDER is defined in the template, we use/render it unconditionally
                QGraphicsSimpleTextItem *textItem = new QGraphicsSimpleTextItem(srSub);
                textItem->setFont(f);
                textItem->setBrush(QBrush(Qt::black));
                double idWidth = fm.horizontalAdvance(srSub);
                double idX = txtX + (txtW - idWidth) / 2.0;
                textItem->setPos(idX, txtY);
                m_scene->addItem(textItem);
                
                if (!selected.isEmpty()) {
                    QString contactCall = selected[0].call;
                    QGraphicsSimpleTextItem *callItem = new QGraphicsSimpleTextItem(contactCall);
                    callItem->setFont(f);
                    callItem->setBrush(QBrush(Qt::black));
                    double callWidth = fm.horizontalAdvance(contactCall);
                    double callX = txtX + (txtW - callWidth) / 2.0;
                    double callY = txtY + fontHeight;
                    callItem->setPos(callX, callY);
                    m_scene->addItem(callItem);
                }
            }
            continue; // always skip normal rendering
        }
        
        double x = m["x"].toDouble();
        double y = m["y"].toDouble();
        double w = m["w"].toDouble();
        double h = m["h"].toDouble();
        double rot = m["rot"].toDouble();
        QFont font(m["font"].toString(), m["size"].toInt());
        QColor color(m["color"].toString());
        if (!color.isValid()) {
            color = Qt::black;
        }
        
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        QString firstCellLower = m["cellData"].toStringList().isEmpty() ? m["text"].toString().toLower() : m["cellData"].toStringList().first().toLower();
        if (!m_currentTemplate.isEmpty()) {
            if (idStr.toLower() == "call" || idStr.toLower() == "callsign" || idStr.toLower() == "<call>" || firstCellLower == "<call>") {
                QString cf = settings.value(QString("QslTemplateOverrides/%1/CallFont").arg(m_currentTemplate)).toString();
                QString cc = settings.value(QString("QslTemplateOverrides/%1/CallColor").arg(m_currentTemplate)).toString();
                if (!cf.isEmpty()) font.fromString(cf);
                if (!cc.isEmpty()) color = QColor(cc);
            } else if (idStr.toLower() == "<pota_ref>" || idStr.toLower() == "<pota>" || idStr.toLower() == "pota" || idStr.toLower().contains("pota string") || firstCellLower.contains("pota")) {
                QString pf = settings.value(QString("QslTemplateOverrides/%1/PotaFont").arg(m_currentTemplate)).toString();
                QString pc = settings.value(QString("QslTemplateOverrides/%1/PotaColor").arg(m_currentTemplate)).toString();
                if (!pf.isEmpty()) font.fromString(pf);
                if (!pc.isEmpty()) color = QColor(pc);
            } else if (idStr.toLower().contains("address") || idStr.toLower().contains("addr") || idStr.toLower().contains("name") || idStr.toLower() == "city" || idStr.toLower().contains("country") || idStr.toLower() == "<state>" || idStr.toLower() == "<zip>" ||
                       firstCellLower.contains("address") || firstCellLower.contains("full contact name") || firstCellLower.contains("country")) {
                QString af = settings.value(QString("QslTemplateOverrides/%1/AddressFont").arg(m_currentTemplate)).toString();
                QString ac = settings.value(QString("QslTemplateOverrides/%1/AddressColor").arg(m_currentTemplate)).toString();
                if (!af.isEmpty()) font.fromString(af);
                if (!ac.isEmpty()) color = QColor(ac);
            } else if (idStr.toLower().contains("comment") || idStr.toLower().contains("newblock") || idStr.toLower().contains("qslmsg") || idStr.toLower().contains("notes") || idStr.toLower().contains("message") || idStr.toLower().contains("msg")) {
                QString cf = settings.value(QString("QslTemplateOverrides/%1/CommentFont").arg(m_currentTemplate)).toString();
                QString cc = settings.value(QString("QslTemplateOverrides/%1/CommentColor").arg(m_currentTemplate)).toString();
                if (!cf.isEmpty()) font.fromString(cf);
                if (!cc.isEmpty()) color = QColor(cc);
                if (settings.contains(QString("QslTemplateOverrides/%1/CommentRotation").arg(m_currentTemplate))) {
                    rot = settings.value(QString("QslTemplateOverrides/%1/CommentRotation").arg(m_currentTemplate)).toDouble();
                }
            }
        }
        
        // Check if item is an image element (supporting bool flag, string representation, or ITEM_IMAGE type 3)
        QVariant isImgV = m["isImage"];
        bool isImgElem = (isImgV.type() == QVariant::Bool) ? isImgV.toBool() : (isImgV.toString().toLower() == "true" || isImgV.toString() == "1" || m["itemType"].toInt() == 3);
        if (isImgElem) {
            QString imgPath = m["imagePath"].toString();
            bool isQr = m.value("isQrCode", false).toBool() || idStr.startsWith("QR_CODE") || imgPath.contains("qr_");
            if (isQr) {
                // Native vector mode: sharp 2-color without bilinear shading or fading
                QString qrPayload = m.value("qrPayload").toString();
                if (qrPayload.isEmpty() && m.value("text").toString() != "QR_CODE") {
                    qrPayload = m.value("text").toString();
                }
                std::vector<std::vector<bool>> modules;
                if (!qrPayload.isEmpty()) {
                    try {
                        using qrcodegen::QrCode;
                        QrCode qr = QrCode::encodeText(qrPayload.toUtf8().constData(), QrCode::Ecc::MEDIUM);
                        int sz = qr.getSize();
                        modules.assign(sz, std::vector<bool>(sz, false));
                        for (int r = 0; r < sz; ++r) {
                            for (int c = 0; c < sz; ++c) {
                                modules[r][c] = qr.getModule(c, r);
                            }
                        }
                    } catch (...) {}
                }

                double imgZValue = m.contains("zValue") ? m["zValue"].toDouble() : 0.0;
                if (!modules.empty()) {
                    QGraphicsItemGroup *group = new QGraphicsItemGroup();
                    double side = qMin(w, h);
                    int qrSize = modules.size();
                    int border = 2;
                    int totalModules = qrSize + border * 2;
                    double modSize = side / static_cast<double>(totalModules);

                    QGraphicsRectItem *bg = new QGraphicsRectItem(0, 0, side, side);
                    bg->setPen(Qt::NoPen);
                    bg->setBrush(Qt::white);
                    group->addToGroup(bg);

                    for (int r = 0; r < qrSize; ++r) {
                        for (int c = 0; c < qrSize; ++c) {
                            if (modules[r][c]) {
                                QGraphicsRectItem *mod = new QGraphicsRectItem((c + border) * modSize, (r + border) * modSize, modSize, modSize);
                                mod->setPen(Qt::NoPen);
                                mod->setBrush(Qt::black);
                                group->addToGroup(mod);
                            }
                        }
                    }
                    group->setPos(x + (w - side) / 2.0, y + (h - side) / 2.0);
                    group->setRotation(rot);
                    group->setZValue(imgZValue);
                    m_scene->addItem(group);
                    continue;
                } else {
                    // Fallback using nearest-neighbor (FastTransformation) to prevent color bleed
                    QPixmap pix(imgPath);
                    if (!pix.isNull()) {
                        QPixmap scaledPix = pix.scaled(w, h, Qt::KeepAspectRatio, Qt::FastTransformation);
                        QGraphicsPixmapItem *pixItem = m_scene->addPixmap(scaledPix);
                        pixItem->setPos(x, y);
                        pixItem->setZValue(imgZValue);
                        pixItem->setRotation(rot);
                    }
                    continue;
                }
            }

            QPixmap pix(imgPath);
            if (!pix.isNull()) {
                QPixmap scaledPix = pix.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                QGraphicsPixmapItem *pixItem = m_scene->addPixmap(scaledPix);
                pixItem->setPos(x, y);

                // CRITICAL FIX: Respect stored Z-value to guarantee proper stacking order across all layers.
                // In QslDesigner:
                //   - Background images are assigned zValue = -1.5 (staying strictly behind all other items).
                //   - Layered foreground images (badges, QR codes, Thunder Road banner) are assigned zValue = 0.0.
                //   - Standard text and data items are assigned zValue >= 1.0.
                // Previously, this branch hardcoded pixItem->setZValue(0), which caused full-card background images
                // (e.g. Item 18) to be elevated to zValue 0 and paint completely over layered images (Items 16 & 17).
                double imgZValue = 0.0;
                if (m.contains("zValue")) {
                    imgZValue = m["zValue"].toDouble();
                } else if (qFuzzyCompare(x, 0.0) && qFuzzyCompare(y, 0.0) &&
                           qAbs(w - m_paperW) < 2.0 && qAbs(h - m_paperH) < 2.0) {
                    // Fallback for legacy templates where a full-card background image had no explicit zValue saved
                    imgZValue = -1.5;
                }
                pixItem->setZValue(imgZValue);
                pixItem->setRotation(rot);
            }
            continue;
        }

        QGraphicsRectItem *area = m_scene->addRect(0, 0, w, h);
        area->setPos(x, y);
        area->setRotation(rot);
        area->setPen(Qt::NoPen);
        if (idStr.startsWith("CHK_")) {
            area->setZValue(2);
        } else {
            area->setZValue(1);
        }
        
        // Is it the QSO Table block? We rely on looking at cell count or definition
        // For simplicity, ANY block with > 1 row or col could be considered a table,
        // but QslDesigner fields usually say something like day, month in 'cellData'
        int rows = m["rows"].toInt();
        int cols = m["cols"].toInt();
        QStringList cellData = m["cellData"].toStringList();
        
        // Determine if it's the QSO Data table block (often has recognizable headers)
        bool isQsoTable = false;
        if (cols >= 4 && cellData.size() > 0) {
            QString firstRow = cellData.join(" ").toLower();
            if (firstRow.contains("day") || firstRow.contains("mhz") || firstRow.contains("mode") || firstRow.contains("date")) {
                isQsoTable = true;
            }
        }
        // Actually, if it's a table and they didn't name it "day", etc., maybe not marked. 
        // We can just assume the largest table or any table with >=3 cols is the QSO table.
        if (cols >= 3 && rows >= 1) isQsoTable = true;
        
        // CRITICAL FIX: Do NOT include `!cellData.isEmpty()` here because QslDesigner stores 1x1 text block strings
        // in `cellData[0]`. Including `!cellData.isEmpty()` forced every 1x1 text block (including ADDRESS_BOX and <COUNTRY>)
        // into the QSO table loop instead of the General Text Block branch!
        bool isTableBlock = (rows > 1 || cols > 1 || m["itemType"].toInt() == 1 || idStr.toUpper().contains("TABLE") || idStr.toUpper().contains("QSO"));
        
        // Reuse isImgV declared earlier in the loop scope (avoids redeclaration error)
        bool isImage = (isImgV.type() == QVariant::Bool) ? isImgV.toBool() : (isImgV.toString().toLower() == "true" || isImgV.toString() == "1");
        QString imagePath = m["imagePath"].toString();
        
        bool isCheckmark = idStr.startsWith("CHK_");
        bool isCircle = idStr.startsWith("CIRC_");
        bool shouldDrawCheck = false;
        bool shouldDrawCircle = false;

        if (isImage && !imagePath.isEmpty()) {
            double imgZ = 0.0;
            if (m.contains("zValue")) {
                imgZ = m["zValue"].toDouble();
            } else if (qFuzzyCompare(x, 0.0) && qFuzzyCompare(y, 0.0) &&
                       qAbs(w - m_paperW) < 2.0 && qAbs(h - m_paperH) < 2.0) {
                imgZ = -1.5;
            }
            area->setZValue(imgZ);
            
            if (QFile::exists(imagePath)) {
                QPixmap pm(imagePath);
                if (!pm.isNull()) {
                    QGraphicsPixmapItem *pmItem = m_scene->addPixmap(pm.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                    pmItem->setParentItem(area);
                    pmItem->setPos(0, 0);
                    pmItem->setZValue(imgZ);
                }
            }
        } else if (isCheckmark) {
            m_chkOptionsGroup->show();
            
            if (m_customChkBoxStates.contains(idStr)) {
                shouldDrawCheck = m_customChkBoxStates.value(idStr);
            } else {
                if (idStr.contains("QSL") && !idStr.contains("PSE") && !idStr.contains("TNX")) {
                    shouldDrawCheck = true; // Hardcoded QSL checked logic (unless it explicitly says PSE/TNX)
                }
                
                QString nameValue = idStr.mid(4);
                if (nameValue.contains(":")) {
                    nameValue = nameValue.split(":").last().trimmed();
                }
                
                bool matchesBand = false;
                for (const QsoDetail &q : selected) {
                    if (q.band.trimmed().compare(nameValue.trimmed(), Qt::CaseInsensitive) == 0) {
                        matchesBand = true;
                        break;
                    }
                }
                
                if (matchesBand) {
                    shouldDrawCheck = true;
                }
                m_customChkBoxStates[idStr] = shouldDrawCheck;
            }

            if (m_customChkBoxes.contains(idStr) && m_customChkBoxes[idStr]->isChecked() != shouldDrawCheck) {
                m_customChkBoxes[idStr]->blockSignals(true);
                m_customChkBoxes[idStr]->setChecked(shouldDrawCheck);
                m_customChkBoxes[idStr]->blockSignals(false);
            }
        } else if (isCircle) {
            m_chkOptionsGroup->show();
            
            if (m_customChkBoxStates.contains(idStr)) {
                shouldDrawCircle = m_customChkBoxStates.value(idStr);
            } else {
                if (idStr.contains("QSL") && !idStr.contains("PSE") && !idStr.contains("TNX")) {
                    shouldDrawCircle = true;
                }
                
                QString nameValue = idStr.mid(5);
                if (nameValue.contains(":")) {
                    nameValue = nameValue.split(":").last().trimmed();
                }
                
                bool matchesBand = false;
                for (const QsoDetail &q : selected) {
                    if (q.band.trimmed().compare(nameValue.trimmed(), Qt::CaseInsensitive) == 0) {
                        matchesBand = true;
                        break;
                    }
                }
                
                if (matchesBand) {
                    shouldDrawCircle = true;
                }
                m_customChkBoxStates[idStr] = shouldDrawCircle;
            }

            if (m_customChkBoxes.contains(idStr) && m_customChkBoxes[idStr]->isChecked() != shouldDrawCircle) {
                m_customChkBoxes[idStr]->blockSignals(true);
                m_customChkBoxes[idStr]->setChecked(shouldDrawCircle);
                m_customChkBoxes[idStr]->blockSignals(false);
            }
        }

        if (isImage) {
            // Already rendered image above
        } else if (isCheckmark) {
            if (shouldDrawCheck && !m_dataOnly) {
                int chkSize = m_chkSizeSlider->value();
                QPainterPath path;
                if (m_chkTypeCombo->currentIndex() == 1) { // Comical Cross (X)
                    path.moveTo(chkSize * 0.1, chkSize * 0.15);
                    path.cubicTo(chkSize * 0.3, chkSize * 0.4, chkSize * 0.7, chkSize * 0.7, chkSize * 0.9, chkSize * 0.85);
                    path.moveTo(chkSize * 0.85, chkSize * 0.1);
                    path.cubicTo(chkSize * 0.65, chkSize * 0.4, chkSize * 0.35, chkSize * 0.6, chkSize * 0.15, chkSize * 0.9);
                } else if (m_chkTypeCombo->currentIndex() == 2) { // Whimsical Checkmark (✔)
                    path.moveTo(0, chkSize * 0.45);
                    path.cubicTo(chkSize * 0.1, chkSize * 0.35, chkSize * 0.2, chkSize * 0.65, chkSize * 0.35, chkSize * 0.85);
                    path.cubicTo(chkSize * 0.6, chkSize * 0.5, chkSize * 0.85, chkSize * 0.15, chkSize * 0.95, 0);
                    path.quadTo(chkSize * 1.05, chkSize * 0.05, chkSize * 0.98, chkSize * 0.12);
                } else { // Standard Checkmark (✓)
                    path.moveTo(0, chkSize * 0.5);
                    path.lineTo(chkSize * 0.4, chkSize * 0.9);
                    path.lineTo(chkSize, 0);
                }
                QGraphicsPathItem *chk = m_scene->addPath(path, QPen(Qt::black, qMax(2, chkSize / 4), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                chk->setParentItem(area);
                QRectF br = chk->boundingRect();
                chk->setPos((w - br.width()) / 2.0, (h - br.height()) / 2.0);
            }
        } else if (isCircle) {
            if (shouldDrawCircle && !m_dataOnly) {
                int lineWidth = m.contains("circleLineWidth") ? m["circleLineWidth"].toInt() : 2;
                QGraphicsEllipseItem *ellipse = m_scene->addEllipse(0, 0, w, h, QPen(color, lineWidth));
                ellipse->setParentItem(area);
            }
        } else if (isTableBlock) {
            if (!m_currentTemplate.isEmpty()) {
                QString tf = settings.value(QString("QslTemplateOverrides/%1/TableFont").arg(m_currentTemplate)).toString();
                QString tc = settings.value(QString("QslTemplateOverrides/%1/TableColor").arg(m_currentTemplate)).toString();
                if (!tf.isEmpty()) font.fromString(tf);
                if (!tc.isEmpty()) color = QColor(tc);
            }
            
            int templateRows = m["rows"].toInt();
            if (templateRows < 1) templateRows = 1;
            int rows = templateRows;
            
            QStringList rhStr = m["rowHeights"].toString().split(",", Qt::SkipEmptyParts);
            QStringList cwStr = m["colWidths"].toString().split(",", Qt::SkipEmptyParts);
            
            QList<double> rh, cw;
            double totalRh = 0, totalCw = 0;
            for (int i=0; i<rows; ++i) {
                double v = (i < rhStr.size()) ? rhStr[i].toDouble() : 100.0;
                if (v <= 0) v = 100;
                rh << v;
                totalRh += v;
            }
            for (int i=0; i<cols; ++i) {
                double v = (i < cwStr.size()) ? cwStr[i].toDouble() : 100.0;
                if (v <= 0) v = 100;
                cw << v;
                totalCw += v;
            }
            
            if (totalRh > 0) {
                for (int i=0; i<rows; ++i) rh[i] = (rh[i] / totalRh) * h;
            } else {
                for (int i=0; i<rows; ++i) rh << (h / rows);
            }
            
            if (totalCw > 0) {
                for (int i=0; i<cols; ++i) cw[i] = qMax(1.0, (cw[i] / totalCw) * w);
            } else {
                for (int i=0; i<cols; ++i) cw << (w / cols);
            }

            int contactIdx = globalContactIdx;
            double currentY = 0;
            for (int r = 0; r < rows; ++r) {
                bool isHeaderRow = false;
                if (templateRows > 1 && r == 0) {
                    bool firstRowHasTags = false;
                    for (int c = 0; c < cols; ++c) {
                        if (c < cellData.size()) {
                            QString def = cellData[c].toLower().trimmed();
                            if (def.startsWith("<") && def.endsWith(">")) {
                                firstRowHasTags = true;
                                break;
                            }
                        }
                    }
                    isHeaderRow = !firstRowHasTags;
                }
                
                double subRowH = rh[r];
                QGraphicsRectItem *rowItem = new QGraphicsRectItem(0, 0, w, subRowH, area);
                rowItem->setPen(Qt::NoPen);
                
                qreal defX = 0;
                qreal defY = currentY;
                int fontOffset = 0;
                
                if (!m_currentTemplate.isEmpty()) {
                    QString key = QString("QslTemplateOverrides/%1/Pos_%2_%3").arg(m_currentTemplate).arg(idStr).arg(r * 10);
                    if (settings.contains(key + "_x")) defX = settings.value(key + "_x").toDouble();
                    if (settings.contains(key + "_y")) defY = settings.value(key + "_y").toDouble();
                    if (settings.contains(key + "_fontOffset")) fontOffset = settings.value(key + "_fontOffset").toInt();
                }
                rowItem->setPos(defX, defY);
                
                double currentX = 0;
                for (int c = 0; c < cols; ++c) {
                    int idx = (templateRows == 1) ? c : (r * cols + c);
                    QString def = (idx < cellData.size()) ? cellData[idx].toLower().trimmed() : "";
                    
                    // Derive field tag for data rows when cellData only stored header row labels
                    if (def.isEmpty() && r > 0) {
                        QString hdr = (c < cellData.size()) ? cellData[c].toLower().trimmed() : "";
                        if (hdr.contains("date") || hdr.contains("day")) def = "<date>";
                        else if (hdr.contains("time") || hdr.contains("utc")) def = "<time_utc>";
                        else if (hdr.contains("band") || hdr.contains("freq") || hdr.contains("mhz")) def = "<band>";
                        else if (hdr.contains("mode")) def = "<mode>";
                        else if (hdr.contains("rst") || hdr.contains("report")) def = "<rst_sent>";
                        else if (hdr.contains("call")) def = "<call>";
                        else def = hdr;
                    }
                    
                    QsoDetail qso;
                    if (contactIdx < numContacts) qso = selected[contactIdx];
                    
                    QString txt = "";
                    QString qYear = qso.date;
                    QString qMonth = "";
                    QString qDay = "";
                    if (qso.date.length() == 8) {
                        qYear = qso.date.mid(0, 4);
                        qMonth = qso.date.mid(4, 2);
                        qDay = qso.date.mid(6, 2);
                    }
                    
                    QString qTime = qso.time;
                    if (qTime.length() >= 4) qTime = qTime.left(4);
                    
                    if (def == "<qso_date>") txt = qso.date;
                    else if (def == "<day>" || def.startsWith("<day>")) {
                        txt = qDay.isEmpty() ? "" : QString("%1").arg(qDay.toInt(), 2, 10, QChar('0'));
                    }
                    else if (def == "<month>" || def.startsWith("<month>")) {
                        int mVal = qMonth.toInt();
                        if (def.contains("spelled") || def.contains("january")) {
                            static const QStringList longMonths = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
                            txt = (mVal >= 1 && mVal <= 12) ? longMonths[mVal] : qMonth;
                        } else if (def.contains("abbreviated") || def.contains("jan") || def.contains("abbr")) {
                            static const QStringList shortMonths = {"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
                            txt = (mVal >= 1 && mVal <= 12) ? shortMonths[mVal] : qMonth;
                        } else {
                            txt = (mVal >= 1 && mVal <= 12) ? QString("%1").arg(mVal, 2, 10, QChar('0')) : qMonth;
                        }
                    }
                    else if (def == "<year>" || def.startsWith("<year>")) txt = qYear;
                    else if (def == "<time_on>" || def == "<time_utc>" || def.startsWith("<time_utc>")) txt = qTime;
                    else if (def == "<band>" || def.startsWith("<band>") || def == "<freq>" || def.startsWith("<freq>")) {
                        txt = m_showFreq ? ((!qso.freq.isEmpty()) ? qso.freq : qso.band) : qso.band;
                    }
                    else if (def == "<mode>" || def.startsWith("<mode>")) txt = formatMode(qso.mode);
                    else if (def == "<rst_sent>" || def.startsWith("<rst_sent>")) txt = qso.rst_sent;
                    else if (def == "<rst_rcvd>" || def.startsWith("<rst_rcvd>")) txt = qso.rst_rcvd;
                    else if (def == "<tx_pwr>" || def.startsWith("<tx_pwr>")) txt = getTweakPwr(qso);
                    else if (def == "<qrp>" || def == "qrp" || def.startsWith("<qrp>")) {
                        txt = isQrpContact(qso) ? qrpSubstitutionValue : "";
                    }
                    else if (def == "<tone>" || def.startsWith("<tone>") || def == "tone" ||
                             def == "<tone_sent>" || def.startsWith("<tone_sent>") ||
                             def == "<tone_rcvd>" || def.startsWith("<tone_rcvd>")) {
                        if (useQrpSubstitution && isQrpContact(qso) && !templateHasQrpBox) {
                            txt = qrpSubstitutionValue;
                        } else {
                            QString rst;
                            if (def.contains("sent")) {
                                rst = qso.rst_sent;
                            } else if (def.contains("rcvd")) {
                                rst = qso.rst_rcvd;
                            } else {
                                rst = qso.rst_rcvd.isEmpty() ? qso.rst_sent : qso.rst_rcvd;
                            }
                            rst = rst.trimmed();
                            QString m = qso.mode.toUpper().trimmed();
                            bool isDigital = (m == "FT8" || m == "FT4" || m == "JT65" || m == "JT9" || 
                                              m == "JS8" || m == "WSPR" || m == "MSK144" || m == "Q65" ||
                                              m.startsWith("FT") || m.startsWith("JT"));
                            // A tone report ONLY exists in traditional CW signal reports (e.g. 599, 579).
                            // It never exists for digital modes (dB values like -09, +00) or phone (SSB).
                            // It must have exactly 3 digits, no leading sign (+/-), with R in 1-5, S in 1-9, and T in 1-9.
                            if (!isDigital && !rst.startsWith("+") && !rst.startsWith("-") && 
                                rst.length() == 3 && 
                                rst[0].isDigit() && rst[0] >= '1' && rst[0] <= '5' &&
                                rst[1].isDigit() && rst[1] >= '1' && rst[1] <= '9' &&
                                rst[2].isDigit() && rst[2] >= '1' && rst[2] <= '9') {
                                txt = rst.mid(2, 1);
                            } else {
                                txt = "";
                            }
                        }
                    }
                    else if (def == "<full_name>" || def.startsWith("<full_name>") || def == "<name>" || def.startsWith("<name>")) {
                        txt = (!selected.isEmpty() && !selected[0].name.isEmpty()) ? selected[0].name : m_addrName;
                        if (m_previewingEnvelope) txt = formatNameOrCity(txt);
                    }
                    else if (def == "<f_name>" || def.startsWith("<f_name>")) {
                        QString fullName = (!selected.isEmpty() && !selected[0].name.isEmpty()) ? selected[0].name : m_addrName;
                        txt = fullName.split(" ").first();
                        if (m_previewingEnvelope) txt = formatNameOrCity(txt);
                    }
                    else if (def == "<l_name>" || def.startsWith("<l_name>")) {
                        QString fullName = (!selected.isEmpty() && !selected[0].name.isEmpty()) ? selected[0].name : m_addrName;
                        QStringList parts = fullName.split(" ");
                        txt = parts.size() > 1 ? parts.last() : "";
                        if (m_previewingEnvelope) txt = formatNameOrCity(txt);
                    }
                    else if (def == "<call>" || def.startsWith("<call>") || def == "call" || def == "callsign" || def == "callsign field") {
                        txt = m_callsign.isEmpty() && !selected.isEmpty() ? selected[0].call : m_callsign;
                    }
                    else if (def == "<address>" || def.startsWith("<address>") || def == "address" || def == "addr" || def == "<addr1>" || def == "addr1" || def == "<addr2>" || def == "addr2") {
                        txt = (!selected.isEmpty() && !selected[0].address.isEmpty()) ? selected[0].address : m_addrStreet;
                        if (m_previewingEnvelope) txt = formatAddress(txt);
                    }
                    else if (def == "city" || def == "city/state" || def == "<city>") {
                        txt = (!selected.isEmpty() && !selected[0].city.isEmpty()) ? selected[0].city : m_addrCity;
                        if (m_previewingEnvelope) txt = formatCity(txt);
                    }
                    else if (def == "<country>" || def.startsWith("<country>") || def == "country") {
                        txt = (m_previewingEnvelope && srSub == "D") ? "" : ((!selected.isEmpty() && !selected[0].country.isEmpty()) ? selected[0].country : m_addrCountry);
                        if (m_previewingEnvelope && !txt.isEmpty()) txt = formatCountry(txt);
                    }
                    else if (def == "<gridsquare>") txt = qso.grid;
                    else if (def == "<pota_ref>") {
                        txt = qso.pota;
                    }
                    else if (def == "<comment>" || def == "comment" || def == "commentbox" || def == "<commentbox>" || def == "comment box" ||
                             def == "<qslmsg>" || def == "qslmsg" || def == "<notes>" || def == "notes" ||
                             def == "<message>" || def == "message" || def == "<msg>" || def == "msg" ||
                             def == "newblock" || def == "new block") {
                        txt = m_manualFieldData.value("<comment>").trimmed();
                        QString t = txt.trimmed();
                        if (t.compare("CommentBox", Qt::CaseInsensitive) == 0 ||
                            t.compare("Comment Box", Qt::CaseInsensitive) == 0 ||
                            t.compare("NewBlock", Qt::CaseInsensitive) == 0 ||
                            t.compare("New Block", Qt::CaseInsensitive) == 0 ||
                            t.compare("comment", Qt::CaseInsensitive) == 0 ||
                            t.compare("<comment>", Qt::CaseInsensitive) == 0 ||
                            t.compare("commentbox", Qt::CaseInsensitive) == 0 ||
                            t.compare("<commentbox>", Qt::CaseInsensitive) == 0 ||
                            t.compare("qslmsg", Qt::CaseInsensitive) == 0 ||
                            t.compare("<qslmsg>", Qt::CaseInsensitive) == 0 ||
                            t.compare("notes", Qt::CaseInsensitive) == 0 ||
                            t.compare("<notes>", Qt::CaseInsensitive) == 0 ||
                            t.compare("message", Qt::CaseInsensitive) == 0 ||
                            t.compare("<message>", Qt::CaseInsensitive) == 0 ||
                            t.compare("msg", Qt::CaseInsensitive) == 0 ||
                            t.compare("<msg>", Qt::CaseInsensitive) == 0) {
                            txt = "";
                        }
                    }
                    else if (def == "<state>" || def == "<zip>") txt = "";
                    else if (def == "full address" || def == "mailing address" || def == "<full address>" || def == "address_box" || idStr.toLower() == "address_box" || def.contains("full contact name") || def.contains("address line")) {
                        QString curName = m_addrName.isEmpty() && !selected.isEmpty() ? selected[0].name : m_addrName;
                        QString curStreet = m_addrStreet.isEmpty() && !selected.isEmpty() ? selected[0].address : m_addrStreet;
                        QString curCity = m_addrCity.isEmpty() && !selected.isEmpty() ? selected[0].city : m_addrCity;
                        QString curZip = !selected.isEmpty() ? selected[0].zip : "";
                        QString curCountry = m_addrCountry.isEmpty() && !selected.isEmpty() ? selected[0].country : m_addrCountry;
                        if (m_previewingEnvelope) {
                            curName = formatNameOrCity(curName);
                            curStreet = formatAddress(curStreet);
                            curCity = formatCity(curCity);
                            curCountry = formatCountry(curCountry);
                        }
                        if (!curZip.isEmpty() && m_addrCity.isEmpty()) {
                            if (!curCity.isEmpty()) curCity += " " + curZip;
                            else curCity = curZip;
                        }
                        QString constructed = curName;
                        if (!curStreet.isEmpty()) constructed += (constructed.isEmpty() ? "" : "\n") + curStreet;
                        if (!curCity.isEmpty()) constructed += (constructed.isEmpty() ? "" : "\n") + curCity;
                        if (!(m_previewingEnvelope && srSub == "D") && !curCountry.isEmpty() && curCountry.toLower() != "united states" && curCountry.toLower() != "usa") {
                            constructed += (constructed.isEmpty() ? QString("") : QString("\n")) + curCountry;
                        }
                        // Strict rule: no dummy fallbacks or substitutions without explicit OK
                        txt = constructed.trimmed();
                    }
                    else if (def.contains("day") || def.contains("date")) {
                        txt = qDay.isEmpty() ? qso.date : QString("%1").arg(qDay.toInt(), 2, 10, QChar('0'));
                    }
                    else if (def.contains("mo") || def.contains("month")) {
                        txt = qMonth.isEmpty() ? qso.date : QString("%1").arg(qMonth.toInt(), 2, 10, QChar('0'));
                    }
                    else if (def.contains("yr") || def.contains("year")) txt = qYear.isEmpty() ? qso.date : qYear;
                    else if (def.contains("utc") || def.contains("time")) txt = qTime;
                    else if (def.contains("freq") || def.contains("mhz") || def.contains("band")) {
                        txt = m_showFreq ? ((!qso.freq.isEmpty()) ? qso.freq : qso.band) : qso.band;
                    }
                    else if (def.contains("mode")) txt = formatMode(qso.mode);
                    else if (def.contains("watt") || def.contains("pwr")) txt = getTweakPwr(qso);
                    else if (def.contains("rst") || def.contains("report")) txt = def.contains("rcvd") ? qso.rst_rcvd : qso.rst_sent;
                    else txt = m_dataOnly ? "" : def;
                    
                    if (m_dataOnly && txt.isEmpty()) {
                        currentX += cw[c];
                        continue;
                    }
                    
                    if (!txt.isEmpty()) {
                        qreal baseSize = font.pointSizeF() > 0 ? font.pointSizeF() : (font.pixelSize() > 0 ? font.pixelSize() * 0.75 : 10.0);
                        QFont itemFont = font;
                        itemFont.setPointSizeF(qMax(1.0, baseSize + fontOffset));
                        
                        bool isCallsignCol = (def.contains("call") || def.contains("callsign") || def.contains("<call>"));
                        bool isQrpSub = (def == "<qrp>" || def == "qrp" || def.startsWith("<qrp>") ||
                                         ((def == "<tone>" || def.startsWith("<tone>") || def == "tone" ||
                                           def == "<tone_sent>" || def.startsWith("<tone_sent>") ||
                                           def == "<tone_rcvd>" || def.startsWith("<tone_rcvd>")) && 
                                          txt == qrpSubstitutionValue));
                        bool isPotaCol = (def.contains("pota") || def.contains("park") || def.contains("ref"));
                        bool isCommentCol = (def.contains("comment") || def.contains("msg") || def.contains("note") || def.contains("qslmsg"));
                        bool isAddressCol = (def.contains("address") || def.contains("addr") || def.contains("name") || def.contains("city"));
                        bool isLongCol = isCallsignCol || isPotaCol || isCommentCol || isAddressCol;
                        bool autoShrink = isQrpSub || isLongCol;
                        bool onlyShrink = isLongCol && !isQrpSub;

                        double shrunkPercent = 1.0;
                        addTextOnPathToScene(m_scene, rowItem, txt, QRectF(currentX, 0, cw[c], subRowH), itemFont, color, Qt::AlignCenter, autoShrink, onlyShrink, &shrunkPercent);
                        if (autoShrink && shrunkPercent < 0.85) {
                            m_detectedClippingFields.append(qMakePair(def, shrunkPercent));
                            m_clippingFieldTexts[def] = txt;
                            m_clippingFieldFonts[def] = itemFont;
                        }
                    }
                    currentX += cw[c];
                }
                currentY += rh[r];
                if (!isHeaderRow) {
                    contactIdx++;
                    globalContactIdx++;
                }
            }
        } else {
            // General text block
            QString idStr = m["id"].toString();
            QString textStr = m["cellData"].toStringList().isEmpty() ? m["text"].toString() : m["cellData"].toStringList().first();
            
            QString txt = textStr;
            if (txt.isEmpty() && !idStr.isEmpty()) txt = idStr;
            
            QString lowerId = idStr.toLower();
            QString lowerTextStr = textStr.toLower();
            
            auto isKeyword = [&](const QStringList& keywords) {
                return keywords.contains(lowerId) || keywords.contains(lowerTextStr);
            };
            
            bool isCommentBox = isKeyword({"comment", "<comment>", "commentbox", "comment box", "newblock", "qslmsg", "<qslmsg>", "notes", "<notes>", "message", "<message>", "msg", "<msg>"}) ||
                                lowerId.contains("comment") || lowerTextStr.contains("comment") ||
                                lowerId.contains("commentbox") || lowerTextStr.contains("commentbox") ||
                                lowerId.contains("newblock") || lowerTextStr.contains("newblock") ||
                                lowerId.contains("qslmsg") || lowerTextStr.contains("qslmsg") ||
                                lowerId.contains("notes") || lowerTextStr.contains("notes") ||
                                lowerId.contains("message") || lowerTextStr.contains("message") ||
                                lowerId.contains("msg") || lowerTextStr.contains("msg");

            // Gather address components from local cache, QRZ XML lookup, or logfile without dummy substitutions
            QString curName = m_addrName;
            if (curName.isEmpty() && !selected.isEmpty()) curName = selected[0].name;
            if (curName.isEmpty() && !m_qsos.isEmpty()) curName = m_qsos[0].name;

            QString curStreet = m_addrStreet;
            if (curStreet.isEmpty() && !selected.isEmpty()) curStreet = selected[0].address;
            if (curStreet.isEmpty() && !m_qsos.isEmpty()) curStreet = m_qsos[0].address;

            QString curCity = m_addrCity;
            if (curCity.isEmpty() && !selected.isEmpty()) curCity = selected[0].city;
            if (curCity.isEmpty() && !m_qsos.isEmpty()) {
                curCity = m_qsos[0].city;
                if (!m_qsos[0].state.isEmpty()) curCity += (!curCity.isEmpty() ? ", " : "") + m_qsos[0].state;
                if (!m_qsos[0].zip.isEmpty()) curCity += (!curCity.isEmpty() ? " " : "") + m_qsos[0].zip;
            }

            QString curZip = !selected.isEmpty() ? selected[0].zip : (!m_qsos.isEmpty() ? m_qsos[0].zip : "");
            QString curCountry = m_addrCountry;
            if (curCountry.isEmpty() && !selected.isEmpty()) curCountry = selected[0].country;
            if (curCountry.isEmpty() && !m_qsos.isEmpty()) curCountry = m_qsos[0].country;
            if (m_previewingEnvelope) {
                curName = formatNameOrCity(curName);
                curStreet = formatAddress(curStreet);
                curCity = formatCity(curCity);
                curCountry = formatCountry(curCountry);
            }
            if (!curZip.isEmpty() && m_addrCity.isEmpty()) {
                if (!curCity.isEmpty() && !curCity.contains(curZip)) curCity += " " + curZip;
                else if (curCity.isEmpty()) curCity = curZip;
            }
            
            // Check if template possesses a separate dedicated Country element (e.g. [<COUNTRY>] in 'qsl culture')
            bool templateHasSeparateCountryBox = false;
            for (const QVariantMap &itemMap : m_templateItems) {
                QString itemText = itemMap["cellData"].toStringList().isEmpty() ? itemMap["text"].toString() : itemMap["cellData"].toStringList().first();
                QString itemTag = itemMap["id"].toString().toLower().trimmed();
                QString itemTextLower = itemText.toLower().trimmed();
                if (itemTag == "country" || itemTag == "<country>" || itemTextLower == "country" || itemTextLower == "<country>") {
                    templateHasSeparateCountryBox = true;
                    break;
                }
            }

            QStringList addrLines;
            if (!curName.trimmed().isEmpty()) addrLines << curName.trimmed();
            if (!curStreet.trimmed().isEmpty()) addrLines << curStreet.trimmed();
            if (!curCity.trimmed().isEmpty()) addrLines << curCity.trimmed();
            if (!templateHasSeparateCountryBox && !(m_previewingEnvelope && srSub == "D") &&
                !curCountry.trimmed().isEmpty() && curCountry.toLower() != "united states" && curCountry.toLower() != "usa") {
                addrLines << curCountry.trimmed();
            }
            QString constructed = addrLines.join("\n");

            // Strict Address Box Generation & Rendering (No Workarounds / No Dummy Fallbacks)
            bool isAddressBox = (m["itemType"].toInt() == 2) || 
                                isKeyword({"full address", "mailing address", "<full address>", "address_box", "address box", "addressbox", "<address_box>", "<addressbox>"}) || 
                                lowerId == "address_box" || lowerId == "address box" || lowerId.contains("address_box") || 
                                lowerTextStr.contains("full contact name") || lowerTextStr.contains("address line");

            if (isAddressBox) {
                // Set txt strictly to the constructed address lines (no placeholder or callsign fallback)
                txt = constructed;
                hasEnvelopeAddress = true;
                if (txt.isEmpty()) continue; // Nothing presented to render
            }
            else if (isKeyword({"call", "callsign", "to call", "to radio", "<call>"})) txt = m_callsign.isEmpty() && !selected.isEmpty() ? selected[0].call : m_callsign;
            else if (isKeyword({"name", "<name>"})) txt = curName;
            else if (isKeyword({"addr1", "<addr1>", "addr2", "<addr2>"})) txt = curStreet;
            else if (isKeyword({"address", "addr", "<address>"})) txt = curStreet.isEmpty() ? constructed : curStreet;
            else if (isKeyword({"city", "city/state", "<city>"})) txt = curCity;
            else if (isKeyword({"country", "<country>"})) {
                txt = (m_previewingEnvelope && srSub == "D") ? "" : curCountry;
                if (txt.isEmpty()) continue; // Strict rule: do not substitute literal "Country" when empty
            }
            else if (isKeyword({"<state>"})) txt = "";
            else if (isKeyword({"<zip>"})) txt = "";
            else if (isKeyword({"date", "<qso_date>"})) txt = selected.isEmpty() ? "" : selected[0].date;
            else if (isKeyword({"time", "utc", "<time_on>", "<time_utc>"})) {
                txt = selected.isEmpty() ? "" : selected[0].time;
                if (txt.length() >= 4) txt = txt.left(4);
            }
            else if (isKeyword({"band", "mhz", "<band>", "<freq>"})) {
                if (selected.isEmpty()) {
                    txt = "";
                } else {
                    txt = m_showFreq ? ((!selected[0].freq.isEmpty()) ? selected[0].freq : selected[0].band) : selected[0].band;
                }
            }
            else if (isKeyword({"mode", "<mode>"})) {
                txt = selected.isEmpty() ? "" : formatMode(selected[0].mode);
            }
            else if (isKeyword({"<qrp>", "qrp"})) {
                txt = (!selected.isEmpty() && isQrpContact(selected[0])) ? qrpSubstitutionValue : "";
            }
            else if (isKeyword({"<tone>", "tone", "<tone_sent>", "<tone_rcvd>"})) {
                if (!selected.isEmpty() && useQrpSubstitution && isQrpContact(selected[0]) && !templateHasQrpBox) {
                    txt = qrpSubstitutionValue;
                } else if (!selected.isEmpty()) {
                    QString rst = lowerId.contains("rcvd") ? selected[0].rst_rcvd : selected[0].rst_sent;
                    if (rst.isEmpty()) rst = selected[0].rst_rcvd.isEmpty() ? selected[0].rst_sent : selected[0].rst_rcvd;
                    rst = rst.trimmed();
                    if (rst.length() == 3 && rst[0].isDigit() && rst[1].isDigit() && rst[2].isDigit() && rst[2] >= '1' && rst[2] <= '9') {
                        txt = QString(rst[2]);
                    } else {
                        txt = "";
                    }
                } else {
                    txt = "";
                }
            }
            else if (isKeyword({"<rst_sent>"})) txt = selected.isEmpty() ? "" : selected[0].rst_sent;
            else if (isKeyword({"<rst_rcvd>"})) txt = selected.isEmpty() ? "" : selected[0].rst_rcvd;
            else if (isKeyword({"<tx_pwr>"})) txt = selected.isEmpty() ? "" : getTweakPwr(selected[0]);
            else if (isKeyword({"<gridsquare>"})) txt = selected.isEmpty() ? "" : selected[0].grid;
            else if (isKeyword({"<pota_ref>", "<pota>", "pota"}) || lowerId.contains("pota string") || lowerTextStr.contains("pota string")) {
                potaRendered = true;
                QStringList parks;
                for (const QsoDetail &q : selected) {
                    if (!q.pota.isEmpty() && !parks.contains(q.pota)) {
                        parks << q.pota;
                    }
                }
                txt = parks.join(", ");
            }
            else if (isCommentBox) {
                if (m_chkComment && !m_chkComment->isChecked()) {
                    txt = "";
                } else {
                    txt = m_manualFieldData.value("<comment>").trimmed();
                }
            }
            else if (idStr.isEmpty() && textStr.isEmpty()) {
                txt = "";
            }
            else txt = (txt.isEmpty() && !m_dataOnly) ? (textStr.isEmpty() ? idStr : textStr) : txt; // Ensure template elements always render visibly

            bool isDataField = textStr.startsWith("<") && textStr.endsWith(">");
            if ((isDataField || idStr.compare(textStr, Qt::CaseInsensitive) == 0) && !isCommentBox && !isAddressBox) {
                // If it's empty, and we have manual data, override it.
                if (m_manualFieldData.contains(idStr) && !m_manualFieldData.value(idStr).isEmpty()) {
                    txt = m_manualFieldData.value(idStr);
                } else if (txt.isEmpty() || txt.trimmed().isEmpty()) {
                    // Check if it's a structural field (like blank defaults) 
                    // that hasn't been set. We register it for manual input.
                    blankFields.append(idStr);
                }
            }

            if (isCommentBox) {
                hasCommentBox = true;
                QString t = txt.trimmed();
                if (t.isEmpty() ||
                    t.compare("CommentBox", Qt::CaseInsensitive) == 0 ||
                    t.compare("Comment Box", Qt::CaseInsensitive) == 0 ||
                    t.compare("NewBlock", Qt::CaseInsensitive) == 0 ||
                    t.compare("New Block", Qt::CaseInsensitive) == 0 ||
                    t.compare("comment", Qt::CaseInsensitive) == 0 ||
                    t.compare("<comment>", Qt::CaseInsensitive) == 0 ||
                    t.compare("commentbox", Qt::CaseInsensitive) == 0 ||
                    t.compare("<commentbox>", Qt::CaseInsensitive) == 0 ||
                    t.compare("qslmsg", Qt::CaseInsensitive) == 0 ||
                    t.compare("<qslmsg>", Qt::CaseInsensitive) == 0 ||
                    t.compare("notes", Qt::CaseInsensitive) == 0 ||
                    t.compare("<notes>", Qt::CaseInsensitive) == 0 ||
                    t.compare("message", Qt::CaseInsensitive) == 0 ||
                    t.compare("<message>", Qt::CaseInsensitive) == 0 ||
                    t.compare("msg", Qt::CaseInsensitive) == 0 ||
                    t.compare("<msg>", Qt::CaseInsensitive) == 0) {
                    continue; // Skip rendering/printing entirely!
                }
            }

            if (txt.isEmpty() || txt.trimmed().isEmpty()) {
                if (m_dataOnly) continue; // Skip rendering if no data
            }

            QGraphicsRectItem *rowItem = new QGraphicsRectItem(0, 0, w, h, area);
            rowItem->setPen(Qt::NoPen);

            qreal defX = 0;
            qreal defY = 0;
            int fontOffset = 0;
            if (!m_currentTemplate.isEmpty()) {
                QString key = QString("QslTemplateOverrides/%1/Pos_%2_%3").arg(m_currentTemplate).arg(idStr).arg(-1);
                if (settings.contains(key + "_x")) defX = settings.value(key + "_x").toDouble();
                if (settings.contains(key + "_y")) defY = settings.value(key + "_y").toDouble();
                if (settings.contains(key + "_fontOffset")) fontOffset = settings.value(key + "_fontOffset").toInt();
            }
            rowItem->setPos(defX, defY);

            int maxLines = m["maxLines"].toInt();
            qreal baseSize = font.pointSizeF() > 0 ? font.pointSizeF() : (font.pixelSize() > 0 ? font.pixelSize() * 0.75 : 10.0);
            
            bool isEnvField = lowerId.contains("address") || lowerTextStr.contains("address") ||
                              lowerId.contains("addr") || lowerTextStr.contains("addr") ||
                              lowerId.contains("name") || lowerTextStr.contains("name") ||
                              isKeyword({"city", "<state>", "<zip>"}) ||
                              lowerId.contains("country") || lowerTextStr.contains("country");
            
            bool hasHumanFont = false;
            if (!m_currentTemplate.isEmpty()) {
                if (isKeyword({"call", "callsign", "<call>"})) {
                    hasHumanFont = !settings.value(QString("QslTemplateOverrides/%1/CallFont").arg(m_currentTemplate)).toString().isEmpty();
                } else if (isKeyword({"<pota_ref>", "<pota>", "pota"}) || lowerId.contains("pota string") || lowerTextStr.contains("pota string")) {
                    hasHumanFont = !settings.value(QString("QslTemplateOverrides/%1/PotaFont").arg(m_currentTemplate)).toString().isEmpty();
                } else if (isEnvField) {
                    hasHumanFont = !settings.value(QString("QslTemplateOverrides/%1/AddressFont").arg(m_currentTemplate)).toString().isEmpty();
                } else if (isCommentBox) {
                    hasHumanFont = !settings.value(QString("QslTemplateOverrides/%1/CommentFont").arg(m_currentTemplate)).toString().isEmpty();
                }
            }

            if (maxLines > 0 && !hasHumanFont) {
                font.setPixelSize(qMax(6, (int)(h / maxLines)));
                baseSize = font.pixelSize() * 0.75;
            }
            font.setPointSizeF(qMax(1.0, baseSize + fontOffset));
            
            if (isEnvField) hasEnvelopeAddress = true;
            int overallAlign = m.contains("align") ? m["align"].toInt() : Qt::AlignCenter;
            if (isCommentBox) {
                bool hasCustomCommentFont = false;
                if (!m_currentTemplate.isEmpty()) {
                    hasCustomCommentFont = !settings.value(QString("QslTemplateOverrides/%1/CommentFont").arg(m_currentTemplate)).toString().isEmpty();
                }
                if (hasCustomCommentFont) {
                    QString alignStr = m_manualFieldData.value("<comment_align>").trimmed();
                    if (!alignStr.isEmpty()) {
                        if (alignStr.compare("L Justify", Qt::CaseInsensitive) == 0 || alignStr.compare("Left", Qt::CaseInsensitive) == 0) {
                            overallAlign = Qt::AlignLeft | Qt::AlignVCenter;
                        } else if (alignStr.compare("R Justify", Qt::CaseInsensitive) == 0 || alignStr.compare("Right", Qt::CaseInsensitive) == 0) {
                            overallAlign = Qt::AlignRight | Qt::AlignVCenter;
                        } else if (alignStr.compare("Center", Qt::CaseInsensitive) == 0) {
                            overallAlign = Qt::AlignCenter;
                        }
                    }
                }
            }
            bool isCallsignField = isKeyword({"call", "callsign", "to call", "to radio", "<call>"});
            if (isCallsignField) hasCallsignField = true;
            bool isQrpSub = (isKeyword({"<qrp>", "qrp"}) || 
                             (isKeyword({"<tone>", "tone", "<tone_sent>", "<tone_rcvd>"}) && 
                              txt == qrpSubstitutionValue));
            bool isPotaField = isKeyword({"<pota_ref>", "<pota>", "pota"}) || lowerId.contains("pota string") || lowerTextStr.contains("pota string");
            bool isAddressOrNameField = isEnvField || isKeyword({"name", "<name>", "address", "addr", "<address>", "<addr1>", "addr1", "<addr2>", "addr2", "city", "city/state", "<city>", "country", "<country>", "full address", "mailing address", "<full address>", "address_box"});
            
            bool autoShrink = isQrpSub || isCallsignField || isAddressOrNameField || isPotaField || isCommentBox;
            bool onlyShrink = (isCallsignField || isAddressOrNameField || isPotaField || isCommentBox) && !isQrpSub;

            QString fieldId = idStr.isEmpty() ? textStr : idStr;
            if (m_hasPosOverride && fieldId == m_overrideFieldId) {
                rowItem->setPos(m_overrideX, m_overrideY);
            }
            if (m_hasTextOverride && fieldId == m_overrideFieldId) {
                txt = m_textOverride;
            }
            QFont renderFont = font;
            if (m_hasFontOverride && fieldId == m_overrideFieldId) {
                renderFont = m_fontOverride;
            }

            // Country element overrides (configured via PrintExceptionDialog target element selector)
            bool isCountryField = (!m_countryFieldId.isEmpty() && (fieldId == m_countryFieldId || fieldId.compare(m_countryFieldId, Qt::CaseInsensitive) == 0)) ||
                                  isKeyword({"country", "<country>"}) || 
                                  lowerId == "country" || lowerId == "<country>";
            if (m_hasCountryPosOverride && isCountryField) {
                rowItem->setPos(m_countryOverrideX, m_countryOverrideY);
            }
            if (m_hasCountryTextOverride && isCountryField) {
                txt = m_countryTextOverride;
            }
            if (m_hasCountryFontOverride && isCountryField) {
                renderFont = m_countryFontOverride;
            }

            if (m_forceDisableAutoShrink) {
                autoShrink = false;
            }

            double shrunkPercent = 1.0;
            addTextOnPathToScene(m_scene, rowItem, txt, QRectF(0, 0, w, h), renderFont, color, overallAlign, autoShrink, onlyShrink, &shrunkPercent);
            if (autoShrink && shrunkPercent < 0.85) {
                m_detectedClippingFields.append(qMakePair(fieldId, shrunkPercent));
                m_clippingFieldTexts[fieldId] = txt;
                m_clippingFieldFonts[fieldId] = renderFont;
            }
        }
    }
    
    // POTA footer logic removed
    
    if (m_btnAddressFont) m_btnAddressFont->setVisible(hasEnvelopeAddress);
    if (m_btnCommentFont) m_btnCommentFont->setVisible(hasCommentBox);
    if (m_btnCallFont) m_btnCallFont->setVisible(hasCallsignField);
    if (m_btnFreqBandSwitch) m_btnFreqBandSwitch->setEnabled(templateHasFreqOrBand());

    if (m_chkComment) {
        m_chkOptionsGroup->show();
    }

    // We no longer build dynamic buttons for blank fields
    // The m_blankFieldsWidget (which now contains the Comment Editor) is statically shown.
    
    m_scene->setSceneRect(0, 0, m_paperW, m_paperH);
}

void QslPreviewDialog::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    if (m_scene && m_view && event->size() != event->oldSize()) {
        QTimer::singleShot(10, this, [this]() {
            if (m_scene && m_view && !m_scene->sceneRect().isEmpty()) {
                m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
            }
        });
    }
    if (m_btnHelp && m_printingPrefsGroup) {
        int targetX = m_printingPrefsGroup->width() - m_btnHelp->width() - 8;
        int targetY = 2;
        if (m_btnHelp->x() != targetX || m_btnHelp->y() != targetY) {
            m_btnHelp->move(targetX, targetY);
        }
    }
}

bool QslPreviewDialog::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_printingPrefsGroup && event->type() == QEvent::Resize) {
        if (m_btnHelp) {
            int targetX = m_printingPrefsGroup->width() - m_btnHelp->width() - 8;
            int targetY = 2;
            if (m_btnHelp->x() != targetX || m_btnHelp->y() != targetY) {
                m_btnHelp->move(targetX, targetY);
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

void QslPreviewDialog::closeEvent(QCloseEvent *event) {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.setValue("PrintManager/Geometry", saveGeometry());
    QDialog::closeEvent(event);
}



void QslPreviewDialog::changeAddressFont() {
    if (m_currentTemplate.isEmpty()) return;
    
    QFont f("Arial", 14);
    QColor c(Qt::black);
    for (const QVariantMap &m : m_templateItems) {
        QString idStr = m["id"].toString().toLower();
        if (idStr == "address" || idStr.contains("addr") || idStr == "name" || idStr == "city" || idStr == "country" || idStr == "<state>" || idStr == "<zip>" || idStr == "full address" || idStr == "mailing address" || idStr == "<full address>") {
            f = QFont(m["font"].toString(), qMax(6, m["size"].toInt()));
            c = QColor(m["color"].toString());
            break;
        }
    }
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString cf = settings.value(QString("QslTemplateOverrides/%1/AddressFont").arg(m_currentTemplate)).toString();
    QString cc = settings.value(QString("QslTemplateOverrides/%1/AddressColor").arg(m_currentTemplate)).toString();
    if (!cf.isEmpty()) f.fromString(cf);
    if (!cc.isEmpty()) c = QColor(cc);

    saveStateForUndo();
    bool changed = false;

    StyleDialog dlg(f, c, this);
    auto applyFn = [this, &changed](const QFont &newF, const QColor &newC) {
        changed = true;
        QSettings s(QCoreApplication::organizationName(), "Qwain");
        s.setValue(QString("QslTemplateOverrides/%1/AddressFont").arg(m_currentTemplate), newF.toString());
        s.setValue(QString("QslTemplateOverrides/%1/AddressColor").arg(m_currentTemplate), newC.name());
        renderTemplate();
    };
    dlg.exec();
    
    if (!changed) m_undoStack.takeLast();
}



void QslPreviewDialog::changeCommentFont() {
    if (m_currentTemplate.isEmpty()) return;
    
    QFont f("Arial", 10);
    QColor c(Qt::black);
    double rot = 0.0;
    
    for (const QVariantMap &m : m_templateItems) {
        QString idStr = m["id"].toString().toLower();
        if (idStr.contains("comment") || idStr.contains("newblock") || idStr.contains("qslmsg") || idStr.contains("notes") || idStr.contains("message") || idStr.contains("msg")) {
            f = QFont(m["font"].toString(), qMax(6, m["size"].toInt()));
            c = QColor(m["color"].toString());
            rot = m["rot"].toDouble();
            break;
        }
    }
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString cf = settings.value(QString("QslTemplateOverrides/%1/CommentFont").arg(m_currentTemplate)).toString();
    if (!cf.isEmpty()) f.fromString(cf);
    QString cc = settings.value(QString("QslTemplateOverrides/%1/CommentColor").arg(m_currentTemplate)).toString();
    if (!cc.isEmpty()) c = QColor(cc);
    if (settings.contains(QString("QslTemplateOverrides/%1/CommentRotation").arg(m_currentTemplate))) {
        rot = settings.value(QString("QslTemplateOverrides/%1/CommentRotation").arg(m_currentTemplate)).toDouble();
    }

    saveStateForUndo();
    bool changed = false;

    StyleDialog dlg(f, c, this, false, true, rot);
    auto applyFn = [this, &changed](const QFont &rf, const QColor &rc, double rRot) {
        changed = true;
        QSettings s(QCoreApplication::organizationName(), "Qwain");
        s.setValue(QString("QslTemplateOverrides/%1/CommentFont").arg(m_currentTemplate), rf.toString());
        s.setValue(QString("QslTemplateOverrides/%1/CommentColor").arg(m_currentTemplate), rc.name());
        s.setValue(QString("QslTemplateOverrides/%1/CommentRotation").arg(m_currentTemplate), rRot);
        renderTemplate();
    };

    connect(&dlg, &StyleDialog::styleChangedWithRotation, this, applyFn);

    if (dlg.exec() == QDialog::Accepted) {
        applyFn(dlg.selectedFont(), dlg.selectedColor(), dlg.selectedRotation());
    } else if (changed) {
        undo(); // Rolled back cancelled changes!
    } else {
        m_undoStack.takeLast();
    }
}



void QslPreviewDialog::changeCallFont() {
    if (m_currentTemplate.isEmpty()) return;
    
    QFont f("Arial", 24);
    QColor c(Qt::black);
    for (const QVariantMap &m : m_templateItems) {
        QString idStr = m["id"].toString().toLower();
        if (idStr == "call" || idStr == "callsign" || idStr == "to call" || idStr == "to radio" || idStr == "<call>") {
            f = QFont(m["font"].toString(), qMax(6, m["size"].toInt()));
            c = QColor(m["color"].toString());
            break;
        }
    }
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString cf = settings.value(QString("QslTemplateOverrides/%1/CallFont").arg(m_currentTemplate)).toString();
    QString cc = settings.value(QString("QslTemplateOverrides/%1/CallColor").arg(m_currentTemplate)).toString();
    if (!cf.isEmpty()) f.fromString(cf);
    if (!cc.isEmpty()) c = QColor(cc);

    saveStateForUndo();
    bool changed = false;

    StyleDialog dlg(f, c, this, false, false, 0.0, true);
    auto applyFn = [this, &changed](const QFont &newF, const QColor &newC) {
        changed = true;
        QSettings s(QCoreApplication::organizationName(), "Qwain");
        s.setValue(QString("QslTemplateOverrides/%1/CallFont").arg(m_currentTemplate), newF.toString());
        s.setValue(QString("QslTemplateOverrides/%1/CallColor").arg(m_currentTemplate), newC.name());
        renderTemplate();
    };

    connect(&dlg, &StyleDialog::styleChanged, this, applyFn);
    
    int result = dlg.exec();
    if (result == QDialog::Accepted) {
        applyFn(dlg.selectedFont(), dlg.selectedColor());
    } else if (result == 2) { // OK/Save to Template
        // Apply local preview instantly
        applyFn(dlg.selectedFont(), dlg.selectedColor());
        
        // Write directly to the template configuration in QSettings
        QSettings s(QCoreApplication::organizationName(), "Qwain");
        s.beginGroup("QslTemplates");
        s.beginGroup(m_currentTemplate);
        
        for (int i = 0; i < m_templateItems.size(); ++i) {
            QString idStr = m_templateItems[i]["id"].toString().toLower();
            if (idStr == "call" || idStr == "callsign" || idStr == "to call" || idStr == "to radio" || idStr == "<call>") {
                // Update active in-memory structure
                m_templateItems[i]["font"] = dlg.selectedFont().family();
                m_templateItems[i]["size"] = dlg.selectedFont().pointSize() > 0 ? dlg.selectedFont().pointSize() : 12;
                m_templateItems[i]["color"] = dlg.selectedColor().name();
                
                // Write permanently to settings
                s.beginGroup(QString("Area_%1").arg(i));
                s.setValue("font", dlg.selectedFont().family());
                s.setValue("size", dlg.selectedFont().pointSize() > 0 ? dlg.selectedFont().pointSize() : 12);
                s.setValue("color", dlg.selectedColor().name());
                s.endGroup();
                break;
            }
        }
        s.endGroup();
        s.endGroup();
        
        renderTemplate();
    } else if (changed) {
        undo(); // roll back cancelled changes!
    } else {
        m_undoStack.takeLast();
    }
}


class PrinterAnimationWidget : public QWidget {
public:
    PrinterAnimationWidget(QWidget *parent = nullptr) 
        : QWidget(parent) {
        setFixedSize(300, 250);
        m_yOffset = 250;
        
        QTimer *t = new QTimer(this);
        connect(t, &QTimer::timeout, this, [this]() {
            m_yOffset -= 3;
            if (m_yOffset < 40) m_yOffset = 250;
            update();
        });
        t->start(30);
    }

    PrinterAnimationWidget(const QPixmap &cardPix, QWidget *parent = nullptr) 
        : PrinterAnimationWidget(parent) {
        m_cardPix = cardPix;
    }

    void setPixmap(const QPixmap &pix) {
        m_cardPix = pix;
        m_yOffset = 250;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        
        // Printer Icon at Top
        p.setBrush(Qt::darkGray);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(50, 10, 200, 80, 10, 10);
        
        // slot entry back
        p.setBrush(QColor(20, 20, 20));
        p.drawRect(70, 80, 160, 10);
        p.setPen(QPen(Qt::black, 2));
        p.drawLine(70, 80, 230, 80);
        
        // card
        p.save();
        p.setClipRect(50, 80, 200, 200);
        
        p.translate(150, m_yOffset);
        
        bool rotate270 = (m_cardPix.width() > m_cardPix.height());
        QPixmap scaledCard;
        if (!m_cardPix.isNull()) {
            if (rotate270) {
                p.rotate(270.0);
                scaledCard = m_cardPix.scaled(180, 115, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            } else {
                p.rotate(180.0);
                scaledCard = m_cardPix.scaled(115, 180, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            }
        } else {
            scaledCard = QPixmap(180, 115);
            scaledCard.fill(Qt::white);
        }
        
        p.setBrush(Qt::white);
        p.setPen(QPen(Qt::lightGray, 1));
        p.drawRect(-scaledCard.width()/2, -scaledCard.height()/2, scaledCard.width(), scaledCard.height());
        p.drawPixmap(-scaledCard.width()/2, -scaledCard.height()/2, scaledCard);
        
        p.restore();
        
        // front lip of the slot
        p.setBrush(Qt::gray);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(60, 80, 180, 12, 4, 4);
        p.setBrush(QColor(20, 20, 20));
        p.drawRect(70, 80, 160, 4);
    }

private:
    QPixmap m_cardPix;
    int m_yOffset = 250;
};

class ActivePrintAnimationWidget : public QWidget {
    Q_OBJECT
public:
    ActivePrintAnimationWidget(QWidget *parent = nullptr) 
        : QWidget(parent) 
    {
        setFixedSize(380, 240);
        m_animTimer = new QTimer(this);
        connect(m_animTimer, &QTimer::timeout, this, &ActivePrintAnimationWidget::onTick);
        m_animTimer->start(30);
    }

    void setMedia(const QPixmap &renderedPix, bool isEnvelope) {
        m_renderedPix = renderedPix;
        m_isEnvelope = isEnvelope;
        m_progress = 0.0f;
        update();
    }

    void start() {
        if (!m_animTimer->isActive()) m_animTimer->start(30);
    }

    void stop() {
        m_animTimer->stop();
    }

private slots:
    void onTick() {
        m_progress += 0.012f;
        if (m_progress > 1.25f) {
            m_progress = -0.15f; // Small pause before next card feeds in
        }
        m_headPhase += 0.15f;
        m_ledPhase += 0.10f;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        int widgetW = width();
        int widgetH = height();

        // Dimensions of the printer chassis
        int printerW = 280;
        int printerH = 84;
        int printerX = (widgetW - printerW) / 2; // 50
        int printerY = 78;

        // Slot positions
        int slotW = 190;
        int slotX = (widgetW - slotW) / 2; // 95
        int topSlotY = printerY;           // 78
        int bottomSlotY = printerY + printerH - 6; // 156

        // Card dimensions (scaled for display)
        int cardW = 160;
        int cardH = 95;
        if (m_isEnvelope) {
            cardW = 170;
            cardH = 88;
        }

        // Travel displacement
        float normProg = qBound(0.0f, m_progress, 1.0f);
        int inputCardCenterY = (int)(25 + normProg * 90);
        int outputCardCenterY = (int)(115 + normProg * 90);

        // 1. REAR PAPER FEED GUIDE & INPUT TRAY (behind paper)
        p.setBrush(QColor(42, 45, 54));
        p.setPen(QPen(QColor(60, 64, 76), 1.5));
        QPolygon inputGuide;
        inputGuide << QPoint(slotX - 10, topSlotY)
                   << QPoint(slotX + 5, 12)
                   << QPoint(slotX + slotW - 5, 12)
                   << QPoint(slotX + slotW + 10, topSlotY);
        p.drawPolygon(inputGuide);

        // Subtle guide ruler ticks on input tray
        p.setPen(QColor(80, 85, 100));
        for (int gx = slotX + 15; gx <= slotX + slotW - 15; gx += 15) {
            p.drawLine(gx, 15, gx, 22);
        }

        // 2. THE BARE TEMPLATE CARD (FEEDING IN AT TOP)
        p.save();
        p.setClipRect(slotX - 10, 8, slotW + 20, topSlotY - 6);
        
        int inX = (widgetW - cardW) / 2;
        int inY = inputCardCenterY - cardH / 2;
        QRect inRect(inX, inY, cardW, cardH);

        // Draw bare stock
        p.setBrush(Qt::white);
        p.setPen(QPen(QColor(180, 185, 195), 1));
        p.drawRoundedRect(inRect, 3, 3);

        // Draw bare template styling
        p.setPen(QPen(QColor(220, 224, 230), 1, Qt::DotLine));
        p.drawRect(inRect.adjusted(4, 4, -4, -4));

        if (!m_isEnvelope) {
            // Bare QSL card placeholder lines
            p.setBrush(QColor(245, 247, 250));
            p.setPen(QPen(QColor(200, 205, 215), 1));
            p.drawRoundedRect(inRect.left() + 10, inRect.top() + 8, cardW - 20, 18, 2, 2);
            
            p.setPen(QColor(205, 210, 220));
            QFont f = p.font();
            f.setPointSize(7);
            f.setBold(true);
            p.setFont(f);
            p.drawText(QRect(inRect.left() + 12, inRect.top() + 8, cardW - 24, 18), Qt::AlignCenter, "BLANK CARD STOCK");

            p.setPen(QPen(QColor(225, 228, 235), 1));
            int gridTop = inRect.top() + 32;
            int gridH = 34;
            p.drawRect(inRect.left() + 8, gridTop, cardW - 16, gridH);
            p.drawLine(inRect.left() + 8, gridTop + 11, inRect.right() - 8, gridTop + 11);
            p.drawLine(inRect.left() + 8, gridTop + 22, inRect.right() - 8, gridTop + 22);
            p.drawLine(inRect.left() + 45, gridTop, inRect.left() + 45, gridTop + gridH);
            p.drawLine(inRect.left() + 85, gridTop, inRect.left() + 85, gridTop + gridH);
            p.drawLine(inRect.left() + 120, gridTop, inRect.left() + 120, gridTop + gridH);

            p.drawLine(inRect.left() + 12, inRect.bottom() - 10, inRect.right() - 12, inRect.bottom() - 10);
        } else {
            // Bare Envelope placeholder lines
            p.setBrush(QColor(245, 247, 250));
            p.setPen(QPen(QColor(200, 205, 215), 1));
            p.drawRect(inRect.right() - 24, inRect.top() + 8, 16, 20);

            p.setPen(QPen(QColor(220, 224, 230), 1));
            p.drawLine(inRect.left() + 10, inRect.top() + 10, inRect.left() + 60, inRect.top() + 10);
            p.drawLine(inRect.left() + 10, inRect.top() + 16, inRect.left() + 50, inRect.top() + 16);

            p.drawLine(inRect.left() + 45, inRect.top() + 42, inRect.right() - 25, inRect.top() + 42);
            p.drawLine(inRect.left() + 45, inRect.top() + 50, inRect.right() - 35, inRect.top() + 50);
            p.drawLine(inRect.left() + 45, inRect.top() + 58, inRect.right() - 45, inRect.top() + 58);

            p.setPen(QColor(205, 210, 220));
            QFont f = p.font();
            f.setPointSize(7);
            f.setBold(true);
            p.setFont(f);
            p.drawText(QRect(inRect.left() + 10, inRect.bottom() - 20, cardW - 20, 14), Qt::AlignCenter, "BLANK ENVELOPE");
        }
        p.restore();

        // 3. OUTPUT CATCH TRAY (behind output card)
        p.setBrush(QColor(38, 41, 50, 200));
        p.setPen(QPen(QColor(55, 59, 70), 1.5));
        p.drawRoundedRect(slotX - 12, bottomSlotY + 2, slotW + 24, 74, 5, 5);
        p.setPen(QColor(70, 75, 88));
        p.drawLine(slotX + 10, bottomSlotY + 70, slotX + slotW - 10, bottomSlotY + 70);

        // 4. THE RENDERED CARD (EMERGING AT BOTTOM)
        p.save();
        p.setClipRect(slotX - 10, bottomSlotY + 2, slotW + 20, 80);

        int outX = (widgetW - cardW) / 2;
        int outY = outputCardCenterY - cardH / 2;
        QRect outRect(outX, outY, cardW, cardH);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 60));
        p.drawRoundedRect(outRect.adjusted(2, 2, 2, 2), 3, 3);

        if (!m_renderedPix.isNull()) {
            QPixmap scaled = m_renderedPix.scaled(cardW, cardH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            p.drawPixmap(outX, outY, scaled);
            p.setPen(QPen(QColor(180, 185, 195, 180), 1));
            p.setBrush(Qt::NoBrush);
            p.drawRect(outRect);
        } else {
            p.setBrush(Qt::white);
            p.setPen(QPen(Qt::darkGray, 1));
            p.drawRect(outRect);
        }
        p.restore();

        // 5. PRINTER CHASSIS / BODY
        QLinearGradient bodyGrad(printerX, printerY, printerX, printerY + printerH);
        bodyGrad.setColorAt(0.0, QColor(56, 60, 72));
        bodyGrad.setColorAt(0.4, QColor(42, 45, 55));
        bodyGrad.setColorAt(1.0, QColor(28, 30, 38));
        p.setBrush(bodyGrad);
        p.setPen(QPen(QColor(75, 80, 95), 1.5));
        p.drawRoundedRect(printerX, printerY, printerW, printerH, 8, 8);

        p.setPen(QPen(QColor(115, 122, 140), 1));
        p.drawLine(printerX + 8, printerY + 1, printerX + printerW - 8, printerY + 1);

        p.setBrush(QColor(12, 14, 18));
        p.setPen(QPen(QColor(22, 24, 30), 1));
        p.drawRoundedRect(slotX, topSlotY - 2, slotW, 8, 3, 3);

        p.setBrush(QColor(12, 14, 18));
        p.setPen(QPen(QColor(22, 24, 30), 1));
        p.drawRoundedRect(slotX, bottomSlotY, slotW, 8, 3, 3);

        int viewW = slotW - 20;
        int viewH = 24;
        int viewX = (widgetW - viewW) / 2;
        int viewY = printerY + 28;
        p.setBrush(QColor(16, 18, 24));
        p.setPen(QPen(QColor(50, 54, 66), 1));
        p.drawRoundedRect(viewX, viewY, viewW, viewH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(90, 95, 110));
        p.drawRect(viewX + 6, viewY + 3, viewW - 12, 4);
        p.drawRect(viewX + 6, viewY + viewH - 7, viewW - 12, 4);

        p.setBrush(QColor(30, 32, 40));
        for (int rx = viewX + 20; rx < viewX + viewW - 20; rx += 30) {
            p.drawRect(rx, viewY + 2, 8, 6);
            p.drawRect(rx, viewY + viewH - 8, 8, 6);
        }

        float sweep = 0.5f + 0.5f * (float)sin(m_headPhase);
        int headX = viewX + 10 + (int)(sweep * (viewW - 44));
        
        QLinearGradient headGrad(headX, viewY + 4, headX + 24, viewY + viewH - 4);
        headGrad.setColorAt(0.0, QColor(0, 168, 232));
        headGrad.setColorAt(1.0, QColor(0, 100, 180));
        p.setBrush(headGrad);
        p.setPen(QPen(QColor(120, 220, 255), 1));
        p.drawRoundedRect(headX, viewY + 4, 24, viewH - 8, 3, 3);

        p.setBrush(QColor(0, 230, 255, 180));
        p.setPen(Qt::NoPen);
        p.drawEllipse(headX + 9, viewY + viewH / 2 - 3, 6, 6);

        int ledAlpha = (int)(160 + 95 * sin(m_ledPhase));
        p.setBrush(QColor(0, 225, 140, ledAlpha));
        p.setPen(QPen(QColor(180, 255, 220), 1));
        p.drawEllipse(printerX + 16, printerY + 36, 9, 9);
        
        p.setBrush(QColor(0, 225, 140, ledAlpha / 4));
        p.setPen(Qt::NoPen);
        p.drawEllipse(printerX + 13, printerY + 33, 15, 15);

        p.setPen(QColor(175, 185, 205));
        QFont f = p.font();
        f.setPointSize(8);
        f.setBold(true);
        p.setFont(f);
        p.drawText(printerX + 32, printerY + 44, "ACTIVE THERMAL/INK ENGINE");

        p.setPen(QColor(130, 140, 160));
        f.setPointSize(7);
        f.setBold(false);
        p.setFont(f);
        p.drawText(printerX + printerW - 75, printerY + 44, "PRINTING...");
    }

private:
    QPixmap m_renderedPix;
    bool m_isEnvelope = false;
    float m_progress = 0.0f;
    float m_headPhase = 0.0f;
    float m_ledPhase = 0.0f;
    QTimer *m_animTimer = nullptr;
};

class UnifiedPrintDialog : public QDialog {
    Q_OBJECT
public:
    enum PageIndex {
        PageOrientation = 0,
        PagePrinting = 1
    };

    explicit UnifiedPrintDialog(QWidget *parent = nullptr)
        : QDialog(parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
    {
        setWindowTitle("Print Queue Controller");
        setMinimumWidth(540);
        
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(18, 18, 18, 18);
        mainLayout->setSpacing(12);

        m_stackedWidget = new QStackedWidget(this);
        mainLayout->addWidget(m_stackedWidget);

        // ================= PAGE 0: ORIENTATION CHECK =================
        QWidget *orientationPage = new QWidget(this);
        QVBoxLayout *orientLayout = new QVBoxLayout(orientationPage);
        orientLayout->setContentsMargins(0, 0, 0, 0);
        orientLayout->setSpacing(10);

        m_lblOrientationTitle = new QLabel("<b>CHECK PRINTER ORIENTATION</b>", orientationPage);
        m_lblOrientationTitle->setAlignment(Qt::AlignCenter);
        m_lblOrientationTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #ffffff;");
        orientLayout->addWidget(m_lblOrientationTitle);

        m_lblOrientationWarning = new QLabel(orientationPage);
        m_lblOrientationWarning->setAlignment(Qt::AlignCenter);
        m_lblOrientationWarning->setWordWrap(true);
        m_lblOrientationWarning->setStyleSheet("font-size: 12px; color: #e2e8f0; margin: 4px 10px;");
        orientLayout->addWidget(m_lblOrientationWarning);

        m_orientAnim = new PrinterAnimationWidget(orientationPage);
        orientLayout->addWidget(m_orientAnim);
        orientLayout->setAlignment(m_orientAnim, Qt::AlignCenter);

        m_lblQueueStatus = new QLabel(orientationPage);
        m_lblQueueStatus->setAlignment(Qt::AlignCenter);
        m_lblQueueStatus->setStyleSheet("font-size: 12px; color: #ffb703; font-weight: bold;");
        orientLayout->addWidget(m_lblQueueStatus);

        m_lblPrinterDest = new QLabel(orientationPage);
        m_lblPrinterDest->setAlignment(Qt::AlignCenter);
        m_lblPrinterDest->setStyleSheet("font-size: 12px; color: #e2e8f0;");
        orientLayout->addWidget(m_lblPrinterDest);

        m_printerSelectionContainer = new QWidget(orientationPage);
        QHBoxLayout *printerHBox = new QHBoxLayout(m_printerSelectionContainer);
        printerHBox->setContentsMargins(0, 0, 0, 0);
        QLabel *lblSelect = new QLabel("Select Destination Printer:", m_printerSelectionContainer);
        lblSelect->setStyleSheet("font-weight: bold; font-size: 12px; color: #e2e8f0;");
        printerHBox->addWidget(lblSelect);
        m_comboPrinter = new QComboBox(m_printerSelectionContainer);
        m_comboPrinter->setMinimumWidth(260);
        printerHBox->addWidget(m_comboPrinter);
        orientLayout->addWidget(m_printerSelectionContainer);

        // Buttons for Page 0
        QHBoxLayout *orientBtnLayout = new QHBoxLayout();
        orientBtnLayout->setSpacing(15);

        m_btnCancel = new QPushButton("Cancel Print Job", orientationPage);
        m_btnCancel->setFixedHeight(40);
        m_btnCancel->setStyleSheet(
            "QPushButton {"
            "  background-color: #6c757d;"
            "  color: white;"
            "  font-weight: bold;"
            "  font-size: 13px;"
            "  border-radius: 6px;"
            "  padding: 6px 16px;"
            "}"
            "QPushButton:hover { background-color: #5a6268; }"
            "QPushButton:pressed { background-color: #4e555b; }"
        );
        orientBtnLayout->addWidget(m_btnCancel);

        m_btnProceed = new QPushButton("Proceed to Print", orientationPage);
        m_btnProceed->setFixedHeight(40);
        m_btnProceed->setStyleSheet(
            "QPushButton {"
            "  background-color: #2b9348;"
            "  color: white;"
            "  font-weight: bold;"
            "  font-size: 14px;"
            "  border-radius: 6px;"
            "  padding: 6px 20px;"
            "}"
            "QPushButton:hover { background-color: #38b000; }"
            "QPushButton:pressed { background-color: #007200; }"
        );
        orientBtnLayout->addWidget(m_btnProceed);

        orientLayout->addLayout(orientBtnLayout);
        m_stackedWidget->addWidget(orientationPage);

        // ================= PAGE 1: ACTIVE PRINTING =================
        QWidget *printingPage = new QWidget(this);
        QVBoxLayout *printLayout = new QVBoxLayout(printingPage);
        printLayout->setContentsMargins(0, 0, 0, 0);
        printLayout->setSpacing(10);

        m_lblPrintingTitle = new QLabel("<b>PRINTING IN PROGRESS</b>", printingPage);
        m_lblPrintingTitle->setAlignment(Qt::AlignCenter);
        m_lblPrintingTitle->setStyleSheet("font-size: 15px; font-weight: bold; color: #ffffff;");
        printLayout->addWidget(m_lblPrintingTitle);

        m_activePrintAnim = new ActivePrintAnimationWidget(printingPage);
        printLayout->addWidget(m_activePrintAnim);
        printLayout->setAlignment(m_activePrintAnim, Qt::AlignCenter);

        m_lblPrintingStatus = new QLabel("<b>Preparing print job...</b>", printingPage);
        m_lblPrintingStatus->setAlignment(Qt::AlignCenter);
        m_lblPrintingStatus->setStyleSheet("font-size: 13px; color: #ffb703; font-weight: bold;");
        printLayout->addWidget(m_lblPrintingStatus);

        m_progressBar = new QProgressBar(printingPage);
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
        m_progressBar->setTextVisible(true);
        m_progressBar->setFixedHeight(22);
        m_progressBar->setStyleSheet(
            "QProgressBar {"
            "  border: 1px solid #ced4da;"
            "  border-radius: 5px;"
            "  text-align: center;"
            "  background: #e9ecef;"
            "}"
            "QProgressBar::chunk {"
            "  background-color: #0077b6;"
            "  border-radius: 4px;"
            "}"
        );
        printLayout->addWidget(m_progressBar);

        m_btnAbort = new QPushButton("🚨 ABORT / PANIC PRINT", printingPage);
        m_btnAbort->setFixedHeight(46);
        m_btnAbort->setStyleSheet(
            "QPushButton {"
            "  background-color: #d90429;"
            "  color: white;"
            "  font-weight: bold;"
            "  font-size: 14px;"
            "  border-radius: 6px;"
            "}"
            "QPushButton:hover { background-color: #ef233c; }"
            "QPushButton:pressed { background-color: #b70921; }"
        );
        printLayout->addWidget(m_btnAbort);

        m_stackedWidget->addWidget(printingPage);

        // Connections
        connect(m_btnProceed, &QPushButton::clicked, this, &UnifiedPrintDialog::onProceedClicked);
        connect(m_btnCancel, &QPushButton::clicked, this, &UnifiedPrintDialog::onCancelClicked);
        connect(m_btnAbort, &QPushButton::clicked, this, &UnifiedPrintDialog::onAbortClicked);
    }

    void setupOrientation(bool isEnvelope, bool isOverflow, 
                          int currentItem, int totalItems, 
                          const QString &callsign, 
                          const QPixmap &renderedPix,
                          const QString &defaultPrinterName,
                          bool isContinuous)
    {
        m_confirmed = false;
        m_aborted = false;
        m_currentPrinterName = defaultPrinterName;
        
        QString itemText = isEnvelope ? "Envelope" : (isOverflow ? "Overflow QSL Card" : "QSL Card");
        setWindowTitle(QString("Print %1 Orientation Check").arg(itemText));
        m_lblOrientationTitle->setText(QString("<b>CHECK PRINTER ORIENTATION: %1</b>").arg(itemText.toUpper()));
        
        m_lblOrientationWarning->setText(
            QString("Make sure your %1 is inserted correctly into the printer slot.<br>"
                    "If your printer rotates the %1 or feeds from a specific side, verify before continuing.")
            .arg(isEnvelope ? "envelope" : "QSL card")
        );
        
        m_orientAnim->setPixmap(renderedPix);
        
        if (totalItems > 1) {
            m_lblQueueStatus->setText(QString("<b>Print Queue:</b> Item %1 of %2 (%3)")
                .arg(currentItem).arg(totalItems).arg(callsign));
        } else {
            m_lblQueueStatus->setText(QString("<b>Contact:</b> %1").arg(callsign));
        }
        
        if (isContinuous) {
            m_printerSelectionContainer->setVisible(false);
            m_lblPrinterDest->setText(QString("<b>Destination Printer:</b> %1")
                .arg(defaultPrinterName.isEmpty() ? "System Default" : defaultPrinterName));
            m_lblPrinterDest->setVisible(true);
        } else {
            m_lblPrinterDest->setVisible(false);
            m_printerSelectionContainer->setVisible(true);
            
            m_comboPrinter->clear();
            QSettings settings(QCoreApplication::organizationName(), "Qwain");
            QStringList knownPrinters = settings.value("KnownPrinters").toStringList();
            for (const QString &kp : knownPrinters) {
                QString dest = kp.split(" ").first().trimmed();
                m_comboPrinter->addItem(formatPrinterDisplay(kp), dest);
            }
            if (m_comboPrinter->findData("Print to PDF") < 0) {
                m_comboPrinter->addItem("Print to PDF", "Print to PDF");
            }
            
            if (!defaultPrinterName.isEmpty()) {
                int idx = m_comboPrinter->findData(defaultPrinterName);
                if (idx >= 0) {
                    m_comboPrinter->setCurrentIndex(idx);
                } else {
                    int idxText = m_comboPrinter->findText(defaultPrinterName);
                    if (idxText >= 0) m_comboPrinter->setCurrentIndex(idxText);
                }
            }
        }
        
        m_btnProceed->setText(QString("Proceed to Print %1").arg(itemText));
        m_stackedWidget->setCurrentIndex(PageOrientation);
    }

    void setPrintingActive(bool isEnvelope, int currentItem, int totalItems, 
                           const QString &callsign, const QPixmap &renderedPix)
    {
        setWindowTitle("Printing in Progress...");
        m_lblPrintingTitle->setText("<b>PRINTING IN PROGRESS</b>");
        
        QString typeStr = isEnvelope ? "Envelope" : "Card";
        int total = (totalItems > 0) ? totalItems : 1;
        m_lblPrintingStatus->setText(
            QString("Printing: <b>%1</b> (%2) — Item %3 of %4")
            .arg(callsign).arg(typeStr).arg(currentItem).arg(total)
        );
        
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(currentItem);
        
        m_activePrintAnim->setMedia(renderedPix, isEnvelope);
        m_activePrintAnim->start();
        
        m_btnAbort->setEnabled(true);
        m_btnAbort->setText("🚨 ABORT / PANIC PRINT");
        m_btnAbort->setStyleSheet(
            "QPushButton {"
            "  background-color: #d90429;"
            "  color: white;"
            "  font-weight: bold;"
            "  font-size: 14px;"
            "  border-radius: 6px;"
            "}"
            "QPushButton:hover { background-color: #ef233c; }"
            "QPushButton:pressed { background-color: #b70921; }"
        );
        
        m_stackedWidget->setCurrentIndex(PagePrinting);
        if (!isVisible()) {
            show();
        }
        raise();
        activateWindow();
        qApp->processEvents();
    }

    void updatePrintingProgress(bool isEnvelope, int currentItem, int totalItems, 
                                const QString &callsign, const QPixmap &renderedPix = QPixmap())
    {
        setPrintingActive(isEnvelope, currentItem, totalItems, callsign, renderedPix);
    }

    void updateProgress(int current, int total, const QString &statusText) {
        if (m_progressBar) {
            m_progressBar->setMaximum(total > 0 ? total : 1);
            m_progressBar->setValue(current);
        }
        if (m_lblPrintingStatus) {
            m_lblPrintingStatus->setText(QString("Printing: <b>%1</b> (%2 of %3)").arg(statusText).arg(current).arg(total));
        }
        qApp->processEvents();
    }

    QString getSelectedPrinter() const {
        if (m_printerSelectionContainer && m_printerSelectionContainer->isVisible() && m_comboPrinter) {
            QString s = m_comboPrinter->currentData().toString();
            if (s.isEmpty()) s = m_comboPrinter->currentText();
            return s;
        }
        return m_currentPrinterName;
    }

    bool isAborted() const { return m_aborted; }

signals:
    void panicRequested();

public slots:
    void onProceedClicked() {
        m_confirmed = true;
        if (m_printerSelectionContainer && m_printerSelectionContainer->isVisible() && m_comboPrinter) {
            QString s = m_comboPrinter->currentData().toString();
            if (s.isEmpty()) s = m_comboPrinter->currentText();
            m_currentPrinterName = s;
        }
        if (m_waitingLoop && m_waitingLoop->isRunning()) {
            m_waitingLoop->quit();
        }
    }

    // ========================================================================
    // PRINT DIALOG CANCELLATION & TEARDOWN SAFEGUARDS
    // Purpose: Prevents SIGSEGV crashes caused by dangling pointers to local
    //          event loops on the stack and avoids re-entrant hide() invocations
    //          when the window system or user triggers a closeEvent.
    // ========================================================================
    void onCancelClicked() {
        m_aborted = true;
        m_confirmed = false;
        if (m_waitingLoop && m_waitingLoop->isRunning()) {
            m_waitingLoop->quit();
        }
        if (isVisible()) {
            hide();
        }
    }

    void onAbortClicked() {
        m_aborted = true;
        m_confirmed = false;
        m_btnAbort->setEnabled(false);
        m_btnAbort->setText("Purging Spooler...");
        m_btnAbort->setStyleSheet("background-color: #8d99ae; color: white; font-weight: bold;");
        if (m_waitingLoop && m_waitingLoop->isRunning()) {
            m_waitingLoop->quit();
        }
        if (isVisible()) {
            hide();
        }
        emit panicRequested();
    }

    bool waitForOrientationConfirmation() {
        m_stackedWidget->setCurrentIndex(PageOrientation);
        m_confirmed = false;
        
        // Modal window state prevents interactions with underlying parent windows while waiting for confirmation
        setWindowModality(Qt::ApplicationModal);
        if (!isVisible()) {
            show();
        }
        raise();
        activateWindow();
        
        QEventLoop loop;
        m_waitingLoop = &loop;
        loop.exec();
        m_waitingLoop = nullptr;
        setWindowModality(Qt::NonModal);
        
        return (m_confirmed && !m_aborted);
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        // Handle window 'X' close cleanly: signal termination to the local event loop
        // without calling hide() re-entrantly (Qt's close_helper already handles destruction/hiding)
        m_aborted = true;
        m_confirmed = false;
        if (m_waitingLoop && m_waitingLoop->isRunning()) {
            m_waitingLoop->quit();
        }
        event->accept();
    }

    void reject() override {
        onCancelClicked();
    }

private:
    QStackedWidget *m_stackedWidget = nullptr;
    
    // Page 0: Orientation
    QLabel *m_lblOrientationTitle = nullptr;
    QLabel *m_lblOrientationWarning = nullptr;
    PrinterAnimationWidget *m_orientAnim = nullptr;
    QLabel *m_lblQueueStatus = nullptr;
    QLabel *m_lblPrinterDest = nullptr;
    QWidget *m_printerSelectionContainer = nullptr;
    QComboBox *m_comboPrinter = nullptr;
    QPushButton *m_btnProceed = nullptr;
    QPushButton *m_btnCancel = nullptr;
    
    // Page 1: Active Printing
    QLabel *m_lblPrintingTitle = nullptr;
    ActivePrintAnimationWidget *m_activePrintAnim = nullptr;
    QLabel *m_lblPrintingStatus = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QPushButton *m_btnAbort = nullptr;
    
    // Control variables
    bool m_confirmed = false;
    bool m_aborted = false;
    QString m_currentPrinterName;
    QPointer<QEventLoop> m_waitingLoop;
};

// [ENTRY POINT: Print Dispatcher Gateway]
// Verifies queue statuses, configures target CUPS or physical printer destinations,
// handles single or batch spool setups, and initiates the card/envelope printing loops.
void QslPreviewDialog::onSendToPrinterClicked() {
    if (m_printQueue.isEmpty()) {
        QMessageBox::warning(this, "Empty Print Queue", 
            "The print queue is empty. There is nothing to send to the printer.");
        return;
    }
    bool isCommentNotDefault = false;
    QString commentText = m_manualFieldData.value("<comment>").trimmed();
    if (!commentText.isEmpty()) {
        QString lowerText = commentText.toLower();
        if (lowerText != "comment" &&
            lowerText != "commentbox" &&
            lowerText != "comment box" &&
            lowerText != "newblock" &&
            lowerText != "new block" &&
            lowerText != "<comment>" &&
            lowerText != "<commentbox>" &&
            lowerText != "qslmsg" &&
            lowerText != "<qslmsg>" &&
            lowerText != "notes" &&
            lowerText != "<notes>" &&
            lowerText != "message" &&
            lowerText != "<message>" &&
            lowerText != "msg" &&
            lowerText != "<msg>") {
            isCommentNotDefault = true;
        }
    }

    bool isCommentCheckboxNotSelected = (m_chkComment && !m_chkComment->isChecked());

    bool skipReminder = (m_chkPrintCard && !m_chkPrintCard->isChecked() &&
                         m_chkPrintEnvelope && m_chkPrintEnvelope->isChecked());

    if (isCommentNotDefault && isCommentCheckboxNotSelected && !skipReminder) {
        int reply = QMessageBox::warning(this, "Warning",
            "You have entered a comment, but it is not selected to be printed, Do You Wish To Continue?",
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    bool hasQueueSelected = m_chkCardThenEnv->isChecked() || m_chkAllCardsThenEnvs->isChecked();
    bool hasPacingSelected = m_chkPacingOneAtATime->isChecked() || m_chkPacingContinuous->isChecked();
    
    if (!hasQueueSelected || !hasPacingSelected) {
        QMessageBox::warning(this, "Validation Error", 
            "An option for both Queue Printing and Print Pacing must be selected in Printing Preferences to proceed.");
        return;
    }

    if (m_chkPrintReverse && m_chkPrintReverse->isChecked()) {
        std::reverse(m_printQueue.begin(), m_printQueue.end());
    }

    m_batchSequence = m_chkCardThenEnv->isChecked() ? BatchSequence::CardEnvelope : BatchSequence::CardsThenEnvelopes;
    m_batchPacing = m_chkPacingOneAtATime->isChecked() ? BatchPacing::OneAtATime : BatchPacing::Continuous;
    
    m_batchContinuousPrinting = (m_batchPacing == BatchPacing::Continuous);
    m_batchSelectedPrinter.clear(); // Reset printer choice for new batch
    m_batchConfirmedCard = false;
    m_batchConfirmedEnv = false;
    m_currentItemPrinted = false;
    m_totalCardsSentToPrinter = 0;
    m_anyCardJobUsedMultipleCards = false;
    
    m_overflowQueue.clear();
    m_batchPrintingOverflowPhase = false;
    m_batchOverflowChoice = OverflowChoice::Unset;
    m_userDeclinedOverflow = false;
    
    if (m_batchSequence == BatchSequence::CardsThenEnvelopes) {
        if (!m_chkPrintCard->isChecked() && m_chkPrintEnvelope->isChecked()) {
            m_batchPrintingEnvelopesPhase = true; // Directly do envelopes if card not requested
            m_previewingEnvelope = true;
        } else {
            m_batchPrintingEnvelopesPhase = false; // Start with cards
        }
    }

    m_printQueueIndex = 0;
    
    // Jump to the first queue item
    QString nextCallWithMode = m_printQueue[m_printQueueIndex];
    QString nextCall = nextCallWithMode.split("|")[0].trimmed().toUpper();
    QString mode = nextCallWithMode.contains("|") ? nextCallWithMode.split("|")[1].trimmed() : "unprinted";
    bool isSameCall = (nextCall == m_callsign.trimmed().toUpper());
    if (!isSameCall) {
        m_callsign = nextCall;
        QList<QsoDetail> fileQsos = loadQsosFromDefinedLogfiles(nextCall);
        if (!fileQsos.isEmpty()) {
            m_qsos = fileQsos;
        } else if (m_mainWnd) {
            m_qsos = m_mainWnd->allQsos()[nextCall];
        }
        refreshAddressData(nextCall);
        // Clear manual field data except for comment and comment alignment, which persist until dialog is closed
        QString savedCommentText = m_manualFieldData.value("<comment>");
        QString savedCommentAlign = m_manualFieldData.value("<comment_align>");
        m_manualFieldData.clear();
        if (!savedCommentText.isEmpty()) {
            m_manualFieldData["<comment>"] = savedCommentText;
        }
        if (!savedCommentAlign.isEmpty()) {
            m_manualFieldData["<comment_align>"] = savedCommentAlign;
        }
        refreshContactsTable(mode);
        updatePreview();
    }

    if (m_batchContinuousPrinting) {
        QTimer::singleShot(100, this, &QslPreviewDialog::doContinuousBatchPrint);
    } else {
        QTimer::singleShot(100, this, &QslPreviewDialog::printQsl);
    }
}

void QslPreviewDialog::processNextBatchItem() {
    if (m_printQueue.isEmpty()) {
        return;
    }
    m_currentItemPrinted = false;
    if (m_printQueueIndex < m_printQueue.size() - 1) {
        m_printQueueIndex++;
        if (m_printQueueIndex < 0 || m_printQueueIndex >= m_printQueue.size()) {
            return;
        }
        QString nextCallWithMode = m_printQueue[m_printQueueIndex];
        QString nextCall = nextCallWithMode.split("|")[0].trimmed().toUpper();
        QString mode = nextCallWithMode.contains("|") ? nextCallWithMode.split("|")[1].trimmed() : "unprinted";
        m_callsign = nextCall;
        QList<QsoDetail> fileQsos = loadQsosFromDefinedLogfiles(nextCall);
        if (!fileQsos.isEmpty()) {
            m_qsos = fileQsos;
        } else if (m_mainWnd) {
            m_qsos = m_mainWnd->allQsos()[nextCall];
        }
        
        refreshAddressData(nextCall);
        // Clear manual field data except for comment and comment alignment, which persist until dialog is closed
        QString savedCommentText = m_manualFieldData.value("<comment>");
        QString savedCommentAlign = m_manualFieldData.value("<comment_align>");
        m_manualFieldData.clear();
        m_hasTextOverride = false;
        m_hasFontOverride = false;
        m_forceDisableAutoShrink = false;
        m_hasPosOverride = false;
        m_hasCountryPosOverride = false;
        m_hasCountryTextOverride = false;
        m_hasCountryFontOverride = false;
        if (!savedCommentText.isEmpty()) {
            m_manualFieldData["<comment>"] = savedCommentText;
        }
        if (!savedCommentAlign.isEmpty()) {
            m_manualFieldData["<comment_align>"] = savedCommentAlign;
        }
        refreshContactsTable(mode);
        updatePreview();
        
        if (m_unifiedPrintDlg) {
            m_unifiedPrintDlg->updateProgress(m_printQueueIndex + 1, m_printQueue.size(), m_callsign);
        }
        
        if (m_batchContinuousPrinting) {
            doContinuousBatchPrint();
        } else {
            printQsl();
        }
    } else {
        if (m_batchPrintingOverflowPhase) {
            finishOverflowPhase();
        } else {
            if (checkAndTransitionToOverflow()) {
                return;
            }
        }
        
        if (m_batchSequence == BatchSequence::CardsThenEnvelopes && !m_batchPrintingEnvelopesPhase && m_chkPrintEnvelope->isChecked()) {
            if (m_anyCardJobUsedMultipleCards) {
                int ret = QMessageBox::warning(this, "Multiple Cards Warning",
                    QString("One or more of your Cards used more than one card.\n Check you have %1(total number of cards just sent to the printer.) printed cards.\n If not, your card job is not done printing, insert another card.")
                    .arg(m_totalCardsSentToPrinter), QMessageBox::Ok | QMessageBox::Cancel);
                if (ret == QMessageBox::Cancel) {
                    m_printQueue.clear();
                    if (m_unifiedPrintDlg) {
                        m_unifiedPrintDlg->hide();
                        delete m_unifiedPrintDlg;
                        m_unifiedPrintDlg = nullptr;
                    }
                    this->accept();
                    return;
                }
            }
            m_batchPrintingEnvelopesPhase = true;
            m_printQueueIndex = 0;
            m_batchSelectedPrinter.clear(); // Force orientation warning for envelopes
            QString nextCallWithMode = m_printQueue[m_printQueueIndex];
            QString nextCall = nextCallWithMode.split("|")[0].trimmed().toUpper();
            QString mode = nextCallWithMode.contains("|") ? nextCallWithMode.split("|")[1].trimmed() : "unprinted";
            m_callsign = nextCall;
            QList<QsoDetail> fileQsos = loadQsosFromDefinedLogfiles(nextCall);
            if (!fileQsos.isEmpty()) {
                m_qsos = fileQsos;
            } else if (m_mainWnd) {
                m_qsos = m_mainWnd->allQsos()[nextCall];
            }
            refreshAddressData(nextCall);
            // Clear manual field data except for comment and comment alignment, which persist until dialog is closed
            QString savedCommentText = m_manualFieldData.value("<comment>");
            QString savedCommentAlign = m_manualFieldData.value("<comment_align>");
            m_manualFieldData.clear();
            m_hasTextOverride = false;
            m_hasFontOverride = false;
            m_forceDisableAutoShrink = false;
            m_hasPosOverride = false;
            m_hasCountryPosOverride = false;
            m_hasCountryTextOverride = false;
            m_hasCountryFontOverride = false;
            if (!savedCommentText.isEmpty()) {
                m_manualFieldData["<comment>"] = savedCommentText;
            }
            if (!savedCommentAlign.isEmpty()) {
                m_manualFieldData["<comment_align>"] = savedCommentAlign;
            }
            refreshContactsTable(mode);
            
            m_previewingEnvelope = true;
            updatePreview();
            
            if (m_unifiedPrintDlg) {
                m_unifiedPrintDlg->updateProgress(m_printQueueIndex + 1, m_printQueue.size(), m_callsign + " (Env)");
            }
            
            if (m_batchContinuousPrinting) {
                doContinuousBatchPrint();
            } else {
                printQsl();
            }
        } else {
            QMessageBox::information(this, "Queue Finished", "Finished printing queue.");
            m_printQueue.clear();
            if (m_unifiedPrintDlg) {
                m_unifiedPrintDlg->hide();
                delete m_unifiedPrintDlg;
                m_unifiedPrintDlg = nullptr;
            }
            this->accept();
        }
    }
}

void QslPreviewDialog::doContinuousBatchPrint() {
    // This handles pacing continuous.
    // However, a true continuous print job via a single QPrinter requires looping inside performPrint.
    // If we just bypass the prompts, we need to temporarily set the selected printer and bypass performPrint's confirmDlg.
    // But performPrint doesn't take the printer name as an argument and opens the dialog itself.
    // So we'll have to add continuous support to performPrint later if needed, 
    // or just tell the user this feature relies on the modified printQsl/performPrint logic.
    // For now, let's just bypass by using the normal printQsl but the normal printQsl doesn't know it's continuous.
    // I will refactor printQsl and performPrint to support m_batchContinuousPrinting.
    printQsl();
}

void QslPreviewDialog::printQsl() {
    bool printCard = m_chkPrintCard->isChecked();
    bool printEnvelope = m_chkPrintEnvelope->isChecked();
    
    if (!printCard && !printEnvelope) {
        QMessageBox::warning(this, "Nothing to Print", "You must select Print Card, Print Envelope, or both.");
        return;
    }
    
    if (m_previewingEnvelope && !printEnvelope) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Envelope Not Selected",
            "You appear to have an envelope chosen, but not printing, OK?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply == QMessageBox::No) {
            return;
        }
    }

    bool origPreviewEnv = m_previewingEnvelope;
    
    // ========================================================================
    // OVERFLOW CARD DETECTION & USER PROMPT (SINGLE & BATCH MODES)
    // Purpose: Determines if the number of contacts to be printed exceeds the
    //          capacity of the regular card template for the current item or
    //          any queued item in a batch run. Prompts the user to choose
    //          between using an overflow template or continuing with the
    //          regular template across multiple standard cards.
    // ========================================================================
    if (printCard && !m_batchPrintingOverflowPhase && 
        (!m_batchPrintingEnvelopesPhase || m_batchSequence != BatchSequence::CardsThenEnvelopes)) {
        
        QString cardTpl = getSelectedCardTemplate();
        if (cardTpl.isEmpty() && m_templateList && m_templateList->currentItem()) {
            cardTpl = m_templateList->currentItem()->text();
        }
        int maxCapacity = calculateMaxContactsForCardTemplate(cardTpl);

        int maxContactsFound = 0;
        if (m_contactsTable) {
            for (int i = 0; i < m_contactsTable->rowCount(); ++i) {
                if (m_contactsTable->item(i, 0) && m_contactsTable->item(i, 0)->data(Qt::UserRole).toBool()) {
                    maxContactsFound++;
                }
            }
        }
        if (maxContactsFound == 0 && m_contactsTable && m_contactsTable->rowCount() == 0) {
            maxContactsFound = m_qsos.size();
        }

        // Also scan through batch queue items if printing multiple cards
        if (m_printQueue.size() > 1) {
            for (int qIdx = 0; qIdx < m_printQueue.size(); ++qIdx) {
                if (qIdx == m_printQueueIndex) continue;
                QString item = m_printQueue[qIdx];
                QString call = item.split("|")[0].trimmed().toUpper();
                QString mode = item.contains("|") ? item.split("|")[1].trimmed() : "unprinted";
                QList<QsoDetail> qsos = loadQsosFromDefinedLogfiles(call);
                if (qsos.isEmpty() && m_mainWnd) {
                    qsos = m_mainWnd->allQsos().value(call);
                }
                int unprintedCount = 0;
                for (const auto &qso : qsos) {
                    if (qso.qslsdate.isEmpty()) unprintedCount++;
                }
                int qChecked = 0;
                for (const auto &qso : qsos) {
                    bool checked = (mode == "all") || (mode == "reprint") || 
                                   ((mode == "unprinted" || mode == "prompt") && qso.qslsdate.isEmpty()) ||
                                   (unprintedCount == 0);
                    if (checked) qChecked++;
                }
                if (qChecked > maxContactsFound) {
                    maxContactsFound = qChecked;
                }
            }
        }

        if (maxContactsFound > maxCapacity) {
            if (!promptUserForOverflowTemplate(maxContactsFound, maxCapacity, cardTpl)) {
                m_previewingEnvelope = origPreviewEnv;
                return;
            }
        }
    }
    
    if (m_printQueue.size() > 1 && m_batchSequence == BatchSequence::CardsThenEnvelopes) {
        // We are printing all cards, then all envelopes
        if (!m_batchPrintingEnvelopesPhase && printCard) {
            m_previewingEnvelope = false;
            if (m_templateList->currentItem()) {
                onTemplateChosen(m_templateList->currentItem()->text());
            } else if (m_templateList->count() > 0) {
                onTemplateChosen(m_templateList->item(0)->text());
            }
            if (!performPrint(false)) {
                m_previewingEnvelope = origPreviewEnv;
                return;
            }
        } else if (m_batchPrintingEnvelopesPhase && printEnvelope) {
            m_previewingEnvelope = true;
            if (m_envTemplateList->currentItem()) {
                onTemplateChosen(m_envTemplateList->currentItem()->text());
            } else if (m_envTemplateList->count() > 0) {
                onTemplateChosen(m_envTemplateList->item(0)->text());
            }
            if (!performPrint(true)) {
                m_previewingEnvelope = origPreviewEnv;
                return;
            }
        }
    } else {
        // Normal or CardEnvelope sequence
        if (printCard) {
            m_previewingEnvelope = false;
            if (m_templateList->currentItem()) {
                onTemplateChosen(m_templateList->currentItem()->text());
            } else if (m_templateList->count() > 0) {
                onTemplateChosen(m_templateList->item(0)->text());
            }
            if (!performPrint(false)) {
                m_previewingEnvelope = origPreviewEnv;
                return;
            }
        }
        
        if (printEnvelope) {
            m_previewingEnvelope = true;
            if (m_envTemplateList->currentItem()) {
                onTemplateChosen(m_envTemplateList->currentItem()->text());
            } else if (m_envTemplateList->count() > 0) {
                onTemplateChosen(m_envTemplateList->item(0)->text());
            }
            if (!performPrint(true)) {
                m_previewingEnvelope = origPreviewEnv;
                return;
            }
        }
    }
    
    m_previewingEnvelope = origPreviewEnv;
    if (m_unifiedPrintDlg) {
        m_unifiedPrintDlg->updateProgress(m_printQueueIndex + 1, m_printQueue.size(), m_callsign);
    }
    
    if (m_previewingEnvelope && m_envTemplateList->currentItem()) {
        onTemplateChosen(m_envTemplateList->currentItem()->text());
    } else if (!m_previewingEnvelope && m_templateList->currentItem()) {
        onTemplateChosen(m_templateList->currentItem()->text());
    }
    
    MainWindow *mw = nullptr;
    QWidget *pw = parentWidget();
    while (pw) {
        mw = qobject_cast<MainWindow*>(pw);
        if (mw) break;
        pw = pw->parentWidget();
    }
    if (mw) {
        QString whatPrinted = "Card";
        if (printCard && printEnvelope) whatPrinted = "Both";
        else if (printEnvelope) whatPrinted = "Envelope";
        
        QString details;
        for (const auto &q : m_qsos) {
            if (q.qslsdate == QDateTime::currentDateTime().toString("yyyyMMdd") || details.isEmpty()) {
                details = QString(" Mode: %1 RST Sent: %2 RST Rcvd: %3 TX Power: %4")
                          .arg(q.mode)
                          .arg(q.rst_sent.isEmpty() ? "n/a" : q.rst_sent)
                          .arg(q.rst_rcvd.isEmpty() ? "n/a" : q.rst_rcvd)
                          .arg(q.tx_pwr.isEmpty() ? "n/a" : q.tx_pwr);
                break;
            }
        }
        mw->logSystem(QDateTime::currentDateTimeUtc().toString("dd-MM-yy HH:mm:ss.zzz UTC") + " " + m_callsign + " " + whatPrinted + " Printed" + details);
    }
    
    // Check if we need to proceed to the next item
    m_currentItemPrinted = true;
    
    if (m_printQueueIndex >= m_printQueue.size() - 1) {
        if (m_batchPrintingOverflowPhase) {
            finishOverflowPhase();
        } else {
            if (checkAndTransitionToOverflow()) {
                return;
            }
        }
        if (m_printQueue.size() <= 1) {
            m_userDeclinedOverflow = false;
        }
    }
    
    if (m_printQueue.size() > 1) {
        if (m_batchContinuousPrinting) {
            // Continuous batch pacing: wait configured delay, then advance cleanly to the next item
            int delayMs = m_spinPacingDelay ? (m_spinPacingDelay->value() * 1000) : 4000;
            if (delayMs < 100) delayMs = 100;
            QTimer::singleShot(delayMs, this, [this]() {
                if (m_printQueueIndex < m_printQueue.size() - 1) {
                    processNextBatchItem();
                } else if (m_batchSequence == BatchSequence::CardsThenEnvelopes && !m_batchPrintingEnvelopesPhase && m_chkPrintEnvelope->isChecked()) {
                    processNextBatchItem();
                } else {
                    if (m_unifiedPrintDlg) {
                        m_unifiedPrintDlg->hide();
                        delete m_unifiedPrintDlg;
                        m_unifiedPrintDlg = nullptr;
                    }
                    QMessageBox::information(this, "Queue Finished", "Finished printing queue.");
                    m_printQueue.clear();
                    this->accept();
                }
            });
        } else {
            if (m_printQueueIndex < m_printQueue.size() - 1) {
                int ret = QMessageBox::question(this, "Proceed to Next Card?", 
                    QString("OK To Proceed to Next Item?\nThis is %1 of %2 in the queue.")
                    .arg(m_printQueueIndex + 1).arg(m_printQueue.size()), QMessageBox::Yes | QMessageBox::No);
                    
                if (ret == QMessageBox::Yes) {
                    processNextBatchItem();
                }
            } else if (m_batchSequence == BatchSequence::CardsThenEnvelopes && !m_batchPrintingEnvelopesPhase && m_chkPrintEnvelope->isChecked()) {
                int ret = QMessageBox::question(this, "Proceed to Envelopes?", 
                    "Finished printing cards. Proceed to envelopes?", QMessageBox::Yes | QMessageBox::No);
                if (ret == QMessageBox::Yes) {
                    processNextBatchItem();
                }
            } else {
                if (m_unifiedPrintDlg) {
                    m_unifiedPrintDlg->hide();
                    delete m_unifiedPrintDlg;
                    m_unifiedPrintDlg = nullptr;
                }
                QMessageBox::information(this, "Queue Finished", "Finished printing queue.");
                m_printQueue.clear(); // Clear so it doesn't get re-added to manager queue
                this->accept();
            }
        }
    } else {
        if (m_unifiedPrintDlg) {
            m_unifiedPrintDlg->hide();
            delete m_unifiedPrintDlg;
            m_unifiedPrintDlg = nullptr;
        }
        QMessageBox::information(this, "Queue Finished", "Finished printing queue.");
        m_printQueue.clear();
        this->accept();
    }
}

#include <QRadioButton>
#include <QButtonGroup>
#include <QTextEdit>
#include <QFontComboBox>
#include <QSpinBox>
#include <QSplitter>
#include <QMouseEvent>

PrintExceptionDialog::PrintExceptionDialog(QslPreviewDialog *parentDialog, const QString &fieldId, const QString &initialText, const QFont &initialFont)
    : QDialog(parentDialog), m_parent(parentDialog), m_fieldId(fieldId), m_currentFont(initialFont) {
    setWindowTitle("⚠️ Print Exception: Potential Print Clipping Detected!");
    setMinimumSize(1100, 750);
    
    m_addressText = initialText;
    m_addressFont = initialFont;

    // Detect if parent template has a dedicated separate Country element (e.g. [<COUNTRY>] in 'qsl culture')
    m_hasCountryElement = false;
    for (const QVariantMap &itemMap : m_parent->m_templateItems) {
        QString itemText = itemMap["cellData"].toStringList().isEmpty() ? itemMap["text"].toString() : itemMap["cellData"].toStringList().first();
        QString itemTag = itemMap["id"].toString().toLower().trimmed();
        QString itemTextLower = itemText.toLower().trimmed();
        if (itemTag == "country" || itemTag == "<country>" || itemTextLower == "country" || itemTextLower == "<country>") {
            m_hasCountryElement = true;
            m_countryFieldId = itemMap["id"].toString().isEmpty() ? itemText : itemMap["id"].toString();
            m_countryText = m_parent->m_addrCountry;
            if (m_countryText.isEmpty() && !m_parent->m_qsos.isEmpty()) m_countryText = m_parent->m_qsos[0].country;
            if (m_parent->m_previewingEnvelope) m_countryText = formatCountry(m_countryText);
            m_countryFont = QFont(itemMap["font"].toString(), qMax(6, itemMap["size"].toInt()));

            QSettings settings(QCoreApplication::organizationName(), "Qwain");
            QString key = QString("QslTemplateOverrides/%1/Pos_%2_%3").arg(m_parent->m_currentTemplate).arg(m_countryFieldId).arg(-1);
            if (settings.contains(key + "_x")) m_countryOverrideX = settings.value(key + "_x").toDouble();
            if (settings.contains(key + "_y")) m_countryOverrideY = settings.value(key + "_y").toDouble();
            break;
        }
    }
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);
    
    // Header
    QLabel *lblHeader = new QLabel(this);
    lblHeader->setText("<h2>⚠️ Potential Print Clipping Detected!</h2>"
                       "<p>The text for field <b>" + fieldId + "</b> is extremely long and may clip or look very small. "
                       "Please select your preferred resolution style below.</p>");
    lblHeader->setWordWrap(true);
    lblHeader->setTextFormat(Qt::RichText);
    mainLayout->addWidget(lblHeader);
    
    // Splitter with Left and Right Columns
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left Column: Option 1 and Option 2 stacked vertically
    QWidget *leftCol = new QWidget(this);
    QVBoxLayout *leftLay = new QVBoxLayout(leftCol);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->setSpacing(12);
    
    // Option 1 Panel
    QFrame *frame1 = new QFrame(leftCol);
    frame1->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout *lay1 = new QVBoxLayout(frame1);
    lay1->setContentsMargins(10, 10, 10, 10);
    
    QHBoxLayout *hlay1 = new QHBoxLayout();
    m_radio1 = new QRadioButton(frame1);
    m_radio1->setChecked(true);
    m_lbl1 = new QLabel("<b>Option 1: Original Font Size</b><br>(May Clip/Crop)", frame1);
    m_lbl1->setTextFormat(Qt::RichText);
    m_lbl1->installEventFilter(this);
    hlay1->addWidget(m_radio1);
    hlay1->addWidget(m_lbl1, 1);
    lay1->addLayout(hlay1);
    
    m_preview1 = new QLabel(frame1);
    m_preview1->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_preview1->setAlignment(Qt::AlignCenter);
    m_preview1->setMinimumSize(160, 120);
    lay1->addWidget(m_preview1, 1);
    leftLay->addWidget(frame1, 1);
    
    // Option 2 Panel
    QFrame *frame2 = new QFrame(leftCol);
    frame2->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout *lay2 = new QVBoxLayout(frame2);
    lay2->setContentsMargins(10, 10, 10, 10);
    
    QHBoxLayout *hlay2 = new QHBoxLayout();
    m_radio2 = new QRadioButton(frame2);
    m_lbl2 = new QLabel("<b>Option 2: Auto-Resized Font</b><br>(Qwain Shrink-To-Fit)", frame2);
    m_lbl2->setTextFormat(Qt::RichText);
    m_lbl2->installEventFilter(this);
    hlay2->addWidget(m_radio2);
    hlay2->addWidget(m_lbl2, 1);
    lay2->addLayout(hlay2);
    
    m_preview2 = new QLabel(frame2);
    m_preview2->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_preview2->setAlignment(Qt::AlignCenter);
    m_preview2->setMinimumSize(160, 120);
    lay2->addWidget(m_preview2, 1);
    leftLay->addWidget(frame2, 1);
    
    splitter->addWidget(leftCol);
    
    // Right Column: Option 3 (Custom Override) - Full Height Pane
    QWidget *rightCol = new QWidget(this);
    QVBoxLayout *lay3 = new QVBoxLayout(rightCol);
    lay3->setContentsMargins(0, 0, 0, 0);
    
    QFrame *frame3 = new QFrame(rightCol);
    frame3->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout *frame3Lay = new QVBoxLayout(frame3);
    frame3Lay->setContentsMargins(10, 10, 10, 10);
    
    QHBoxLayout *hlay3 = new QHBoxLayout();
    m_radio3 = new QRadioButton(frame3);
    m_lbl3 = new QLabel("<b>Option 3: Custom Override</b><br>(Edit and Tweak Below)", frame3);
    m_lbl3->setTextFormat(Qt::RichText);
    m_lbl3->installEventFilter(this);
    hlay3->addWidget(m_radio3);
    hlay3->addWidget(m_lbl3, 1);
    frame3Lay->addLayout(hlay3);
    
    // Editor layout inside Option 3
    QVBoxLayout *editLay = new QVBoxLayout();
    editLay->setSpacing(6);
    
    // Target element selector: Radio buttons to switch between Address Box and Country Box
    if (m_hasCountryElement) {
        m_elemSelectWidget = new QWidget(frame3);
        QHBoxLayout *elemLay = new QHBoxLayout(m_elemSelectWidget);
        elemLay->setContentsMargins(0, 0, 0, 4);
        elemLay->setSpacing(12);

        QLabel *lblTarget = new QLabel("<b>Target Element to Adjust:</b>", m_elemSelectWidget);
        m_radioElemAddress = new QRadioButton(QString("Address Box (%1)").arg(m_fieldId), m_elemSelectWidget);
        m_radioElemCountry = new QRadioButton(QString("Country Box (%1)").arg(m_countryFieldId), m_elemSelectWidget);
        m_radioElemAddress->setChecked(true);

        m_elemGroup = new QButtonGroup(this);
        m_elemGroup->addButton(m_radioElemAddress, 1);
        m_elemGroup->addButton(m_radioElemCountry, 2);

        elemLay->addWidget(lblTarget);
        elemLay->addWidget(m_radioElemAddress);
        elemLay->addWidget(m_radioElemCountry);
        elemLay->addStretch();

        editLay->addWidget(m_elemSelectWidget);

        // Connect radio button group to target element toggler (using modern idClicked to avoid deprecated buttonClicked(int))
        connect(m_elemGroup, &QButtonGroup::idClicked, this, &PrintExceptionDialog::onTargetElementToggled);
    }
    
    m_textEdit = new QTextEdit(frame3);
    m_textEdit->setPlainText(initialText);
    m_textEdit->setMaximumHeight(80);
    QLabel *editLbl = new QLabel("Edit Text (use \n or enter for lines):", frame3);
    editLay->addWidget(editLbl);
    editLay->addWidget(m_textEdit);
    
    QHBoxLayout *fontLay = new QHBoxLayout();
    m_fontCombo = new QFontComboBox(frame3);
    m_fontCombo->setCurrentFont(initialFont);
    m_spinFontSize = new QSpinBox(frame3);
    m_spinFontSize->setRange(4, 72);
    m_spinFontSize->setValue(initialFont.pointSize() > 0 ? initialFont.pointSize() : (initialFont.pixelSize() > 0 ? (int)(initialFont.pixelSize() * 0.75) : 10));
    fontLay->addWidget(m_fontCombo, 1);
    fontLay->addWidget(m_spinFontSize);
    editLay->addLayout(fontLay);
    
    frame3Lay->addLayout(editLay);
    
    m_preview3 = new QLabel(frame3);
    m_preview3->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_preview3->setAlignment(Qt::AlignCenter);
    m_preview3->installEventFilter(this);
    m_preview3->setCursor(Qt::SizeAllCursor);
    m_preview3->setToolTip("Click and drag to reposition the text on the card template!");
    frame3Lay->addWidget(m_preview3, 1);
    
    lay3->addWidget(frame3, 1);
    splitter->addWidget(rightCol);
    
    mainLayout->addWidget(splitter, 1);
    
    // Set splitter ratios so that Option 3 occupies more space (the most important)
    splitter->setSizes(QList<int>() << 450 << 650);
    
    // Button Group for radio buttons
    m_btnGroup = new QButtonGroup(this);
    m_btnGroup->addButton(m_radio1, 1);
    m_btnGroup->addButton(m_radio2, 2);
    m_btnGroup->addButton(m_radio3, 3);
    
    // Connect changes to update custom preview
    connect(m_textEdit, &QTextEdit::textChanged, this, &PrintExceptionDialog::onCustomChanged);
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, &PrintExceptionDialog::onCustomChanged);
    connect(m_spinFontSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &PrintExceptionDialog::onCustomChanged);
    
    // Also auto-select Option 3 radio button if user interacts with Option 3 controls
    connect(m_textEdit, &QTextEdit::textChanged, [this]() { m_radio3->setChecked(true); });
    connect(m_fontCombo, &QFontComboBox::currentFontChanged, [this]() { m_radio3->setChecked(true); });
    connect(m_spinFontSize, QOverload<int>::of(&QSpinBox::valueChanged), [this]() { m_radio3->setChecked(true); });
    
    // Bottom Actions layout
    QHBoxLayout *actionLay = new QHBoxLayout();
    
    m_btnPostpone = new QPushButton("⏸️ Postpone This Card/Envelope Printing", this);
    m_btnPostpone->setStyleSheet("background-color: #f0ad4e; color: black; font-weight: bold; padding: 6px 12px;");
    actionLay->addWidget(m_btnPostpone);
    
    actionLay->addStretch();
    
    m_btnProceed = new QPushButton("✅ Proceed with Selected Version", this);
    m_btnProceed->setStyleSheet("background-color: #5cb85c; color: white; font-weight: bold; padding: 6px 16px;");
    m_btnProceed->setDefault(true);
    actionLay->addWidget(m_btnProceed);
    
    m_btnCancel = new QPushButton("❌ Cancel Batch", this);
    actionLay->addWidget(m_btnCancel);
    
    mainLayout->addLayout(actionLay);
    
    connect(m_btnProceed, &QPushButton::clicked, this, &PrintExceptionDialog::onProceedClicked);
    connect(m_btnPostpone, &QPushButton::clicked, this, &PrintExceptionDialog::onPostponeClicked);
    connect(m_btnCancel, &QPushButton::clicked, this, &PrintExceptionDialog::onCancelClicked);
    
    // Initialize previews!
    generatePreviews();
}

void PrintExceptionDialog::onTargetElementToggled(int id) {
    if (id == m_targetElement) return;

    // Save state from previous element
    if (m_targetElement == 1) { // Was Address Box
        m_addressText = m_textEdit->toPlainText();
        m_addressFont = m_fontCombo->currentFont();
        m_addressFont.setPointSize(m_spinFontSize->value());
    } else if (m_targetElement == 2) { // Was Country Box
        m_countryText = m_textEdit->toPlainText();
        m_countryFont = m_fontCombo->currentFont();
        m_countryFont.setPointSize(m_spinFontSize->value());
        m_hasCountryTextOverride = true;
        m_hasCountryFontOverride = true;
    }

    m_targetElement = id;

    // Load state for newly selected element into editor controls
    m_textEdit->blockSignals(true);
    m_fontCombo->blockSignals(true);
    m_spinFontSize->blockSignals(true);

    if (m_targetElement == 1) { // Switch to Address Box
        m_textEdit->setPlainText(m_addressText);
        m_fontCombo->setCurrentFont(m_addressFont);
        m_spinFontSize->setValue(m_addressFont.pointSize() > 0 ? m_addressFont.pointSize() : (m_addressFont.pixelSize() > 0 ? (int)(m_addressFont.pixelSize() * 0.75) : 10));
    } else { // Switch to Country Box
        m_textEdit->setPlainText(m_countryText);
        m_fontCombo->setCurrentFont(m_countryFont);
        m_spinFontSize->setValue(m_countryFont.pointSize() > 0 ? m_countryFont.pointSize() : (m_countryFont.pixelSize() > 0 ? (int)(m_countryFont.pixelSize() * 0.75) : 10));
    }

    m_textEdit->blockSignals(false);
    m_fontCombo->blockSignals(false);
    m_spinFontSize->blockSignals(false);

    m_radio3->setChecked(true);
    onCustomChanged();
}

bool PrintExceptionDialog::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_lbl1 || watched == m_lbl2 || watched == m_lbl3) {
        if (event->type() == QEvent::MouseButtonPress) {
            if (watched == m_lbl1) {
                m_radio1->setChecked(true);
            } else if (watched == m_lbl2) {
                m_radio2->setChecked(true);
            } else if (watched == m_lbl3) {
                m_radio3->setChecked(true);
            }
            return true;
        }
    }
    
    if (watched == m_preview3) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                m_isDragging = true;
                m_dragStartMousePos = mouseEvent->pos();
                
                if (m_targetElement == 2) { // Country Box
                    if (!m_hasCountryPosOverride) {
                        qreal defX = 0;
                        qreal defY = 0;
                        QSettings settings(QCoreApplication::organizationName(), "Qwain");
                        QString key = QString("QslTemplateOverrides/%1/Pos_%2_%3").arg(m_parent->m_currentTemplate).arg(m_countryFieldId).arg(-1);
                        if (settings.contains(key + "_x")) defX = settings.value(key + "_x").toDouble();
                        if (settings.contains(key + "_y")) defY = settings.value(key + "_y").toDouble();
                        m_countryOverrideX = defX;
                        m_countryOverrideY = defY;
                    }
                    m_dragStartOverridePos = QPointF(m_countryOverrideX, m_countryOverrideY);
                } else { // Address Box
                    if (!m_hasPosOverride) {
                        qreal defX = 0;
                        qreal defY = 0;
                        QSettings settings(QCoreApplication::organizationName(), "Qwain");
                        QString key = QString("QslTemplateOverrides/%1/Pos_%2_%3").arg(m_parent->m_currentTemplate).arg(m_fieldId).arg(-1);
                        if (settings.contains(key + "_x")) defX = settings.value(key + "_x").toDouble();
                        if (settings.contains(key + "_y")) defY = settings.value(key + "_y").toDouble();
                        m_overrideX = defX;
                        m_overrideY = defY;
                    }
                    m_dragStartOverridePos = QPointF(m_overrideX, m_overrideY);
                }
                
                m_radio3->setChecked(true);
                return true;
            }
        } else if (event->type() == QEvent::MouseMove) {
            if (m_isDragging) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                QSize originalSize = m_parent->m_scene->sceneRect().size().toSize();
                if (originalSize.width() > 0 && originalSize.height() > 0) {
                    QSize scaledSize = originalSize.scaled(480, 360, Qt::KeepAspectRatio);
                    double ratio = (double)originalSize.width() / scaledSize.width();
                    
                    double dX = (mouseEvent->pos().x() - m_dragStartMousePos.x()) * ratio;
                    double dY = (mouseEvent->pos().y() - m_dragStartMousePos.y()) * ratio;
                    
                    if (m_targetElement == 2) {
                        m_countryOverrideX = m_dragStartOverridePos.x() + dX;
                        m_countryOverrideY = m_dragStartOverridePos.y() + dY;
                        m_hasCountryPosOverride = true;
                    } else {
                        m_overrideX = m_dragStartOverridePos.x() + dX;
                        m_overrideY = m_dragStartOverridePos.y() + dY;
                        m_hasPosOverride = true;
                    }
                    
                    onCustomChanged();
                }
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            if (m_isDragging) {
                m_isDragging = false;
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

void PrintExceptionDialog::onCustomChanged() {
    if (m_targetElement == 1) {
        m_addressFont = m_fontCombo->currentFont();
        m_addressFont.setPointSize(m_spinFontSize->value());
        m_addressText = m_textEdit->toPlainText();
        m_currentFont = m_addressFont;
    } else if (m_targetElement == 2) {
        m_countryFont = m_fontCombo->currentFont();
        m_countryFont.setPointSize(m_spinFontSize->value());
        m_countryText = m_textEdit->toPlainText();
        m_hasCountryTextOverride = true;
        m_hasCountryFontOverride = true;
    }
    
    bool savedParentHasPos = m_parent->m_hasPosOverride;
    qreal savedParentX = m_parent->m_overrideX;
    qreal savedParentY = m_parent->m_overrideY;
    QString savedParentFieldId = m_parent->m_overrideFieldId;
    
    m_parent->m_hasPosOverride = m_hasPosOverride;
    m_parent->m_overrideX = m_overrideX;
    m_parent->m_overrideY = m_overrideY;
    m_parent->m_overrideFieldId = m_fieldId;

    bool savedCountryHasPos = m_parent->m_hasCountryPosOverride;
    qreal savedCountryX = m_parent->m_countryOverrideX;
    qreal savedCountryY = m_parent->m_countryOverrideY;
    bool savedCountryHasText = m_parent->m_hasCountryTextOverride;
    QString savedCountryText = m_parent->m_countryTextOverride;
    bool savedCountryHasFont = m_parent->m_hasCountryFontOverride;
    QFont savedCountryFont = m_parent->m_countryFontOverride;
    QString savedCountryFieldId = m_parent->m_countryFieldId;

    m_parent->m_hasCountryPosOverride = m_hasCountryPosOverride;
    m_parent->m_countryOverrideX = m_countryOverrideX;
    m_parent->m_countryOverrideY = m_countryOverrideY;
    m_parent->m_hasCountryTextOverride = m_hasCountryTextOverride;
    m_parent->m_countryTextOverride = m_countryText;
    m_parent->m_hasCountryFontOverride = m_hasCountryFontOverride;
    m_parent->m_countryFontOverride = m_countryFont;
    m_parent->m_countryFieldId = m_countryFieldId;
    
    QPixmap pix = m_parent->grabOverridePixmap(m_fieldId, m_addressText, m_addressFont, true, true, true);
    
    m_parent->m_hasPosOverride = savedParentHasPos;
    m_parent->m_overrideX = savedParentX;
    m_parent->m_overrideY = savedParentY;
    m_parent->m_overrideFieldId = savedParentFieldId;

    m_parent->m_hasCountryPosOverride = savedCountryHasPos;
    m_parent->m_countryOverrideX = savedCountryX;
    m_parent->m_countryOverrideY = savedCountryY;
    m_parent->m_hasCountryTextOverride = savedCountryHasText;
    m_parent->m_countryTextOverride = savedCountryText;
    m_parent->m_hasCountryFontOverride = savedCountryHasFont;
    m_parent->m_countryFontOverride = savedCountryFont;
    m_parent->m_countryFieldId = savedCountryFieldId;
    
    if (!pix.isNull()) {
        m_preview3->setPixmap(pix.scaled(480, 360, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void PrintExceptionDialog::onProceedClicked() {
    m_selectedOption = m_btnGroup->checkedId();
    if (m_targetElement == 1) {
        m_addressText = m_textEdit->toPlainText();
        m_addressFont = m_fontCombo->currentFont();
        m_addressFont.setPointSize(m_spinFontSize->value());
    } else {
        m_countryText = m_textEdit->toPlainText();
        m_countryFont = m_fontCombo->currentFont();
        m_countryFont.setPointSize(m_spinFontSize->value());
        m_hasCountryTextOverride = true;
        m_hasCountryFontOverride = true;
    }
    m_overrideText = m_addressText;
    m_overrideFont = m_addressFont;
    accept();
}

void PrintExceptionDialog::onPostponeClicked() {
    m_selectedOption = 4; // Special Postpone value
    reject();
}

void PrintExceptionDialog::onCancelClicked() {
    m_selectedOption = 5; // Special Cancel value
    reject();
}

void PrintExceptionDialog::generatePreviews() {
    // Option 1: Original Size (Auto-shrink disabled)
    QPixmap pix1 = m_parent->grabOverridePixmap(m_fieldId, "", m_currentFont, false, false, true);
    if (!pix1.isNull()) {
        m_preview1->setPixmap(pix1.scaled(280, 210, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    
    // Option 2: Auto-Resized (Auto-shrink enabled)
    QPixmap pix2 = m_parent->grabOverridePixmap(m_fieldId, "", m_currentFont, false, false, false);
    if (!pix2.isNull()) {
        m_preview2->setPixmap(pix2.scaled(280, 210, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    
    // Option 3: Custom Override (Initial)
    onCustomChanged();
}

void PrintExceptionDialog::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    generatePreviews();
}

QPixmap QslPreviewDialog::grabOverridePixmap(const QString &fieldId, const QString &text, const QFont &font, bool hasText, bool hasFont, bool disableAutoShrink) {
    bool savedHasText = m_hasTextOverride;
    bool savedHasFont = m_hasFontOverride;
    bool savedDisable = m_forceDisableAutoShrink;
    QString savedFieldId = m_overrideFieldId;
    QString savedText = m_textOverride;
    QFont savedFont = m_fontOverride;
    
    m_hasTextOverride = hasText;
    m_hasFontOverride = hasFont;
    m_forceDisableAutoShrink = disableAutoShrink;
    m_overrideFieldId = fieldId;
    m_textOverride = text;
    m_fontOverride = font;
    
    renderTemplate();
    
    QPixmap pix(m_scene->sceneRect().size().toSize() * 2);
    pix.fill(Qt::white);
    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);
    m_scene->render(&painter);
    painter.end();
    
    m_hasTextOverride = savedHasText;
    m_hasFontOverride = savedHasFont;
    m_forceDisableAutoShrink = savedDisable;
    m_overrideFieldId = savedFieldId;
    m_textOverride = savedText;
    m_fontOverride = savedFont;
    
    renderTemplate();
    
    return pix;
}

bool QslPreviewDialog::performPrint(bool isEnvelope) {
    bool hasPseOrTnx = false;
    bool anyPseOrTnxChecked = false;
    
    for (auto it = m_customChkBoxes.begin(); it != m_customChkBoxes.end(); ++it) {
        if (it.key().contains("PSE") || it.key().contains("TNX")) {
            hasPseOrTnx = true;
            if (it.value()->isChecked()) {
                anyPseOrTnxChecked = true;
            }
        }
    }
    
    if (hasPseOrTnx && !anyPseOrTnxChecked) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Checkmarks Missing", 
            "You have not checked 'PSE' or 'TNX'. Are you sure you want to print?", 
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (reply == QMessageBox::No) {
            return false;
        }
    }

    if (!m_detectedClippingFields.isEmpty()) {
        QString fieldId = m_detectedClippingFields.first().first;
        QString initialText = m_clippingFieldTexts.value(fieldId);
        QFont initialFont = m_clippingFieldFonts.value(fieldId);
        
        PrintExceptionDialog exceptionDlg(this, fieldId, initialText, initialFont);
        int result = exceptionDlg.exec();
        
        int choice = exceptionDlg.selectedOption();
        if (choice == 4) { // Postpone
            if (m_printQueue.size() > 0 && m_printQueueIndex >= 0 && m_printQueueIndex < m_printQueue.size()) {
                m_printQueue.removeAt(m_printQueueIndex);
                m_printQueueIndex--; // Decrement so next processNextBatchItem() increment lands on current index (now next item)
            }
            m_detectedClippingFields.clear();
            
            m_hasTextOverride = false;
            m_hasFontOverride = false;
            m_forceDisableAutoShrink = false;
            m_hasPosOverride = false;
            m_hasCountryPosOverride = false;
            m_hasCountryTextOverride = false;
            m_hasCountryFontOverride = false;
            renderTemplate();
            
            QTimer::singleShot(100, this, &QslPreviewDialog::processNextBatchItem);
            return false;
        } else if (choice == 5 || result == QDialog::Rejected) { // Cancel
            // Preserve the print queue so the unprinted templates remain in the persistent print queue
            if (m_unifiedPrintDlg) {
                m_unifiedPrintDlg->hide();
                delete m_unifiedPrintDlg;
                m_unifiedPrintDlg = nullptr;
            }
            this->reject();
            return false;
        } else {
            // Apply override
            if (choice == 1) { // Original
                m_hasTextOverride = false;
                m_hasFontOverride = false;
                m_forceDisableAutoShrink = true;
                m_hasPosOverride = false;
                m_hasCountryPosOverride = false;
                m_hasCountryTextOverride = false;
                m_hasCountryFontOverride = false;
            } else if (choice == 2) { // Auto-resized
                m_hasTextOverride = false;
                m_hasFontOverride = false;
                m_forceDisableAutoShrink = false;
                m_hasPosOverride = false;
                m_hasCountryPosOverride = false;
                m_hasCountryTextOverride = false;
                m_hasCountryFontOverride = false;
            } else if (choice == 3) { // Custom override
                m_hasTextOverride = true;
                m_hasFontOverride = true;
                m_forceDisableAutoShrink = true;
                m_overrideFieldId = fieldId;
                m_textOverride = exceptionDlg.getOverrideText();
                m_fontOverride = exceptionDlg.getOverrideFont();
                if (exceptionDlg.hasPosOverride()) {
                    m_hasPosOverride = true;
                    m_overrideX = exceptionDlg.overrideX();
                    m_overrideY = exceptionDlg.overrideY();
                } else {
                    m_hasPosOverride = false;
                }
                if (exceptionDlg.hasCountryPosOverride()) {
                    m_hasCountryPosOverride = true;
                    m_countryOverrideX = exceptionDlg.countryOverrideX();
                    m_countryOverrideY = exceptionDlg.countryOverrideY();
                } else {
                    m_hasCountryPosOverride = false;
                }
                if (exceptionDlg.hasCountryTextOverride()) {
                    m_hasCountryTextOverride = true;
                    m_countryTextOverride = exceptionDlg.getCountryOverrideText();
                } else {
                    m_hasCountryTextOverride = false;
                }
                if (exceptionDlg.hasCountryFontOverride()) {
                    m_hasCountryFontOverride = true;
                    m_countryFontOverride = exceptionDlg.getCountryOverrideFont();
                } else {
                    m_hasCountryFontOverride = false;
                }
                m_countryFieldId = exceptionDlg.countryFieldId();
            }
            renderTemplate();
        }
    }

    // Guarantee that the scene and current template match the job type (card vs envelope)
    m_previewingEnvelope = isEnvelope;
    QString targetTemplate = isEnvelope ? getSelectedEnvTemplate() : getSelectedCardTemplate();
    if (targetTemplate.isEmpty()) {
        if (isEnvelope && m_envTemplateList && m_envTemplateList->count() > 0) {
            targetTemplate = m_envTemplateList->item(0)->text();
        } else if (!isEnvelope && m_templateList && m_templateList->count() > 0) {
            targetTemplate = m_templateList->item(0)->text();
        }
    }
    if (!targetTemplate.isEmpty() && m_currentTemplate != targetTemplate) {
        loadTemplate(targetTemplate);
        renderTemplate();
    }

    QPixmap previewPixmap(m_scene->sceneRect().size().toSize() * 2);
    previewPixmap.fill(Qt::white);
    QPainter pixPainter(&previewPixmap);
    pixPainter.setRenderHint(QPainter::Antialiasing);
    m_scene->render(&pixPainter);
    pixPainter.end();

    if (!m_unifiedPrintDlg) {
        m_unifiedPrintDlg = new UnifiedPrintDialog(this);
        connect(m_unifiedPrintDlg, &UnifiedPrintDialog::panicRequested, this, &QslPreviewDialog::onPanicTriggered);
    }

    // Determine the pre-selected printer
    QString selectedPrinter = isEnvelope ? m_comboEnvPrinter->currentData().toString() : m_comboCardPrinter->currentData().toString();
    if (selectedPrinter.isEmpty()) {
        selectedPrinter = isEnvelope ? m_comboEnvPrinter->currentText() : m_comboCardPrinter->currentText();
    }

    bool showDlg = true;
    if (m_batchContinuousPrinting) {
        if (isEnvelope && m_batchConfirmedEnv) {
            showDlg = false;
        } else if (!isEnvelope && m_batchConfirmedCard) {
            showDlg = false;
        }
    }

    if (showDlg) {
        if (!m_unifiedPrintDlg) {
            return false;
        }
        m_unifiedPrintDlg->setupOrientation(
            isEnvelope,
            m_batchPrintingOverflowPhase,
            m_printQueueIndex + 1,
            m_printQueue.size() > 0 ? m_printQueue.size() : 1,
            m_callsign,
            previewPixmap,
            selectedPrinter,
            m_batchContinuousPrinting
        );

        if (!m_unifiedPrintDlg || !m_unifiedPrintDlg->waitForOrientationConfirmation()) {
            return false;
        }

        if (m_batchContinuousPrinting) {
            if (isEnvelope) m_batchConfirmedEnv = true;
            else m_batchConfirmedCard = true;
        }
    }

    if (!m_unifiedPrintDlg) {
        return false;
    }
    selectedPrinter = m_unifiedPrintDlg->getSelectedPrinter();

    // Immediately switch to the active printing view with the animation!
    if (m_unifiedPrintDlg) {
        m_unifiedPrintDlg->setPrintingActive(
            isEnvelope,
            m_printQueueIndex + 1,
            m_printQueue.size() > 0 ? m_printQueue.size() : 1,
            m_callsign,
            previewPixmap
        );
    }

    // Direct synchronous spooling using the designated template; queue advance is managed by printQsl
    spoolToPrinter(isEnvelope, selectedPrinter, previewPixmap, targetTemplate);
    return true;
}

void QslPreviewDialog::spoolToPrinter(bool isEnvelope, QString selectedPrinter, const QPixmap &previewPixmap, const QString &jobTemplate) {
    // Preserve UI template state across print execution
    bool origPreviewEnv = m_previewingEnvelope;
    QString origTemplate = m_currentTemplate;

    m_previewingEnvelope = isEnvelope;
    QString activeJobTemplate = !jobTemplate.isEmpty() ? jobTemplate : (isEnvelope ? getSelectedEnvTemplate() : getSelectedCardTemplate());
    if (activeJobTemplate.isEmpty()) {
        if (isEnvelope && m_envTemplateList && m_envTemplateList->count() > 0) {
            activeJobTemplate = m_envTemplateList->item(0)->text();
        } else if (!isEnvelope && m_templateList && m_templateList->count() > 0) {
            activeJobTemplate = m_templateList->item(0)->text();
        }
    }
    if (!activeJobTemplate.isEmpty() && m_currentTemplate != activeJobTemplate) {
        loadTemplate(activeJobTemplate);
    }
    QPrinter *printerPtr = nullptr;
    if (selectedPrinter == "Print to PDF") {
        printerPtr = new QPrinter(QPrinter::HighResolution);
    } else {
        QPrinterInfo mainPrinterInfo = QPrinterInfo::defaultPrinter();
        if (!selectedPrinter.isEmpty()) {
            QPrinterInfo foundInfo;
            for (const QPrinterInfo &pInfo : QPrinterInfo::availablePrinters()) {
                if (pInfo.printerName().compare(selectedPrinter, Qt::CaseInsensitive) == 0) {
                    foundInfo = pInfo;
                    break;
                }
            }
            if (!foundInfo.isNull()) {
                mainPrinterInfo = foundInfo;
                selectedPrinter = foundInfo.printerName(); // Sync case with system printer name exactly!
            } else {
                QPrinterInfo info = QPrinterInfo::printerInfo(selectedPrinter);
                if (!info.isNull()) {
                    mainPrinterInfo = info;
                }
            }
        }
        printerPtr = new QPrinter(mainPrinterInfo, QPrinter::HighResolution);
    }
    QPrinter &printer = *printerPtr;
    printer.setFullPage(true);

    if (selectedPrinter == "Print to PDF") {
        QString defaultName = m_callsign;
        if (m_printQueue.size() > 1) {
            defaultName = "Batch_" + m_callsign;
        }
        defaultName += isEnvelope ? "_Envelope" : "_Card";
        
        QString fileName;
        if (m_batchContinuousPrinting) {
            static int pdfCounter = 1;
            fileName = QDir::homePath() + "/" + defaultName + QString("_%1.pdf").arg(pdfCounter++);
        } else {
            fileName = QFileDialog::getSaveFileName(this, "Save PDF", defaultName + ".pdf", "PDF Files (*.pdf)");
            if (fileName.isEmpty()) {
                m_previewingEnvelope = origPreviewEnv;
                if (!origTemplate.isEmpty() && m_currentTemplate != origTemplate) {
                    loadTemplate(origTemplate);
                    renderTemplate();
                }
                delete printerPtr;
                return;
            }
        }
        
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
    } else if (!selectedPrinter.isEmpty()) {
        printer.setPrinterName(selectedPrinter);
    }
    
    double finalRotation = 180.0 + m_printCorrection;
    while (finalRotation >= 360.0) finalRotation -= 360.0;
    while (finalRotation <= -360.0) finalRotation += 360.0;
    if (finalRotation < 0.0) finalRotation += 360.0;

    double pageW = m_paperW / 96.0;
    double pageH = m_paperH / 96.0;
    if (finalRotation == 90.0 || finalRotation == 270.0) {
        double temp = pageW;
        pageW = pageH;
        pageH = temp;
    }
    
    bool isPdfOutput = selectedPrinter == "Print to PDF" || selectedPrinter.contains("PDF", Qt::CaseInsensitive);
    if (isPdfOutput) {
        QPageSize customPageSize(QSizeF(pageW, pageH), QPageSize::Inch);
        printer.setPageSize(customPageSize);
    }
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Inch);

    QList<QsoDetail> selected;
    for (int i=0; i<m_contactsTable->rowCount(); ++i) {
        if (m_contactsTable->item(i, 0)->data(Qt::UserRole).toBool()) {
            selected.append(m_qsos[i]);
        }
    }
    if (m_batchPrintingOverflowPhase && !m_savedOriginalTemplate.isEmpty() && !isEnvelope) {
        int normalMax = calculateMaxContactsForCardTemplate(m_savedOriginalTemplate);
        if (selected.size() > normalMax) {
            selected = selected.mid(normalMax);
        } else {
            selected.clear();
        }
    }
    
    for (int i = 0; i < selected.size(); ++i) {
        selected[i] = adjustQsoToEastern(selected[i]);
    }
    
    int maxContactsPerCard = calculateMaxContactsForCardTemplate(m_currentTemplate);
    
    int totalCards = qMax(1, (selected.size() + maxContactsPerCard - 1) / maxContactsPerCard);
    if (isEnvelope) {
        totalCards = 1;
    }
    int originalIndex = m_currentCardIndex;
    
    QPainter *painter = nullptr;
    QPrinter *currentPrinter = &printer;
    
    if (isPdfOutput) {
        painter = new QPainter();
        if (!painter->begin(currentPrinter)) {
            m_previewingEnvelope = origPreviewEnv;
            if (!origTemplate.isEmpty() && m_currentTemplate != origTemplate) {
                loadTemplate(origTemplate);
                renderTemplate();
            }
            delete painter;
            delete printerPtr;
            return;
        }
    }
    
    for (int page = 0; page < totalCards; ++page) {
        if (!m_userDeclinedOverflow && isOverflowSupportActive() && !isEnvelope) {
            if (!m_batchPrintingOverflowPhase) {
                if (page > 0) {
                    if (m_printQueueIndex >= 0 && m_printQueueIndex < m_printQueue.size()) {
                        QString callWithMode = m_printQueue[m_printQueueIndex];
                        if (!m_overflowQueue.contains(callWithMode)) {
                            m_overflowQueue.append(callWithMode);
                        }
                    }
                    continue;
                }
            }
        }
        
        if (!isPdfOutput) {
            QPrinterInfo info = QPrinterInfo::defaultPrinter();
            if (!selectedPrinter.isEmpty()) {
                QPrinterInfo foundInfo;
                for (const QPrinterInfo &pInfo : QPrinterInfo::availablePrinters()) {
                    if (pInfo.printerName().compare(selectedPrinter, Qt::CaseInsensitive) == 0) {
                        foundInfo = pInfo;
                        break;
                    }
                }
                if (!foundInfo.isNull()) {
                    info = foundInfo;
                } else {
                    QPrinterInfo targetInfo = QPrinterInfo::printerInfo(selectedPrinter);
                    if (!targetInfo.isNull()) {
                        info = targetInfo;
                    }
                }
            }
            currentPrinter = new QPrinter(info, QPrinter::HighResolution);
            currentPrinter->setFullPage(true);
            if (!selectedPrinter.isEmpty()) {
                currentPrinter->setPrinterName(info.printerName());
            }
            // Set unique name so CUPS processes them as completely independent jobs
            currentPrinter->setDocName(QString("QWAIN_%1_%2_Page_%3").arg(m_callsign).arg(isEnvelope ? "Env" : "Card").arg(page + 1));
            
            if (isPdfOutput) {
                QPageSize customPageSize(QSizeF(pageW, pageH), QPageSize::Inch);
                currentPrinter->setPageSize(customPageSize);
            }
            currentPrinter->setPageOrientation(QPageLayout::Portrait);
            currentPrinter->setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Inch);
            
            painter = new QPainter();
            if (!painter->begin(currentPrinter)) {
                delete painter;
                delete currentPrinter;
                currentPrinter = nullptr;
                if (totalCards == 1) {
                    delete printerPtr;
                    return;
                }
            }
        }

        m_scene->setProperty("isPrinting", true); // Must be true before renderTemplate so it knows not to reset index
        m_currentCardIndex = page;
        renderTemplate();
        
        // Strict WYSIWYG Print Rendering:
        // The template's defined bounds (0, 0, m_paperW, m_paperH) are the definitive frame of reference.
        // We do not expand the envelope based on overflowing items (which would shift or shrink the entire template).
        // Instead, the source rect is strictly the template dimensions, and painter clipping is enforced on targetRect.
        QRectF templateSourceRect(0, 0, m_paperW, m_paperH);
        
        m_scene->setProperty("isPrinting", true);
        QList<QGraphicsItem*> tempHidden;
        bool isPdf = selectedPrinter == "Print to PDF" || selectedPrinter.contains("PDF", Qt::CaseInsensitive);
        if (!isPdf) {
            for (QGraphicsItem *item : m_scene->items()) {
                if (item->zValue() < 0) {
                    item->setVisible(false);
                    tempHidden.append(item);
                }
            }
        }
        
        double scale = currentPrinter->resolution() / 96.0;
        double physicalW = m_paperW * scale;
        double physicalH = m_paperH * scale;
        
        QRectF pageRect = currentPrinter->pageLayout().paintRectPixels(currentPrinter->resolution());
        
        double finalRotation = 180.0 + m_printCorrection;
        while (finalRotation >= 360.0) finalRotation -= 360.0;
        while (finalRotation <= -360.0) finalRotation += 360.0;
        if (finalRotation < 0.0) finalRotation += 360.0;

        double renderW = physicalW;
        double renderH = physicalH;
        if (finalRotation == 90.0 || finalRotation == 270.0) {
            double temp = renderW;
            renderW = renderH;
            renderH = temp;
        }

        double leftShift = (m_chkUsePrintTweaks && m_chkUsePrintTweaks->isChecked()) ? m_leftMarginSpin->value() : 0.0;
        double topShift = (m_chkUsePrintTweaks && m_chkUsePrintTweaks->isChecked()) ? m_topMarginSpin->value() : 0.0;
        double startX = pageRect.left() + (pageRect.width() - renderW) / 2.0 + ((leftShift + m_printOffsetX) * currentPrinter->resolution());
        double startY = pageRect.top() + ((topShift + m_printOffsetY) * currentPrinter->resolution());
        
        {
            QFile dbgFile("/home/w4djf/Projects/adif_gui/print_debug.txt");
            if (dbgFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                QTextStream out(&dbgFile);
                out << "========================================\n"
                    << "Job Page: " << page << "\n"
                    << "Printer Name: " << currentPrinter->printerName() << "\n"
                    << "Resolution (DPI): " << currentPrinter->resolution() << "\n"
                    << "Full Page Mode Enabled: " << (currentPrinter->fullPage() ? "YES" : "NO") << "\n"
                    << "Page Margins (left/top): " << currentPrinter->pageLayout().margins().left() << ", " << currentPrinter->pageLayout().margins().top() << "\n"
                    << "Unprintable Paint Rect: left=" << pageRect.left() << ", top=" << pageRect.top() << ", width=" << pageRect.width() << ", height=" << pageRect.height() << "\n"
                    << "Computed Render Dimensions: renderW=" << renderW << ", renderH=" << renderH << "\n"
                    << "StartX/Y Position (Absolute Paper Corner): " << startX << ", " << startY << "\n"
                    << "Original Template Sizing (PaperW/H): " << m_paperW << ", " << m_paperH << "\n"
                    << "Active Spinbox Tweaks: TopSpin=" << m_topMarginSpin->value() << ", LeftSpin=" << m_leftMarginSpin->value() << "\n"
                    << "Stored Print Offsets: PrintOffsetX/Y=" << m_printOffsetX << ", " << m_printOffsetY << "\n"
                    << "========================================\n\n";
            }
        }
        
        painter->save();
        painter->translate(startX + renderW / 2.0, startY + renderH / 2.0);
        painter->rotate(finalRotation);
        
        QRectF targetRect;
        if (finalRotation == 90.0 || finalRotation == 270.0) {
             targetRect = QRectF(-renderH / 2.0, -renderW / 2.0, renderH, renderW);
        } else {
             targetRect = QRectF(-renderW / 2.0, -renderH / 2.0, renderW, renderH);
        }
        // Strict WYSIWYG clipping: Clip to the target card rectangle on paper so any elements
        // protruding past the card edges are clipped cleanly without displacing or scaling the template.
        painter->setClipRect(targetRect);
        m_scene->render(painter, targetRect, templateSourceRect);
        
        QSettings printSettings("Qwain", "Qwain");
        printSettings.setValue("Stats/TotalTemplatesPrinted", printSettings.value("Stats/TotalTemplatesPrinted", 0).toInt() + 1);

        painter->restore();
        
        for (QGraphicsItem *item : tempHidden) {
            item->setVisible(true);
        }
        m_scene->setProperty("isPrinting", false);
        
        if (isPdfOutput) {
            if (page < totalCards - 1) {
                currentPrinter->newPage();
            }
        } else {
            if (painter) {
                painter->end();
                delete painter;
                painter = nullptr;
            }
            if (currentPrinter && currentPrinter != &printer) {
                delete currentPrinter;
                currentPrinter = nullptr;
            }
        }
    }
    
    if (isPdfOutput && painter) {
        painter->end();
        delete painter;
        painter = nullptr;
    }
    
    if (!isEnvelope) {
        QList<QsoDetail> selected;
        for (int i=0; i<m_contactsTable->rowCount(); ++i) {
            if (m_contactsTable->item(i, 0)->data(Qt::UserRole).toBool()) {
                selected.append(m_qsos[i]);
            }
        }
        
        if (m_batchPrintingOverflowPhase && !m_savedOriginalTemplate.isEmpty() && !isEnvelope) {
            int normalMax = calculateMaxContactsForCardTemplate(m_savedOriginalTemplate);
            if (selected.size() > normalMax) {
                selected = selected.mid(normalMax);
            } else {
                selected.clear();
            }
        }
        
        int contactsToUpdate = selected.size();
        if (isOverflowSupportActive() && !isEnvelope && !m_batchPrintingOverflowPhase) {
            if (contactsToUpdate > maxContactsPerCard) {
                contactsToUpdate = maxContactsPerCard;
            }
        }
        while (selected.size() > contactsToUpdate) {
            selected.removeLast();
        }
        
        // Load selected radio and antenna from defined card template checkboxes (m_customChkBoxStates)
        QStringList checkedRadios;
        QStringList checkedAntennas;
        for (auto it = m_customChkBoxStates.begin(); it != m_customChkBoxStates.end(); ++it) {
            if (it.value()) {
                QString idStr = it.key();
                if (idStr.startsWith("CHK_RADIOS:")) {
                    checkedRadios.append(idStr.mid(11));
                } else if (idStr.startsWith("CHK_ANTENNAS:")) {
                    checkedAntennas.append(idStr.mid(13));
                }
            }
        }
        QString selectedRadio = checkedRadios.isEmpty() ? "None" : checkedRadios.join(", ");
        QString selectedAntenna = checkedAntennas.isEmpty() ? "None" : checkedAntennas.join(", ");

        // Load settings for Log Tweaks
        QSettings tweaksSettings(QCoreApplication::organizationName(), "Qwain");
        tweaksSettings.beginGroup("LogTweaker");
        bool useTweaks = tweaksSettings.value("useTweaks", false).toBool();
        bool useTweaks_FT2 = tweaksSettings.value("useTweaks_FT2", true).toBool();
        bool useTweaks_FT4 = tweaksSettings.value("useTweaks_FT4", true).toBool();
        bool useTweaks_FT8 = tweaksSettings.value("useTweaks_FT8", true).toBool();
        QMap<QString, QPair<int, int>> tweaksMinMax;
        if (useTweaks) {
            QStringList modes = {"FT2", "FT4", "FT8", "SSB"};
            for (const QString &mode : modes) {
                tweaksMinMax[mode] = qMakePair(tweaksSettings.value(mode + "_min", 10).toInt(), tweaksSettings.value(mode + "_max", 100).toInt());
            }
        }
        tweaksSettings.endGroup();

        auto getTweakPwrForUpdate = [&](const QsoDetail &q) -> QString {
            if (!useTweaks) return "";
            if (!q.tx_pwr.isEmpty() && q.tx_pwr.trimmed().toLower() != "n/a" && q.tx_pwr.trimmed() != "0") {
                return q.tx_pwr.trimmed();
            }
            QString modeKey = q.mode.toUpper();
            if (modeKey == "FT2" && !useTweaks_FT2) return "";
            if (modeKey == "FT4" && !useTweaks_FT4) return "";
            if (modeKey == "FT8" && !useTweaks_FT8) return "";
            if (modeKey == "J3E" || modeKey == "LSB" || modeKey == "USB" || modeKey == "SSB") modeKey = "SSB";
            if (tweaksMinMax.contains(modeKey)) {
                int minPwr = tweaksMinMax[modeKey].first;
                int maxPwr = tweaksMinMax[modeKey].second;
                if (minPwr > maxPwr) qSwap(minPwr, maxPwr);
                uint seed = qHash(q.date + q.time + q.band + q.mode);
                QRandomGenerator rand(seed);
                int steps = (maxPwr - minPwr) / 5;
                if (steps < 0) steps = 0;
                int val = minPwr + (rand.bounded(steps + 1) * 5);
                return QString::number(val);
            }
            return "";
        };

        QString today = QDateTime::currentDateTime().toString("yyyyMMdd");
        for (int i = 0; i < selected.size(); ++i) {
            QString powerVal = "n/a";
            if (useTweaks) {
                QString computedPwr = getTweakPwrForUpdate(selected[i]);
                if (!computedPwr.isEmpty()) {
                    powerVal = computedPwr;
                    selected[i].tx_pwr = computedPwr;
                }
            } else if (!selected[i].tx_pwr.isEmpty()) {
                powerVal = selected[i].tx_pwr;
            }
            selected[i].notes = QString("Transmitter: %1 Antenna: %2 Tx Power: %3")
                                    .arg(selectedRadio)
                                    .arg(selectedAntenna)
                                    .arg(powerVal);
            selected[i].qsl_sent = "Y";
            selected[i].qslsdate = today;
        }

        // Update in-memory m_qsos
        for (const auto &sel : selected) {
            for (auto &q : m_qsos) {
                if (q.call == sel.call && q.date == sel.date && q.time == sel.time) {
                    q.qsl_sent = "Y";
                    q.qslsdate = today;
                    if (!sel.tx_pwr.isEmpty()) q.tx_pwr = sel.tx_pwr;
                    if (!sel.notes.isEmpty()) q.notes = sel.notes;
                }
            }
        }
        
        MainWindow *mw = nullptr;
        QWidget *pw = parentWidget();
        while (pw) {
            mw = qobject_cast<MainWindow*>(pw);
            if (mw) break;
            pw = pw->parentWidget();
        }
        if (mw) mw->updateLocalAdifQslStatus(selected);

        if (m_chkQRZ && m_chkQRZ->isChecked()) {
            updateQrzLogForSelected(selected);
        }
    }
    
    m_currentCardIndex = originalIndex;
    m_previewingEnvelope = origPreviewEnv;
    if (!origTemplate.isEmpty() && m_currentTemplate != origTemplate) {
        loadTemplate(origTemplate);
    }
    renderTemplate();

    if (!isEnvelope) {
        m_totalCardsSentToPrinter += totalCards;
        if (totalCards >= 2) {
            m_anyCardJobUsedMultipleCards = true;
        }
    }
    delete printerPtr;
}

void QslPreviewDialog::refreshContactsTable(const QString &mode) {
    m_contactsTable->setRowCount(m_qsos.size());
    int unprintedCount = 0;
    for (const auto &qso : m_qsos) {
        if (qso.qslsdate.isEmpty()) {
            unprintedCount++;
        }
    }
    for (int i=0; i<m_qsos.size(); ++i) {
        QsoDetail adjustedQso = adjustQsoToEastern(m_qsos[i]);
        bool checked = (mode == "all") || (mode == "reprint") || 
                       (mode == "unprinted" && m_qsos[i].qslsdate.isEmpty()) || 
                       (mode == "prompt" && m_qsos[i].qslsdate.isEmpty()) ||
                       (unprintedCount == 0);
        QTableWidgetItem *chkItem = new QTableWidgetItem(checked ? "✔" : "");
        chkItem->setForeground(QBrush(Qt::green));
        chkItem->setData(Qt::UserRole, checked);
        chkItem->setTextAlignment(Qt::AlignCenter);
        m_contactsTable->setItem(i, 0, chkItem);
        
        m_contactsTable->setItem(i, 1, new QTableWidgetItem(QDate::fromString(adjustedQso.date, "yyyyMMdd").toString("dd-MM-yyyy")));
        QString formattedTime = adjustedQso.time.left(4); // "1430"
        if (formattedTime.length() == 4) formattedTime.insert(2, ":"); // "14:30"
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        QString tzAbbr = settings.value("UI/alwaysEastern", false).toBool() ? "et" : "utc";
        m_contactsTable->setItem(i, 2, new QTableWidgetItem(formattedTime + " " + tzAbbr));
        m_contactsTable->setItem(i, 3, new QTableWidgetItem(adjustedQso.band));
        m_contactsTable->setItem(i, 4, new QTableWidgetItem(adjustedQso.mode));
    }
    updatePreview();
}

void QslPreviewDialog::onLogfileClicked(const QString &) {
    QStringList files = QFileDialog::getOpenFileNames(this, "Select Logfile(s)", "", "ADIF Files (*.adi *.adif);;All Files (*)");
    
    QString combinedLogName;
    if (files.isEmpty()) {
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        QString defLog = settings.value("Files/logfile").toString();
        if (defLog.isEmpty()) return;
        files = defLog.split(";", Qt::SkipEmptyParts);
        
        if (files.size() == 1) {
            combinedLogName = QFileInfo(files.first()).fileName();
        } else {
            combinedLogName = QString("%1 and %2 others").arg(QFileInfo(files.first()).fileName()).arg(files.size() - 1);
        }
    } else if (files.size() == 1) {
        QFileInfo fi(files.first());
        combinedLogName = fi.fileName();
    } else {
        combinedLogName = QString("%1 and %2 others").arg(QFileInfo(files.first()).fileName()).arg(files.size() - 1);
        
        QString tempPath = QDir::tempPath() + "/qwain_merged.adi";
        QFile tempOut(tempPath);
        if (tempOut.open(QIODevice::WriteOnly | QIODevice::Text)) {
            for (const QString &fpath : files) {
                QFile f(fpath);
                if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    tempOut.write(f.readAll());
                    tempOut.write("\n");
                }
            }
            tempOut.close();
        }
    }
    
    QList<QsoDetail> matchedQsos;
    QRegularExpression eorRe("<eor>", QRegularExpression::CaseInsensitiveOption);
    
    // Parse through all files to find QSOs matching m_callsign
    for (const QString &fpath : files) {
        QFile file(fpath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QString content = file.readAll();
        file.close();
        
        QStringList records = content.split(eorRe);
        for (const QString &rec : records) {
            auto extractField = [](const QString &r, const QString &field) -> QString {
                QRegularExpression fieldRe(QString("<%1:(\\d+)[^>]*>").arg(field), QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch match = fieldRe.match(r);
                if (match.hasMatch()) {
                    int len = match.captured(1).toInt();
                    int offset = match.capturedEnd();
                    return r.mid(offset, len).trimmed();
                }
                return QString();
            };
            
            QString call = extractField(rec, "CALL").toUpper();
            if (m_callsign.isEmpty() || call == m_callsign.toUpper()) {
                QsoDetail d;
                d.date = extractField(rec, "QSO_DATE");
                d.time = extractField(rec, "TIME_ON");
                d.band = extractField(rec, "BAND").toUpper();
                d.mode = extractField(rec, "MODE").toUpper();
                d.call = call;
                d.country = extractField(rec, "COUNTRY").toUpper();
                d.state = extractField(rec, "STATE").toUpper();
                d.dxcc = extractField(rec, "DXCC");
                d.grid = extractField(rec, "GRIDSQUARE").toUpper();
                d.rst_rcvd = extractField(rec, "RST_RCVD");
                d.rst_sent = extractField(rec, "RST_SENT");
                d.tx_pwr = extractField(rec, "TX_PWR");
                d.qslsdate = extractField(rec, "QSLSDATE");
                d.qslrdate = extractField(rec, "QSLRDATE");
                d.qsl_sent = extractField(rec, "QSL_SENT");
                d.qsl_rcvd = extractField(rec, "QSL_RCVD");

                QString lotwRx = extractField(rec, "APP_LOTW_RXQSL");
                d.lotwRcvd = extractField(rec, "APP_LOTW_QSO_RCVD").toUpper() == "Y" || extractField(rec, "LOTW_QSL_RCVD").toUpper() == "Y" || (!lotwRx.isEmpty() && lotwRx != "00000000");
                d.mqslRcvd = extractField(rec, "QSL_RCVD").toUpper() == "Y";
                d.qrzRcvd = extractField(rec, "APP_QRZLOG_STATUS").toUpper() == "C" || extractField(rec, "QRZCOM_QSL_RCVD").toUpper() == "Y" || extractField(rec, "QRZCOM_QSL_RCVD").toUpper() == "C" || extractField(rec, "APP_QRZ_QSO_RCVD").toUpper() == "Y";
                d.cardRcvd = (d.qsl_rcvd.toUpper() == "Y" || !d.qslrdate.isEmpty());

                d.name = extractField(rec, "NAME");
                d.address = extractField(rec, "ADDRESS");
                if (d.address.isEmpty()) d.address = extractField(rec, "ADDR1");
                if (d.address.isEmpty()) d.address = extractField(rec, "ADDR2");
                d.city = extractField(rec, "CITY");
                d.zip = extractField(rec, "ZIP");
                if (d.zip.isEmpty()) d.zip = extractField(rec, "POSTAL_CODE");
                
                QString freqStr = extractField(rec, "FREQ");
                if (!freqStr.isEmpty()) {
                    bool ok;
                    double f = freqStr.toDouble(&ok);
                    if (ok) d.freq = QString::number(f, 'f', 4);
                    else d.freq = freqStr;
                }
                
                QString potaRef = extractField(rec, "POTA_REF").toUpper();
                QString sig = extractField(rec, "SIG").toUpper();
                QString sigInfo = extractField(rec, "SIG_INFO").toUpper();
                if (!potaRef.isEmpty()) d.pota = potaRef;
                else if (sig == "POTA" && !sigInfo.isEmpty()) d.pota = sigInfo;
                
                matchedQsos.append(d);
            }
        }
    }
    
    if (matchedQsos.isEmpty()) {
        QMessageBox::information(this, "No QSOs Found", "No QSOs with " + m_callsign + " found in the selected logfile(s).");
    } else {
        m_qsos = matchedQsos;
        refreshContactsTable();
        if (m_lblLogfile) {
            m_lblLogfile->setText(QString("<b>Using Logfile(s):</b> <a href='#'>%1</a>").arg(combinedLogName));
        }
    }
}

void QslPreviewDialog::updateQrzLogForSelected(const QList<QsoDetail> &selectedList)
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString apiKey = settings.value("Credentials/qrzApiKey", "").toString();
    
    if (apiKey.isEmpty()) {
        QMessageBox::warning(this, "QRZ Error", "No QRZ API Key configured in Settings under QRZ Credentials.");
        return;
    }
    
    QList<QsoDetail> selected = selectedList;
    if (selected.isEmpty()) {
        for (int i=0; i<m_contactsTable->rowCount(); ++i) {
            if (m_contactsTable->item(i, 0)->data(Qt::UserRole).toBool()) {
                selected.append(m_qsos[i]);
            }
        }
    }
    
    if (selected.isEmpty()) {
        QMessageBox::information(this, "No selection", "No contacts selected to update.");
        return;
    }
    
    if (!m_netManager) {
        m_netManager = new QNetworkAccessManager(this);
    }
    
    QString today = QDateTime::currentDateTime().toString("yyyyMMdd");
    
    for (const QsoDetail &d : selected) {
        QString adif = "QRZLogbook Update from Qwain\n<eoh>\n";
        adif += QString("<CALL:%1>%2 ")
                .arg(d.call.length()).arg(d.call);
        QString dateVal = d.date; dateVal.replace("-", ""); dateVal.replace("/", "");
        adif += QString("<QSO_DATE:%1>%2 ")
                .arg(dateVal.length()).arg(dateVal);
        QString timeVal = d.time; timeVal.replace(":", "");
        if (timeVal.length() > 4 && !timeVal.contains(QRegularExpression("[^0-9]"))) {
            timeVal = timeVal.left(4); // Keep to 4 length HHMM
        } else if (timeVal.isEmpty()) {
            timeVal = "0000";
        }
        adif += QString("<TIME_ON:%1>%2 ")
                .arg(timeVal.length()).arg(timeVal);
        adif += QString("<BAND:%1>%2 ")
                .arg(d.band.length()).arg(d.band);
        adif += QString("<MODE:%1>%2 ")
                .arg(d.mode.length()).arg(d.mode);
        if (!d.tx_pwr.isEmpty()) {
            adif += QString("<TX_PWR:%1>%2 ")
                    .arg(d.tx_pwr.length()).arg(d.tx_pwr);
        }
        if (!d.rst_sent.isEmpty()) {
            adif += QString("<RST_SENT:%1>%2 ")
                    .arg(d.rst_sent.length()).arg(d.rst_sent);
        }
        if (!d.rst_rcvd.isEmpty()) {
            adif += QString("<RST_RCVD:%1>%2 ")
                    .arg(d.rst_rcvd.length()).arg(d.rst_rcvd);
        }
        if (!d.notes.isEmpty()) {
            adif += QString("<NOTES:%1>%2 ")
                    .arg(d.notes.length()).arg(d.notes);
        }
        adif += QString("<QSL_SENT:1>Y <QSLSDATE:%1>%2 <EOR>\n")
                .arg(today.length()).arg(today);
                
        QUrl url("https://logbook.qrz.com/api");
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        
        QUrlQuery postData;
        postData.addQueryItem("KEY", apiKey);
        postData.addQueryItem("ACTION", "INSERT");
        // Replace is fine, it will overwrite the record if found by exact match 
        postData.addQueryItem("OPTION", "REPLACE");
        postData.addQueryItem("ADIF", adif);
        
        QNetworkReply *reply = m_netManager->post(req, postData.toString(QUrl::FullyEncoded).toUtf8());
        connect(reply, &QNetworkReply::finished, this, &QslPreviewDialog::onQrzLogUpdateReply);
    }
}

void QslPreviewDialog::onQrzLogUpdateReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    reply->deleteLater();
}

QString QslPreviewDialog::getSelectedCardTemplate() const {
    if (m_templateList && m_templateList->currentItem()) return m_templateList->currentItem()->text();
    return "";
}

QString QslPreviewDialog::getSelectedEnvTemplate() const {
    if (m_envTemplateList && m_envTemplateList->currentItem()) return m_envTemplateList->currentItem()->text();
    return "";
}

bool QslPreviewDialog::getPrintCard() const { return m_chkPrintCard && m_chkPrintCard->isChecked(); }
bool QslPreviewDialog::getPrintEnvelope() const { return m_chkPrintEnvelope && m_chkPrintEnvelope->isChecked(); }

void QslPreviewDialog::setSelectedOptions(const QString &cardTemp, const QString &envTemp, bool pCard, bool pEnv) {
    if (m_chkPrintCard) m_chkPrintCard->setChecked(pCard);
    if (m_chkPrintEnvelope) m_chkPrintEnvelope->setChecked(pEnv);
    
    if (m_templateList) {
        auto items = m_templateList->findItems(cardTemp, Qt::MatchExactly);
        if (!items.isEmpty()) m_templateList->setCurrentItem(items.first());
    }
    if (m_envTemplateList) {
        auto items = m_envTemplateList->findItems(envTemp, Qt::MatchExactly);
        if (!items.isEmpty()) m_envTemplateList->setCurrentItem(items.first());
    }
}

void QslPreviewDialog::setPrintQueue(const QStringList &queue, MainWindow *mainWnd) {
    m_printQueue = queue;
    m_mainWnd = mainWnd;
    m_printQueueIndex = 0;
    updateSendToPrinterButtonText();
}

QStringList QslPreviewDialog::getRemainingQueue() const {
    if (m_printQueue.isEmpty()) return QStringList();
    
    QStringList remaining;
    int startIndex = m_currentItemPrinted ? m_printQueueIndex + 1 : m_printQueueIndex;
    for (int i = startIndex; i < m_printQueue.size(); ++i) {
        remaining << m_printQueue[i];
    }
    return remaining;
}

QVariantMap QslPreviewDialog::getPreviewState() const {
    return QVariantMap(); // Not needed
}

void QslPreviewDialog::doBatchPrint() {
}

void QslPreviewDialog::refreshAddressData(const QString& callsign) {
    m_addressManuallyAccepted = false;
    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);
    qrzCache.beginGroup(callsign);
    
    m_addrName.clear();
    m_addrStreet.clear();
    m_addrCity.clear();
    m_addrCountry.clear();
    
    // Check if we have explicit QRZ address fields
    if (qrzCache.contains("qrz_fname") || qrzCache.contains("qrz_name")) {
        QString fname = qrzCache.value("qrz_fname").toString();
        QString lname = qrzCache.value("qrz_name").toString();
        m_addrName = fname;
        if (!m_addrName.isEmpty() && !lname.isEmpty()) m_addrName += " ";
        m_addrName += lname;
        
        QString adifName = qrzCache.value("name").toString();
        if (fname.isEmpty() && !adifName.isEmpty() && adifName.contains(' ') && !lname.contains(' ')) {
            m_addrName = adifName;
        }
        
        m_addrStreet = qrzCache.value("qrz_addr1").toString();
        
        QString city = qrzCache.value("qrz_city").toString();
        if (city.isEmpty()) city = qrzCache.value("qrz_addr2").toString();
        QString state = qrzCache.value("qrz_state").toString();
        QString zip = qrzCache.value("qrz_zip").toString();
        
        QString cityState = (!city.isEmpty() && !state.isEmpty()) ? QString("%1, %2").arg(city, state) : city + state;
        if (!zip.isEmpty()) cityState += " " + zip;
        m_addrCity = cityState;
        
        m_addrCountry = qrzCache.value("qrz_country").toString();
    }
    
    // Fallback if we don't have street/city address from the explicit fields but have html
    if (m_addrStreet.isEmpty() && m_addrCity.isEmpty()) {
        QString infoHtml = qrzCache.value("htmlInfo").toString();
        if (infoHtml.isEmpty()) {
            infoHtml = m_infoHtml;
        }
        
        if (!infoHtml.isEmpty()) {
            QTextBrowser tempBrowser;
            tempBrowser.setHtml(infoHtml);
            QString plainInfo = tempBrowser.toPlainText();
            
            auto extractPlain = [&](const QString& label) {
                QStringList lines = plainInfo.split('\n');
                for (int i = 0; i < lines.size(); ++i) {
                    QString line = lines[i].trimmed();
                    if (line.startsWith(label + ":", Qt::CaseInsensitive)) {
                        QString val = line.mid(label.length() + 1).trimmed();
                        if (val.isEmpty() && i + 1 < lines.size()) {
                            val = lines[i + 1].trimmed();
                        }
                        return val;
                    }
                }
                return QString();
            };
            
            if (m_addrName.isEmpty()) m_addrName = extractPlain("Name");
            m_addrStreet = extractPlain("Address");
            m_addrCity = extractPlain("City");
            m_addrCountry = extractPlain("Country");
        }
    }
    qrzCache.endGroup();
    
    // Fallback to ADIF log records if address details are missing in QRZ cache
    for (const QsoDetail &q : m_qsos) {
        if (q.call.toUpper() == callsign.toUpper()) {
            if (m_addrName.isEmpty() && !q.name.isEmpty()) m_addrName = q.name;
            if (m_addrStreet.isEmpty() && !q.address.isEmpty()) m_addrStreet = q.address;
            if (m_addrCity.isEmpty()) {
                QString cityStr = q.city;
                if (!q.state.isEmpty()) {
                    cityStr += (!cityStr.isEmpty() ? ", " : "") + q.state;
                }
                if (!q.zip.isEmpty()) {
                    cityStr += (!cityStr.isEmpty() ? " " : "") + q.zip;
                }
                if (!cityStr.isEmpty()) m_addrCity = cityStr;
            }
            if (m_addrCountry.isEmpty() && !q.country.isEmpty()) m_addrCountry = q.country;
            break;
        }
    }

    m_addrName = formatNameOrCity(m_addrName);
    m_addrStreet = formatAddress(m_addrStreet);
    m_addrCity = formatCity(m_addrCity);
    m_addrCountry = formatCountry(m_addrCountry);
}

int QslPreviewDialog::calculateMaxContactsForCardTemplate(const QString &cardTemplateName) {
    QString cleanName = cardTemplateName;
    if (cleanName.startsWith("🏷️ ")) cleanName = cleanName.mid(3).trimmed();
    if (cleanName.isEmpty()) return 1;

    // Check in-memory template layout first if this is the currently loaded card template
    if (!m_previewingEnvelope && (cleanName == m_currentTemplate || cardTemplateName == m_currentTemplate) && !m_templateItems.isEmpty()) {
        int inMemMax = 0;
        for (const QVariantMap &m : m_templateItems) {
            int rows = m["rows"].toInt();
            int cols = m["cols"].toInt();
            if (cols >= 3 && rows > 1) {
                QStringList cellData = m["cellData"].toStringList();
                bool firstRowHasTags = false;
                for (int c = 0; c < cols; ++c) {
                    if (c < cellData.size()) {
                        QString def = cellData[c].toLower().trimmed();
                        if (def.startsWith("<") && def.endsWith(">")) {
                            firstRowHasTags = true;
                            break;
                        }
                    }
                }
                int tableCapacity = firstRowHasTags ? rows : (rows - 1);
                inMemMax += tableCapacity;
            }
        }
        if (inMemMax > 0) return inMemMax;
    }
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");
    settings.beginGroup(cleanName);
    
    int maxContactsPerCard = 0;
    int count = settings.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        settings.beginGroup(QString("Area_%1").arg(i));
        int rows = settings.value("rows", 0).toInt();
        int cols = settings.value("cols", 0).toInt();
        if (cols >= 3 && rows > 1) {
            int cdCount = settings.beginReadArray("CellData");
            QStringList cellData;
            for (int j = 0; j < cdCount; ++j) {
                settings.setArrayIndex(j);
                cellData.append(settings.value("data").toString());
            }
            settings.endArray();
            
            bool firstRowHasTags = false;
            for (int c = 0; c < cols; ++c) {
                if (c < cellData.size()) {
                    QString def = cellData[c].toLower().trimmed();
                    if (def.startsWith("<") && def.endsWith(">")) {
                        firstRowHasTags = true;
                        break;
                    }
                }
            }
            int tableCapacity = firstRowHasTags ? rows : (rows - 1);
            maxContactsPerCard += tableCapacity;
        }
        settings.endGroup();
    }
    settings.endGroup();
    settings.endGroup();
    
    if (maxContactsPerCard < 1) maxContactsPerCard = 1;
    return maxContactsPerCard;
}

void QslPreviewDialog::updateSendToPrinterButtonText() {
    int maxContactsPerCard = 0;
    if (!m_previewingEnvelope) {
        for (const QVariantMap &m : m_templateItems) {
            int rows = m["rows"].toInt();
            int cols = m["cols"].toInt();
            if (cols >= 3 && rows > 1) {
                QStringList cellData = m["cellData"].toStringList();
                bool firstRowHasTags = false;
                for (int c = 0; c < cols; ++c) {
                    if (c < cellData.size()) {
                        QString def = cellData[c].toLower().trimmed();
                        if (def.startsWith("<") && def.endsWith(">")) {
                            firstRowHasTags = true;
                            break;
                        }
                    }
                }
                int tableCapacity = firstRowHasTags ? rows : (rows - 1);
                maxContactsPerCard += tableCapacity;
            }
        }
    } else {
        maxContactsPerCard = calculateMaxContactsForCardTemplate(getSelectedCardTemplate());
    }
    if (maxContactsPerCard < 1) maxContactsPerCard = 1;

    int totalCardsNeeded = 0;
    int totalEnvelopesNeeded = 0;

    bool printCard = m_chkPrintCard && m_chkPrintCard->isChecked();
    bool printEnvelope = m_chkPrintEnvelope && m_chkPrintEnvelope->isChecked();

    // 1. Scan to see if any print job in the queue requires overflow (needs > maxContactsPerCard)
    bool needsOverflow = false;
    if (m_printQueue.size() > 1) {
        for (int qIdx = 0; qIdx < m_printQueue.size(); ++qIdx) {
            QString item = m_printQueue[qIdx];
            QString call = item.split("|")[0].trimmed().toUpper();
            QString mode = item.contains("|") ? item.split("|")[1].trimmed() : "unprinted";
            
            int checkedCount = 0;
            if (qIdx == m_printQueueIndex && m_contactsTable) {
                for (int i = 0; i < m_contactsTable->rowCount(); ++i) {
                    if (m_contactsTable->item(i, 0) && m_contactsTable->item(i, 0)->data(Qt::UserRole).toBool()) {
                        checkedCount++;
                    }
                }
            } else {
                QList<QsoDetail> qsos = loadQsosFromDefinedLogfiles(call);
                if (qsos.isEmpty()) {
                    if (m_mainWnd) {
                        qsos = m_mainWnd->allQsos()[call];
                    } else if (call == m_callsign) {
                        qsos = m_qsos;
                    }
                }
                
                int unprintedCount = 0;
                for (const auto &qso : qsos) {
                    if (qso.qslsdate.isEmpty()) {
                        unprintedCount++;
                    }
                }
                for (const auto &qso : qsos) {
                    bool checked = (mode == "all") || (mode == "reprint") || 
                                   ((mode == "unprinted" || mode == "prompt") && qso.qslsdate.isEmpty()) ||
                                   (unprintedCount == 0);
                    if (checked) {
                        checkedCount++;
                    }
                }
            }
            if (checkedCount > maxContactsPerCard) {
                needsOverflow = true;
                break;
            }
        }
    } else {
        int checkedCount = 0;
        if (m_contactsTable) {
            for (int i = 0; i < m_contactsTable->rowCount(); ++i) {
                if (m_contactsTable->item(i, 0) && m_contactsTable->item(i, 0)->data(Qt::UserRole).toBool()) {
                    checkedCount++;
                }
            }
        } else {
            checkedCount = m_qsos.size();
        }
        if (checkedCount > maxContactsPerCard) {
            needsOverflow = true;
        }
    }

    // 3. Determine Multi-QSO template capacity for estimations
    int multiCapacity = 10;
    if (isOverflowSupportActive()) {
        QString overflowTemplate = getAssociatedOverflowTemplate();
        if (!overflowTemplate.isEmpty()) {
            multiCapacity = calculateMaxContactsForCardTemplate(overflowTemplate);
        }
    }
    if (multiCapacity < 1) multiCapacity = 10;

    // 4. Calculate actual card and envelope requirements
    if (m_printQueue.size() > 1) {
        for (int qIdx = 0; qIdx < m_printQueue.size(); ++qIdx) {
            QString item = m_printQueue[qIdx];
            QString call = item.split("|")[0].trimmed().toUpper();
            QString mode = item.contains("|") ? item.split("|")[1].trimmed() : "unprinted";
            
            int checkedCount = 0;
            if (qIdx == m_printQueueIndex && m_contactsTable) {
                for (int i = 0; i < m_contactsTable->rowCount(); ++i) {
                    if (m_contactsTable->item(i, 0) && m_contactsTable->item(i, 0)->data(Qt::UserRole).toBool()) {
                        checkedCount++;
                    }
                }
            } else {
                QList<QsoDetail> qsos = loadQsosFromDefinedLogfiles(call);
                if (qsos.isEmpty()) {
                    if (m_mainWnd) {
                        qsos = m_mainWnd->allQsos()[call];
                    } else if (call == m_callsign) {
                        qsos = m_qsos;
                    }
                }
                
                int unprintedCount = 0;
                for (const auto &qso : qsos) {
                    if (qso.qslsdate.isEmpty()) {
                        unprintedCount++;
                    }
                }
                for (const auto &qso : qsos) {
                    bool checked = (mode == "all") || (mode == "reprint") || 
                                   ((mode == "unprinted" || mode == "prompt") && qso.qslsdate.isEmpty()) ||
                                   (unprintedCount == 0);
                    if (checked) {
                        checkedCount++;
                    }
                }
            }
            
            if (checkedCount > 0) {
                if (printCard) {
                    if (isOverflowSupportActive() && checkedCount > maxContactsPerCard) {
                        int overflowCount = checkedCount - maxContactsPerCard;
                        int overflowCards = qMax(1, (overflowCount + multiCapacity - 1) / multiCapacity);
                        totalCardsNeeded += (1 + overflowCards);
                    } else {
                        totalCardsNeeded += qMax(1, (checkedCount + maxContactsPerCard - 1) / maxContactsPerCard);
                    }
                }
                if (printEnvelope) {
                    totalEnvelopesNeeded += 1;
                }
            }
        }
    } else {
        int checkedCount = 0;
        if (m_contactsTable) {
            for (int i = 0; i < m_contactsTable->rowCount(); ++i) {
                if (m_contactsTable->item(i, 0) && m_contactsTable->item(i, 0)->data(Qt::UserRole).toBool()) {
                    checkedCount++;
                }
            }
        } else {
            checkedCount = m_qsos.size();
        }
        
        if (checkedCount > 0) {
            if (printCard) {
                if (isOverflowSupportActive() && checkedCount > maxContactsPerCard) {
                    int overflowCount = checkedCount - maxContactsPerCard;
                    int overflowCards = qMax(1, (overflowCount + multiCapacity - 1) / multiCapacity);
                    totalCardsNeeded += (1 + overflowCards);
                } else {
                    totalCardsNeeded += qMax(1, (checkedCount + maxContactsPerCard - 1) / maxContactsPerCard);
                }
            }
            if (printEnvelope) {
                totalEnvelopesNeeded += 1;
            }
        }
    }

    if (m_btnPrint) {
        QString cardStr = (totalCardsNeeded > 1) ? "Cards" : "Card";
        QString envStr = (totalEnvelopesNeeded > 1) ? "Envelopes" : "Envelope";
        m_btnPrint->setText(QString("Send To Printer\n%1 %2  %3 %4")
            .arg(totalCardsNeeded).arg(cardStr)
            .arg(totalEnvelopesNeeded).arg(envStr));
    }
}

bool QslPreviewDialog::templateHasFreqOrBand() const {
    for (const QVariantMap &m : m_templateItems) {
        QString idStr = m["id"].toString().toLower();
        if (idStr.contains("freq") || idStr.contains("mhz") || idStr.contains("band")) {
            return true;
        }
        QStringList cellData = m["cellData"].toStringList();
        if (cellData.isEmpty()) {
            QString textStr = m["text"].toString().toLower();
            if (textStr.contains("freq") || textStr.contains("mhz") || textStr.contains("band")) {
                return true;
            }
        } else {
            for (const QString &cell : cellData) {
                QString c = cell.toLower();
                if (c.contains("freq") || c.contains("mhz") || c.contains("band")) {
                    return true;
                }
            }
        }
    }
    return false;
}

void QslPreviewDialog::onFeedbackClicked() {
    QDialog feedbackDlg(this);
    feedbackDlg.setWindowTitle("Print Preview & Rendering Feedback");
    feedbackDlg.resize(900, 650);
    QVBoxLayout *layout = new QVBoxLayout(&feedbackDlg);

    QTextEdit *edit = new QTextEdit(&feedbackDlg);
    edit->setReadOnly(true);
    
    // Build html report
    QString html = "<h3>QSL PRINT PREVIEW &amp; RENDERING FEEDBACK REPORT</h3>";
    html += "<p>Generated on: <b>" + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + "</b></p>";
    
    // [1] TEMPLATE PROPERTIES
    html += "<h4>[1] ACTIVE TEMPLATE PROPERTIES</h4>";
    html += "<ul>";
    html += "<li><b>Template Name:</b> " + m_currentTemplate + "</li>";
    html += "<li><b>Template Type:</b> " + QString(m_previewingEnvelope ? "Envelope" : "QSL Card") + "</li>";
    html += "<li><b>Paper Width (px):</b> " + QString::number(m_paperW) + " (" + QString::number(m_paperW / 96.0, 'f', 2) + " in)</li>";
    html += "<li><b>Paper Height (px):</b> " + QString::number(m_paperH) + " (" + QString::number(m_paperH / 96.0, 'f', 2) + " in)</li>";
    html += "<li><b>Target Printer Name:</b> " + (m_printerName.isEmpty() ? "None specified (will use default)" : m_printerName) + "</li>";
    html += "<li><b>Print Correction (Rotation Angle):</b> " + QString::number(m_printCorrection) + "°</li>";
    html += "<li><b>Horizontal Offset (X):</b> " + QString::number(m_printOffsetX, 'f', 2) + " in</li>";
    html += "<li><b>Vertical Offset (Y):</b> " + QString::number(m_printOffsetY, 'f', 2) + " in</li>";
    html += "</ul>";

    // [2] CHECKBOX STATUSES
    html += "<h4>[2] CHECKBOX STATUSES</h4>";
    html += "<ul>";
    html += "<li><b>Print Card Checkbox:</b> " + QString(m_chkPrintCard && m_chkPrintCard->isChecked() ? "CHECKED" : "UNCHECKED") + "</li>";
    html += "<li><b>Print Envelope Checkbox:</b> " + QString(m_chkPrintEnvelope && m_chkPrintEnvelope->isChecked() ? "CHECKED" : "UNCHECKED") + "</li>";
    html += "<li><b>Show Comment Checkbox:</b> " + QString(m_chkComment && m_chkComment->isChecked() ? "CHECKED" : "UNCHECKED") + "</li>";

    html += "<li><b>Upload to QRZ Checkbox:</b> " + QString(m_chkQRZ && m_chkQRZ->isChecked() ? "CHECKED" : "UNCHECKED") + "</li>";
    html += "<li><b>Upload to eQSL Checkbox:</b> " + QString(m_chkEqsl && m_chkEqsl->isChecked() ? "CHECKED" : "UNCHECKED") + "</li>";
    
    // Custom Checkboxes
    if (!m_customChkBoxes.isEmpty()) {
        html += "<li><b>Custom Checkmarks (PSE/TNX/etc.):</b><ul>";
        for (auto it = m_customChkBoxes.begin(); it != m_customChkBoxes.end(); ++it) {
            html += "<li><b>" + it.key() + ":</b> " + QString(it.value()->isChecked() ? "CHECKED" : "UNCHECKED") + "</li>";
        }
        html += "</ul></li>";
    }
    html += "</ul>";

    // [3] ENTERED TEXT & MANUAL FIELDS
    html += "<h4>[3] ENTERED TEXT &amp; MANUAL FIELDS</h4>";
    html += "<ul>";
    html += "<li><b>Recipient Callsign:</b> " + m_callsign + "</li>";
    html += "<li><b>Recipient Address Name:</b> " + (m_addrName.isEmpty() ? "(Empty)" : m_addrName) + "</li>";
    html += "<li><b>Recipient Address Street:</b> " + (m_addrStreet.isEmpty() ? "(Empty)" : m_addrStreet) + "</li>";
    html += "<li><b>Recipient Address City/State/Zip:</b> " + (m_addrCity.isEmpty() ? "(Empty)" : m_addrCity) + "</li>";
    html += "<li><b>Recipient Address Country:</b> " + (m_addrCountry.isEmpty() ? "(Empty)" : m_addrCountry) + "</li>";
    html += "<li><b>Manual Comment text:</b> <pre>" + m_manualFieldData.value("<comment>").toHtmlEscaped() + "</pre></li>";
    html += "<li><b>Manual Comment alignment:</b> " + m_manualFieldData.value("<comment_align>") + "</li>";

    if (m_manualFieldData.size() > 2) {
        html += "<li><b>Other Manual Fields:</b><ul>";
        for (auto it = m_manualFieldData.begin(); it != m_manualFieldData.end(); ++it) {
            if (it.key() != "<comment>" && it.key() != "<comment_align>") {
                html += "<li><b>" + it.key().toHtmlEscaped() + ":</b> " + it.value().toHtmlEscaped() + "</li>";
            }
        }
        html += "</ul></li>";
    }
    html += "</ul>";

    // [4] SELECTED CONTACTS (QSOs) TO RENDER
    html += "<h4>[4] SELECTED CONTACTS (QSOs) TO PRINT/RENDER</h4>";
    QList<QsoDetail> selected;
    for (int i = 0; i < m_contactsTable->rowCount(); ++i) {
        if (m_contactsTable->item(i, 0)->data(Qt::UserRole).toBool()) {
            selected.append(m_qsos[i]);
        }
    }
    
    if (selected.isEmpty()) {
        html += "<p style='color: red;'><b>No contacts selected for printing!</b> Please check at least one contact checkbox in the contacts list.</p>";
    } else {
        html += "<table border='1' cellpadding='5' style='border-collapse: collapse; width: 100%; border: 1px solid #ccc;'>";
        html += "<tr style='background-color: #f5f5f5;'>";
        html += "<th>Callsign</th><th>Date</th><th>Time (UTC)</th><th>Band</th><th>Freq (MHz)</th><th>Mode</th><th>RST Sent</th><th>RST Rcvd</th>";
        html += "</tr>";
        for (const QsoDetail &q : selected) {
            html += "<tr>";
            html += "<td>" + q.call.toHtmlEscaped() + "</td>";
            html += "<td>" + q.date.toHtmlEscaped() + "</td>";
            html += "<td>" + q.time.toHtmlEscaped() + "</td>";
            html += "<td>" + q.band.toHtmlEscaped() + "</td>";
            html += "<td>" + q.freq.toHtmlEscaped() + "</td>";
            html += "<td>" + q.mode.toHtmlEscaped() + "</td>";
            html += "<td>" + q.rst_sent.toHtmlEscaped() + "</td>";
            html += "<td>" + q.rst_rcvd.toHtmlEscaped() + "</td>";
            html += "</tr>";
        }
        html += "</table>";
    }

    // [5] ACTIVE TEMPLATE ELEMENT INVENTORY
    html += "<h4>[5] ACTIVE TEMPLATE ELEMENT INVENTORY</h4>";
    if (m_templateItems.isEmpty()) {
        html += "<p>No layout elements defined in this template.</p>";
    } else {
        html += "<table border='1' cellpadding='5' style='border-collapse: collapse; width: 100%; border: 1px solid #ccc;'>";
        html += "<tr style='background-color: #f5f5f5;'>";
        html += "<th>ID</th><th>Type/Default Text</th><th>X (px)</th><th>Y (px)</th><th>W (px)</th><th>H (px)</th><th>Font</th><th>Size (pt)</th><th>Color</th><th>Rot</th>";
        html += "</tr>";
        for (const QVariantMap &item : m_templateItems) {
            html += "<tr>";
            html += "<td>" + item["id"].toString().toHtmlEscaped() + "</td>";
            html += "<td>" + item["text"].toString().toHtmlEscaped() + "</td>";
            html += "<td>" + QString::number(item["x"].toDouble(), 'f', 1) + "</td>";
            html += "<td>" + QString::number(item["y"].toDouble(), 'f', 1) + "</td>";
            html += "<td>" + QString::number(item["w"].toDouble(), 'f', 1) + "</td>";
            html += "<td>" + QString::number(item["h"].toDouble(), 'f', 1) + "</td>";
            html += "<td>" + item["font"].toString().toHtmlEscaped() + "</td>";
            html += "<td>" + QString::number(item["size"].toInt()) + "</td>";
            html += "<td>" + item["color"].toString().toHtmlEscaped() + "</td>";
            html += "<td>" + QString::number(item["rot"].toDouble(), 'f', 1) + "</td>";
            html += "</tr>";
        }
        html += "</table>";
    }

    edit->setHtml(html);
    layout->addWidget(edit);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *btnCopy = new QPushButton("Copy to Clipboard", &feedbackDlg);
    QPushButton *btnClose = new QPushButton("Close", &feedbackDlg);
    btnLayout->addWidget(btnCopy);
    btnLayout->addWidget(btnClose);
    layout->addLayout(btnLayout);

    connect(btnCopy, &QPushButton::clicked, this, [edit]() {
        QGuiApplication::clipboard()->setText(edit->toPlainText());
        QMessageBox::information(edit, "Copied", "Feedback report copied to clipboard!");
    });
    connect(btnClose, &QPushButton::clicked, &feedbackDlg, &QDialog::accept);

    feedbackDlg.exec();
}

QList<QsoDetail> QslPreviewDialog::loadQsosFromDefinedLogfiles(const QString &callsign) {
    QString key = callsign.toUpper();
    if (!key.isEmpty() && m_cachedQueueQsos.contains(key)) {
        return m_cachedQueueQsos.value(key);
    }

    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString defLog = settings.value("Files/logfile").toString();
    if (defLog.isEmpty()) {
        defLog = settings.value("logfile").toString();
    }
    if (defLog.isEmpty() && m_mainWnd) {
        QList<QsoDetail> res = m_mainWnd->allQsos().value(key);
        if (!key.isEmpty()) {
            m_cachedQueueQsos.insert(key, res);
        }
        return res;
    }

    QStringList files = defLog.split(";", Qt::SkipEmptyParts);
    QList<QsoDetail> matchedQsos;
    QRegularExpression eorRe("<eor>", QRegularExpression::CaseInsensitiveOption);

    for (const QString &fpath : files) {
        QFile file(fpath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QString content = file.readAll();
        file.close();

        QStringList records = content.split(eorRe);
        for (const QString &rec : records) {
            auto extractField = [](const QString &r, const QString &field) -> QString {
                QRegularExpression fieldRe(QString("<%1:(\\d+)[^>]*>").arg(field), QRegularExpression::CaseInsensitiveOption);
                QRegularExpressionMatch match = fieldRe.match(r);
                if (match.hasMatch()) {
                    int len = match.captured(1).toInt();
                    int offset = match.capturedEnd();
                    return r.mid(offset, len).trimmed();
                }
                return QString();
            };

            QString call = extractField(rec, "CALL").toUpper();
            if (callsign.isEmpty() || call == key) {
                QsoDetail d;
                d.date = extractField(rec, "QSO_DATE");
                d.time = extractField(rec, "TIME_ON");
                d.band = extractField(rec, "BAND").toUpper();
                d.mode = extractField(rec, "MODE").toUpper();
                d.call = call;
                d.country = extractField(rec, "COUNTRY").toUpper();
                d.state = extractField(rec, "STATE").toUpper();
                d.dxcc = extractField(rec, "DXCC");
                d.grid = extractField(rec, "GRIDSQUARE").toUpper();
                d.rst_rcvd = extractField(rec, "RST_RCVD");
                d.rst_sent = extractField(rec, "RST_SENT");
                d.tx_pwr = extractField(rec, "TX_PWR");
                d.qslsdate = extractField(rec, "QSLSDATE");
                d.qslrdate = extractField(rec, "QSLRDATE");
                d.qsl_sent = extractField(rec, "QSL_SENT");
                d.qsl_rcvd = extractField(rec, "QSL_RCVD");
                QString lotwRx = extractField(rec, "APP_LOTW_RXQSL");
                d.lotwRcvd = extractField(rec, "APP_LOTW_QSO_RCVD").toUpper() == "Y" || extractField(rec, "LOTW_QSL_RCVD").toUpper() == "Y" || (!lotwRx.isEmpty() && lotwRx != "00000000");
                d.mqslRcvd = extractField(rec, "QSL_RCVD").toUpper() == "Y";
                d.qrzRcvd = extractField(rec, "APP_QRZLOG_STATUS").toUpper() == "C" || extractField(rec, "QRZCOM_QSL_RCVD").toUpper() == "Y" || extractField(rec, "QRZCOM_QSL_RCVD").toUpper() == "C" || extractField(rec, "APP_QRZ_QSO_RCVD").toUpper() == "Y";
                d.cardRcvd = (d.qsl_rcvd.toUpper() == "Y" || !d.qslrdate.isEmpty());

                d.name = extractField(rec, "NAME");
                d.address = extractField(rec, "ADDRESS");
                if (d.address.isEmpty()) d.address = extractField(rec, "ADDR1");
                if (d.address.isEmpty()) d.address = extractField(rec, "ADDR2");
                d.city = extractField(rec, "CITY");
                d.zip = extractField(rec, "ZIP");
                if (d.zip.isEmpty()) d.zip = extractField(rec, "POSTAL_CODE");

                QString freqStr = extractField(rec, "FREQ");
                if (!freqStr.isEmpty()) {
                    bool ok;
                    double f = freqStr.toDouble(&ok);
                    if (ok) d.freq = QString::number(f, 'f', 4);
                    else d.freq = freqStr;
                }

                QString potaRef = extractField(rec, "POTA_REF").toUpper();
                QString sig = extractField(rec, "SIG").toUpper();
                QString sigInfo = extractField(rec, "SIG_INFO").toUpper();
                if (!potaRef.isEmpty()) d.pota = potaRef;
                else if (sig == "POTA" && !sigInfo.isEmpty()) d.pota = sigInfo;

                matchedQsos.append(d);
            }
        }
    }
    if (!key.isEmpty()) {
        m_cachedQueueQsos.insert(key, matchedQsos);
    }
    return matchedQsos;
}

bool QslPreviewDialog::checkAndTransitionToOverflow() {
    if (m_userDeclinedOverflow) {
        return false;
    }
    if (isOverflowSupportActive() && !m_overflowQueue.isEmpty() && !m_batchPrintingOverflowPhase && !m_batchPrintingEnvelopesPhase) {
        QString multiTemp = getAssociatedOverflowTemplate();
        if (multiTemp.isEmpty()) {
            return false;
        }
        
        m_savedOriginalQueue = m_printQueue;
        m_savedOriginalQueueIndex = m_printQueueIndex;
        if (m_templateList && m_templateList->currentItem()) {
            m_savedOriginalTemplate = m_templateList->currentItem()->text();
        }
        
        // Render clean stock layout by enabling emptyPreview property on the scene
        if (m_scene) {
            m_scene->setProperty("emptyPreview", true);
        }
        
        // Load multi template first to render it in m_scene
        m_isProgrammaticTemplateChange = true;
        onTemplateChosen(multiTemp);
        if (m_templateList) {
            auto items = m_templateList->findItems(multiTemp, Qt::MatchExactly);
            if (!items.isEmpty()) m_templateList->setCurrentItem(items.first());
        }
        m_isProgrammaticTemplateChange = false;
        
        // Render overflow card related to the regular qsl template
        QPixmap cardPixmap(m_scene->sceneRect().size().toSize());
        cardPixmap.fill(Qt::white);
        QPainter pixPainter(&cardPixmap);
        pixPainter.setRenderHint(QPainter::Antialiasing);
        m_scene->render(&pixPainter);
        pixPainter.end();
        
        // Immediately restore the dynamic preview representation for the live UI
        if (m_scene) {
            m_scene->setProperty("emptyPreview", false);
            renderTemplate();
        }
        
        // Construct custom transition dialog
        QDialog dlg(this);
        dlg.setWindowTitle("Load Multi-QSO Template");
        QVBoxLayout *layout = new QVBoxLayout(&dlg);
        dlg.setMinimumWidth(550);
        
        int N = m_overflowQueue.size();
        QString pluralVerb = (N > 1) ? "are" : "is";
        QString pluralNoun = (N > 1) ? "cards" : "card";
        QString msgText = QString(
            "Standard QSL card run complete.  There %1 %2 overflow %3 to be printed.\n\n"
            "Please load the overflow card shown below, and click OK to begin overflow printing."
        ).arg(pluralVerb).arg(N).arg(pluralNoun);
        
        QLabel *textLabel = new QLabel(msgText, &dlg);
        textLabel->setWordWrap(true);
        textLabel->setStyleSheet("font-size: 14px; font-weight: bold; margin-bottom: 10px;");
        layout->addWidget(textLabel);
        
        // Show 60% size card related to the regular qsl template
        QSize targetSize = (m_scene->sceneRect().size() * 0.60).toSize();
        QLabel *pixLabel = new QLabel(&dlg);
        pixLabel->setPixmap(cardPixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        pixLabel->setAlignment(Qt::AlignCenter);
        pixLabel->setStyleSheet("border: 2px solid gray; background-color: white;");
        layout->addWidget(pixLabel);
        layout->setAlignment(pixLabel, Qt::AlignCenter);
        
        // Spacer
        layout->addSpacing(15);
        
        QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        layout->addWidget(btnBox);
        
        if (dlg.exec() != QDialog::Accepted) {
            // Restore original template and cancel
            m_isProgrammaticTemplateChange = true;
            if (!m_savedOriginalTemplate.isEmpty() && m_templateList) {
                onTemplateChosen(m_savedOriginalTemplate);
                auto items = m_templateList->findItems(m_savedOriginalTemplate, Qt::MatchExactly);
                if (!items.isEmpty()) m_templateList->setCurrentItem(items.first());
            }
            m_isProgrammaticTemplateChange = false;
            return false;
        }
        
        m_printQueue = m_overflowQueue;
        m_printQueueIndex = -1;
        m_batchPrintingOverflowPhase = true;
        m_batchConfirmedCard = false;
        
        processNextBatchItem();
        return true;
    }
    return false;
}

void QslPreviewDialog::finishOverflowPhase() {
    if (m_batchPrintingOverflowPhase) {
        m_isProgrammaticTemplateChange = true;
        if (!m_savedOriginalTemplate.isEmpty() && m_templateList) {
            onTemplateChosen(m_savedOriginalTemplate);
            auto items = m_templateList->findItems(m_savedOriginalTemplate, Qt::MatchExactly);
            if (!items.isEmpty()) m_templateList->setCurrentItem(items.first());
        }
        m_isProgrammaticTemplateChange = false;
        m_printQueue = m_savedOriginalQueue;
        m_printQueueIndex = m_savedOriginalQueueIndex;
        m_batchPrintingOverflowPhase = false;
        m_overflowQueue.clear();
        m_userDeclinedOverflow = false;
    }
}

// ============================================================================
// FUNCTION: promptUserForOverflowTemplate
// PURPOSE: When the number of selected contacts exceeds the capacity of the
//          regular card template, asks the user whether to use the overflow
//          template (or associate one) or continue using the regular template
//          across multiple standard cards.
// ============================================================================
bool QslPreviewDialog::promptUserForOverflowTemplate(int contactCount, int maxCapacity, const QString &cardTemplate) {
    if (m_batchOverflowChoice == OverflowChoice::UseOverflowTemplate) {
        m_userDeclinedOverflow = false;
        return true;
    } else if (m_batchOverflowChoice == OverflowChoice::ContinueRegularTemplate) {
        m_userDeclinedOverflow = true;
        return true;
    }

    QString associatedOverflow = getAssociatedOverflowTemplate();
    QStringList availableTemplates;
    if (m_templateList) {
        for (int i = 0; i < m_templateList->count(); ++i) {
            QString tName = m_templateList->item(i)->text();
            if (tName != cardTemplate) {
                availableTemplates << tName;
            }
        }
    }

    QDialog dlg(this);
    dlg.setWindowTitle("Overflow Template Selection");
    dlg.setMinimumWidth(500);
    QVBoxLayout *vbox = new QVBoxLayout(&dlg);

    QLabel *msgLabel = new QLabel(
        QString("<b>Overflow Card Triggered</b><br><br>"
                "The number of contacts to be printed (<b>%1</b>) exceeds the capacity "
                "of the regular card template \"<b>%2</b>\" (<b>%3</b> contacts per card).<br><br>"
                "Would you like to use an overflow template, or continue to use the regular template?")
        .arg(contactCount).arg(cardTemplate).arg(maxCapacity), &dlg);
    msgLabel->setWordWrap(true);
    vbox->addWidget(msgLabel);

    vbox->addSpacing(10);

    QComboBox *comboOverflow = new QComboBox(&dlg);
    int defaultIndex = -1;
    if (!associatedOverflow.isEmpty() && availableTemplates.contains(associatedOverflow)) {
        defaultIndex = availableTemplates.indexOf(associatedOverflow);
    } else if (!associatedOverflow.isEmpty()) {
        comboOverflow->addItem(associatedOverflow);
        defaultIndex = 0;
    }
    for (int i = 0; i < availableTemplates.size(); ++i) {
        if (comboOverflow->findText(availableTemplates[i]) == -1) {
            comboOverflow->addItem(availableTemplates[i]);
            if (defaultIndex == -1 && 
                (availableTemplates[i].contains("overflow", Qt::CaseInsensitive) || 
                 availableTemplates[i].contains("multi", Qt::CaseInsensitive))) {
                defaultIndex = comboOverflow->count() - 1;
            }
        }
    }
    if (defaultIndex >= 0) {
        comboOverflow->setCurrentIndex(defaultIndex);
    }

    QHBoxLayout *comboLayout = new QHBoxLayout();
    comboLayout->addWidget(new QLabel("Overflow Template:", &dlg));
    comboLayout->addWidget(comboOverflow, 1);
    vbox->addLayout(comboLayout);

    QCheckBox *chkApplyAll = nullptr;
    if (m_printQueue.size() > 1) {
        chkApplyAll = new QCheckBox("Apply this choice to all remaining overflow cards in this batch", &dlg);
        chkApplyAll->setChecked(true);
        vbox->addSpacing(5);
        vbox->addWidget(chkApplyAll);
    }

    vbox->addSpacing(15);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnUseOverflow = new QPushButton("Use Overflow Template", &dlg);
    btnUseOverflow->setStyleSheet("QPushButton { font-weight: bold; padding: 6px 14px; }");
    QPushButton *btnUseRegular = new QPushButton("Continue with Regular Template", &dlg);
    btnUseRegular->setStyleSheet("QPushButton { padding: 6px 14px; }");
    QPushButton *btnCancel = new QPushButton("Cancel", &dlg);
    btnCancel->setStyleSheet("QPushButton { padding: 6px 14px; }");

    btnLayout->addWidget(btnUseOverflow);
    btnLayout->addWidget(btnUseRegular);
    btnLayout->addWidget(btnCancel);
    vbox->addLayout(btnLayout);

    bool accepted = false;
    bool choseOverflow = false;

    connect(btnUseOverflow, &QPushButton::clicked, &dlg, [&]() {
        accepted = true;
        choseOverflow = true;
        dlg.accept();
    });

    connect(btnUseRegular, &QPushButton::clicked, &dlg, [&]() {
        accepted = true;
        choseOverflow = false;
        dlg.accept();
    });

    connect(btnCancel, &QPushButton::clicked, &dlg, [&]() {
        accepted = false;
        dlg.reject();
    });

    dlg.exec();

    if (!accepted) {
        return false;
    }

    bool remember = (chkApplyAll && chkApplyAll->isChecked());

    if (choseOverflow) {
        m_userDeclinedOverflow = false;
        if (remember) {
            m_batchOverflowChoice = OverflowChoice::UseOverflowTemplate;
        }
        QString chosenTemplate = comboOverflow->currentText().trimmed();
        if (!chosenTemplate.isEmpty()) {
            QString cleanChosen = chosenTemplate;
            if (cleanChosen.startsWith("🏷️ ")) cleanChosen = cleanChosen.mid(3).trimmed();
            QString cleanCard = cardTemplate;
            if (cleanCard.startsWith("🏷️ ")) cleanCard = cleanCard.mid(3).trimmed();
            QSettings settings(QCoreApplication::organizationName(), "Qwain");
            settings.beginGroup("QslTemplates");
            settings.beginGroup(cleanCard);
            settings.setValue("OverflowTemplate", cleanChosen);
            settings.endGroup();
            settings.endGroup();
        }
    } else {
        m_userDeclinedOverflow = true;
        if (remember) {
            m_batchOverflowChoice = OverflowChoice::ContinueRegularTemplate;
        }
    }

    return true;
}

QString QslPreviewDialog::getAssociatedOverflowTemplate() const {
    QString normalTemplate = getSelectedCardTemplate();
    if (normalTemplate.startsWith("🏷️ ")) normalTemplate = normalTemplate.mid(3).trimmed();
    if (normalTemplate.isEmpty()) return "";
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");
    settings.beginGroup(normalTemplate);
    QString overflow = settings.value("OverflowTemplate", "").toString();
    settings.endGroup();
    settings.endGroup();
    return overflow;
}

bool QslPreviewDialog::isOverflowSupportActive() const {
    if (m_userDeclinedOverflow) return false;
    if (m_batchPrintingOverflowPhase) return true;
    return !getAssociatedOverflowTemplate().isEmpty();
}

void QslPreviewDialog::onPanicTriggered() {
    m_printQueue.clear();
    
    // Wipe CUPS queues for configured printers
    QString cardPrinter = m_comboCardPrinter ? m_comboCardPrinter->currentData().toString() : "";
    if (cardPrinter.isEmpty() && m_comboCardPrinter) cardPrinter = m_comboCardPrinter->currentText();
    QString envPrinter = m_comboEnvPrinter ? m_comboEnvPrinter->currentData().toString() : "";
    if (envPrinter.isEmpty() && m_comboEnvPrinter) envPrinter = m_comboEnvPrinter->currentText();
    
    // Purge CUPS queues via QProcess
    if (!cardPrinter.isEmpty() && cardPrinter != "Print to PDF") {
        QProcess::execute("cancel", QStringList() << "-a" << cardPrinter);
    }
    if (!envPrinter.isEmpty() && envPrinter != "Print to PDF" && envPrinter != cardPrinter) {
        QProcess::execute("cancel", QStringList() << "-a" << envPrinter);
    }
    
    // Also try general cancel for current user
    QProcess::execute("cancel", QStringList() << "-a");
    
    if (m_unifiedPrintDlg) {
        m_unifiedPrintDlg->hide();
    }
    
    // Present the critical notification asynchronously to allow the stays-on-top window 
    // to dismantle completely first, ensuring the "OK" button responds flawlessly.
    QTimer::singleShot(100, this, [this]() {
        QMessageBox::critical(this, "Emergency Stop Issued", 
            "<b>PRINTING ABORTED</b><br><br>The local printer queue has been cleared.<br>"
            "If your printer is still printing, it is running from its own internal memory. "
            "Turn the printer's power OFF to wipe its local cache.");
    });
}

QslPreviewDialog::~QslPreviewDialog() {
    if (m_unifiedPrintDlg) {
        delete m_unifiedPrintDlg;
        m_unifiedPrintDlg = nullptr;
    }
    if (m_previewWindow) {
        m_previewWindow->disconnect(this);
        delete m_previewWindow;
        m_previewWindow = nullptr;
    }
    if (m_printingPrefsGroup) {
        m_printingPrefsGroup->removeEventFilter(this);
    }
    QSettings mainSettings(QCoreApplication::organizationName(), "Qwain");
    if (m_chkQRZ) mainSettings.setValue("QslUpdates/QRZ", m_chkQRZ->isChecked());
    if (m_chkEqsl) mainSettings.setValue("QslUpdates/eQSL", m_chkEqsl->isChecked());
    if (m_chkPrintCard) mainSettings.setValue("QslUpdates/PrintCard", m_chkPrintCard->isChecked());
    if (m_chkPrintEnvelope) mainSettings.setValue("QslUpdates/PrintEnvelope", m_chkPrintEnvelope->isChecked());
    if (m_chkPrintReverse) mainSettings.setValue("QslUpdates/PrintReverse", m_chkPrintReverse->isChecked());
    
    if (m_templateList && m_templateList->currentItem()) {
        QString targetTemplate = !m_savedOriginalTemplate.isEmpty() ? m_savedOriginalTemplate : m_templateList->currentItem()->text();
        mainSettings.setValue("QslUpdates/LastCardTemplate", targetTemplate);
    }
    if (m_envTemplateList && m_envTemplateList->currentItem()) {
        mainSettings.setValue("QslUpdates/LastEnvTemplate", m_envTemplateList->currentItem()->text());
    }
}

#include "qslpreviewdialog.moc"




// Dedicated TEST Address Diagnostic Laboratory Window

void QslPreviewDialog::openAddressTestDialog() {
    AddressRenderInspectorWindow inspector(m_callsign, m_currentTemplate,
                                            m_addrName, m_addrStreet,
                                            m_addrCity, m_addrCountry,
                                            m_scene, this);
    inspector.exec();
}
