// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
// ============================================================================
// COMPONENT: Application Entry Point (main.cpp)
// ROLE: Bootstraps the Qwain ecosystem application context.
//       Configures organization settings, high-DPI scaling, instantiates
//       the splash screen, initializes MainWindow, and enters app.exec().
// INVOKED BY: Operating System process launcher.
// PRODUCES: Running Qwain Application process.
// ============================================================================

#include <QApplication>
#include <QLineEdit>
#include <QRegularExpression>
#include "mainwindow.h"
#include "paths.h"
#include <QProcessEnvironment>
#include <QSettings>
#include <QFont>
#include <QSplashScreen>
#include <QTimer>
#include <QScreen>
#include <QPropertyAnimation>
#include <QEventLoop>
#include <QPixmap>
#include <QImage>
#include <QGraphicsOpacityEffect>
#include <QStyle>
#include <QKeyEvent>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QClipboard>
#include <QSharedMemory>
#include <QMessageBox>
#include <QTextEdit>
#include <QTableWidget>
#include <QListWidget>
#include <QComboBox>
#include <QDir>
#include <QFile>

QPixmap getAppLogo() {
    QPixmap pm("qwain_blue_small_200.png");
    if (pm.isNull()) pm = QPixmap("cpp_source/qwain_blue_small_200.png");
    if (pm.isNull()) pm = QPixmap(":/qwain_blue_small_200.png");
    if (pm.isNull()) pm = QPixmap("logo_QWAIN.png");
    if (pm.isNull()) pm = QPixmap("cpp_source/logo_QWAIN.png");
    if (pm.isNull()) pm = QPixmap(":/logo.png");
    if (pm.isNull()) pm = QPixmap("logo.png");
    if (pm.isNull()) pm = QPixmap("cpp_source/logo.png");
    if (pm.isNull()) pm = QPixmap("arrl.png"); 
    return pm;
}

#include "loadingsplash.h"

class GlobalShortcutFilter : public QObject {
public:
    explicit GlobalShortcutFilter(QObject *parent = nullptr) : QObject(parent) {}
    
    QJsonObject dumpWidgetRecursive(QWidget *widget, int depth) {
        QJsonObject obj;
        obj["class"] = widget->metaObject()->className();
        obj["w"] = widget->width();
        obj["h"] = widget->height();
        
        QVariant textProp = widget->property("text");
        if (textProp.isValid() && !textProp.toString().trimmed().isEmpty()) {
            obj["text"] = textProp.toString();
        } else if (widget->property("title").isValid() && !widget->property("title").toString().isEmpty()) {
            obj["title"] = widget->property("title").toString();
        } else if (widget->property("plainText").isValid() && !widget->property("plainText").toString().isEmpty()) {
            obj["text"] = widget->property("plainText").toString();
        } else if (widget->property("windowTitle").isValid() && !widget->property("windowTitle").toString().isEmpty()) {
            obj["title"] = widget->property("windowTitle").toString();
        }

        // Capture data from common widgets to capture ALL panes and all data shown
        if (QTextEdit *te = qobject_cast<QTextEdit*>(widget)) {
            obj["text"] = te->toPlainText();
        } else if (QTableWidget *tw = qobject_cast<QTableWidget*>(widget)) {
            QJsonArray colWidths;
            for (int c = 0; c < tw->columnCount(); ++c) {
                colWidths.append(tw->columnWidth(c));
            }
            obj["columnWidths"] = colWidths;

            QJsonArray rows;
            for (int r = 0; r < tw->rowCount(); ++r) {
                QJsonArray cols;
                for (int c = 0; c < tw->columnCount(); ++c) {
                    QTableWidgetItem *item = tw->item(r, c);
                    cols.append(item ? item->text() : "");
                }
                rows.append(cols);
            }
            obj["tableData"] = rows;
        } else if (QListWidget *lw = qobject_cast<QListWidget*>(widget)) {
            QJsonArray items;
            for (int i = 0; i < lw->count(); ++i) {
                QListWidgetItem *item = lw->item(i);
                items.append(item ? item->text() : "");
            }
            obj["listData"] = items;
        } else if (QComboBox *cb = qobject_cast<QComboBox*>(widget)) {
            QJsonArray items;
            for (int i = 0; i < cb->count(); ++i) {
                items.append(cb->itemText(i));
            }
            obj["comboData"] = items;
            obj["currentText"] = cb->currentText();
        }
        
        if (depth < 20) {
            QJsonObject childrenObj;
            for (QObject *child : widget->children()) {
                if (child->isWidgetType()) {
                    QWidget *cw = static_cast<QWidget*>(child);
                    if (cw->isVisible()) {
                        QString key = cw->objectName();
                        if (key.isEmpty()) key = QString(cw->metaObject()->className()) + "_" + QString::number((quintptr)cw, 16);
                        childrenObj[key] = dumpWidgetRecursive(cw, depth + 1);
                    }
                }
            }
            if (!childrenObj.isEmpty()) obj["children"] = childrenObj;
        }
        return obj;
    }

