enum ItemType {
    ITEM_TEXT = 0,
    ITEM_TABLE = 1,
    ITEM_ADDRESS = 2,
    ITEM_IMAGE = 3,
    ITEM_STAMP = 4,
    ITEM_REF_POINT = 5,
    ITEM_CHECKMARK = 6,
    ITEM_CIRCLE = 7
};

// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
// ============================================================================
// COMPONENT: QSL Card & Envelope Designer (qsldesigner.h)
// ROLE: Vector layout designer hosting a QGraphicsScene canvas.
//       Supports drag-and-drop elements, auto-incrementing element names,
//       QSO Block table editors, font styling, and JSON template serialization.
// INVOKED BY: MainWindow menu actions / QSL Manager toolbar.
// PRODUCES: QSL card & mailing envelope JSON templates (*.json).
// ============================================================================

#ifndef QSLDESIGNER_H
#define QSLDESIGNER_H

#include <QDialog>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <QListWidget>
#include <QTcpSocket>

class MyGraphicsView : public QGraphicsView {
    Q_OBJECT
public:
    explicit MyGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr);
signals:
    void mouseMoved(QPointF scenePos);
    void mouseReleased();
protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
private:
    bool m_isPanning = false;
    QPoint m_panStartPos;
};

class QslDesigner : public QDialog
{
public:
    QGraphicsScene* scene() const { return m_scene; }
    QString currentTemplateName() const { return m_currentTemplateName; }

    Q_OBJECT
    friend class MyGraphicsView;
    friend class QslTemplateIO;
public:
    explicit QslDesigner(QWidget *parent = nullptr);
    QString getCurrentTemplateName() const { return m_currentTemplateName; }
    ~QslDesigner();
    void reject() override;
    void pushUndo();
    void setModified(bool mod = true) { m_isModified = mod; }
    void loadTemplate(const QString &name);
    
    QList<QVariantMap> m_copiedElements;
    void copySelectedElements();
    void pasteElements();
    bool canUndo() const;
    
    QString m_addMode;
    QString m_addArg;
    bool m_isEnvelope = false;
    
    bool m_isOpenedByQslProcessor = false;

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

public slots:
    void updateTemplateList();
private slots:
    void removePrintArea();
    void setBackgroundImage(const QString &forcePath = "");
    void saveCurrentTemplate();
    void saveTemplateWithName(const QString &name);
    void loadSelectedTemplate();
    void renameTemplate();
    void deleteTemplate();
    bool changePaperSize();
    void onSelectionChanged();
    void updateActiveItemProperties();
    void updateBlockList();
    void onMouseMoved(QPointF scenePos);
    
    void testPrintWifi();
    void duplicateTemplate();
    
    void newTemplate();
    void undoAction();
    void addCustomCheckmark();
    void addReferencePoint();
    void defineFields();
    void toggleBackground();
    void printerSettings();
    void addNewField();
    void triggerDebugDump();
    
    // Calculates a visible, non-overlapping initial placement for newly added elements
    QPointF findInitialElementPos(double itemW, double itemH);
    
private:
    void updateButtonStates();
    QList<QVariantMap> m_undoStack;
    QVariantMap serializeState();
    void restoreState(const QVariantMap &state);
    
    bool m_isModified;
    int m_addType;
    int m_addFieldMaxLines;
    int m_qsoTableRows;
    int m_qsoTableCols;
    bool m_qsoTableShowHeaders;
    bool m_qsoTableShowBorders;
    double m_qsoTableBorderThickness;
    QStringList m_qsoTableCellData;
    QGraphicsScene *m_scene = nullptr;
    MyGraphicsView *m_view = nullptr;
    QListWidget *m_cardTemplateList = nullptr;
    QListWidget *m_envTemplateList = nullptr;
    QListWidgetItem* getSelectedTemplateItem();
    int m_buttonConfigMode;
    
    // Crosshairs and labels
    QGraphicsLineItem *m_crosshairH = nullptr;
    QGraphicsLineItem *m_crosshairV = nullptr;
    class QLabel *m_cursorLabel = nullptr;
    class QComboBox *m_unitCombo = nullptr;
    class QTextBrowser *m_telemetryBrowser = nullptr;
    QPointF m_lastMouseScenePos;
    void updateTelemetry(QPointF mousePos = QPointF());
    
    // Properties widgets
    class QLineEdit *m_idEdit = nullptr;
    class QComboBox *m_fontCombo = nullptr;
    class QSpinBox *m_sizeSpin = nullptr;
    class QPushButton *m_colorBtn = nullptr;
    class QSpinBox *m_rotSpin = nullptr;
    class QSpinBox *m_widthSpin = nullptr;
    class QSpinBox *m_heightSpin = nullptr;
    class QTableWidget *m_blockList = nullptr;
    class QDialog *m_blocksWindow = nullptr;
    class QGraphicsPixmapItem *m_bgImageItem = nullptr;
    
    class QPushButton *btnCopy = nullptr;
    class QPushButton *btnPaste = nullptr;
    class QPushButton *btnRm = nullptr;
    class QPushButton *btnUndo = nullptr;
    class QToolButton *btnTestView = nullptr;
    QColor m_activeColor;

    QString m_bgImagePath;
    double m_paperW;
    double m_paperH;
    int m_printCorrection;
    double m_printOffsetX;
    double m_printOffsetY;
    QString m_printerName;
    QString m_currentTemplateName;
    bool m_templateActive = false;
    bool m_hasCopiedLayers = false;
    bool m_isTestView = false;
    bool m_isDestroying = false;

    friend class TelemetryDialog;

    void drawBackgroundRulers();
};

#endif // QSLDESIGNER_H
