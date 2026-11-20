#ifndef SCHEMEDESIGNER_H
#define SCHEMEDESIGNER_H

// ============================================================================
// COMPONENT: In-Situ Window Scheme & Appearance Tuner (schemedesigner.h)
// ROLE: Provides in-situ per-window style, color, and font customization.
//       Toggled via an unobtrusive bracketed checkbox "{ [ ] }" in the bottom-right
//       corner of each window (defaults to unchecked on open).
//       Applies real-time live preview directly to host elements, and persists
//       customizations directly to ~/.config/Qwain/Qwain.conf under
//       [WindowStyles/<Clean_Functional_Name>].
// ============================================================================

#include <QDialog>
#include <QPointer>
#include <QString>
#include <QFont>
#include <QListWidget>
#include <QPushButton>
#include <QMap>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QFontDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QSettings>
#include <QCoreApplication>
#include <QApplication>
#include <QFrame>
#include <QPainter>
#include <QPen>
#include <QEvent>
#include <QRadioButton>
#include <QComboBox>
#include <QTableWidget>
#include <QTableView>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QListView>
#include <QTabBar>
#include <QToolButton>
#include <QAction>

class SchemeDesignerDialog;

// ============================================================================
// CLASS: WindowSchemeManager
// Purpose: Handles clean functional window name resolution, persistence to
//          Qwain.conf, and creation of the corner bracketed toggle checkbox.
// ============================================================================
class WindowSchemeManager {
public:
    // Resolves a clean, professional functional name for any window or dialog
    static QString getFunctionalId(QWidget *widget) {
        if (!widget) return "Application_Window";
        if (widget->property("functional_id").isValid() && !widget->property("functional_id").toString().isEmpty()) {
            return widget->property("functional_id").toString();
        }

        QString cls = widget->metaObject()->className();
        if (cls == "QslDesigner" || cls == "AdvLayerDialog" || cls == "TelemetryDialog") {
            return "Card_Designer";
        }
        if (cls == "QslManagerDialog" || cls == "ProcessingStatusDialog" || 
            cls == "DateRangeDialog" || cls == "CountryDxccDialog") {
            return "QSL_Print_Manager";
        }
        if (cls == "ADIFMonitorDialog") {
            return "ADIF_Monitor";
        }
        if (cls == "ConfigDialog" || cls == "UnderDevelopmentOverlay") {
            return "Station_Preferences";
        }
        if (cls == "ImageViewer") {
            return "Image_Viewer";
        }
        if (cls == "ImageEditorDialog") {
            return "Image_Editor";
        }
        if (cls == "QslPreviewDialog" || cls == "UnifiedPrintDialog" || 
            cls == "CommentEditorDialog" || cls == "FinalTweaksDialog" || 
            cls == "QslTweakAdjustDialog" || cls == "PrintExceptionDialog") {
            return "Print_Preview";
        }
        if (cls == "ReceiveQslsDialog") {
            return "Receive_QSLs";
        }
        if (cls == "StatsDialog") {
            return "Station_Statistics";
        }
        if (cls == "CacheViewerDialog" || cls == "RebuildProgressDialog") {
            return "Cache_Viewer";
        }
        if (cls == "AwardsWidget") {
            return "Awards_Dashboard";
        }
        if (cls == "GraylineWidget") {
            return "Grayline_Widget";
        }
        if (cls == "DxClusterWidget") {
            return "DX_Cluster";
        }
        if (cls == "MailCheckDialog") {
            return "Mail_Check";
        }
        if (cls == "PrefetchFlagsDialog") {
            return "Prefetch_Flags";
        }
        if (cls == "QrzSyncDialog") {
            return "QRZ_Sync";
        }
        if (cls == "CabrilloConverterDialog") {
            return "Cabrillo_Converter";
        }
        if (cls == "CountyPrepDialog") {
            return "County_Prep";
        }
        if (cls == "LiveStreamViewer") {
            return "Live_Stream_Viewer";
        }
        if (cls == "QslStatusDashboard") {
            return "QSL_Status_Dashboard";
        }
        if (cls == "MainWindow") {
            return "Main_Window";
        }
        return cls;
    }

    // Generates a stable key for saving element-specific styles
    static QString getElementKey(QWidget *w, QWidget *targetWindow) {
        if (!w || w == targetWindow) return "Window_Root";
        if (!w->objectName().isEmpty()) return w->objectName();
        QString text = w->property("text").toString().trimmed();
        if (text.isEmpty()) text = w->property("title").toString().trimmed();
        if (!text.isEmpty()) {
            text.remove(QChar('&'));
            text.replace(" ", "_");
            return QString("%1_%2").arg(w->metaObject()->className(), text.left(24));
        }
        return QString("%1_%2").arg(w->metaObject()->className(), QString::number((quintptr)w, 16));
    }

    // Applies saved customizations from Qwain.conf to the window and its widgets
    static void applySavedStyles(QWidget *window) {
        if (!window) return;
        QString functionalId = getFunctionalId(window);
        QSettings settings("Qwain", "Qwain");
        QString groupName = QString("WindowStyles/%1").arg(functionalId);

        if (!settings.childGroups().contains("WindowStyles")) {
            settings.beginGroup("WindowStyles");
            bool hasGroup = settings.childGroups().contains(functionalId);
            settings.endGroup();
            if (!hasGroup) return;
        }

        settings.beginGroup(groupName);

        // Apply window-level background override if present
        QString windowBg = settings.value("WindowBg").toString();
        if (!windowBg.isEmpty()) {
            QString curStyle = window->styleSheet();
            QString bgRule = QString(" background-color: %1; ").arg(windowBg);
            if (!curStyle.contains("background-color:")) {
                window->setStyleSheet(curStyle + bgRule);
            }
        }

        // Apply saved element overrides
        settings.beginGroup("Elements");
        QStringList keys = settings.childGroups();
        for (const QString &elemKey : keys) {
            settings.beginGroup(elemKey);
            QString savedStyle = settings.value("styleSheet").toString();
            QString fontStr = settings.value("font").toString();
            settings.endGroup();

            if (elemKey == "Window_Root") {
                if (!savedStyle.isEmpty()) window->setStyleSheet(savedStyle);
                if (!fontStr.isEmpty()) {
                    QFont f;
                    f.fromString(fontStr);
                    window->setFont(f);
                }
                continue;
            }

            // Find matching child widget
            QList<QWidget*> children = window->findChildren<QWidget*>();
            for (QWidget *cw : children) {
                if (isLogoWidget(cw)) continue; // Rule: Logo is never allowed as a themeable element
                if (getElementKey(cw, window) == elemKey) {
                    if (!savedStyle.isEmpty()) cw->setStyleSheet(savedStyle);
                    if (!fontStr.isEmpty()) {
                        QFont f;
                        f.fromString(fontStr);
                        cw->setFont(f);
                    }
                    break;
                }
            }
        }
        settings.endGroup(); // Elements
        settings.endGroup(); // WindowStyles/<functionalId>
    }