    // ========================================================================
    // FUNCTIONAL WINDOW RESOLVER & TITLE CLEANER
    // Purpose: Maps Qt windows to clean functional IDs for configuration,
    //          and ensures all window titles are free of internal source filenames
    //          (e.g., [qsldesigner.cpp] or [<source>.cpp]) for clean, professional UX.
    // ========================================================================
    static QString resolveWindowFunctionalId(QWidget *widget) {
        if (!widget) return QString();
        if (widget->property("functional_id").isValid() && !widget->property("functional_id").toString().isEmpty()) {
            return widget->property("functional_id").toString();
        }

        auto classToFunctionalId = [](const QString &cls) -> QString {
            if (cls == "MainWindow" || cls == "SpaceInvadersWindow" || 
                cls == "BackupPreviewDialog" || cls == "BackupProgressDialog") {
                return "Main_Window";
            }
            if (cls == "QslDesigner" || cls == "AdvLayerDialog" || cls == "TelemetryDialog") {
                return "Card_Designer";
            }
            if (cls == "QslPreviewDialog" || cls == "UnifiedPrintDialog" || 
                cls == "CommentEditorDialog" || cls == "FinalTweaksDialog" || 
                cls == "QslTweakAdjustDialog" || cls == "PrintExceptionDialog" || 
                cls == "PrinterAnimationWidget" || cls == "ActivePrintAnimationWidget") {
                return "Print_Preview";
            }
            if (cls == "QslManagerDialog" || cls == "ProcessingStatusDialog" || 
                cls == "DateRangeDialog" || cls == "CountryDxccDialog") {
                return "QSL_Print_Manager";
            }
            if (cls == "ReceiveQslsDialog") {
                return "Receive_QSLs";
            }
            if (cls == "ConfigDialog" || cls == "UnderDevelopmentOverlay") {
                return "Station_Preferences";
            }
            if (cls == "StatsDialog") {
                return "Station_Statistics";
            }
            if (cls == "CacheViewerDialog" || cls == "RebuildProgressDialog") {
                return "Cache_Viewer";
            }
            if (cls == "ImageViewer") {
                return "Image_Viewer";
            }
            if (cls == "ImageEditorDialog") {
                return "Image_Editor";
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
            if (cls == "QslTemplateInspector") {
                return "Template_Inspector";
            }
            if (cls == "AddressRenderInspectorWindow") {
                return "Address_Render_Inspector";
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
            if (cls == "IconToolsDialog") {
                return "Icon_Tools";
            }
            if (cls == "GtkFontChooserWidget") {
                return "Font_Chooser";
            }
            if (cls == "StyleDialog") {
                return "Style_Manager";
            }
            if (cls == "TxTweakerWidget") {
                return "Tx_Tweaker";
            }
            if (cls == "SchemeDesignerDialog") {
                return "Scheme_Designer";
            }
            if (cls == "LoadingSplash") {
                return "Splash_Screen";
            }
            return "Application_Window";
        };

        // 1. Direct class check
        QString selfId = classToFunctionalId(widget->metaObject()->className());
        if (!selfId.isEmpty() && selfId != "Application_Window") return selfId;

        // 2. Parent hierarchy check
        for (QWidget *p = widget->parentWidget(); p != nullptr; p = p->parentWidget()) {
            if (p->property("functional_id").isValid() && !p->property("functional_id").toString().isEmpty()) {
                return p->property("functional_id").toString();
            }
            QString pId = classToFunctionalId(p->metaObject()->className());
            if (!pId.isEmpty() && pId != "Application_Window") return pId;
        }

        return "Application_Window";
    }

    // ========================================================================
    // SOURCE CODE FILENAME RESOLVER & TITLE ANNOTATOR
    // Purpose: Restores developer source code filename tags (e.g., [qsldesigner.cpp],
    //          [qslmanagerdialog.cpp], [mainwindow.cpp]) to window title bars.
    // ========================================================================
    static QString resolveSourceFileForWidget(QWidget *widget) {
        if (!widget) return QString();

        auto classToSourceFile = [](const QString &cls) -> QString {
            if (cls == "MainWindow" || cls == "SpaceInvadersWindow" || 
                cls == "BackupPreviewDialog" || cls == "BackupProgressDialog") {
                return "mainwindow.cpp";
            }
            if (cls == "QslDesigner" || cls == "AdvLayerDialog" || cls == "TelemetryDialog") {
                return "qsldesigner.cpp";
            }
            if (cls == "QslPreviewDialog" || cls == "UnifiedPrintDialog" || 
                cls == "CommentEditorDialog" || cls == "FinalTweaksDialog" || 
                cls == "QslTweakAdjustDialog" || cls == "PrintExceptionDialog" || 
                cls == "PrinterAnimationWidget" || cls == "ActivePrintAnimationWidget") {
                return "qslpreviewdialog.cpp";
            }
            if (cls == "QslManagerDialog" || cls == "ProcessingStatusDialog" || 
                cls == "DateRangeDialog" || cls == "CountryDxccDialog") {
                return "qslmanagerdialog.cpp";
            }
            if (cls == "ReceiveQslsDialog") {
                return "receiveqsldialog.cpp";
            }
            if (cls == "ConfigDialog" || cls == "UnderDevelopmentOverlay") {
                return "configdialog.cpp";
            }
            if (cls == "StatsDialog") {
                return "statsdialog.cpp";
            }
            if (cls == "CacheViewerDialog" || cls == "RebuildProgressDialog") {
                return "cacheviewerdialog.cpp";
            }
            if (cls == "ImageViewer") {
                return "imageviewer.cpp";
            }
            if (cls == "ImageEditorDialog") {
                return "imageeditordialog.cpp";
            }
            if (cls == "AwardsWidget") {
                return "awardswidget.cpp";
            }
            if (cls == "GraylineWidget") {
                return "graylinewidget.cpp";
            }
            if (cls == "DxClusterWidget") {
                return "dxclusterwidget.cpp";
            }
            if (cls == "MailCheckDialog") {
                return "mailcheckdialog.cpp";
            }
            if (cls == "PrefetchFlagsDialog") {
                return "prefetchflagsdialog.cpp";
            }
            if (cls == "QrzSyncDialog") {
                return "qrzsyncdialog.cpp";
            }
            if (cls == "CabrilloConverterDialog") {
                return "cabrilloconverterdialog.cpp";
            }
            if (cls == "QslTemplateInspector") {
                return "qsltemplateinspector.cpp";
            }
            if (cls == "AddressRenderInspectorWindow") {
                return "addressrenderinspectorwindow.cpp";
            }
            if (cls == "CountyPrepDialog") {
                return "countyprepdialog.cpp";
            }
            if (cls == "LiveStreamViewer") {
                return "livestreamviewer.cpp";
            }
            if (cls == "QslStatusDashboard") {
                return "qslstatusdashboard.cpp";
            }
            if (cls == "IconToolsDialog") {
                return "icontoolsdialog.cpp";
            }
            if (cls == "GtkFontChooserWidget") {
                return "gtkfontchooserwidget.cpp";
            }
            if (cls == "StyleDialog") {
                return "styledialog.cpp";
            }
            if (cls == "TxTweakerWidget") {
                return "txtweakerwidget.cpp";
            }
            if (cls == "SchemeDesignerDialog") {
                return "schemedesigner.h";
            }
            return QString();
        };

        QString selfCls = widget->metaObject()->className();
        QString src = classToSourceFile(selfCls);
        if (!src.isEmpty()) return src;

        // Check parent hierarchy
        for (QWidget *p = widget->parentWidget(); p != nullptr; p = p->parentWidget()) {
            QString pCls = p->metaObject()->className();
            QString pSrc = classToSourceFile(pCls);
            if (!pSrc.isEmpty()) return pSrc;
        }

        return selfCls.toLower() + ".cpp";
    }

    // Appends developer sourcecode filename tag (e.g., '[qsldesigner.cpp]') to window titles
    void updateWindowTitleWithSourceCpp(QWidget *widget) {
        if (!widget || !widget->isWindow()) return;
        Qt::WindowType wt = widget->windowType();
        if (wt == Qt::ToolTip || wt == Qt::Popup || wt == Qt::SplashScreen) return;

        if (widget->property("_updating_source_title").toBool()) return;

        QString sourceFile = resolveSourceFileForWidget(widget);
        if (sourceFile.isEmpty()) return;

        QString tag = QString("[%1]").arg(sourceFile);
        QString currentTitle = widget->windowTitle();

        // If title already contains this tag, do nothing
        if (currentTitle.contains(tag)) return;

        // Strip any existing source tags [*.cpp] or [*.h] first
        static QRegularExpression oldTagRegex(R"(\s*\[[a-zA-Z0-9_\-\.]+\.(cpp|h)\]\s*)");
        QString clean = currentTitle;
        clean.remove(oldTagRegex);
        clean = clean.trimmed();

        QString newTitle = clean.isEmpty() ? tag : (clean + " " + tag);

        widget->setProperty("_updating_source_title", true);
        widget->setWindowTitle(newTitle);
        widget->setProperty("_updating_source_title", false);
    }

    bool eventFilter(QObject *obj, QEvent *event) override {
        // Ensure open windows and dialogs display their source code filename in the window title bar
        if (obj && obj->isWidgetType()) {
            QWidget *w = static_cast<QWidget*>(obj);
            if (w->isWindow()) {
                if (event->type() == QEvent::Show || event->type() == QEvent::WindowTitleChange) {
                    updateWindowTitleWithSourceCpp(w);
                }
            }
        }

        if (obj) {
            if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(obj)) {
                if (!lineEdit->property("slashedZeroHooked").toBool()) {
                    lineEdit->setProperty("slashedZeroHooked", true);
                    connect(lineEdit, &QLineEdit::textEdited, lineEdit, [lineEdit](const QString &text) {
                        QString normalized = text;
                        normalized.replace(QRegularExpression("[Øø∅]"), "0");
                        if (normalized != text) {
                            int pos = lineEdit->cursorPosition();
                            lineEdit->setText(normalized);
                            lineEdit->setCursorPosition(pos);
                        }
                    });
                }
            }
        }

        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_W && keyEvent->modifiers() == Qt::NoModifier) {
                QWidget *focused = QApplication::focusWidget();
                if (focused) {
                    bool isEditableText = false;
                    if (focused->inherits("QLineEdit") || 
                        focused->inherits("QTextEdit") || 
                        focused->inherits("QPlainTextEdit")) {
                        if (!focused->property("readOnly").toBool()) {
                            isEditableText = true;
                        }
                    } else if (focused->inherits("QComboBox") || 
                               focused->inherits("QAbstractSpinBox")) {
                        isEditableText = true;
                    }
                    if (isEditableText) {
                        return QObject::eventFilter(obj, event);
                    }
                }
                
                QJsonObject sizingData;
                QWidget *active = QApplication::activeWindow();
                if (active) sizingData["_activeWindow"] = active->objectName().isEmpty() ? active->windowTitle() : active->objectName();
                
                for (QWidget *widget : QApplication::topLevelWidgets()) {
                    if (widget->isVisible()) {
                        QString key = widget->objectName();
                        if (key.isEmpty()) key = widget->windowTitle();
                        if (key.isEmpty()) key = widget->metaObject()->className();
                        sizingData[key] = dumpWidgetRecursive(widget, 0);
                    }
                }
                
                QByteArray jsonStr = QJsonDocument(sizingData).toJson(QJsonDocument::Indented);
                QApplication::clipboard()->setText(QString::fromUtf8(jsonStr));
            }
        }
        return QObject::eventFilter(obj, event);
    }
};