    // =========================================================================
    // EXCLUSION: Application Branding & Logos
    // Rule: Logo is never allowed as a schemedesigner selection on any window.
    // Logos are fixed branding assets (pixmaps/icons) and must not be treated
    // as themeable font or color UI elements.
    // =========================================================================
    static bool isLogoWidget(QWidget *w) {
        if (!w) return false;

        // 1. Direct objectName check (case-insensitive)
        QString obj = w->objectName().toLower();
        if (obj.contains("logo")) {
            return true;
        }

        // 2. Explicit property flag check
        if (w->property("isLogo").toBool()) {
            return true;
        }

        // 3. Tooltip or accessible name check
        if (w->toolTip().toLower().contains("logo") || w->accessibleName().toLower().contains("logo")) {
            return true;
        }

        // 4. Pure graphic / pixmap QLabel without text (application logos, badges, branding image boxes)
        if (QLabel *lbl = qobject_cast<QLabel*>(w)) {
            if (lbl->text().trimmed().isEmpty()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                if (!lbl->pixmap().isNull()) return true;
#elif QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
                if (!lbl->pixmap(Qt::ReturnByValue).isNull()) return true;
#else
                if (lbl->pixmap() && !lbl->pixmap()->isNull()) return true;
#endif
            }
        }

        // 5. Parent hierarchy check: if widget is hosted inside a dedicated logo container
        QWidget *p = w->parentWidget();
        while (p) {
            QString pObj = p->objectName().toLower();
            if (pObj.contains("logo") || p->property("isLogo").toBool()) {
                return true;
            }
            p = p->parentWidget();
        }

        return false;
    }

    // Creates the corner toggle checkbox in the bottom-right corner of each window
    // (naked checkbox without bracketed text, defaulting to unchecked false upon window open).
    static QCheckBox* createCornerToggle(QWidget *targetWindow, QWidget *parent = nullptr);
};

// ============================================================================
// CLASS: SelectionHighlightOverlay
// Purpose: Renders a high-contrast visual highlight framing around the selected
//          element on the target window when Scheme Designer is active.
//          Does not mutate underlying element stylesheets and allows click-through.
// ============================================================================
class SelectionHighlightOverlay : public QWidget {
public:
    explicit SelectionHighlightOverlay(QWidget *parent = nullptr) : QWidget(parent) {
        setObjectName("schemeSelectionHighlightOverlay");
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAttribute(Qt::WA_TranslucentBackground, true);
        hide();
    }

    // Moves and sizes the highlight overlay to frame the selected widget within the target window
    void highlightWidget(QWidget *target, QWidget *window) {
        if (!target || !window || !window->isVisible() || WindowSchemeManager::isLogoWidget(target)) {
            hide();
            return;
        }

        if (target == window) {
            // Outline the inside perimeter of the root window
            setGeometry(2, 2, window->width() - 4, window->height() - 4);
            raise();
            show();
            update();
            return;
        }

        // Map target widget's top-left corner into window coordinates
        QPoint pos = target->mapTo(window, QPoint(0, 0));
        // Frame the element with a 2px margin around it
        int x = qMax(0, pos.x() - 2);
        int y = qMax(0, pos.y() - 2);
        int w = target->width() + 4;
        int h = target->height() + 4;

        setGeometry(x, y, w, h);
        raise();
        show();
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        // 1. Dark outer drop-contrast line to stand out clearly on bright backgrounds
        p.setPen(QPen(QColor(15, 23, 42, 200), 3, Qt::SolidLine));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(1, 1, width() - 2, height() - 2, 3, 3);

        // 2. High-visibility bright amber/gold highlight ring
        p.setPen(QPen(QColor(245, 158, 11), 2, Qt::SolidLine));
        p.setBrush(QColor(245, 158, 11, 35)); // Subtle luminous tint
        p.drawRoundedRect(1, 1, width() - 2, height() - 2, 3, 3);

        // 3. Precision corner brackets (cyan #38bdf8) for distinctive element selection indicator
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(56, 189, 248));
        const int L = 7;
        const int T = 2;
        // Top-left
        p.drawRect(0, 0, L, T);
        p.drawRect(0, 0, T, L);
        // Top-right
        p.drawRect(width() - L, 0, L, T);
        p.drawRect(width() - T, 0, T, L);
        // Bottom-left
        p.drawRect(0, height() - T, L, T);
        p.drawRect(0, height() - L, T, L);
        // Bottom-right
        p.drawRect(width() - L, height() - T, L, T);
        p.drawRect(width() - T, height() - L, T, L);
    }
};

// ============================================================================
// CLASS: SchemeDesignerDialog
// Purpose: Interactive live tuner allowing matching of fields/elements to
//          exact colors, sizes, and fonts with real-time preview and Save to INI.
// ============================================================================
class SchemeDesignerDialog : public QDialog {
    Q_OBJECT
public:
    struct WidgetState {
        QPointer<QWidget> widget;
        QString elementKey;
        QString descriptiveName;
        QString originalStyleSheet;
        QFont originalFont;
        QString currentStyleSheet;
        QFont currentFont;
        QString customBg;
        QString customFg;
        bool changed;
    };

    struct UndoAction {
        QPointer<QWidget> widget;
        QString styleSheetBefore;
        QFont fontBefore;
        QString bgBefore;
        QString fgBefore;
        QString styleSheetAfter;
        QFont fontAfter;
        QString bgAfter;
        QString fgAfter;
    }; // struct UndoAction definition closure

    QPointer<QWidget> m_targetWnd;
    QString m_functionalId;
    QLabel *m_elemHeaderLabel;
    QPushButton *m_btnRefresh;
    QListWidget *m_elementsList;
    QPushButton *m_btnFont;
    QSpinBox *m_fontSizeSpin;
    QCheckBox *m_boldCheck;
    QPushButton *m_btnBgColor;
    QPushButton *m_btnFgColor;
    QLabel *m_bgSwatch;
    QLabel *m_fgSwatch;
    QPushButton *m_btnUndo;
    QPushButton *m_btnSave;
    QPushButton *m_btnReset;
    QLabel *m_statusFeedback;
    
    QMap<QWidget*, WidgetState> m_states;
    QMap<QListWidgetItem*, QPointer<QWidget>> m_itemToWidget;
    SelectionHighlightOverlay *m_highlightOverlay;
    QList<UndoAction> m_history;
    bool m_savedPermanently;

    SchemeDesignerDialog(QWidget *target, QWidget *parent = nullptr) 
        : QDialog(parent), m_targetWnd(target), m_elemHeaderLabel(nullptr),
          m_btnRefresh(nullptr),
          m_highlightOverlay(nullptr), m_savedPermanently(false)
    {
        m_functionalId = WindowSchemeManager::getFunctionalId(target);
        setWindowFlags(Qt::Tool);
        setAttribute(Qt::WA_DeleteOnClose);
        setWindowTitle(QString("%1 — Appearance & Scheme Tuner").arg(m_functionalId.replace("_", " ")));
        resize(540, 560);

        // Instantiate live selection highlight overlay on target window
        if (m_targetWnd) {
            m_highlightOverlay = new SelectionHighlightOverlay(m_targetWnd);
            m_targetWnd->installEventFilter(this);
            if (qApp) {
                qApp->installEventFilter(this);
            }

            // Automatically re-filter visible elements if user switches active tabs in target window
            QList<QTabWidget*> tabWidgets = m_targetWnd->findChildren<QTabWidget*>();
            for (QTabWidget *tb : tabWidgets) {
                connect(tb, &QTabWidget::currentChanged, this, [this](int) {
                    populateElements();
                });
            }
        }

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(8);

        // Header info banner
        QFrame *headerFrame = new QFrame(this);
        headerFrame->setStyleSheet("QFrame { background-color: #1e293b; border: 1px solid #334155; border-radius: 6px; padding: 6px; }");
        QVBoxLayout *headerLayout = new QVBoxLayout(headerFrame);
        headerLayout->setContentsMargins(6, 6, 6, 6);
        headerLayout->setSpacing(2);

        QLabel *titleLbl = new QLabel(QString("<b>In-Situ Scheme Tuner:</b> %1").arg(m_functionalId), headerFrame);
        titleLbl->setStyleSheet("QLabel { color: #38bdf8; font-size: 11pt; font-weight: bold; }");
        QLabel *subLbl = new QLabel("Match fields/elements to exact colors, sizes, and fonts with real-time preview.", headerFrame);
        subLbl->setStyleSheet("QLabel { color: #94a3b8; font-size: 8.5pt; }");
        headerLayout->addWidget(titleLbl);
        headerLayout->addWidget(subLbl);
        mainLayout->addWidget(headerFrame);

        // Elements selector header row with live count and Refresh Visible button
        QHBoxLayout *elemHeaderLayout = new QHBoxLayout();
        m_elemHeaderLabel = new QLabel("<b>Select Window Element:</b>", this);
        elemHeaderLayout->addWidget(m_elemHeaderLabel);
        elemHeaderLayout->addStretch();
        
        m_btnRefresh = new QPushButton("🔄 Refresh Visible", this);
        m_btnRefresh->setToolTip("Refreshes list to include only currently visible elements (e.g. after switching tabs or displaying panels)");
        m_btnRefresh->setStyleSheet(
            "QPushButton { background-color: #334155; color: #94a3b8; border: 1px solid #475569; border-radius: 3px; padding: 2px 8px; font-size: 8pt; }"
            "QPushButton:hover { color: #f1f5f9; border-color: #38bdf8; background-color: #475569; }"
        );
        connect(m_btnRefresh, &QPushButton::clicked, this, [this]() {
            populateElements();
        });
        elemHeaderLayout->addWidget(m_btnRefresh);
        mainLayout->addLayout(elemHeaderLayout);

        m_elementsList = new QListWidget(this);
        m_elementsList->setStyleSheet(
            "QListWidget { background-color: #0f172a; color: #f1f5f9; border: 1px solid #334155; border-radius: 4px; padding: 4px; }"
            "QListWidget::item:selected { background-color: #2563eb; color: #ffffff; border-radius: 3px; }"
        );
        mainLayout->addWidget(m_elementsList, 1);

        // Populate visible elements list
        populateElements();

        // Color & Font Controls Group
        QGroupBox *controlsBox = new QGroupBox("Element Appearance", this);
        QVBoxLayout *controlsLayout = new QVBoxLayout(controlsBox);
        controlsLayout->setSpacing(8);

        // Row 1: Background & Foreground Color Pickers with live swatches
        QHBoxLayout *colorRow = new QHBoxLayout();
        m_btnBgColor = new QPushButton("Background Color...", this);
        m_bgSwatch = new QLabel(this);
        m_bgSwatch->setFixedSize(26, 24);
        m_bgSwatch->setStyleSheet("background-color: transparent; border: 1px solid #64748b; border-radius: 3px;");

        m_btnFgColor = new QPushButton("Text Color...", this);
        m_fgSwatch = new QLabel(this);
        m_fgSwatch->setFixedSize(26, 24);
        m_fgSwatch->setStyleSheet("background-color: transparent; border: 1px solid #64748b; border-radius: 3px;");

        colorRow->addWidget(m_btnBgColor);
        colorRow->addWidget(m_bgSwatch);
        colorRow->addSpacing(12);
        colorRow->addWidget(m_btnFgColor);
        colorRow->addWidget(m_fgSwatch);
        colorRow->addStretch();
        controlsLayout->addLayout(colorRow);

        // Row 2: Quick Palette Swatches
        QHBoxLayout *paletteRow = new QHBoxLayout();
        paletteRow->addWidget(new QLabel("Quick Palette:", this));
        auto addChip = [this, paletteRow](const QString &hex, const QString &label) {
            QPushButton *chip = new QPushButton(label, this);
            chip->setToolTip(hex);
            chip->setStyleSheet(QString(
                "QPushButton { background-color: %1; color: %2; border: 1px solid #475569; border-radius: 3px; padding: 3px 8px; font-size: 8pt; }"
                "QPushButton:hover { border: 1px solid #38bdf8; }"
            ).arg(hex, (hex == "#ffffff" || hex == "#e2e8f0" || hex == "#f59e0b") ? "#000000" : "#ffffff"));
            connect(chip, &QPushButton::clicked, this, [this, hex]() {
                applyBgColor(hex);
            });
            paletteRow->addWidget(chip);
        };
        addChip("#1e1e1e", "Dark");
        addChip("#2b2b2b", "Charcoal");
        addChip("#0f172a", "Slate");
        addChip("#3b82f6", "Blue");
        addChip("#f59e0b", "Amber");
        addChip("#ffffff", "White");
        paletteRow->addStretch();
        controlsLayout->addLayout(paletteRow);

        // Row 3: Font Picker & Size Spinner
        QHBoxLayout *fontRow = new QHBoxLayout();
        m_btnFont = new QPushButton("Font Family...", this);
        fontRow->addWidget(m_btnFont);

        fontRow->addSpacing(8);
        fontRow->addWidget(new QLabel("Size:", this));
        m_fontSizeSpin = new QSpinBox(this);
        m_fontSizeSpin->setRange(6, 48);
        m_fontSizeSpin->setValue(9);
        fontRow->addWidget(m_fontSizeSpin);

        fontRow->addSpacing(8);
        m_boldCheck = new QCheckBox("Bold", this);
        fontRow->addWidget(m_boldCheck);

        m_btnUndo = new QPushButton("Undo", this);
        m_btnUndo->setEnabled(false);
        fontRow->addStretch();
        fontRow->addWidget(m_btnUndo);
        controlsLayout->addLayout(fontRow);

        mainLayout->addWidget(controlsBox);

        // Status feedback line
        m_statusFeedback = new QLabel("Ready. Select an element above to match colors and sizes.", this);
        m_statusFeedback->setStyleSheet("QLabel { color: #38bdf8; font-size: 8.5pt; font-family: monospace; }");
        mainLayout->addWidget(m_statusFeedback);

        // Action Buttons Row (Save to Qwain.conf, Reset, Close)
        QHBoxLayout *bottomLayout = new QHBoxLayout();
        m_btnSave = new QPushButton("💾 Save to Window Theme", this);
        m_btnSave->setStyleSheet(
            "QPushButton { background-color: #059669; color: #ffffff; font-weight: bold; padding: 6px 14px; border-radius: 4px; border: 1px solid #10b981; }"
            "QPushButton:hover { background-color: #10b981; }"
        );
        m_btnReset = new QPushButton("Reset Changes", this);
        m_btnReset->setToolTip("Reverts the window design back to what it was when this window opened.");
        QPushButton *btnClose = new QPushButton("Close", this);

        bottomLayout->addWidget(m_btnSave);
        bottomLayout->addWidget(m_btnReset);
        bottomLayout->addStretch();
        bottomLayout->addWidget(btnClose);
        mainLayout->addLayout(bottomLayout);

        // Wire connections
        connect(m_elementsList, &QListWidget::currentItemChanged, this, &SchemeDesignerDialog::onCurrentItemChanged);
        connect(m_btnBgColor, &QPushButton::clicked, this, &SchemeDesignerDialog::chooseBgColor);
        connect(m_btnFgColor, &QPushButton::clicked, this, &SchemeDesignerDialog::chooseFgColor);
        connect(m_btnFont, &QPushButton::clicked, this, &SchemeDesignerDialog::chooseFont);
        connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SchemeDesignerDialog::onFontSizeChanged);
        connect(m_boldCheck, &QCheckBox::toggled, this, &SchemeDesignerDialog::onBoldToggled);
        connect(m_btnUndo, &QPushButton::clicked, this, &SchemeDesignerDialog::undo);
        connect(m_btnSave, &QPushButton::clicked, this, &SchemeDesignerDialog::saveToIni);
        connect(m_btnReset, &QPushButton::clicked, this, &SchemeDesignerDialog::resetChanges);
        connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