int main(int argc, char *argv[])
{
    qputenv("QT_ACCESSIBILITY", "0");
    qputenv("QT_LINUX_ACCESSIBILITY_ALWAYS_ON", "0");
    qunsetenv("QT_LINUX_ACCESSIBILITY_ALWAYS_ON");
    qputenv("NO_AT_BRIDGE", "1");
    qputenv("GTK_A11Y", "none");
    qputenv("QT_PLATFORM_THEME", "");
    qputenv("QT_QPA_PLATFORMTHEME", "");
    // Also ignore GTK's native atk-bridge module loading warning if possible
    qputenv("GTK_MODULES", "");
    
    QApplication app(argc, argv);
    
    QSharedMemory sharedMemory("QwainSingleInstanceMemoryKey");
    if (!sharedMemory.create(1)) {
        // Stale segment cleanup: try to attach and detach to release it if orphaned
        sharedMemory.attach();
        sharedMemory.detach();
        if (!sharedMemory.create(1)) {
            QMessageBox::warning(nullptr, "Qwain", "An instance of Qwain is already running.");
            return 0;
        }
    }

    app.installEventFilter(new GlobalShortcutFilter(&app));
    QApplication::setApplicationName("Qwain");
    app.setStyleSheet("QMessageBox QLabel { qproperty-wordWrap: true; } QLabel { qproperty-wordWrap: true; }");
    
    QPixmap appLogo = getAppLogo();
    if (!appLogo.isNull()) {
        app.setWindowIcon(QIcon(appLogo));
    } else {
        app.setWindowIcon(QIcon("arrl_small.png")); // fallback
    }
    
    QString callsign = qEnvironmentVariable("USER");
    if (callsign.isEmpty()) callsign = qEnvironmentVariable("USERNAME");
    if (callsign.isEmpty()) callsign = "DefaultCallsign";
    
    // Set standard organization name so everything starts life under ~/.config/Qwain
    QApplication::setOrganizationName("Qwain");
    QApplication::setApplicationName("Qwain");
    QApplication::setApplicationVersion(getAppVersion());

    // ---- Animated Splash Screen Logic ----
    LoadingSplash *splash = new LoadingSplash();
    splash->show();
    QCoreApplication::processEvents();
    // ----------------------------------------

    QSettings newSettings("Qwain", "Qwain");

    if (newSettings.value("Misc/Diagnostics", false).toBool() || app.arguments().contains("--diag")) {
        // Watchdog removed
    }

    QSettings oldSettings("MyCompany", "Qwain");
    if (oldSettings.allKeys().size() > 0 && newSettings.allKeys().size() == 0) {
        for (const QString &key : oldSettings.allKeys()) {
            newSettings.setValue(key, oldSettings.value(key));
        }
        newSettings.sync();
        
        // Also try migrating Qwain_EXIF
        QSettings oldSettingsExif("MyCompany", "Qwain_EXIF");
        QSettings newSettingsExif("Qwain", "Qwain_EXIF");
        for (const QString &key : oldSettingsExif.allKeys()) {
            newSettingsExif.setValue(key, oldSettingsExif.value(key));
        }
        newSettingsExif.sync();
    }

    MainWindow w(splash);
    QScreen *primaryScreen = app.primaryScreen();
    if (primaryScreen) {
        QRect screenGeom = primaryScreen->availableGeometry();
        w.setGeometry(
            QStyle::alignedRect(
                Qt::LeftToRight,
                Qt::AlignCenter,
                w.sizeHint(),
                screenGeom
            )
        );
    }
    w.show();

    if (splash) {
        // Program readiness achieved, now fade out
        // Note: The UI is already shown in the background (centered).
        QPropertyAnimation *fadeOut = new QPropertyAnimation(splash, "windowOpacity");
        fadeOut->setDuration(1000);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        QObject::connect(fadeOut, &QPropertyAnimation::finished, splash, [splash, &w]() {
            splash->close();
            w.raise();
            w.activateWindow();
        });
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    }

    w.raise();
    w.activateWindow();

    return app.exec();
}