        if (m_elementsList->count() > 0) {
            m_elementsList->setCurrentRow(0);
        }
    }

    ~SchemeDesignerDialog() {
        if (qApp) {
            qApp->removeEventFilter(this);
        }
        if (m_targetWnd) {
            m_targetWnd->removeEventFilter(this);
        }
        if (m_highlightOverlay) {
            m_highlightOverlay->hide();
            m_highlightOverlay->deleteLater();
            m_highlightOverlay = nullptr;
        }

        // If not saved permanently, revert changes back to original baseline
        if (!m_savedPermanently) {
            for (auto it = m_states.begin(); it != m_states.end(); ++it) {
                if (it.value().widget) {
                    it.value().widget->setStyleSheet(it.value().originalStyleSheet);
                    it.value().widget->setFont(it.value().originalFont);
                }
            }
        }
    }

    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched == m_targetWnd && (event->type() == QEvent::Resize || event->type() == QEvent::Move || event->type() == QEvent::LayoutRequest)) {
            updateHighlight();
        }

        // =====================================================================
        // FEATURE: In-Situ Click-to-Focus & Interaction Suppression
        // When Scheme Designer is active and user clicks any element in the
        // target window:
        // 1. Focus that element in the Scheme Designer list.
        // 2. Consume mouse events (press, release, double click) to suppress
        //    normal interaction (e.g. button click actions, text edits, cell edits)
        //    allowing safe design inspection without triggering application actions.
        // Clicks on Scheme Designer itself and the corner toggle are preserved.
        // =====================================================================
        QEvent::Type et = event->type();
        if (et == QEvent::MouseButtonPress || et == QEvent::MouseButtonRelease || et == QEvent::MouseButtonDblClick) {
            if (QWidget *clicked = qobject_cast<QWidget*>(watched)) {
                if (m_targetWnd && (clicked == m_targetWnd || m_targetWnd->isAncestorOf(clicked))) {
                    // Ignore clicks on Scheme Designer itself, overlay, and toggle
                    if (clicked != this && !this->isAncestorOf(clicked) &&
                        clicked != m_highlightOverlay && !(m_highlightOverlay && m_highlightOverlay->isAncestorOf(clicked)) &&
                        clicked->objectName() != "schemeCornerToggle") {
                        
                        if (et == QEvent::MouseButtonPress) {
                            // Rule: Logo is never allowed as a schemedesigner selection on any window
                            if (!WindowSchemeManager::isLogoWidget(clicked)) {
                                // Locate matching widget in m_itemToWidget, walking up parent chain if clicking a sub-widget
                                QWidget *curr = clicked;
                                QListWidgetItem *foundItem = nullptr;
                                while (curr && (curr == m_targetWnd || m_targetWnd->isAncestorOf(curr))) {
                                    if (WindowSchemeManager::isLogoWidget(curr)) {
                                        break; // Do not select logo or its parents
                                    }
                                    for (auto it = m_itemToWidget.begin(); it != m_itemToWidget.end(); ++it) {
                                        if (it.value().data() == curr) {
                                            foundItem = it.key();
                                            break;
                                        }
                                    }
                                    if (foundItem) break;
                                    curr = curr->parentWidget();
                                }

                                if (foundItem) {
                                    m_elementsList->setCurrentItem(foundItem);
                                    m_elementsList->scrollToItem(foundItem);
                                }
                            }
                        }

                        // Consume event: Suppress normal interaction while Scheme Designer is active
                        return true;
                    }
                }
            }
        }

        return QDialog::eventFilter(watched, event);
    }

    void updateHighlight() {
        QWidget *w = selectedWidget();
        if (m_highlightOverlay && m_targetWnd) {
            m_highlightOverlay->highlightWidget(w, m_targetWnd);
        }
    }

    // ========================================================================
    // Check if an element is genuinely visible to the user:
    // 1. Must be currently visible within the Qt hierarchy (w->isVisible()).
    //    Returns false if w or any parent widget is hidden (e.g. inactive tab).
    // 2. Must possess positive on-screen geometry (w->width() > 0 && w->height() > 0).
    // 3. Must intersect target window geometry (excludes offscreen positioned helpers).
    // 4. Excludes scheme tuner internal widgets (overlay highlight, corner toggle, tuner dialog).
    // 5. Excludes anonymous blank container QWidgets that merely wrap other controls.
    // 6. Excludes application branding / logos (never allowed as scheme selection).
    // ========================================================================
    bool isElementVisibleToUser(QWidget *w) const {
        if (!w || !m_targetWnd) return false;
        if (w == m_targetWnd) {
            return m_targetWnd->isVisible();
        }
        // Must be visible within Qt's widget tree (false if widget or any parent is hidden,
        // such as widgets on non-active tabs of a QTabWidget)
        if (!w->isVisible()) {
            return false;
        }
        // Must have real geometric dimensions on screen
        if (w->width() <= 0 || w->height() <= 0) {
            return false;
        }
        // Exclude internal Scheme Designer components and all of its child controls
        if (w == this || this->isAncestorOf(w) || w->window() == this) {
            return false;
        }
        if (w == m_highlightOverlay || (m_highlightOverlay && m_highlightOverlay->isAncestorOf(w)) || w->objectName() == "schemeSelectionHighlightOverlay") {
            return false;
        }
        if (w->objectName() == "schemeCornerToggle") {
            return false;
        }
        // Rule: Logo is never allowed as a schemedesigner selection on any window
        if (WindowSchemeManager::isLogoWidget(w)) {
            return false;
        }
        // Ensure widget is actually an in-hierarchy descendant of m_targetWnd (required for safe mapTo operations)
        if (w != m_targetWnd && !m_targetWnd->isAncestorOf(w)) {
            return false;
        }
        // Exclude anonymous pure QWidget grouping containers that contain child controls
        QString cls = w->metaObject()->className();
        if (cls == "QWidget" && w->objectName().isEmpty() && !w->findChildren<QWidget*>().isEmpty()) {
            return false;
        }
        // Ensure its position intersects the target window's rect
        QPoint pos = w->mapTo(m_targetWnd, QPoint(0, 0));
        QRect bounds(pos, w->size());
        if (!m_targetWnd->rect().intersects(bounds)) {
            return false;
        }
        return true;
    }

    // =========================================================================
    // FEATURE: Human-Friendly Element Naming Engine
    // PURPOSE: Converts raw Qt widget class names, developer variable prefixes,
    //          and accelerator ampersands into clear, human-friendly operator terms.
    // =========================================================================
    static QString cleanVisibleText(QString text) {
        // Strip HTML markup if present
        while (text.contains('<') && text.contains('>')) {
            int start = text.indexOf('<');
            int end = text.indexOf('>', start);
            if (end == -1) break;
            text.remove(start, end - start + 1);
        }
        // Strip Qt accelerator ampersands
        text.replace("&", "");
        // Strip trailing colons
        text = text.trimmed();
        while (text.endsWith(':')) {
            text.chop(1);
            text = text.trimmed();
        }
        text.replace("\n", " ");
        text.replace("\"", "");
        return text.simplified();
    }

    static QString cleanIdentifier(const QString &raw) {
        if (raw.isEmpty()) return "";
        QString s = raw;
        // Strip common developer prefixes (e.g. btn_, cmb_, txt_, le_, chk_, tbl_)
        QStringList prefixes = {"btn_", "btn", "cmb_", "cmb", "txt_", "txt", "le_", "le",
                                "chk_", "chk", "tbl_", "tbl", "spn_", "spn", "lbl_", "lbl",
                                "rad_", "rad", "gb_", "gb", "frame_", "frame"};
        for (const QString &p : prefixes) {
            if (s.startsWith(p, Qt::CaseInsensitive) && s.length() > p.length() && 
                (p.endsWith("_") || s[p.length()].isUpper() || s[p.length()] == '_')) {
                s = s.mid(p.length());
                if (s.startsWith("_")) s = s.mid(1);
                break;
            }
        }
        // Convert camelCase to spaced words
        QString result;
        for (int i = 0; i < s.length(); ++i) {
            QChar c = s[i];
            if (i > 0 && c.isUpper() && s[i - 1].isLower()) {
                result += ' ';
            }
            result += c;
        }
        result = result.replace("_", " ").simplified();
        // Capitalize words for clean readability
        QStringList words = result.split(' ', Qt::SkipEmptyParts);
        for (int i = 0; i < words.size(); ++i) {
            if (!words[i].isEmpty()) {
                words[i] = words[i].left(1).toUpper() + words[i].mid(1);
            }
        }
        return words.join(' ');
    }

    static QString findWidgetLabelContext(QWidget *w, QWidget *targetWnd) {
        if (!w || !targetWnd) return "";
        // 1. Check buddy label in parent window
        QList<QLabel*> labels = targetWnd->findChildren<QLabel*>();
        for (QLabel *lbl : labels) {
            if (lbl->buddy() == w) {
                QString t = cleanVisibleText(lbl->text());
                if (!t.isEmpty()) return t;
            }
        }
        // 2. Check tooltip
        if (!w->toolTip().isEmpty()) {
            QString tt = cleanVisibleText(w->toolTip());
            if (!tt.isEmpty() && tt.length() <= 30) return tt;
        }
        // 3. Check accessible name
        if (!w->accessibleName().isEmpty()) {
            QString an = cleanVisibleText(w->accessibleName());
            if (!an.isEmpty()) return an;
        }
        // 4. Check placeholder text if QLineEdit
        if (QLineEdit *le = qobject_cast<QLineEdit*>(w)) {
            if (!le->placeholderText().isEmpty()) {
                QString ph = cleanVisibleText(le->placeholderText());
                if (!ph.isEmpty()) return ph;
            }
        }
        // 5. Clean objectName
        QString obj = cleanIdentifier(w->objectName());
        if (!obj.isEmpty()) return obj;

        // 6. Check parent group box
        QWidget *p = w->parentWidget();
        while (p && p != targetWnd) {
            if (QGroupBox *gb = qobject_cast<QGroupBox*>(p)) {
                QString gt = cleanVisibleText(gb->title());
                if (!gt.isEmpty()) return gt;
            }
            p = p->parentWidget();
        }
        return "";
    }

private:
    void populateElements() {
        if (!m_targetWnd) return;

        // Remember currently selected widget to re-select it after refresh if still visible
        QWidget *previouslySelected = selectedWidget();

        m_elementsList->blockSignals(true);
        m_elementsList->clear();
        m_itemToWidget.clear();

        QList<QWidget*> allWidgets = m_targetWnd->findChildren<QWidget*>();
        allWidgets.prepend(m_targetWnd);

        int visibleCount = 0;
        QListWidgetItem *itemToSelect = nullptr;

        for (QWidget *w : allWidgets) {
            // Filter: limit elements available to be changed to only elements visible to user
            if (!isElementVisibleToUser(w)) {
                continue;
            }

            QString desc;
            QString className = w->metaObject()->className();

            if (w == m_targetWnd) {
                desc = "Main Window Background";
            } else if (QToolButton *tb = qobject_cast<QToolButton*>(w)) {
                // QToolButton: inspect button face text first, then action text/tooltip/context
                QString t = cleanVisibleText(tb->text());
                if (t.isEmpty() || t == "...") {
                    if (tb->defaultAction()) {
                        QString at = cleanVisibleText(tb->defaultAction()->text());
                        if (!at.isEmpty() && at != "...") t = at;
                        else if (!tb->defaultAction()->iconText().isEmpty() && tb->defaultAction()->iconText() != "...")
                            t = cleanVisibleText(tb->defaultAction()->iconText());
                    }
                    if (t.isEmpty() || t == "...") {
                        QString ctx = findWidgetLabelContext(w, m_targetWnd);
                        if (!ctx.isEmpty()) t = ctx;
                        else if (t.isEmpty()) t = "Tool Button";
                    }
                }
                desc = QString("Button: %1").arg(t);
            } else if (QPushButton *btn = qobject_cast<QPushButton*>(w)) {
                QString t = cleanVisibleText(btn->text());
                if (t.isEmpty() || t == "...") {
                    QString ctx = findWidgetLabelContext(w, m_targetWnd);
                    t = ctx.isEmpty() ? "Action" : ctx;
                }
                desc = QString("Button: %1").arg(t);
            } else if (QCheckBox *chk = qobject_cast<QCheckBox*>(w)) {
                QString t = cleanVisibleText(chk->text());
                if (t.isEmpty()) t = findWidgetLabelContext(w, m_targetWnd);
                desc = QString("Option: %1").arg(t.isEmpty() ? "Checkbox" : t);
            } else if (QRadioButton *rad = qobject_cast<QRadioButton*>(w)) {
                QString t = cleanVisibleText(rad->text());
                if (t.isEmpty()) t = findWidgetLabelContext(w, m_targetWnd);
                desc = QString("Choice: %1").arg(t.isEmpty() ? "Radio Option" : t);
            } else if (QGroupBox *gb = qobject_cast<QGroupBox*>(w)) {
                QString t = cleanVisibleText(gb->title());
                if (t.isEmpty()) t = findWidgetLabelContext(w, m_targetWnd);
                desc = QString("Section: %1").arg(t.isEmpty() ? "Group Frame" : t);
            } else if (QLabel *lbl = qobject_cast<QLabel*>(w)) {
                QString t = cleanVisibleText(lbl->text());
                if (t.length() > 30) t = t.left(27) + "...";
                if (t.isEmpty()) t = findWidgetLabelContext(w, m_targetWnd);
                desc = QString("Label: %1").arg(t.isEmpty() ? "Text Display" : t);
            } else if (qobject_cast<QLineEdit*>(w)) {
                QString ctx = findWidgetLabelContext(w, m_targetWnd);
                if (ctx.isEmpty()) ctx = "Text Entry";
                if (ctx.contains("search", Qt::CaseInsensitive) || ctx.contains("filter", Qt::CaseInsensitive)) {
                    desc = QString("%1 Box").arg(ctx);
                } else {
                    desc = QString("%1 Field").arg(ctx);
                }
            } else if (qobject_cast<QComboBox*>(w)) {
                QString ctx = findWidgetLabelContext(w, m_targetWnd);
                desc = QString("%1 Selection Menu").arg(ctx.isEmpty() ? "Option" : ctx);
            } else if (qobject_cast<QSpinBox*>(w) || qobject_cast<QDoubleSpinBox*>(w)) {
                QString ctx = findWidgetLabelContext(w, m_targetWnd);
                desc = QString("%1 Number Box").arg(ctx.isEmpty() ? "Quantity" : ctx);
            } else if (qobject_cast<QTableWidget*>(w) || qobject_cast<QTableView*>(w)) {
                QString ctx = cleanIdentifier(w->objectName());
                if (ctx.isEmpty()) ctx = "Data";
                desc = QString("%1 Table").arg(ctx);
            } else if (qobject_cast<QListWidget*>(w) || qobject_cast<QListView*>(w)) {
                QString ctx = cleanIdentifier(w->objectName());
                if (ctx.isEmpty()) ctx = "Items";
                desc = QString("%1 List").arg(ctx);
            } else if (qobject_cast<QTextEdit*>(w) || qobject_cast<QPlainTextEdit*>(w)) {
                QString ctx = findWidgetLabelContext(w, m_targetWnd);
                desc = QString("%1 Text Area").arg(ctx.isEmpty() ? "Editor" : ctx);
            } else if (className == "QTabBar") {
                desc = "Navigation Tabs";
            } else {
                QString ctx = cleanIdentifier(w->objectName());
                if (ctx.isEmpty()) ctx = className;
                desc = QString("%1 Control").arg(ctx);
            }

            // Only initialize WidgetState if not already tracked in m_states
            // (preserves active unsaved changes if user refreshes the visible list)
            if (!m_states.contains(w)) {
                WidgetState state;
                state.widget = w;
                state.elementKey = WindowSchemeManager::getElementKey(w, m_targetWnd);
                state.descriptiveName = desc;
                state.originalStyleSheet = w->styleSheet();
                state.originalFont = w->font();
                state.currentStyleSheet = w->styleSheet();
                state.currentFont = w->font();
                state.customBg = "";
                state.customFg = "";
                state.changed = false;
                m_states[w] = state;
            }

            QListWidgetItem *item = new QListWidgetItem(desc, m_elementsList);
            m_itemToWidget[item] = w;
            visibleCount++;

            if (w == previouslySelected) {
                itemToSelect = item;
            }
        }

        m_elementsList->blockSignals(false);

        if (m_elemHeaderLabel) {
            m_elemHeaderLabel->setText(QString("<b>Select Window Element:</b> <span style='color:#38bdf8;'>(%1 visible)</span>").arg(visibleCount));
        }

        if (itemToSelect) {
            m_elementsList->setCurrentItem(itemToSelect);
        } else if (m_elementsList->count() > 0) {
            m_elementsList->setCurrentRow(0);
        }

        updateHighlight();
    }

    QWidget* selectedWidget() {
        QListWidgetItem *item = m_elementsList->currentItem();
        if (!item || !m_itemToWidget.contains(item)) return nullptr;
        QWidget *w = m_itemToWidget[item].data();
        if (WindowSchemeManager::isLogoWidget(w)) return nullptr;
        return w;
    }

    void onCurrentItemChanged(QListWidgetItem *current, QListWidgetItem *previous) {
        Q_UNUSED(previous);

        QWidget *w = nullptr;
        if (current && m_itemToWidget.contains(current)) {
            w = m_itemToWidget[current].data();
        }

        if (w && m_states.contains(w)) {
            // Update control indicators
            QFont f = m_states[w].currentFont;
            m_fontSizeSpin->blockSignals(true);
            m_fontSizeSpin->setValue(f.pointSize() > 0 ? f.pointSize() : 9);
            m_fontSizeSpin->blockSignals(false);

            m_boldCheck->blockSignals(true);
            m_boldCheck->setChecked(f.bold());
            m_boldCheck->blockSignals(false);

            // Update swatch indicators
            QString bg = m_states[w].customBg;
            m_bgSwatch->setStyleSheet(QString("background-color: %1; border: 1px solid #64748b; border-radius: 3px;")
                .arg(bg.isEmpty() ? "#1e293b" : bg));
            QString fg = m_states[w].customFg;
            m_fgSwatch->setStyleSheet(QString("background-color: %1; border: 1px solid #64748b; border-radius: 3px;")
                .arg(fg.isEmpty() ? "#f1f5f9" : fg));

            // Position and show the visible selection highlight overlay framing the element
            updateHighlight();
            m_statusFeedback->setText(QString("Editing: %1 (Real-time preview active)").arg(m_states[w].descriptiveName));
        } else {
            if (m_highlightOverlay) {
                m_highlightOverlay->hide();
            }
        }
    }

    void applyChange(QWidget *w, const QString &newStyle, const QFont &newFont, const QString &bg, const QString &fg) {
        if (!w || !m_states.contains(w)) return;
        UndoAction action;
        action.widget = w;
        action.styleSheetBefore = m_states[w].currentStyleSheet;
        action.fontBefore = m_states[w].currentFont;
        action.bgBefore = m_states[w].customBg;
        action.fgBefore = m_states[w].customFg;
        action.styleSheetAfter = newStyle;
        action.fontAfter = newFont;
        action.bgAfter = bg;
        action.fgAfter = fg;
        m_history.append(action);

        m_states[w].currentStyleSheet = newStyle;
        m_states[w].currentFont = newFont;
        m_states[w].customBg = bg;
        m_states[w].customFg = fg;
        m_states[w].changed = true;

        w->setStyleSheet(newStyle);
        w->setFont(newFont);
        m_btnUndo->setEnabled(true);
        updateHighlight();

        m_bgSwatch->setStyleSheet(QString("background-color: %1; border: 1px solid #64748b; border-radius: 3px;")
            .arg(bg.isEmpty() ? "#1e293b" : bg));
        m_fgSwatch->setStyleSheet(QString("background-color: %1; border: 1px solid #64748b; border-radius: 3px;")
            .arg(fg.isEmpty() ? "#f1f5f9" : fg));
        m_statusFeedback->setText("✓ Applied in real-time. Click 'Save' to persist to Qwain.conf.");
    }

    void updateWidgetStyle(QWidget *w, const QString &bg, const QString &fg) {
        if (!w) return;
        bool isDialog = (w == m_targetWnd);
        QString baseStyle = m_states[w].originalStyleSheet;
        
        if (isDialog) {
            QString extra;
            if (!bg.isEmpty()) extra += QString(" background-color: %1; ").arg(bg);
            if (!fg.isEmpty()) extra += QString(" color: %1; ").arg(fg);
            applyChange(w, baseStyle + extra, m_states[w].currentFont, bg, fg);
        } else {
            QString extra;
            if (!bg.isEmpty()) extra += QString(" background-color: %1;").arg(bg);
            if (!fg.isEmpty()) extra += QString(" color: %1;").arg(fg);
            
            QString newStyle = baseStyle;
            if (!extra.isEmpty()) {
                if (!newStyle.isEmpty() && !newStyle.endsWith(";")) newStyle += ";";
                newStyle += extra;
            }
            applyChange(w, newStyle, m_states[w].currentFont, bg, fg);
        }
    }

    void applyBgColor(const QString &hex) {
        QWidget *w = selectedWidget();
        if (!w) return;
        updateWidgetStyle(w, hex, m_states[w].customFg);
    }

    void chooseBgColor() {
        QWidget *w = selectedWidget();
        if (!w) return;
        QColor col = QColorDialog::getColor(Qt::white, this, "Choose Background Color");
        if (col.isValid()) {
            applyBgColor(col.name());
        }
    }

    void chooseFgColor() {
        QWidget *w = selectedWidget();
        if (!w) return;
        QColor col = QColorDialog::getColor(Qt::black, this, "Choose Text/Foreground Color");
        if (col.isValid()) {
            updateWidgetStyle(w, m_states[w].customBg, col.name());
        }
    }

    void chooseFont() {
        QWidget *w = selectedWidget();
        if (!w) return;
        bool ok;
        QFont font = QFontDialog::getFont(&ok, w->font(), this);
        if (ok) {
            m_fontSizeSpin->setValue(font.pointSize() > 0 ? font.pointSize() : 9);
            m_boldCheck->setChecked(font.bold());
            applyChange(w, m_states[w].currentStyleSheet, font, m_states[w].customBg, m_states[w].customFg);
        }
    }

    void onFontSizeChanged(int sz) {
        QWidget *w = selectedWidget();
        if (!w) return;
        QFont f = m_states[w].currentFont;
        f.setPointSize(sz);
        applyChange(w, m_states[w].currentStyleSheet, f, m_states[w].customBg, m_states[w].customFg);
    }

    void onBoldToggled(bool b) {
        QWidget *w = selectedWidget();
        if (!w) return;
        QFont f = m_states[w].currentFont;
        f.setBold(b);
        applyChange(w, m_states[w].currentStyleSheet, f, m_states[w].customBg, m_states[w].customFg);
    }

    void undo() {
        if (m_history.isEmpty()) return;
        UndoAction last = m_history.takeLast();
        if (last.widget) {
            QWidget *w = last.widget;
            m_states[w].currentStyleSheet = last.styleSheetBefore;
            m_states[w].currentFont = last.fontBefore;
            m_states[w].customBg = last.bgBefore;
            m_states[w].customFg = last.fgBefore;
            m_states[w].changed = (m_states[w].currentStyleSheet != m_states[w].originalStyleSheet || 
                                   m_states[w].currentFont != m_states[w].originalFont);

            w->setStyleSheet(last.styleSheetBefore);
            w->setFont(last.fontBefore);
            updateHighlight();
        }
        m_btnUndo->setEnabled(!m_history.isEmpty());
        m_statusFeedback->setText("Action undone.");
    }

    void saveToIni() {
        QSettings settings("Qwain", "Qwain");
        QString groupName = QString("WindowStyles/%1").arg(m_functionalId);
        settings.beginGroup(groupName);

        // Store window background
        if (m_states.contains(m_targetWnd) && !m_states[m_targetWnd].customBg.isEmpty()) {
            settings.setValue("WindowBg", m_states[m_targetWnd].customBg);
        }

        settings.beginGroup("Elements");
        int count = 0;
        for (auto it = m_states.begin(); it != m_states.end(); ++it) {
            if (it.value().changed && it.value().widget) {
                QString key = it.value().elementKey;
                settings.beginGroup(key);
                settings.setValue("styleSheet", it.value().currentStyleSheet);
                settings.setValue("font", it.value().currentFont.toString());
                settings.endGroup();
                count++;
            }
        }
        settings.endGroup(); // Elements
        settings.endGroup(); // WindowStyles/<functionalId>
        settings.sync();

        m_savedPermanently = true;
        m_statusFeedback->setText(QString("★ Successfully saved %1 customizations to [WindowStyles/%2] in Qwain.conf!").arg(count).arg(m_functionalId));
        QMessageBox::information(this, "Theme Saved", 
            QString("Successfully saved %1 customizations for '%2'!\n\nThese overrides are stored in ~/.config/Qwain/Qwain.conf and will load automatically.")
            .arg(count).arg(m_functionalId));
    }

    // =========================================================================
    // FEATURE: Reset Changes
    // PURPOSE: Reverts the window design back to what it was when the window opened.
    //          Restores the original stylesheets and fonts captured at window opening,
    //          without wiping stored configuration presets in Qwain.conf.
    // =========================================================================
    void resetChanges() {
        for (auto it = m_states.begin(); it != m_states.end(); ++it) {
            if (it.value().widget) {
                // Revert each element back to baseline state captured when the window opened
                it.value().widget->setStyleSheet(it.value().originalStyleSheet);
                it.value().widget->setFont(it.value().originalFont);
                it.value().currentStyleSheet = it.value().originalStyleSheet;
                it.value().currentFont = it.value().originalFont;
                it.value().customBg = "";
                it.value().customFg = "";
                it.value().changed = false;
            }
        }
        m_history.clear();
        m_btnUndo->setEnabled(false);
        m_savedPermanently = false;

        // Refresh visual control indicators for the currently active element
        QWidget *w = selectedWidget();
        if (w && m_states.contains(w)) {
            QFont f = m_states[w].currentFont;
            m_fontSizeSpin->blockSignals(true);
            m_fontSizeSpin->setValue(f.pointSize() > 0 ? f.pointSize() : 9);
            m_fontSizeSpin->blockSignals(false);

            m_boldCheck->blockSignals(true);
            m_boldCheck->setChecked(f.bold());
            m_boldCheck->blockSignals(false);

            m_bgSwatch->setStyleSheet("background-color: #1e293b; border: 1px solid #64748b; border-radius: 3px;");
            m_fgSwatch->setStyleSheet("background-color: #f1f5f9; border: 1px solid #64748b; border-radius: 3px;");
        }

        // Reposition selection highlight overlay
        updateHighlight();

        m_statusFeedback->setText("✓ Reset changes: Window design reverted to state when window opened.");
        QMessageBox::information(this, "Reset Changes", 
            QString("Window design for '%1' has been reverted to what it was when the window opened.")
            .arg(m_functionalId));
    }
};

// Implementation of WindowSchemeManager::createCornerToggle
// Creates the corner toggle checkbox in the lowest right corner of target windows
// (naked checkbox without bracketed text, defaulting to unchecked false upon opening).
inline QCheckBox* WindowSchemeManager::createCornerToggle(QWidget *targetWindow, QWidget *parent) {
    QWidget *p = parent ? parent : targetWindow;
    QCheckBox *cb = new QCheckBox(p);
    cb->setObjectName("schemeCornerToggle");
    cb->setChecked(false); // DEFAULTS TO UNCHECKED on window opening per specification
    cb->setToolTip("Toggle Window Scheme & Appearance Tuner (In-Situ Element Matcher)");
    cb->setStyleSheet(
        "QCheckBox#schemeCornerToggle { "
        "  margin: 0px; padding: 2px; "
        "} "
    );

    QPointer<QWidget> tgt(targetWindow);
    QPointer<QCheckBox> safeCb(cb);
    QObject::connect(cb, &QCheckBox::toggled, [safeCb, tgt](bool checked) {
        if (!tgt) return;
        if (checked) {
            SchemeDesignerDialog *dlg = new SchemeDesignerDialog(tgt, tgt);
            // Use safeCb.data() as context object so Qt automatically cleans up connection if cb is deleted first
            QObject::connect(dlg, &QDialog::destroyed, safeCb.data(), [safeCb]() {
                if (safeCb) {
                    safeCb->blockSignals(true);
                    safeCb->setChecked(false);
                    safeCb->blockSignals(false);
                }
            });
            dlg->show();
        } else {
            // Find existing SchemeDesignerDialog and close it
            QList<SchemeDesignerDialog*> openDlgs = tgt->findChildren<SchemeDesignerDialog*>();
            for (SchemeDesignerDialog *d : openDlgs) {
                d->close();
            }
        }
    });

    return cb;
}

#endif // SCHEMEDESIGNER_H
