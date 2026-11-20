#include "qsltemplateinspector.h"
#include "qrcodegen.hpp"
#include "schemedesigner.h"
#include <QDateTime>
// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#include <QCoreApplication>
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QUrl>
#include <QDataStream>
#include <QProcess>

// ============================================================================
// COMPONENT: QslDesigner
// ROLE: Interactive WYSIWYG QSL card and envelope layout canvas editor.
//       This component coordinates mouse events, drag-and-drop panning, 
//       undo/redo history state, crosshair cursor-tracking, custom reference
//       points, and design elements serialization/deserialization.
// INVOKED BY: MainWindow (via the "Design QSLs" action).
// PRODUCES: Custom structured templates (XML, JSON) loaded into QslPreviewDialog.
// ============================================================================

#include "qsldesigner.h"
#include "qsldesigner_tpl.h"
#include "paths.h"
#include <QFileDialog>
#include <QKeyEvent>
#include "qslpreviewdialog.h"
#include "gtkfontchooserwidget.h"
#include <QListWidget>
#include <QRegularExpression>
#include <QToolButton>
#include <QBuffer>
#include <memory>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QGraphicsRectItem>
#include <QInputDialog>
#include <QColorDialog>
#include "styledialog.h"
#include <QSplitter>
#include <QSettings>
#include <QSet>
#include <QPen>
#include <QScrollBar>
#include <QBrush>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QFontDatabase>
#include <QGraphicsSceneMouseEvent>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QTextEdit>
#include <QMouseEvent>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrinterInfo>
#include <QPageSize>
#include <QPageLayout>
#include <QPdfWriter>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QProgressDialog>
#include <QTableWidget>
#include <QFileInfo>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QSlider>

#include <QDial>
#include <QFontComboBox>
#include <QTime>


static void showCalibrationMessage(QWidget *parent, const QString &title, const QString &text) {
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(title);
    msgBox.setText(text);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStyleSheet(
        "QMessageBox {"
        "  background-color: #1e222b;"
        "  border: 1px solid #3f4452;"
        "  border-radius: 8px;"
        "}"
        "QLabel {"
        "  color: #f1f5f9;"
        "  font-family: 'Segoe UI', Arial, sans-serif;"
        "  font-size: 11pt;"
        "  padding: 10px;"
        "}"
        "QPushButton {"
        "  background-color: #3b82f6;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 8px 16px;"
        "  font-weight: bold;"
        "  font-size: 10pt;"
        "  min-width: 100px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2563eb;"
        "}"
    );
    msgBox.exec();
}

MyGraphicsView::MyGraphicsView(QGraphicsScene *scene, QWidget *parent) : QGraphicsView(scene, parent) {
    setMouseTracking(true);
    m_isPanning = false;
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

// Removed from here

void MyGraphicsView::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() > 0) {
        scale(1.1, 1.1);
    } else {
        scale(1.0 / 1.1, 1.0 / 1.1);
    }
    event->accept();
}

void MyGraphicsView::keyPressEvent(QKeyEvent *event) {
    if (scene() && scene()->property("isTestView").toBool()) {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    if (!scene()) {
        QGraphicsView::keyPressEvent(event);
        return;
    }
    QList<QGraphicsItem*> selected = scene()->selectedItems();
    if (!selected.isEmpty() && (
        event->key() == Qt::Key_Left ||
        event->key() == Qt::Key_Right ||
        event->key() == Qt::Key_Up ||
        event->key() == Qt::Key_Down)) {
        
        double step = 1.0;
        if (event->modifiers() & Qt::ShiftModifier) {
            step = 10.0;
        }
        
        double dx = 0;
        double dy = 0;
        if (event->key() == Qt::Key_Left) dx = -step;
        else if (event->key() == Qt::Key_Right) dx = step;
        else if (event->key() == Qt::Key_Up) dy = -step;
        else if (event->key() == Qt::Key_Down) dy = step;
        
        QslDesigner *designer = nullptr;
        QWidget *p = parentWidget();
        while (p) {
            designer = qobject_cast<QslDesigner*>(p);
            if (designer) break;
            p = p->parentWidget();
        }
        
        if (designer) {
            designer->pushUndo();
            designer->setModified(true);
        }
        
        for (QGraphicsItem *item : selected) {
            item->setPos(item->pos() + QPointF(dx, dy));
        }
        
        if (designer) {
            designer->drawBackgroundRulers();
            designer->updateTelemetry(QPointF());
        }
        
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

#include <QMenu>
#include <QFormLayout>
#include <QShortcut>
#include <QKeySequence>
#include <QScrollArea>
#include <QTimer>
#include <QPropertyAnimation>
#include <QEasingCurve>

class PrintTableArea : public QGraphicsRectItem {
public:
    static QString translateTestViewText(const QString &text) {
        QString res = text.trimmed();
        QString lower = res.toLower();
        
        // If the entire text matches a standard field name case-insensitively with or without brackets
        QString clean = lower;
        if (clean.startsWith("<") && clean.endsWith(">")) {
            clean = clean.mid(1, clean.length() - 2).trimmed();
        }
        
        if (clean == "call" || clean == "callsign") {
            return "W4DJF";
        }
        if (clean == "commentbox" || clean == "comment" || clean == "remarks" || clean == "notes" || clean == "msg" || clean == "qslmsg" || clean == "message") {
            return "Thanks for the nice QSO! 73, Joe";
        }
        if (clean == "name") {
            return "John Doe";
        }
        if (clean == "qth" || clean == "city") {
            return "New York, NY";
        }
        if (clean == "state") {
            return "NY";
        }
        if (clean == "country") {
            return "UNITED STATES";
        }
        if (clean == "gridsquare" || clean == "grid") {
            return "FN30";
        }
        if (clean == "cnty" || clean == "county") {
            return "Fulton";
        }
        if (clean == "zip" || clean == "postal") {
            return "10001";
        }
        if (clean == "address" || clean == "addr" || clean == "full address" || clean == "address_box") {
            return "John Doe\n123 Ham Radio Way\nNew York, NY 10001";
        }
        if (clean == "day" || clean == "dy" || clean == "d") {
            return "27";
        }
        if (clean == "month" || clean == "mo" || clean == "m") {
            return "10";
        }
        if (clean == "year" || clean == "yr" || clean == "y") {
            return "2026";
        }
        if (clean == "date" || clean == "qso_date") {
            return "2026-10-27";
        }
        if (clean == "time" || clean == "utc" || clean == "time_on" || clean == "time_utc") {
            return "14:30";
        }
        if (clean == "band") {
            return "20m";
        }
        if (clean == "freq" || clean == "mhz") {
            return "14.250";
        }
        if (clean == "mode") {
            return "FT8";
        }
        if (clean == "pwr" || clean == "watt" || clean == "power" || clean == "tx_pwr") {
            return "100W";
        }
        if (clean == "rst_sent" || clean == "sent" || clean == "tx" || clean == "rst") {
            return "599";
        }
        if (clean == "rst_rcvd" || clean == "rcvd" || clean == "rx") {
            return "599";
        }
        if (clean == "pota_ref" || clean == "pota") {
            return "K-3221";
        }
        
        // Also do inline replacements inside a larger string (e.g. text templates like "73 de <CALL>")
        // Let's replace any <...> based on its inner content
        QRegularExpression re("<([^>]+)>");
        QRegularExpressionMatchIterator i = re.globalMatch(res);
        int offset = 0;
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString fullTag = match.captured(0);
            QString tagContent = match.captured(1).trimmed().toLower();
            QString replacement = fullTag; // fallback to original
            
            if (tagContent.contains("callsign") || tagContent == "call") {
                replacement = "W4DJF";
            } else if (tagContent.contains("comment") || tagContent.contains("remarks") || tagContent.contains("notes") || tagContent == "msg" || tagContent == "qslmsg" || tagContent == "message") {
                replacement = "Thanks for the nice QSO! 73, Joe";
            } else if (tagContent == "name") {
                replacement = "John Doe";
            } else if (tagContent == "qth" || tagContent == "city") {
                replacement = "New York, NY";
            } else if (tagContent == "state") {
                replacement = "NY";
            } else if (tagContent == "country") {
                replacement = "UNITED STATES";
            } else if (tagContent.contains("grid")) {
                replacement = "FN30";
            } else if (tagContent == "cnty" || tagContent == "county") {
                replacement = "Fulton";
            } else if (tagContent == "zip" || tagContent == "postal") {
                replacement = "10001";
            } else if (tagContent.contains("address") || tagContent == "addr" || tagContent == "address_box") {
                replacement = "John Doe\n123 Ham Radio Way\nNew York, NY 10001";
            } else if (tagContent == "day" || tagContent == "dy" || tagContent == "d") {
                replacement = "27";
            } else if (tagContent == "month" || tagContent == "mo" || tagContent == "m") {
                replacement = "10";
            } else if (tagContent == "year" || tagContent == "yr" || tagContent == "y") {
                replacement = "2026";
            } else if (tagContent.contains("date")) {
                replacement = "2026-10-27";
            } else if (tagContent.contains("time") || tagContent == "utc") {
                replacement = "14:30";
            } else if (tagContent == "band") {
                replacement = "20m";
            } else if (tagContent.contains("freq") || tagContent == "mhz") {
                replacement = "14.250";
            } else if (tagContent == "mode") {
                replacement = "FT8";
            } else if (tagContent.contains("pwr") || tagContent.contains("watt") || tagContent.contains("power")) {
                replacement = "100W";
            } else if (tagContent.contains("sent") || tagContent == "tx" || tagContent == "rst") {
                replacement = "599";
            } else if (tagContent.contains("rcvd") || tagContent == "rx") {
                replacement = "599";
            } else if (tagContent.contains("pota")) {
                replacement = "K-3221";
            }
            
            int matchPos = match.capturedStart(0) + offset;
            int matchLen = match.capturedLength(0);
            res.replace(matchPos, matchLen, replacement);
            offset += (replacement.length() - matchLen);
        }
        
        // Finally, if any <...> tags remain in test view, we should strip them or clean them up so they aren't visible
        res.remove(QRegularExpression("<[^>]*>"));
        return res;
    }

    QString areaId;
    QString textContent;
    QFont m_font;
    QColor m_color;
    int m_rows;
    int m_cols;
    int m_maxLines;
    int m_align;
    QString m_rowHeightsStr;
    bool m_isImage = false;
    int m_itemType = ITEM_TEXT;
    QString m_imagePath;
    QPixmap m_pixmap;
    bool m_isQrCode = false;
    QString m_qrPayload;
    std::vector<std::vector<bool>> m_qrModules;

    bool isQrCode() const {
        return m_isQrCode || areaId.startsWith("QR_CODE") || m_imagePath.contains("qr_");
    }

    void ensureQrModules() {
        if (!m_qrModules.empty()) return;
        QString payload = m_qrPayload;
        if (payload.isEmpty() && !textContent.isEmpty() && textContent != "QR_CODE") {
            payload = textContent;
        }
        if (!payload.isEmpty()) {
            try {
                using qrcodegen::QrCode;
                QrCode qr = QrCode::encodeText(payload.toUtf8().constData(), QrCode::Ecc::MEDIUM);
                int sz = qr.getSize();
                m_qrModules.assign(sz, std::vector<bool>(sz, false));
                for (int y = 0; y < sz; ++y) {
                    for (int x = 0; x < sz; ++x) {
                        m_qrModules[y][x] = qr.getModule(x, y);
                    }
                }
            } catch (...) {}
        }
    }
    QString m_colWidthsStr;
    bool m_isResizing;
    int m_resizeEdge;
    int m_draggingColDivider;
    int m_draggingRowDivider;
    QPointF m_resizeStart;
    QStringList m_cellData;
    bool m_isAddressBox;
    int m_circleLineWidth;
    bool m_showHeaders;
    bool m_showBorders;
    double m_borderThickness;
    
    // Bezier curve path properties
    bool m_useBezierPath;
    QPointF m_p0_norm;
    QPointF m_p1_norm;
    QPointF m_p2_norm;
    QPointF m_p3_norm;
    int m_draggingControlPoint;
    QPointF m_initialScenePos;
    QRectF m_initialRect;
    QString m_initialColWidthsStr;
    QString m_initialRowHeightsStr;
    
    PrintTableArea(const QString& text, bool isImage=false, const QString& pth="") : QGraphicsRectItem(0, 0, 100, 50) {
        m_showHeaders = true;
        m_showBorders = true;
        m_borderThickness = 1.0;
        m_imagePath = pth;
        if(isImage && !pth.isEmpty()) m_pixmap.load(pth);
        areaId = text;
        if (areaId.startsWith("QR_CODE") || (isImage && pth.contains("qr_"))) {
            m_isQrCode = true;
        }
        if (m_isImage) {
            // Layered graphics default to zValue 1.5 so they are immediately clickable and draggable above tables
            setZValue(1.5);
        } else if (areaId.startsWith("CHK_") || areaId.startsWith("CIRC_")) {
            setZValue(2);
        } else {
            setZValue(1);
        }
        setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsFocusable);
        setAcceptHoverEvents(true);
        m_isAddressBox = (areaId == "ADDRESS_BOX");
        if (m_isAddressBox) {
            textContent = "Full Contact Name\nAddress Line 2\nCity, State, Zip";
        } else if (areaId == "<COUNTRY>") {
            textContent = "Country";
        } else {
            textContent = text;
        }
        m_color = Qt::black;
        m_circleLineWidth = 2;
        m_font = QFont("Arial", 12);
        m_rows = 1;
        m_cols = 1;
        m_maxLines = 0;
        m_align = m_isAddressBox ? Qt::AlignLeft : Qt::AlignCenter;
        m_cellData.append(textContent);
        m_isResizing = false;
        m_resizeEdge = 0;
        m_draggingColDivider = -1;
        m_draggingRowDivider = -1;
        
        m_useBezierPath = false;
        m_p0_norm = QPointF(0.05, 0.85);
        m_p1_norm = QPointF(0.33, 0.15);
        m_p2_norm = QPointF(0.67, 0.15);
        m_p3_norm = QPointF(0.95, 0.85);
        m_draggingControlPoint = -1;
        
        if (areaId.startsWith("CHK_")) {
            setRect(0, 0, 5, 5);
        } else {
            setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
        }
    }
    
    QPointF getP0() const { return QPointF(m_p0_norm.x() * rect().width(), m_p0_norm.y() * rect().height()); }
    QPointF getP1() const { return QPointF(m_p1_norm.x() * rect().width(), m_p1_norm.y() * rect().height()); }
    QPointF getP2() const { return QPointF(m_p2_norm.x() * rect().width(), m_p2_norm.y() * rect().height()); }
    QPointF getP3() const { return QPointF(m_p3_norm.x() * rect().width(), m_p3_norm.y() * rect().height()); }

    void setP0(const QPointF& p) { prepareGeometryChange(); m_p0_norm = QPointF(p.x() / rect().width(), p.y() / rect().height()); }
    void setP1(const QPointF& p) { prepareGeometryChange(); m_p1_norm = QPointF(p.x() / rect().width(), p.y() / rect().height()); }
    void setP2(const QPointF& p) { prepareGeometryChange(); m_p2_norm = QPointF(p.x() / rect().width(), p.y() / rect().height()); }
    void setP3(const QPointF& p) { prepareGeometryChange(); m_p3_norm = QPointF(p.x() / rect().width(), p.y() / rect().height()); }

    QPainterPath getBezierPath() const {
        QPainterPath path;
        path.moveTo(getP0());
        path.cubicTo(getP1(), getP2(), getP3());
        return path;
    }
    
    void drawBezierHandles(QPainter *painter) {
        bool isPrinting = scene() && scene()->property("isPrinting").toBool();
        if (isPrinting) return;
        
        painter->save();
        painter->setPen(QPen(QColor(100, 150, 255), 1, Qt::SolidLine));
        painter->drawLine(getP0(), getP1());
        painter->drawLine(getP3(), getP2());
        
        QPainterPath curvePath = getBezierPath();
        painter->setPen(QPen(QColor(59, 130, 246), 2, Qt::DashLine));
        painter->drawPath(curvePath);
        
        painter->setPen(QPen(QColor(37, 99, 235), 2));
        
        painter->setBrush(QBrush(Qt::white));
        painter->drawRect(QRectF(getP0().x() - 4, getP0().y() - 4, 8, 8));
        painter->drawRect(QRectF(getP3().x() - 4, getP3().y() - 4, 8, 8));
        
        painter->setBrush(QBrush(QColor(59, 130, 246)));
        painter->drawEllipse(getP1(), 4, 4);
        painter->drawEllipse(getP2(), 4, 4);
        painter->restore();
    }

    QRectF boundingRect() const override {
        QRectF r = QGraphicsRectItem::boundingRect();
        if (m_useBezierPath) {
            QPointF p0 = getP0();
            QPointF p1 = getP1();
            QPointF p2 = getP2();
            QPointF p3 = getP3();
            double minX = qMin(r.left(), qMin(p0.x(), qMin(p1.x(), qMin(p2.x(), p3.x()))));
            double maxX = qMax(r.right(), qMax(p0.x(), qMax(p1.x(), qMax(p2.x(), p3.x()))));
            double minY = qMin(r.top(), qMin(p0.y(), qMin(p1.y(), qMin(p2.y(), p3.y()))));
            double maxY = qMax(r.bottom(), qMax(p0.y(), qMax(p1.y(), qMax(p2.y(), p3.y()))));
            r = QRectF(minX, minY, maxX - minX, maxY - minY);
            r.adjust(-20, -20, 20, 20);
        }
        return r;
    }

    QPainterPath shape() const override {
        QPainterPath path;
        path.addRect(rect());
        if (m_useBezierPath) {
            path.addRect(QRectF(getP0().x() - 15, getP0().y() - 15, 30, 30));
            path.addRect(QRectF(getP1().x() - 15, getP1().y() - 15, 30, 30));
            path.addRect(QRectF(getP2().x() - 15, getP2().y() - 15, 30, 30));
            path.addRect(QRectF(getP3().x() - 15, getP3().y() - 15, 30, 30));
            QPainterPath curve = getBezierPath();
            QPainterPathStroker stroker;
            stroker.setWidth(30);
            path.addPath(stroker.createStroke(curve));
        }
        return path;
    }
    
    QList<double> getColWeights() const {
        QList<double> colWeights;
        if (!m_colWidthsStr.isEmpty()) {
            QStringList parts = m_colWidthsStr.split(",");
            for (int i = 0; i < qMin(m_cols, parts.size()); ++i) {
                double v = parts[i].trimmed().toDouble();
                if (v <= 0) v = 1.0;
                colWeights.append(v);
            }
        }
        while (colWeights.size() < m_cols) colWeights.append(1.0);
        return colWeights;
    }

    QList<double> getRowWeights() const {
        QList<double> rowWeights;
        if (!m_rowHeightsStr.isEmpty()) {
            QStringList parts = m_rowHeightsStr.split(",");
            for (int i = 0; i < qMin(m_rows, parts.size()); ++i) {
                double v = parts[i].trimmed().toDouble();
                if (v <= 0) v = 1.0;
                rowWeights.append(v);
            }
        }
        while (rowWeights.size() < m_rows) rowWeights.append(1.0);
        return rowWeights;
    }

    QList<double> getColOffsets() const {
        QList<double> w = getColWeights();
        double sum = 0; for(double x:w) sum+=x;
        QList<double> offs;
        double cur = 0;
        double W = rect().width();
        for (double v : w) { offs.append(cur); cur += W * (v / qMax(0.001, sum)); }
        offs.append(W);
        return offs;
    }

    QList<double> getRowOffsets() const {
        QList<double> w = getRowWeights();
        double sum = 0; for(double x:w) sum+=x;
        QList<double> offs;
        double cur = 0;
        double H = rect().height();
        for (double v : w) { offs.append(cur); cur += H * (v / qMax(0.001, sum)); }
        offs.append(H);
        return offs;
    }

    void notifyModified() {
        if (scene()) scene()->selectionChanged();
        if (scene() && !scene()->views().isEmpty()) {
            QWidget *window = scene()->views().first()->window();
            if (QslDesigner *designer = qobject_cast<QslDesigner*>(window)) {
                designer->setModified(true);
            }
        }
    }

    void resizeCells() {
        QStringList newCells;
        for (int r = 0; r < m_rows; ++r) {
            for (int c = 0; c < m_cols; ++c) {
                int idx = r * m_cols + c;
                if (idx < m_cellData.size()) {
                    newCells.append(m_cellData[idx]);
                } else {
                    QString row0Val = (c < m_cellData.size()) ? m_cellData[c] : "";
                    newCells.append(row0Val);
                }
            }
        }
        m_cellData = newCells;
    }
    
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override {
        QPointF p = event->pos();
        
        if (m_useBezierPath && isSelected()) {
            if (QLineF(p, getP0()).length() < 15.0) { setCursor(Qt::PointingHandCursor); return; }
            if (QLineF(p, getP1()).length() < 15.0) { setCursor(Qt::PointingHandCursor); return; }
            if (QLineF(p, getP2()).length() < 15.0) { setCursor(Qt::PointingHandCursor); return; }
            if (QLineF(p, getP3()).length() < 15.0) { setCursor(Qt::PointingHandCursor); return; }
        }
        
        int edge = 0;
        int m = 10;
        if (rect().width() < 30 || rect().height() < 30) {
            m = qMin(static_cast<int>(rect().width() / 3), static_cast<int>(rect().height() / 3));
            if (m < 2) m = 2;
        }
        if (p.x() <= m) edge |= 1;
        else if (p.x() >= rect().width() - m) edge |= 2;
        if (p.y() <= m) edge |= 4;
        else if (p.y() >= rect().height() - m) edge |= 8;
        
        if (isQrCode()) {
            // Native vector QR Code: only allow diagonal corners (5, 6, 9, 10) to maintain proportion
            if (edge == 5 || edge == 10) setCursor(Qt::SizeFDiagCursor);
            else if (edge == 6 || edge == 9) setCursor(Qt::SizeBDiagCursor);
            else {
                setCursor(Qt::ArrowCursor);
                QGraphicsRectItem::hoverMoveEvent(event);
                return;
            }
            return;
        }

        if (edge == 5 || edge == 10) setCursor(Qt::SizeFDiagCursor); // Top-left or bottom-right
        else if (edge == 6 || edge == 9) setCursor(Qt::SizeBDiagCursor); // Top-right or bottom-left
        else if (edge == 1 || edge == 2) setCursor(Qt::SizeHorCursor); // Left or right
        else if (edge == 4 || edge == 8) setCursor(Qt::SizeVerCursor); // Top or bottom
        if (edge != 0) return;
        
        if (areaId.startsWith("CHK_")) {
            setCursor(Qt::ArrowCursor);
            QGraphicsRectItem::hoverMoveEvent(event);
            return;
        }
        
        QList<double> cOffs = getColOffsets();
        for (int i=1; i < m_cols; ++i) {
            if (qAbs(p.x() - cOffs[i]) < 5) {
                setCursor(Qt::SplitHCursor);
                return;
            }
        }
        
        QList<double> rOffs = getRowOffsets();
        for (int i=1; i < m_rows; ++i) {
            if (qAbs(p.y() - rOffs[i]) < 5) {
                setCursor(Qt::SplitVCursor);
                return;
            }
        }
        
        setCursor(Qt::ArrowCursor);
        QGraphicsRectItem::hoverMoveEvent(event);
    }
    
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override {
        m_initialScenePos = scenePos();
        m_initialRect = rect();
        m_initialColWidthsStr = m_colWidthsStr;
        m_initialRowHeightsStr = m_rowHeightsStr;
        
        if (m_useBezierPath && event->button() == Qt::LeftButton) {
            QPointF p = event->pos();
            double d0 = QLineF(p, getP0()).length();
            double d1 = QLineF(p, getP1()).length();
            double d2 = QLineF(p, getP2()).length();
            double d3 = QLineF(p, getP3()).length();
            
            int hitPoint = -1;
            double minD = 999999.0;
            
            if (d0 < 15.0 && d0 < minD) { minD = d0; hitPoint = 0; }
            if (d1 < 15.0 && d1 < minD) { minD = d1; hitPoint = 1; }
            if (d2 < 15.0 && d2 < minD) { minD = d2; hitPoint = 2; }
            if (d3 < 15.0 && d3 < minD) { minD = d3; hitPoint = 3; }
            
            if (hitPoint != -1) {
                if (scene()) {
                    scene()->clearSelection();
                }
                setSelected(true);
                m_draggingControlPoint = hitPoint;
                return;
            }
        }
        if (event->button() != Qt::LeftButton) {
            QGraphicsRectItem::mousePressEvent(event);
            return;
        }
        QPointF p = event->pos();
        int edge = 0;
        int m = 10;
        if (rect().width() < 30 || rect().height() < 30) {
            m = qMin(static_cast<int>(rect().width() / 3), static_cast<int>(rect().height() / 3));
            if (m < 2) m = 2;
        }
        if (p.x() <= m) edge |= 1;
        else if (p.x() >= rect().width() - m) edge |= 2;
        if (p.y() <= m) edge |= 4;
        else if (p.y() >= rect().height() - m) edge |= 8;

        if (isQrCode() && edge != 5 && edge != 6 && edge != 9 && edge != 10) {
            edge = 0; // Strictly restrict resizing to diagonal corners only
        }
        
        if (edge != 0) {
            if (scene()) {
                scene()->clearSelection();
            }
            setSelected(true);
            m_isResizing = true;
            m_resizeEdge = edge;
            m_resizeStart = event->scenePos();
            return;
        }
        
        if (areaId.startsWith("CHK_")) {
            QGraphicsRectItem::mousePressEvent(event);
            return;
        }
        
        QList<double> cOffs = getColOffsets();
        for (int i=1; i < m_cols; ++i) {
            if (qAbs(p.x() - cOffs[i]) < 5) {
                if (scene()) {
                    scene()->clearSelection();
                }
                setSelected(true);
                m_draggingColDivider = i;
                return;
            }
        }
        
        QList<double> rOffs = getRowOffsets();
        for (int i=1; i < m_rows; ++i) {
            if (qAbs(p.y() - rOffs[i]) < 5) {
                if (scene()) {
                    scene()->clearSelection();
                }
                setSelected(true);
                m_draggingRowDivider = i;
                return;
            }
        }
        
        QGraphicsRectItem::mousePressEvent(event);
    }
    
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override {
        if (m_draggingControlPoint != -1) {
            QPointF p = event->pos();
            if (m_draggingControlPoint == 0) setP0(p);
            else if (m_draggingControlPoint == 1) setP1(p);
            else if (m_draggingControlPoint == 2) setP2(p);
            else if (m_draggingControlPoint == 3) setP3(p);
            update();
            notifyModified();
            return;
        }
        
        if (m_isResizing) {
            QPointF localDelta = event->pos() - event->lastPos();
            double dx = localDelta.x();
            double dy = localDelta.y();
            
            QRectF r = rect();
            if (isQrCode()) {
                // Diagonal-only proportional 1:1 resizing to maintain perfect square proportion
                double delta = 0.0;
                if (m_resizeEdge == 10) { // Bottom-right
                    delta = (dx + dy) / 2.0;
                    double newDim = qMax(20.0, r.width() + delta);
                    setRect(0, 0, newDim, newDim);
                } else if (m_resizeEdge == 5) { // Top-left
                    delta = (dx + dy) / 2.0;
                    double newDim = qMax(20.0, r.width() - delta);
                    double shift = r.width() - newDim;
                    moveBy(shift, shift);
                    setRect(0, 0, newDim, newDim);
                } else if (m_resizeEdge == 6) { // Top-right
                    delta = (dx - dy) / 2.0;
                    double newDim = qMax(20.0, r.width() + delta);
                    double shiftY = r.height() - newDim;
                    moveBy(0, shiftY);
                    setRect(0, 0, newDim, newDim);
                } else if (m_resizeEdge == 9) { // Bottom-left
                    delta = (-dx + dy) / 2.0;
                    double newDim = qMax(20.0, r.width() + delta);
                    double shiftX = r.width() - newDim;
                    moveBy(shiftX, 0);
                    setRect(0, 0, newDim, newDim);
                }
                if (scene()) scene()->update();
                notifyModified();
                return;
            }

            double nx = r.x();
            double ny = r.y();
            double nw = r.width();
            double nh = r.height();
            
            if (m_resizeEdge & 1) { nx += dx; nw -= dx; } // left
            if (m_resizeEdge & 2) { nw += dx; } // right
            if (m_resizeEdge & 4) { ny += dy; nh -= dy; } // top
            if (m_resizeEdge & 8) { nh += dy; } // bottom
            
            double minW = areaId.startsWith("CHK_") ? 4 : 20;
            double minH = areaId.startsWith("CHK_") ? 4 : 20;
            
            if (nw < minW) {
                if (m_resizeEdge & 1) nx -= (minW - nw);
                nw = minW;
            }
            if (nh < minH) {
                if (m_resizeEdge & 4) ny -= (minH - nh);
                nh = minH;
            }
            
            setRect(0, 0, nw, nh);
            if (nx != 0 || ny != 0) {
                moveBy(nx, ny);
                // After moving, the local origin is shifted back to 0,0 for the rect
            }
            
            if (scene()) scene()->update();
            return;
        }
        
        if (m_draggingColDivider != -1) {
            double newX = event->pos().x();
            QList<double> cOffs = getColOffsets();
            double minX = cOffs[m_draggingColDivider-1] + 10;
            double maxX = cOffs[m_draggingColDivider+1] - 10;
            newX = qBound(minX, newX, maxX);
            
            QList<double> weights = getColWeights();
            double leftArea = newX - cOffs[m_draggingColDivider-1];
            double rightArea = cOffs[m_draggingColDivider+1] - newX;
            
            double sumOld = weights[m_draggingColDivider-1] + weights[m_draggingColDivider];
            weights[m_draggingColDivider-1] = sumOld * (leftArea / (leftArea + rightArea));
            weights[m_draggingColDivider] = sumOld * (rightArea / (leftArea + rightArea));
            
            QStringList str;
            for(double w: weights) str << QString::number(w, 'f', 2);
            m_colWidthsStr = str.join(",");
            update();
            return;
        }
        
        if (m_draggingRowDivider != -1) {
            double newY = event->pos().y();
            QList<double> rOffs = getRowOffsets();
            double minY = rOffs[m_draggingRowDivider-1] + 10;
            double maxY = rOffs[m_draggingRowDivider+1] - 10;
            newY = qBound(minY, newY, maxY);
            
            QList<double> weights = getRowWeights();
            double topArea = newY - rOffs[m_draggingRowDivider-1];
            double bottomArea = rOffs[m_draggingRowDivider+1] - newY;
            
            double sumOld = weights[m_draggingRowDivider-1] + weights[m_draggingRowDivider];
            weights[m_draggingRowDivider-1] = sumOld * (topArea / (topArea + bottomArea));
            weights[m_draggingRowDivider] = sumOld * (bottomArea / (topArea + bottomArea));
            
            QStringList str;
            for(double w: weights) str << QString::number(w, 'f', 2);
            m_rowHeightsStr = str.join(",");
            update();
            return;
        }
        
        QGraphicsRectItem::mouseMoveEvent(event);
    }
    
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override {
        bool wasDraggingBezier = (m_draggingControlPoint != -1);
        if (m_draggingControlPoint != -1) {
            m_draggingControlPoint = -1;
        } else if (m_isResizing) {
            m_isResizing = false;
        } else if (m_draggingColDivider != -1 || m_draggingRowDivider != -1) {
            m_draggingColDivider = -1;
            m_draggingRowDivider = -1;
        } else {
            QGraphicsRectItem::mouseReleaseEvent(event);
        }
        
        bool hasMoved = (scenePos() != m_initialScenePos);
        bool hasResized = (rect() != m_initialRect);
        bool hasColsChanged = (m_colWidthsStr != m_initialColWidthsStr);
        bool hasRowsChanged = (m_rowHeightsStr != m_initialRowHeightsStr);
        
        if (wasDraggingBezier || hasMoved || hasResized || hasColsChanged || hasRowsChanged) {
            notifyModified(); // trigger property update
        }
    }
    
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override {
        QString idLower = areaId.toLower().trimmed();
        QString valLower = (m_cellData.isEmpty() ? textContent : m_cellData.first()).toLower().trimmed();
        bool isCallsignElement = (idLower == "call" || idLower == "callsign" || idLower == "<call>" ||
                                  valLower == "call" || valLower == "callsign" || valLower == "<call>");
        
        if (isCallsignElement) {
            QWidget *parentWnd = nullptr; 
            if (scene() && !scene()->views().isEmpty()) parentWnd = scene()->views().first()->window();
            GtkFontChooserWidget sd(m_font, m_color, parentWnd);
            if (sd.exec() == QDialog::Accepted) {
                m_font = sd.selectedFont();
                m_color = sd.selectedColor();
                update();
                notifyModified();
            }
            return;
        }

        QWidget *parentWnd = nullptr; if (scene() && !scene()->views().isEmpty()) parentWnd = scene()->views().first()->window(); QDialog d(parentWnd);
        d.setWindowTitle("Edit Value");
        d.resize(400, 300);
        QFormLayout *fl = new QFormLayout();
        QVBoxLayout *mainLayout = new QVBoxLayout(&d);
        mainLayout->addLayout(fl);
        
        QString currentVal = m_cellData.isEmpty() ? textContent : m_cellData.first();
        QTextEdit *valEdit = new QTextEdit(currentVal, &d);
        
        QComboBox *alignCombo = new QComboBox(&d);
        alignCombo->addItem("Left");
        alignCombo->addItem("Center");
        alignCombo->addItem("Right");
        alignCombo->addItem("Justify");
        
        int alignIdx = 1; // Center
        if (m_align == Qt::AlignLeft) alignIdx = 0;
        else if (m_align == Qt::AlignRight) alignIdx = 2;
        else if (m_align == Qt::AlignJustify) alignIdx = 3;
        alignCombo->setCurrentIndex(alignIdx);
        
        QFontComboBox *fontCombo = new QFontComboBox(&d);
        fontCombo->setCurrentFont(m_font);
        QSpinBox *sizeSpin = new QSpinBox(&d);
        sizeSpin->setRange(6, 144);
        sizeSpin->setValue(m_font.pointSize());
        QCheckBox *chkBold = new QCheckBox("Bold", &d);
        chkBold->setChecked(m_font.bold());
        QCheckBox *chkItalic = new QCheckBox("Italic", &d);
        chkItalic->setChecked(m_font.italic());
        QHBoxLayout *styleLayout = new QHBoxLayout;
        styleLayout->addWidget(chkBold);
        styleLayout->addWidget(chkItalic);
        QPushButton *colorBtn = new QPushButton("🎨 Choose Color", &d);
        
        fl->addRow("Value:", valEdit);
        fl->addRow("Alignment:", alignCombo);
        fl->addRow("Font Face:", fontCombo);
        fl->addRow("Font Size:", sizeSpin);
        fl->addRow("Font Style:", styleLayout);
        fl->addRow("Color:", colorBtn);

        QString origText = textContent;
        QString origId = areaId;
        QString origCell0 = m_cellData.isEmpty() ? "" : m_cellData.first();
        int origAlign = m_align;
        QFont origFont = m_font;
        QColor origColor = m_color;
        QColor activeColor = origColor;

        auto updateArea = [&]() {
            QString text = valEdit->toPlainText();
            textContent = text;
            if (m_isAddressBox) {
                areaId = "ADDRESS_BOX";
            } else {
                areaId = text;
            }
            if(!m_cellData.isEmpty()) m_cellData[0] = text;
            else m_cellData.append(text);
            
            int align = Qt::AlignCenter;
            if (alignCombo->currentIndex() == 0) align = Qt::AlignLeft;
            else if (alignCombo->currentIndex() == 2) align = Qt::AlignRight;
            else if (alignCombo->currentIndex() == 3) align = Qt::AlignJustify;
            m_align = align;
            
            m_useBezierPath = false;
            
            QFont f = fontCombo->currentFont();
            f.setPointSize(sizeSpin->value());
            f.setBold(chkBold->isChecked());
            f.setItalic(chkItalic->isChecked());
            m_font = f;
            m_color = activeColor;
            update();
        };

        QObject::connect(valEdit, &QTextEdit::textChanged, updateArea);
        QObject::connect(alignCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), updateArea);
        QObject::connect(fontCombo, &QFontComboBox::currentFontChanged, updateArea);
        QObject::connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), updateArea);
        QObject::connect(chkBold, &QCheckBox::toggled, updateArea);
        QObject::connect(chkItalic, &QCheckBox::toggled, updateArea);
        QObject::connect(colorBtn, &QPushButton::clicked, [&]() {
            QFont f = fontCombo->currentFont(); f.setPointSize(sizeSpin->value());
            f.setBold(chkBold->isChecked()); f.setItalic(chkItalic->isChecked());
            StyleDialog dlg(f, activeColor, &d);
            if (dlg.exec() == QDialog::Accepted) {
                activeColor = dlg.selectedColor();
                fontCombo->setCurrentFont(dlg.selectedFont());
                sizeSpin->setValue(dlg.selectedFont().pointSize());
                chkBold->setChecked(dlg.selectedFont().bold());
                chkItalic->setChecked(dlg.selectedFont().italic());
                updateArea();
            }
        });

        QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &d);
        mainLayout->addWidget(box);
        QObject::connect(box, &QDialogButtonBox::accepted, &d, &QDialog::accept);
        QObject::connect(box, &QDialogButtonBox::rejected, &d, &QDialog::reject);

        if (d.exec() == QDialog::Accepted) {
            notifyModified();
        } else {
            textContent = origText;
            areaId = origId;
            if(!m_cellData.isEmpty()) m_cellData[0] = origCell0;
            else m_cellData.append(origCell0);
            m_align = origAlign;
            m_useBezierPath = false;
            m_font = origFont;
            m_color = origColor;
            update();
        }
        QGraphicsRectItem::mouseDoubleClickEvent(event);
    }
    
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override {
        QMenu menu;
        QAction *rotAction = menu.addAction("Rotate Field...");
        
        QAction *sel = menu.exec(event->screenPos());

        if (sel == rotAction) {
            QWidget *parentWnd = nullptr;
            if (scene() && !scene()->views().isEmpty()) parentWnd = scene()->views().first()->window();
            QDialog d(parentWnd);
            d.setWindowTitle("Rotate Field");
            QVBoxLayout *vl = new QVBoxLayout(&d);
            
            QDial *dial = new QDial(&d);
            dial->setRange(-360, 360);
            dial->setValue(rotation());
            dial->setNotchesVisible(true);
            vl->addWidget(dial);
            
            QSpinBox *spin = new QSpinBox(&d);
            spin->setRange(-360, 360);
            spin->setValue(rotation());
            vl->addWidget(spin);
            
            QObject::connect(dial, &QDial::valueChanged, spin, &QSpinBox::setValue);
            QObject::connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), dial, &QDial::setValue);
            
            QHBoxLayout *hl = new QHBoxLayout();
            auto addPreset = [&](int r) {
                QPushButton *b = new QPushButton(QString::number(r) + "°", &d);
                QObject::connect(b, &QPushButton::clicked, [spin, r]() { spin->setValue(r); });
                hl->addWidget(b);
            };
            addPreset(-180);
            addPreset(-90);
            addPreset(0);
            addPreset(90);
            addPreset(180);
            vl->addLayout(hl);
            
            if (scene() && !scene()->views().isEmpty()) {
                if (auto designer = qobject_cast<QslDesigner*>(scene()->views().first()->window())) {
                    designer->pushUndo();
                }
            }
            
            int origRot = rotation();
            QObject::connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), [this](int r) {
                setTransformOriginPoint(rect().center());
                setRotation(r);
                update();
            });
            
            QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &d);
            vl->addWidget(box);
            QObject::connect(box, &QDialogButtonBox::accepted, &d, &QDialog::accept);
            QObject::connect(box, &QDialogButtonBox::rejected, &d, &QDialog::reject);
            
            if (d.exec() == QDialog::Accepted) {
                notifyModified();
            } else {
                setTransformOriginPoint(rect().center());
                setRotation(origRot);
                update();
            }
        }
    }
    
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override {
        m_useBezierPath = false;
        bool isPrinting = scene() && scene()->property("isPrinting").toBool();
        bool isTestView = scene() && scene()->property("isTestView").toBool();
        
        if (isQrCode()) {
            ensureQrModules();
            double boxW = rect().width();
            double boxH = rect().height();
            double qrSide = qMin(boxW, boxH);
            double dx = (boxW - qrSide) / 2.0;
            double dy = (boxH - qrSide) / 2.0;
            QRectF targetRect(rect().left() + dx, rect().top() + dy, qrSide, qrSide);

            painter->save();
            // Native vector mode: pure 2-color white background and sharp black vector modules
            painter->setRenderHint(QPainter::Antialiasing, false);
            painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
            painter->fillRect(targetRect, Qt::white);

            if (!m_qrModules.empty()) {
                int qrSize = m_qrModules.size();
                int border = 2; // Standard 2-module quiet zone border
                int totalModules = qrSize + border * 2;
                double moduleSize = qrSide / static_cast<double>(totalModules);

                painter->setPen(Qt::NoPen);
                painter->setBrush(Qt::black);

                for (int y = 0; y < qrSize; ++y) {
                    for (int x = 0; x < qrSize; ++x) {
                        if (m_qrModules[y][x]) {
                            QRectF modRect(
                                targetRect.left() + (x + border) * moduleSize,
                                targetRect.top() + (y + border) * moduleSize,
                                moduleSize,
                                moduleSize
                            );
                            painter->fillRect(modRect, Qt::black);
                        }
                    }
                }
            } else if (!m_pixmap.isNull()) {
                painter->drawPixmap(targetRect, m_pixmap, QRectF(m_pixmap.rect()));
            }
            painter->restore();

            if (isSelected() && !isPrinting) {
                painter->fillRect(rect(), QColor(239, 68, 68, 40));
                painter->setPen(QPen(QColor(239, 68, 68), 2.0, Qt::SolidLine));
                painter->drawRect(rect());
            }
            return;
        }

        if (m_isImage && !m_pixmap.isNull()) {
            if (m_useBezierPath) {
                painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
                QPainterPath path = getBezierPath();
                double W_local = rect().width();
                double H_local = rect().height();
                double W_src = m_pixmap.width();
                double H_src = m_pixmap.height();
                for (double x = 0; x < W_local; x += 1.0) {
                    double t = x / W_local;
                    QPointF pt = path.pointAtPercent(t);
                    double angle = path.angleAtPercent(t);
                    
                    painter->save();
                    painter->translate(pt);
                    painter->rotate(-angle);
                    
                    QRectF sourceRect(t * W_src, 0, 1.5, H_src);
                    QRectF targetRect(-0.75, -H_local / 2.0, 1.5, H_local);
                    painter->drawPixmap(targetRect, m_pixmap, sourceRect);
                    
                    painter->restore();
                }
            } else {
                painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
                double boxW = rect().width();
                double boxH = rect().height();
                double imgW = m_pixmap.width();
                double imgH = m_pixmap.height();
                if (imgW > 0 && imgH > 0) {
                    double scale = qMin(boxW / imgW, boxH / imgH);
                    double drawW = imgW * scale;
                    double drawH = imgH * scale;
                    double dx = (boxW - drawW) / 2.0;
                    double dy = (boxH - drawH) / 2.0;
                    QRectF targetRect(rect().left() + dx, rect().top() + dy, drawW, drawH);
                    painter->drawPixmap(targetRect, m_pixmap, QRectF(m_pixmap.rect()));
                } else {
                    painter->drawPixmap(rect().toRect(), m_pixmap);
                }
            }
            if (isSelected() && !isPrinting) {
                painter->fillRect(rect(), QColor(239, 68, 68, 60));
                painter->setPen(QPen(QColor(239, 68, 68), 2.5, Qt::SolidLine));
                painter->drawRect(rect());
                if (m_useBezierPath) {
                    drawBezierHandles(painter);
                }
            }
            return;
        }

        if (areaId.startsWith("REF_")) {
            painter->setPen(QPen(isPrinting ? Qt::black : Qt::red, 1));
            painter->drawLine(rect().width()/2, 0, rect().width()/2, rect().height());
            painter->drawLine(0, rect().height()/2, rect().width(), rect().height()/2);
            painter->drawEllipse(rect());
            if (isSelected() && !isPrinting) {
                painter->fillRect(rect().adjusted(-2,-2,2,2), QColor(239, 68, 68, 60));
                painter->setPen(QPen(QColor(239, 68, 68), 2.5, Qt::SolidLine));
                painter->drawRect(rect().adjusted(-2,-2,2,2));
            }
            return;
        }

        if (areaId.startsWith("CHK_")) {
            if (!isPrinting && areaId != "CHK_CUSTOM" && areaId != "CHK_") {
                painter->setBrush(QBrush(QColor(255, 0, 0, 80))); // dim red when customized
            } else {
                painter->setBrush(isPrinting ? Qt::black : QBrush(m_color));
            }
            painter->setPen(isPrinting ? Qt::NoPen : QPen(Qt::black));
            painter->drawRect(rect());
            
            if (isSelected() && !isPrinting) {
                painter->fillRect(rect().adjusted(-2,-2,2,2), QColor(239, 68, 68, 60));
                painter->setPen(QPen(QColor(239, 68, 68), 2.5, Qt::SolidLine));
                painter->setBrush(Qt::NoBrush);
                painter->drawRect(rect().adjusted(-2,-2,2,2));
            }
            return;
        }

        if (areaId.startsWith("CIRC_")) {
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(isPrinting ? Qt::black : m_color, m_circleLineWidth));
            painter->drawEllipse(rect());
            
            if (isSelected() && !isPrinting) {
                painter->fillRect(rect(), QColor(239, 68, 68, 60));
                painter->setPen(QPen(QColor(239, 68, 68), 2.5, Qt::SolidLine));
                painter->drawRect(rect());
            }
            return;
        }
        
        if (areaId == "STAMP_REMINDER") {
            QFont f("Arial", 6, QFont::Bold);
            painter->setFont(f);
            painter->setPen(Qt::black);
            painter->drawText(rect(), Qt::AlignCenter, "SR");
            if (isSelected() && !isPrinting) {
                painter->fillRect(rect(), QColor(239, 68, 68, 60));
                painter->setPen(QPen(QColor(239, 68, 68), 2.5, Qt::SolidLine));
                painter->drawRect(rect());
            }
            return;
        }
        
        if (isTestView || isPrinting) {
            if (brush().style() != Qt::NoBrush) {
                painter->save();
                painter->setPen(Qt::NoPen);
                painter->setBrush(brush());
                painter->drawRect(rect());
                painter->restore();
            }
        } else {
            QGraphicsRectItem::paint(painter, option, widget);
        }

        painter->setFont(m_font);
        painter->setPen(m_color);
        
        double w = rect().width();
        double h = rect().height();
        
        QList<double> rOffs = getRowOffsets();
        QList<double> cOffs = getColOffsets();

        if (m_useBezierPath) {
            // 1. Draw curved grid lines if table has rows/cols
            if (m_rows > 1 || m_cols > 1) {
                QPainterPath path = getBezierPath();
                // Draw horizontal row lines
                for (int r = 0; r <= m_rows; ++r) {
                    double r_pct = (double)r / m_rows;
                    double yOffset = (r_pct * h) - h / 2.0;
                    
                    QPainterPath rowPath;
                    bool first = true;
                    for (double x = 0; x <= w; x += 2.0) {
                        double t = x / w;
                        QPointF pt = path.pointAtPercent(t);
                        double angle = path.angleAtPercent(t);
                        double rad = angle * M_PI / 180.0;
                        QPointF normal(std::sin(rad), std::cos(rad));
                        QPointF offsetPt = pt + yOffset * normal;
                        if (first) {
                            rowPath.moveTo(offsetPt);
                            first = false;
                        } else {
                            rowPath.lineTo(offsetPt);
                        }
                    }
                    painter->setPen(QPen(m_color, 1));
                    painter->drawPath(rowPath);
                }
                
                // Draw vertical column lines
                for (int c = 0; c <= m_cols; ++c) {
                    double c_pct = (double)c / m_cols;
                    QPointF pt = path.pointAtPercent(c_pct);
                    double angle = path.angleAtPercent(c_pct);
                    double rad = angle * M_PI / 180.0;
                    QPointF normal(std::sin(rad), std::cos(rad));
                    
                    QPointF topPt = pt - (h / 2.0) * normal;
                    QPointF botPt = pt + (h / 2.0) * normal;
                    
                    painter->setPen(QPen(m_color, 1));
                    painter->drawLine(topPt, botPt);
                }
            }

            // 2. Draw curved text/cell content
            QPainterPath path = getBezierPath();
            double pathLength = path.length();
            
            for (int r = 0; r < m_rows; ++r) {
                double r_pct_mid = (double)(r + 0.5) / m_rows;
                double yOffset = (r_pct_mid * h) - h / 2.0;
                
                for (int c = 0; c < m_cols; ++c) {
                    double tLeft = (double)c / m_cols;
                    double tRight = (double)(c + 1) / m_cols;
                    
                    int idx = r * m_cols + c;
                    QString origText = (idx < m_cellData.size()) ? m_cellData[idx] : "";
                    if (origText.isEmpty() && r > 0) {
                        int firstRowIdx = c;
                        origText = (firstRowIdx < m_cellData.size()) ? m_cellData[firstRowIdx] : "";
                    }
                    if (origText.isEmpty() && m_rows == 1 && m_cols == 1) {
                        origText = textContent;
                    }
                    QString cText = origText;
                    if (isTestView) {
                        cText = translateTestViewText(cText);
                        if (r > 0) {
                            QString header = (c < m_cellData.size()) ? m_cellData[c].toLower().trimmed() : "";
                            bool isPlaceholder = cText.startsWith("<") && cText.endsWith(">");
                            bool isHeaderFallback = (cText.toLower().trimmed() == header);
                            if (isPlaceholder || isHeaderFallback || cText == origText || cText.isEmpty()) {
                                if (header.contains("day") || header == "dy" || header == "d") {
                                    cText = QString::number(27 - r + 1);
                                } else if (header.contains("month") || header == "mo" || header == "m") {
                                    cText = "10";
                                } else if (header.contains("year") || header == "yr" || header == "y") {
                                    cText = "2026";
                                } else if (header.contains("date")) {
                                    cText = "2026-10-27";
                               } else if (header.contains("time") || header.contains("utc")) {
                                    cText = "14:30";
                                } else if (header.contains("band")) {
                                    cText = "20m";
                                } else if (header.contains("freq") || header.contains("mhz")) {
                                    cText = "14.250";
                                } else if (header.contains("mode")) {
                                    cText = "FT8";
                                } else if (header.contains("watt") || header.contains("pwr") || header.contains("power")) {
                                    cText = "100W";
                                } else if (header.contains("sent") || header.contains("tx") || header.contains("rst_s") || header == "r" || header == "rst" || header.contains(" s")) {
                                    cText = "599";
                                } else if (header.contains("rcvd") || header.contains("rx") || header.contains("rst_r") || header == "s" || header.contains(" r")) {
                                    cText = "599";
                                } else {
                                    cText = "599";
                                }
                            }
                        }
                    }
                    
                    if (cText.isEmpty()) {
                        cText = "";
                    }
                    
                    if (!isPrinting && (cText == "TABLE" || cText == "Table")) {
                        cText = "";
                    }
                    
                    if (!cText.isEmpty() && !cText.startsWith("[")) {
                        QString lower = cText.toLower();
                        if (lower.startsWith("<") && lower.endsWith(">")) {
                            if (lower.contains("day")) cText = "27";
                            else if (lower.contains("month") || lower == "<mo>") cText = "10";
                            else if (lower.contains("year")) cText = "2026";
                            else if (lower.contains("utc") || lower.contains("time")) cText = "14:30";
                            else if (lower.contains("band")) cText = "20m";
                            else if (lower.contains("mhz") || lower.contains("freq")) cText = "14.250";
                            else if (lower.contains("mode")) cText = "FT8";
                            else if (lower.contains("watt") || lower.contains("pwr")) cText = "100W";
                            else if (lower.contains("rst_sent") || lower.contains("tx") || lower.contains("sent")) cText = "59";
                            else if (lower.contains("rst_rcvd") || lower.contains("rx") || lower.contains("rcvd") || lower.contains("rst")) cText = "59";
                            else if (lower.contains("call")) cText = "W4DJF";
                            else if (lower.contains("name")) cText = "Arthur";
                            else if (lower.contains("qth")) cText = "Atlanta, GA";
                            else if (lower.contains("county")) cText = "Fulton";
                            else if (lower.contains("grid")) cText = "EM73";
                            else if (lower.contains("country")) cText = "UNITED STATES";
                        }
                    }
                    
                    if (cText.isEmpty()) continue;
                    
                    QFont drawFont = m_font;
                    QFontMetricsF fm(drawFont);
                    
                    QStringList lines = cText.split(QRegularExpression("<br/?>|\\n"), Qt::SkipEmptyParts);
                    if (lines.isEmpty() && !cText.isEmpty()) {
                        lines << cText;
                    }
                    
                    double fontHeight = fm.height();
                    for (int l = 0; l < lines.size(); ++l) {
                        QString line = lines[l];
                        line.remove(QRegularExpression("<(b|i|span|div|br|p|/b|/i|/span|/div|/br|/p)( [^>]*)?>", QRegularExpression::CaseInsensitiveOption));
                        line = line.trimmed();
                        if (line.isEmpty()) continue;
                        
                        double totalTextWidth = 0;
                        for (int cIdx = 0; cIdx < line.length(); ++cIdx) {
                            totalTextWidth += fm.horizontalAdvance(line[cIdx]);
                        }
                        
                        double cellStartLen = pathLength * tLeft;
                        double cellEndLen = pathLength * tRight;
                        double cellWidthLen = cellEndLen - cellStartLen;
                        
                        double currentLength = cellStartLen + (cellWidthLen - totalTextWidth) / 2.0;
                        if (currentLength < cellStartLen) currentLength = cellStartLen;
                        
                        double lineYOffset = yOffset + (l - (lines.size() - 1) / 2.0) * (fontHeight + 2);
                        
                        painter->save();
                        painter->setFont(drawFont);
                        painter->setPen(m_color);
                        
                        for (int cIdx = 0; cIdx < line.length(); ++cIdx) {
                            QChar chChar = line[cIdx];
                            double charWidth = fm.horizontalAdvance(chChar);
                            double charCenterLength = currentLength + charWidth / 2.0;
                            if (charCenterLength > cellEndLen) break;
                            if (charCenterLength > pathLength) break;
                            
                            double percent = path.percentAtLength(charCenterLength);
                            QPointF pt = path.pointAtPercent(percent);
                            double angle = path.angleAtPercent(percent);
                            
                            double rad = angle * M_PI / 180.0;
                            QPointF normal(std::sin(rad), std::cos(rad));
                            QPointF offsetPt = pt + lineYOffset * normal;
                            
                            painter->save();
                            painter->translate(offsetPt);
                            painter->rotate(-angle);
                            
                            QRectF charRect(-charWidth / 2.0, -fm.ascent(), charWidth, fm.height());
                            painter->drawText(charRect, Qt::AlignCenter, QString(chChar));
                            
                            painter->restore();
                            currentLength += charWidth;
                        }
                        painter->restore();
                    }
                }
            }
        } else {
            // Standard straight table and curved cell text (legacy mode)
            for (int r = 0; r < m_rows; ++r) {
                double ch = rOffs[r+1] - rOffs[r];
                for (int c = 0; c < m_cols; ++c) {
                    double cw = cOffs[c+1] - cOffs[c];
                    QRectF cell(cOffs[c], rOffs[r], cw, ch);
                    
                    int idx = r * m_cols + c;
                    QString origText = (idx < m_cellData.size()) ? m_cellData[idx] : "";
                    if (origText.isEmpty() && r > 0) {
                        int firstRowIdx = c;
                        origText = (firstRowIdx < m_cellData.size()) ? m_cellData[firstRowIdx] : "";
                    }
                    if (origText.isEmpty() && m_rows == 1 && m_cols == 1) {
                        origText = textContent;
                    }
                    QString cText = origText;
                    if (isTestView) {
                        cText = translateTestViewText(cText);
                        if (r > 0) {
                            QString header = (c < m_cellData.size()) ? m_cellData[c].toLower().trimmed() : "";
                            bool isPlaceholder = cText.startsWith("<") && cText.endsWith(">");
                            bool isHeaderFallback = (cText.toLower().trimmed() == header);
                            if (isPlaceholder || isHeaderFallback || cText == origText || cText.isEmpty()) {
                                if (header.contains("day") || header == "dy" || header == "d") {
                                    cText = QString::number(27 - r + 1);
                                } else if (header.contains("month") || header == "mo" || header == "m") {
                                    cText = "10";
                                } else if (header.contains("year") || header == "yr" || header == "y") {
                                    cText = "2026";
                                } else if (header.contains("date")) {
                                    cText = "2026-10-27";
                                } else if (header.contains("time") || header.contains("utc")) {
                                    cText = "14:30";
                                } else if (header.contains("band")) {
                                    cText = "20m";
                                } else if (header.contains("freq") || header.contains("mhz")) {
                                    cText = "14.250";
                                } else if (header.contains("mode")) {
                                    cText = "FT8";
                                } else if (header.contains("watt") || header.contains("pwr") || header.contains("power")) {
                                    cText = "100W";
                                } else if (header.contains("sent") || header.contains("tx") || header.contains("rst_s") || header == "r" || header == "rst" || header.contains(" s")) {
                                    cText = "599";
                                } else if (header.contains("rcvd") || header.contains("rx") || header.contains("rst_r") || header == "s" || header.contains(" r")) {
                                    cText = "599";
                                } else {
                                    cText = "599";
                                }
                            }
                        }
                    }
                    
                    painter->save();
                    
                    if (cText.isEmpty()) {
                        cText = "";
                    }
                    
                    if (!isPrinting && (cText == "TABLE" || cText == "Table")) {
                        cText = "";
                    }
                    
                    if (!cText.isEmpty() && !cText.startsWith("[")) {
                        QString lower = cText.toLower();
                        if (lower.startsWith("<") && lower.endsWith(">")) {
                            if (lower.contains("date")) cText = "2026-10-27";
                            else if (lower.contains("day")) cText = "27";
                            else if (lower.contains("month") || lower == "<mo>") cText = "10";
                            else if (lower.contains("year")) cText = "2026";
                            else if (lower.contains("utc") || lower.contains("time")) cText = "14:30";
                            else if (lower.contains("band")) cText = "20m";
                            else if (lower.contains("freq") || lower.contains("mhz")) cText = "14.250";
                            else if (lower.contains("mode")) cText = "FT8";
                            else if (lower.contains("watt") || lower.contains("pwr")) cText = "100W";
                            else if (lower.contains("rst_sent") || lower.contains("tx") || lower.contains("sent")) cText = "59";
                            else if (lower.contains("rst_rcvd") || lower.contains("rx") || lower.contains("rcvd") || lower.contains("rst")) cText = "59";
                            else if (lower.contains("call")) cText = "W4DJF";
                            else if (lower.contains("name")) cText = "Arthur";
                            else if (lower.contains("qth")) cText = "Atlanta, GA";
                            else if (lower.contains("county")) cText = "Fulton";
                            else if (lower.contains("grid")) cText = "EM73";
                            else if (lower.contains("country")) cText = "UNITED STATES";
                        }
                    }

                    if (m_showBorders) {
                        painter->setPen(QPen(m_color, qMax(0.5, m_borderThickness)));
                        painter->drawRect(cell);
                    } else if (!isPrinting) {
                        // Keep subtle dashed cell borders visible during canvas placement, selection, and alignment
                        painter->setPen(QPen(QColor(100, 140, 220, 180), 1, Qt::DashLine));
                        painter->drawRect(cell);
                    }
                    painter->setClipRect(cell);
                    if (!cText.isEmpty()) {
                        if (cText.contains("<div") || cText.contains("<span>") || cText.contains("<br>")) {
                            painter->save();
                            QTextDocument doc;
                            QString txt = cText;
                            doc.setHtml(QString("<div style='color:%2;'>%1</div>").arg(txt.replace("\n", "<br>"), m_color.name()));
                            doc.setTextWidth(cell.width());
                            
                            QFont shrinkFont = painter->font();
                            int ps = shrinkFont.pixelSize();
                            if (ps <= 0) {
                                ps = qRound(shrinkFont.pointSizeF() * painter->device()->logicalDpiY() / 72.0);
                                if (ps <= 0) ps = 12;
                                shrinkFont.setPixelSize(ps);
                            }
                            doc.setDefaultFont(shrinkFont);
                            while (ps > 4 && (doc.idealWidth() > cell.width() - 4 || doc.size().height() > cell.height() - 4)) {
                                ps--;
                                shrinkFont.setPixelSize(ps);
                                doc.setDefaultFont(shrinkFont);
                            }
                            
                            // Vertical centering for HTML text doc
                            double dy = (cell.height() - doc.size().height()) / 2.0;
                            if (dy < 0) dy = 0;
                            painter->translate(cell.left(), cell.top() + dy);
                            
                            doc.drawContents(painter);
                            painter->restore();
                        } else {
                            QFont shrinkFont = painter->font();
                            int ps = shrinkFont.pixelSize();
                            if (ps <= 0) {
                                ps = qRound(shrinkFont.pointSizeF() * painter->device()->logicalDpiY() / 72.0);
                                if (ps <= 0) ps = 12;
                                shrinkFont.setPixelSize(ps);
                            }
                            QFontMetricsF fm(shrinkFont);
                            while (ps > 4 && (fm.horizontalAdvance(cText) > (cell.width() - 4) || fm.height() > (cell.height() - 2))) {
                                ps--;
                                shrinkFont.setPixelSize(ps);
                                fm = QFontMetricsF(shrinkFont);
                            }
                            painter->setFont(shrinkFont);
                            painter->drawText(cell, m_align | Qt::AlignVCenter | Qt::TextWordWrap, cText);
                        }
                    }
                    
                    painter->restore();
                }
            }
        }
        
        if (isSelected() && !isPrinting) {
            painter->fillRect(rect(), QColor(239, 68, 68, 60));
            if (m_useBezierPath) {
                drawBezierHandles(painter);
            } else {
                painter->setPen(QPen(QColor(239, 68, 68), 2.5, Qt::SolidLine));
                painter->drawRect(rect());
            }
        }
        
        // Draw resize handle
        if (!isPrinting) {
            painter->fillRect(QRectF(w - 10, h - 10, 10, 10), Qt::gray);
        }
    }
};

class DraggableLayerItem : public QGraphicsRectItem {
public:
    PrintTableArea *m_originalArea;
    QString id;
    bool *selectAllPtr;
    QList<DraggableLayerItem*> *allLayersPtr;
    bool m_isDragging;
    QPointF m_lastPos;

    DraggableLayerItem(PrintTableArea *origArea, const QString &layerId, const QRectF &rect, bool *selectAll, QList<DraggableLayerItem*> *allLayers)
        : QGraphicsRectItem(rect), m_originalArea(origArea), id(layerId), selectAllPtr(selectAll), allLayersPtr(allLayers), m_isDragging(false) {
        setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
        setCursor(Qt::OpenHandCursor);
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override {
        setCursor(Qt::ClosedHandCursor);
        m_isDragging = true;
        m_lastPos = pos();
        QGraphicsRectItem::mousePressEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override {
        setCursor(Qt::OpenHandCursor);
        m_isDragging = false;
        QGraphicsRectItem::mouseReleaseEvent(event);
    }

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override {
        if (change == ItemPositionHasChanged) {
            QPointF newPos = value.toPointF();
            QPointF delta = newPos - m_lastPos;
            m_lastPos = newPos;

            if (selectAllPtr && *selectAllPtr && allLayersPtr && delta != QPointF(0, 0)) {
                static bool s_propagating = false;
                if (!s_propagating) {
                    s_propagating = true;
                    for (DraggableLayerItem *other : *allLayersPtr) {
                        if (other != this) {
                            other->setPos(other->pos() + delta);
                            other->m_lastPos = other->pos();
                        }
                    }
                    s_propagating = false;
                }
            }
        }
        return QGraphicsRectItem::itemChange(change, value);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override {
        // Draw the exact original template item first (fonts, texts, alignments, columns, images etc.)
        if (m_originalArea) {
            painter->save();
            m_originalArea->paint(painter, option, widget);
            painter->restore();
        }

        painter->save();
        // Superimpose an elegant translucent layer with highlight border to represent selection & interactive status
        if (isSelected()) {
            painter->setBrush(QColor(239, 68, 68, 60)); // high-visibility active translucent red-orange fill
            painter->setPen(QPen(QColor(239, 68, 68, 255), 2.5, Qt::SolidLine));
        } else {
            painter->setBrush(QColor(107, 114, 128, 15)); // subtle warm gray
            painter->setPen(QPen(QColor(107, 114, 128, 100), 1.0, Qt::DashLine));
        }
        painter->drawRect(rect());

        // Draw a neat label badge for the layer identification (e.g. text/field ID)
        if (!id.isEmpty()) {
            painter->setFont(QFont("Arial", 8, QFont::Bold));
            QFontMetricsF fm(painter->font());
            double textW = fm.horizontalAdvance(id);
            double textH = fm.height();
            QRectF badgeRect(rect().left() + 4, rect().top() + 4, textW + 8, textH + 4);
            
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(17, 24, 39, 210)); // Sleek dark round pill
            painter->drawRoundedRect(badgeRect, 4, 4);
            
            painter->setPen(Qt::white);
            painter->drawText(badgeRect, Qt::AlignCenter, id);
        }
        painter->restore();
    }
};

class DesignerZoomableView : public QGraphicsView {
public:
    DesignerZoomableView(QGraphicsScene *scene, QWidget *parent = nullptr) : QGraphicsView(scene, parent) {
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

class AdvLayerDialog : public QDialog {
public:
    bool m_selectAll;
    QList<DraggableLayerItem*> m_allLayers;
    QList<PrintTableArea*> m_originalAreas;
    QGraphicsScene *m_scene;
    DesignerZoomableView *m_view;

    AdvLayerDialog(double paperW, double paperH, QGraphicsPixmapItem *bgImageItem, const QList<PrintTableArea*> &originalAreas, QWidget *parent = nullptr)
        : QDialog(parent), m_selectAll(true)
    {
        setWindowTitle("Advanced Layer Repositioning");
        resize(850, 650);
        
        setStyleSheet(
            "QDialog {"
            "  background-color: #1e222b;"
            "  border: 1px solid #3f4452;"
            "  border-radius: 8px;"
            "}"
            "QLabel {"
            "  color: #f1f5f9;"
            "  font-family: 'Segoe UI', Arial, sans-serif;"
            "  font-size: 10pt;"
            "}"
            "QCheckBox {"
            "  color: #f1f5f9;"
            "  font-family: 'Segoe UI', Arial, sans-serif;"
            "  font-size: 11pt;"
            "  font-weight: bold;"
            "}"
            "QPushButton {"
            "  background-color: #3b82f6;"
            "  color: white;"
            "  border: none;"
            "  border-radius: 4px;"
            "  padding: 8px 16px;"
            "  font-weight: bold;"
            "  font-size: 10pt;"
            "}"
            "QPushButton:hover {"
            "  background-color: #2563eb;"
            "}"
            "QPushButton#btnCancel {"
            "  background-color: #4b5563;"
            "}"
        );
        
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(10);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        
        QHBoxLayout *topBar = new QHBoxLayout();
        QCheckBox *chkSelectAll = new QCheckBox("Select All Layers", this);
        chkSelectAll->setChecked(true);
        topBar->addWidget(chkSelectAll);
        
        QLabel *helpLabel = new QLabel("ℹ️ Drag any layer to move. Use mouse wheel to zoom.", this);
        helpLabel->setStyleSheet("color: #9ca3af; font-size: 9pt;");
        topBar->addWidget(helpLabel);
        topBar->addStretch();
        mainLayout->addLayout(topBar);
        
        m_scene = new QGraphicsScene(this);
        m_view = new DesignerZoomableView(m_scene, this);
        mainLayout->addWidget(m_view);
        
        // Add paper background
        QGraphicsRectItem *paperItem = m_scene->addRect(0, 0, paperW, paperH, QPen(Qt::black, 2), Qt::white);
        paperItem->setZValue(-2);
        
        // Add background image if loaded
        if (bgImageItem && !bgImageItem->pixmap().isNull()) {
            QGraphicsPixmapItem *bgItem = m_scene->addPixmap(bgImageItem->pixmap());
            bgItem->setPos(bgImageItem->pos());
            bgItem->setZValue(-1);
        }
        
        // Add all layer items
        for (PrintTableArea *area : originalAreas) {
            DraggableLayerItem *layerItem = new DraggableLayerItem(area, area->areaId, area->rect(), &m_selectAll, &m_allLayers);
            layerItem->setPos(area->pos());
            layerItem->setRotation(area->rotation());
            layerItem->setZValue(area->zValue());
            m_scene->addItem(layerItem);
            m_allLayers.append(layerItem);
            m_originalAreas.append(area);
        }
        
        connect(chkSelectAll, &QCheckBox::toggled, this, [this](bool checked) {
            m_selectAll = checked;
        });
        
        QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QPushButton *okBtn = box->button(QDialogButtonBox::Ok);
        QPushButton *cancelBtn = box->button(QDialogButtonBox::Cancel);
        cancelBtn->setObjectName("btnCancel");
        
        connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
        mainLayout->addWidget(box);
        
        // Fit view nicely
        QTimer::singleShot(50, this, [this, paperW, paperH]() {
            m_view->fitInView(QRectF(0, 0, paperW, paperH), Qt::KeepAspectRatio);
        });
    }
    
    QList<DraggableLayerItem*> getLayers() const { return m_allLayers; }
    QList<PrintTableArea*> getOriginalAreas() const { return m_originalAreas; }
};


#include <QScreen>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMenuBar>
#include "imageviewer.h"

void MyGraphicsView::mouseDoubleClickEvent(QMouseEvent *event) {
    if (scene() && scene()->property("isTestView").toBool()) {
        QGraphicsView::mouseDoubleClickEvent(event);
        return;
    }
    if (QGraphicsItem *item = scene()->itemAt(mapToScene(event->pos()), transform())) {
        if (auto *area = dynamic_cast<PrintTableArea*>(item)) {
            QDialog d(this);
            d.setWindowTitle("Edit Field Properties");
            QFormLayout *fl = new QFormLayout(&d);
            
            QString currentVal = area->m_cellData.isEmpty() ? area->textContent : area->m_cellData.first();
            currentVal.replace("\n", "\\n");
            QLineEdit *valEdit = new QLineEdit(currentVal, &d);
            
            QComboBox *alignCombo = new QComboBox(&d);
            alignCombo->addItem("Left");
            alignCombo->addItem("Center");
            alignCombo->addItem("Right");
            alignCombo->addItem("Justify");
            
            int alignIdx = 1; // Center
            if (area->m_align == Qt::AlignLeft) alignIdx = 0;
            else if (area->m_align == Qt::AlignRight) alignIdx = 2;
            else if (area->m_align == Qt::AlignJustify) alignIdx = 3;
            alignCombo->setCurrentIndex(alignIdx);
            
            QFontComboBox *fontCombo = new QFontComboBox(&d);
            fontCombo->setCurrentFont(area->m_font);
            QSpinBox *sizeSpin = new QSpinBox(&d);
            sizeSpin->setRange(6, 144);
            sizeSpin->setValue(area->m_font.pointSize());
            QPushButton *colorBtn = new QPushButton("🎨 Choose Color", &d);
            
            fl->addRow("Value:", valEdit);
            fl->addRow("Alignment:", alignCombo);
            
            QSlider *widthSlider = nullptr;
            QLabel *widthValueLabel = nullptr;
            bool isCircle = area->areaId.startsWith("CIRC_");
            if (isCircle) {
                widthSlider = new QSlider(Qt::Horizontal, &d);
                widthSlider->setRange(1, 10);
                widthSlider->setValue(area->m_circleLineWidth);
                widthValueLabel = new QLabel(QString::number(area->m_circleLineWidth), &d);
                QHBoxLayout *sliderLayout = new QHBoxLayout;
                sliderLayout->addWidget(widthSlider);
                sliderLayout->addWidget(widthValueLabel);
                fl->addRow("Line Width:", sliderLayout);
            } else {
                fl->addRow("Font Face:", fontCombo);
                fl->addRow("Font Size:", sizeSpin);
            }
            fl->addRow("Color:", colorBtn);

            if (auto designer = qobject_cast<QslDesigner*>(window())) {
                designer->pushUndo();
            }

            QString origText = area->textContent;
            QString origId = area->areaId;
            QString origCell0 = area->m_cellData.isEmpty() ? "" : area->m_cellData.first();
            int origAlign = area->m_align;
            QFont origFont = area->m_font;
            QColor origColor = area->m_color;
            int origLineWidth = area->m_circleLineWidth;

            QColor activeColor = origColor;

            auto updateArea = [&, area, isCircle]() {
                QString text = valEdit->text().replace("\\n", "\n");
                area->textContent = text;
                area->areaId = text;
                if(!area->m_cellData.isEmpty()) area->m_cellData[0] = text;
                else area->m_cellData.append(text);
                
                int align = Qt::AlignCenter;
                if (alignCombo->currentIndex() == 0) align = Qt::AlignLeft;
                else if (alignCombo->currentIndex() == 2) align = Qt::AlignRight;
                else if (alignCombo->currentIndex() == 3) align = Qt::AlignJustify;
                area->m_align = align;
                
                if (!isCircle) {
                    QFont f = fontCombo->currentFont();
                    f.setPointSize(sizeSpin->value());
                    area->m_font = f;
                } else if (widthSlider) {
                    area->m_circleLineWidth = widthSlider->value();
                }
                area->m_color = activeColor;
                area->update();
            };

            QObject::connect(valEdit, &QLineEdit::textChanged, updateArea);
            QObject::connect(alignCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), updateArea);
            if (!isCircle) {
                QObject::connect(fontCombo, &QFontComboBox::currentFontChanged, updateArea);
                QObject::connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), updateArea);
            } else if (widthSlider) {
                QObject::connect(widthSlider, &QSlider::valueChanged, this, [updateArea, widthValueLabel](int val) {
                    widthValueLabel->setText(QString::number(val));
                    updateArea();
                });
            }
            QObject::connect(colorBtn, &QPushButton::clicked, [&]() {
                if (isCircle) {
                    QColor col = QColorDialog::getColor(activeColor, &d, "Choose Circle Color");
                    if (col.isValid()) {
                        activeColor = col;
                        updateArea();
                    }
                } else {
                    QFont f = fontCombo->currentFont(); f.setPointSize(sizeSpin->value());
                    StyleDialog dlg(f, activeColor, &d);
                    if (dlg.exec() == QDialog::Accepted) {
                        activeColor = dlg.selectedColor();
                        fontCombo->setCurrentFont(dlg.selectedFont());
                        sizeSpin->setValue(dlg.selectedFont().pointSize());
                        updateArea();
                    }
                }
            });

            QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &d);
            fl->addRow(box);
            QObject::connect(box, &QDialogButtonBox::accepted, &d, &QDialog::accept);
            QObject::connect(box, &QDialogButtonBox::rejected, &d, &QDialog::reject);

            if (d.exec() == QDialog::Accepted) {
                area->notifyModified();
            } else {
                area->textContent = origText;
                area->areaId = origId;
                if(!area->m_cellData.isEmpty()) area->m_cellData[0] = origCell0;
                else area->m_cellData.append(origCell0);
                area->m_align = origAlign;
                area->m_font = origFont;
                area->m_color = origColor;
                area->m_circleLineWidth = origLineWidth;
                area->update();
            }
            return;
        }
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void MyGraphicsView::contextMenuEvent(QContextMenuEvent *event) {
    QGraphicsView::contextMenuEvent(event);
}

QslDesigner::QslDesigner(QWidget *parent) : QDialog(parent), m_isModified(false), m_paperW(6.5 * 96), m_paperH(3.625 * 96), m_printCorrection(0), m_printOffsetX(0.0), m_printOffsetY(0.0), m_bgImageItem(nullptr), m_templateActive(false), m_qsoTableRows(1), m_qsoTableCols(1), m_qsoTableShowHeaders(true), m_qsoTableShowBorders(true), m_qsoTableBorderThickness(1.0)
{
    setWindowTitle("QSL Manager (WYSIWYG)");
    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    setFixedSize(1170, 590);


    m_scene = new QGraphicsScene(this);
    m_view = new MyGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setBackgroundBrush(Qt::lightGray);

    connect(m_scene, &QGraphicsScene::selectionChanged, this, &QslDesigner::onSelectionChanged);
    connect(m_view, &MyGraphicsView::mouseMoved, this, &QslDesigner::onMouseMoved);
    connect(m_view, &MyGraphicsView::mouseReleased, this, &QslDesigner::drawBackgroundRulers);

    // Crosshairs
    m_crosshairH = m_scene->addLine(-1000, 0, 2000, 0, QPen(Qt::red, 1, Qt::DotLine));
    m_crosshairV = m_scene->addLine(0, -1000, 0, 2000, QPen(Qt::red, 1, Qt::DotLine));
    m_crosshairH->setZValue(100);
    m_crosshairV->setZValue(100);

    drawBackgroundRulers();

    auto createBtnIcon = [](const QString &type, const QString &themeName = QString()) {
        if (!themeName.isEmpty()) {
            QIcon themeIcon = QIcon::fromTheme(themeName);
            if (!themeIcon.isNull()) {
                return themeIcon;
            }
        }
        QPixmap pix(48, 48);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        
        p.setPen(QPen(QColor(30, 30, 30), 2));
        QLinearGradient grad(0, 0, 0, 48);
        grad.setColorAt(0, QColor(160, 165, 170));
        grad.setColorAt(0.4, QColor(110, 115, 120));
        grad.setColorAt(1, QColor(80, 85, 90));
        p.setBrush(grad);
        p.drawRect(6, 6, 36, 36);
        
        p.setPen(QPen(QColor(240, 240, 240), 2));
        
        if (type == "add" || type == "insert_text" || type == "insert-text") { p.drawLine(24, 14, 24, 34); p.drawLine(14, 24, 34, 24); }
        else if (type == "refpoint") { p.setBrush(Qt::red); p.drawEllipse(18, 18, 12, 12); }
        else if (type == "check" || type == "media-playback-stop" || type == "media_playback_stop") { p.setPen(QPen(QColor(120, 255, 120), 3)); p.drawLine(16, 24, 22, 30); p.drawLine(22, 30, 32, 16); }
        else if (type == "remove" || type == "delete" || type == "edit_cut" || type == "edit-cut" || type == "edit-delete") { p.setPen(QPen(QColor(255, 100, 100), 3)); p.drawLine(16, 16, 32, 32); p.drawLine(32, 16, 16, 32); }
        else if (type == "save" || type == "document-save" || type == "document_save") { p.setBrush(Qt::transparent); p.drawRect(14, 14, 20, 20); p.fillRect(16, 14, 16, 6, Qt::white); }
        else if (type == "undo" || type == "edit-undo" || type == "edit_undo") { p.drawLine(16, 24, 24, 16); p.drawLine(16, 24, 24, 32); p.drawLine(16, 24, 32, 24); }
        else if (type == "printer" || type == "preview" || type == "help_contents" || type == "help-contents") { p.setBrush(Qt::transparent); p.drawRect(14, 20, 20, 14); p.drawRect(18, 14, 12, 6); }
        else if (type == "duplicate" || type == "copy" || type == "edit-copy" || type == "edit_copy") { p.setBrush(Qt::transparent); p.drawRect(14, 14, 14, 14); p.drawRect(20, 20, 14, 14); }
        else if (type == "bgimage" || type == "insert_image" || type == "insert-image") { p.setBrush(Qt::transparent); p.drawRect(12, 14, 24, 20); p.drawEllipse(16, 18, 4, 4); p.drawLine(12, 30, 20, 22); p.drawLine(20, 22, 36, 34); }
        else if (type == "circle" || type == "addcircle") { p.setBrush(Qt::transparent); p.setPen(QPen(QColor(240, 240, 240), 2)); p.drawEllipse(14, 14, 20, 20); }
        else if (type == "toggle_bg") { p.setBrush(Qt::transparent); p.drawEllipse(24, 24, 12, 8); p.drawEllipse(24, 24, 4, 4); }
        else if (type == "rename" || type == "system_file_manager" || type == "system-file-manager") { p.setBrush(Qt::transparent); p.drawRect(14, 20, 20, 8); p.drawLine(18, 20, 18, 28); }
        else if (type == "load" || type == "mail-unread" || type == "mail_unread") { p.setBrush(Qt::transparent); p.drawRect(12, 16, 24, 16); p.drawRect(12, 12, 10, 4); }
        else if (type == "fields" || type == "emblem-symbolic-link" || type == "emblem_symbolic_link") { p.setBrush(Qt::transparent); p.drawRect(14, 12, 20, 24); p.drawLine(18, 18, 30, 18); p.drawLine(18, 24, 30, 24); }
        else if (type == "qr" || type == "qrcode") {
            p.setBrush(Qt::white);
            p.drawRect(10, 10, 28, 28);
            p.setBrush(Qt::black);
            p.drawRect(12, 12, 8, 8);
            p.drawRect(28, 12, 8, 8);
            p.drawRect(12, 28, 8, 8);
            p.fillRect(15, 15, 2, 2, Qt::white);
            p.fillRect(31, 15, 2, 2, Qt::white);
            p.fillRect(15, 31, 2, 2, Qt::white);
            p.fillRect(23, 23, 3, 3, Qt::black);
            p.fillRect(27, 27, 4, 4, Qt::black);
        }
        else if (type == "new" || type == "edit_clear_all" || type == "edit-clear-all") { p.setBrush(Qt::white); p.drawRect(16, 12, 16, 24); }
        else if (type == "configure" || type == "window-restore" || type == "window_restore") { p.setBrush(Qt::transparent); p.drawRect(12, 16, 24, 16); p.drawLine(12, 16, 24, 24); p.drawLine(24, 24, 36, 16); }
        else { p.setBrush(Qt::transparent); p.drawRect(14, 14, 20, 20); }
        
        return QIcon(pix);
    };

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QHBoxLayout *topLayout = new QHBoxLayout();
    
    // Left controls
    QVBoxLayout *leftCtrl = new QVBoxLayout();
    
    // Top cursor info
    QHBoxLayout *cursorH = new QHBoxLayout();
    m_cursorLabel = new QLabel("<b>X: 0.00 in &nbsp; Y: 0.00 in</b>", this);
    m_cursorLabel->setFixedSize(296, 39);
    m_cursorLabel->setStyleSheet("QLabel { background-color: white; border: 1px solid black; padding: 5px; font-size: 11pt; color: blue; }");
    m_unitCombo = new QComboBox(this);
    m_unitCombo->setFixedSize(75, 32);
    m_unitCombo->addItems({"inches", "cm"});
    
    cursorH->addWidget(m_cursorLabel);
    cursorH->addWidget(m_unitCombo);
    // Removed from leftCtrl to move to right panel
    
    QToolButton *btnSize = new QToolButton(this);
    btnSize->setText("✉ Configure Envelope");
    btnSize->setIcon(createBtnIcon("configure"));
    btnSize->setIconSize(QSize(48, 48));
    btnSize->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnSize, &QToolButton::clicked, this, &QslDesigner::changePaperSize);
    
    m_view->viewport()->installEventFilter(this);

    QToolButton *btnAdd = new QToolButton(this);
    btnAdd->setText("Add Text");
    btnAdd->setIcon(createBtnIcon("add", "insert-text"));
    btnAdd->setIconSize(QSize(16, 16));
    btnAdd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnAdd, &QToolButton::clicked, this, [this]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }
        int maxBoxNum = 0;
        if (m_scene) {
            for (QGraphicsItem *item : m_scene->items()) {
                if (auto *area = dynamic_cast<PrintTableArea*>(item)) {
                    if (area->areaId.startsWith("Text Box ")) {
                        int num = area->areaId.mid(9).toInt();
                        if (num > maxBoxNum) maxBoxNum = num;
                    }
                }
            }
        }
        QString textBoxName = QString("Text Box %1").arg(maxBoxNum + 1);

        m_addMode = "TEXT";
        m_addArg = textBoxName;
        m_addType = 0; // Text Box
        m_addFieldMaxLines = 0;
        QApplication::setOverrideCursor(Qt::CrossCursor);
    });

    QToolButton *btnAddQso = new QToolButton(this);
    btnAddQso->setText("Add QSO Block");
    btnAddQso->setIcon(createBtnIcon("fields", "emblem-symbolic-link"));
    btnAddQso->setIconSize(QSize(16, 16));
    btnAddQso->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnAddQso, &QToolButton::clicked, this, [this]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }

        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        int lastCols = settings.value("QslUpdates/LastQsoBlockCols", 0).toInt();
        int lastContactRows = settings.value("QslUpdates/LastQsoBlockRows", 0).toInt();
        bool lastShowHeaders = settings.value("QslUpdates/LastQsoBlockShowHeaders", true).toBool();
        bool lastShowBorders = settings.value("QslUpdates/LastQsoBlockShowBorders", true).toBool();
        double lastBorderThickness = settings.value("QslUpdates/LastQsoBlockBorderThickness", 1.0).toDouble();
        QStringList lastCellData = settings.value("QslUpdates/LastQsoBlockCellData").toStringList();

        bool useLast = false;
        if (lastCols > 0 && lastContactRows > 0 && !lastCellData.isEmpty()) {
            QString fieldsSummary = lastCellData.mid(0, lastCols).join(", ");
            QMessageBox::StandardButton reply = QMessageBox::question(
                this,
                "Add QSO Block",
                QString("This was your last QSO Block layout:\n\n"
                        "Columns: %1\n"
                        "Rows (Contacts): %2\n"
                        "Show Column Headers: %3\n"
                        "Show Table Borders: %4 (Thickness: %5)\n"
                        "Fields: %6\n\n"
                        "Do you wish to use it again?")
                    .arg(lastCols)
                    .arg(lastContactRows)
                    .arg(lastShowHeaders ? "Yes" : "No")
                    .arg(lastShowBorders ? "Yes" : "No")
                    .arg(lastBorderThickness)
                    .arg(fieldsSummary),
                QMessageBox::Yes | QMessageBox::No
            );
            if (reply == QMessageBox::Yes) {
                useLast = true;
            }
        }

        if (useLast) {
            int totalRows = lastContactRows + (lastShowHeaders ? 1 : 0);
            m_qsoTableRows = totalRows;
            m_qsoTableCols = lastCols;
            m_qsoTableShowHeaders = lastShowHeaders;
            m_qsoTableShowBorders = lastShowBorders;
            m_qsoTableBorderThickness = lastBorderThickness;
            m_qsoTableCellData = lastCellData;

            m_addMode = "TEXT";
            m_addArg = "TABLE";
            m_addType = 1; // Table
            m_addFieldMaxLines = 0;
            QApplication::setOverrideCursor(Qt::CrossCursor);
            return;
        }

        QDialog d(this);
        d.setWindowTitle("Add QSO Block");
        d.resize(650, 520);
        d.setStyleSheet(
            "QDialog {"
            "  background-color: #1e222b;"
            "  border: 1px solid #3f4452;"
            "  border-radius: 8px;"
            "}"
            "QLabel {"
            "  color: #f1f5f9;"
            "  font-family: 'Segoe UI', Arial, sans-serif;"
            "  font-size: 10pt;"
            "}"
            "QSpinBox, QDoubleSpinBox {"
            "  background-color: #111318;"
            "  color: #f1f5f9;"
            "  border: 1px solid #2d313f;"
            "  border-radius: 4px;"
            "  padding: 4px;"
            "}"
            "QCheckBox {"
            "  color: #f1f5f9;"
            "  font-size: 10pt;"
            "}"
            "QTableWidget {"
            "  background-color: #111318;"
            "  gridline-color: #3f4452;"
            "  color: #e2e8f0;"
            "  font-family: 'Segoe UI', Arial, sans-serif;"
            "  font-size: 9pt;"
            "  border: 1px solid #2d313f;"
            "  border-radius: 6px;"
            "}"
            "QPushButton {"
            "  background-color: #3b82f6;"
            "  color: white;"
            "  border: none;"
            "  border-radius: 4px;"
            "  padding: 6px 14px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background-color: #2563eb;"
            "}"
            "QToolButton {"
            "  background-color: #374151;"
            "  color: #f3f4f6;"
            "  border: 1px solid #4b5563;"
            "  border-radius: 10px;"
            "  font-weight: bold;"
            "}"
        );

        QVBoxLayout *vl = new QVBoxLayout(&d);
        vl->setSpacing(10);
        vl->setContentsMargins(16, 16, 16, 16);

        QFormLayout *form = new QFormLayout();
        form->setSpacing(8);

        QSpinBox *spinRows = new QSpinBox(&d);
        spinRows->setRange(1, 100);
        spinRows->setValue(3);

        QSpinBox *spinCols = new QSpinBox(&d);
        spinCols->setRange(1, 20);
        spinCols->setValue(5);

        QWidget *headerContainer = new QWidget(&d);
        QHBoxLayout *hlHeader = new QHBoxLayout(headerContainer);
        hlHeader->setContentsMargins(0,0,0,0);
        hlHeader->setSpacing(6);

        QCheckBox *chkShowHeaders = new QCheckBox("Show column headers?", headerContainer);
        chkShowHeaders->setChecked(true);

        QToolButton *btnHeaderHelp = new QToolButton(headerContainer);
        btnHeaderHelp->setText("?");
        btnHeaderHelp->setFixedSize(20, 20);
        btnHeaderHelp->setToolTip("Use this if your template does not have a dedicated table");
        connect(btnHeaderHelp, &QToolButton::clicked, this, [&d]() {
            QMessageBox::information(&d, "Column Headers Help", "? Use this if your template does not have a dedicated table");
        });

        hlHeader->addWidget(chkShowHeaders);
        hlHeader->addWidget(btnHeaderHelp);
        hlHeader->addStretch();

        QCheckBox *chkShowBorders = new QCheckBox("Show table borders?", &d);
        chkShowBorders->setChecked(true);

        QDoubleSpinBox *spinThickness = new QDoubleSpinBox(&d);
        spinThickness->setRange(0.5, 4.0);
        spinThickness->setSingleStep(0.5);
        spinThickness->setValue(1.0);
        spinThickness->setEnabled(true);

        connect(chkShowBorders, &QCheckBox::toggled, spinThickness, &QDoubleSpinBox::setEnabled);

        form->addRow("Number of rows?", spinRows);
        form->addRow("Number of columns?", spinCols);
        form->addRow("", headerContainer);
        form->addRow("", chkShowBorders);
        form->addRow("Thickness? (0.5 - 4.0):", spinThickness);

        vl->addLayout(form);

        QLabel *lblHeaders = new QLabel("Column Header Text", &d);
        lblHeaders->setStyleSheet("font-weight: bold; color: #38bdf8; margin-top: 4px;");
        vl->addWidget(lblHeaders);

        QHBoxLayout *hdrFontLayout = new QHBoxLayout;
        QFontComboBox *hdrFontCombo = new QFontComboBox(&d);
        hdrFontCombo->setCurrentFont(QFont("Segoe UI", 9));
        QSpinBox *hdrSizeSpin = new QSpinBox(&d);
        hdrSizeSpin->setRange(6, 72);
        hdrSizeSpin->setValue(9);
        QCheckBox *chkHdrBold = new QCheckBox("Bold", &d);
        chkHdrBold->setChecked(true);
        QCheckBox *chkHdrItalic = new QCheckBox("Italic", &d);
        chkHdrItalic->setChecked(false);

        hdrFontLayout->addWidget(new QLabel("Header Font:", &d));
        hdrFontLayout->addWidget(hdrFontCombo, 1);
        hdrFontLayout->addWidget(new QLabel("Size:", &d));
        hdrFontLayout->addWidget(hdrSizeSpin);
        hdrFontLayout->addWidget(chkHdrBold);
        hdrFontLayout->addWidget(chkHdrItalic);
        vl->addLayout(hdrFontLayout);

        QTableWidget *twHeaders = new QTableWidget(&d);
        twHeaders->setRowCount(1);
        twHeaders->setColumnCount(5);
        twHeaders->setFixedHeight(42);
        twHeaders->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        twHeaders->verticalHeader()->setVisible(false);
        twHeaders->horizontalHeader()->setVisible(false);
        twHeaders->setEditTriggers(QAbstractItemView::AllEditTriggers);
        twHeaders->setStyleSheet(
            "QTableWidget { background-color: #111318; color: #38bdf8; font-weight: bold; font-size: 9pt; border: 1px solid #2d313f; border-radius: 4px; }"
            "QTableWidget::item { border: 1px solid #2d313f; padding: 2px; }"
            "QLineEdit { background-color: #1e293b; color: #38bdf8; font-weight: bold; border: 1px solid #3b82f6; border-radius: 2px; }"
        );

        for (int c = 0; c < 5; ++c) {
            QTableWidgetItem *hItem = new QTableWidgetItem("");
            hItem->setTextAlignment(Qt::AlignCenter);
            twHeaders->setItem(0, c, hItem);
        }
        vl->addWidget(twHeaders);

        QLabel *lblAssign = new QLabel("Column Fields (Double-click to change field tag):", &d);
        lblAssign->setStyleSheet("font-weight: bold; color: #f59e0b; margin-top: 4px;");
        vl->addWidget(lblAssign);

        QTableWidget *twFields = new QTableWidget(&d);
        twFields->setRowCount(1);
        twFields->setColumnCount(5);
        twFields->setFixedHeight(42);
        twFields->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        twFields->verticalHeader()->setVisible(false);
        twFields->horizontalHeader()->setVisible(false);
        twFields->setEditTriggers(QAbstractItemView::NoEditTriggers);

        QStringList defaultFields = {"<date>", "<time_utc>", "<band>", "<mode>", "<rst_sent>"};
        for (int c = 0; c < 5; ++c) {
            QTableWidgetItem *item = new QTableWidgetItem(defaultFields[c]);
            item->setTextAlignment(Qt::AlignCenter);
            twFields->setItem(0, c, item);
        }
        vl->addWidget(twFields);

        QLabel *lblPreview = new QLabel("Proposed Table Layout (Simplified Live Preview):", &d);
        lblPreview->setStyleSheet("font-weight: bold; color: #60a5fa; margin-top: 6px;");
        vl->addWidget(lblPreview);

        QTableWidget *twPreview = new QTableWidget(&d);
        twPreview->setEditTriggers(QAbstractItemView::NoEditTriggers);
        twPreview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        twPreview->verticalHeader()->setDefaultSectionSize(24);
        vl->addWidget(twPreview, 1);

        auto updateColsInFields = [twFields, twHeaders, &defaultFields](int newCols) {
            int oldCols = twFields->columnCount();
            twFields->setColumnCount(newCols);
            twHeaders->setColumnCount(newCols);
            for (int c = oldCols; c < newCols; ++c) {
                QString val = (c < defaultFields.size()) ? defaultFields[c] : "<rst_rcvd>";
                QTableWidgetItem *item = new QTableWidgetItem(val);
                item->setTextAlignment(Qt::AlignCenter);
                twFields->setItem(0, c, item);

                QTableWidgetItem *hItem = new QTableWidgetItem("");
                hItem->setTextAlignment(Qt::AlignCenter);
                twHeaders->setItem(0, c, hItem);
            }
        };

        auto updateLivePreview = [spinRows, spinCols, chkShowHeaders, chkShowBorders, spinThickness, twFields, twHeaders, twPreview, hdrFontCombo, hdrSizeSpin, chkHdrBold, chkHdrItalic]() {
            int contactRows = spinRows->value();
            int cols = spinCols->value();
            bool showHeaders = chkShowHeaders->isChecked();
            bool showBorders = chkShowBorders->isChecked();
            double thick = spinThickness->value();

            int totalRows = contactRows + (showHeaders ? 1 : 0);
            twPreview->setRowCount(totalRows);
            twPreview->setColumnCount(cols);
            twPreview->setShowGrid(showBorders);

            if (showBorders) {
                twPreview->setStyleSheet(QString(
                    "QTableWidget { background-color: #111318; gridline-color: #64748b; color: #e2e8f0; border: %1px solid #64748b; }"
                    "QTableWidget::item { border: %1px solid #64748b; padding: 2px; }"
                ).arg(qMax(1, qRound(thick))));
            } else {
                twPreview->setStyleSheet(
                    "QTableWidget { background-color: #111318; gridline-color: transparent; color: #94a3b8; border: 1px dashed #3f4452; }"
                    "QTableWidget::item { border: none; padding: 2px; }"
                );
            }

            int startContactRow = showHeaders ? 1 : 0;
            if (showHeaders) {
                twPreview->verticalHeader()->setVisible(false);
                twPreview->horizontalHeader()->setVisible(false);
                for (int c = 0; c < cols; ++c) {
                    QString headerText = (twHeaders->item(0, c)) ? twHeaders->item(0, c)->text().trimmed() : "";
                    QTableWidgetItem *hItem = new QTableWidgetItem(headerText);
                    hItem->setTextAlignment(Qt::AlignCenter);
                    QFont hFont = hdrFontCombo->currentFont();
                    hFont.setPointSize(hdrSizeSpin->value());
                    hFont.setBold(chkHdrBold->isChecked());
                    hFont.setItalic(chkHdrItalic->isChecked());
                    hItem->setFont(hFont);
                    hItem->setForeground(QBrush(QColor("#f59e0b")));
                    hItem->setBackground(QBrush(QColor("#1e293b")));
                    twPreview->setItem(0, c, hItem);
                }
            } else {
                twPreview->verticalHeader()->setVisible(false);
                twPreview->horizontalHeader()->setVisible(false);
            }

            for (int r = 0; r < contactRows; ++r) {
                int targetRow = startContactRow + r;
                for (int c = 0; c < cols; ++c) {
                    QString fieldTag = (twFields->item(0, c)) ? twFields->item(0, c)->text().toLower() : "";
                    QString sampleVal = "599";
                    if (fieldTag.contains("date")) sampleVal = QString("2026-10-%1").arg(27 - r);
                    else if (fieldTag.contains("time") || fieldTag.contains("utc")) sampleVal = QString("14:%10").arg(30 + r * 5);
                    else if (fieldTag.contains("band")) sampleVal = (r % 2 == 0) ? "20m" : "40m";
                    else if (fieldTag.contains("mode")) sampleVal = (r % 2 == 0) ? "FT8" : "CW";
                    else if (fieldTag.contains("sent") || fieldTag.contains("tx")) sampleVal = "599";
                    else if (fieldTag.contains("rcvd") || fieldTag.contains("rx")) sampleVal = "599";
                    else if (fieldTag.contains("call")) sampleVal = "W4DJF";
                    else if (fieldTag.contains("pwr") || fieldTag.contains("watt")) sampleVal = "100W";

                    QTableWidgetItem *item = new QTableWidgetItem(sampleVal);
                    item->setTextAlignment(Qt::AlignCenter);
                    item->setForeground(QBrush(QColor("#e2e8f0")));
                    twPreview->setItem(targetRow, c, item);
                }
            }
        };

        connect(spinRows, QOverload<int>::of(&QSpinBox::valueChanged), this, [spinCols, updateColsInFields, updateLivePreview](int val) {
            Q_UNUSED(val);
            updateLivePreview();
        });
        connect(spinCols, QOverload<int>::of(&QSpinBox::valueChanged), this, [updateColsInFields, updateLivePreview](int newCols) {
            updateColsInFields(newCols);
            updateLivePreview();
        });
        connect(chkShowHeaders, &QCheckBox::toggled, this, [updateLivePreview](bool) { updateLivePreview(); });
        connect(chkShowBorders, &QCheckBox::toggled, this, [updateLivePreview](bool) { updateLivePreview(); });
        connect(spinThickness, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [updateLivePreview](double) { updateLivePreview(); });
        connect(twHeaders, &QTableWidget::cellChanged, this, [updateLivePreview](int, int) { updateLivePreview(); });
        connect(hdrFontCombo, &QFontComboBox::currentFontChanged, this, [updateLivePreview](const QFont &) { updateLivePreview(); });
        connect(hdrSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [updateLivePreview](int) { updateLivePreview(); });
        connect(chkHdrBold, &QCheckBox::toggled, this, [updateLivePreview](bool) { updateLivePreview(); });
        connect(chkHdrItalic, &QCheckBox::toggled, this, [updateLivePreview](bool) { updateLivePreview(); });

        connect(twFields, &QTableWidget::cellDoubleClicked, this, [this, twFields, &d, updateLivePreview](int r, int c) {
            Q_UNUSED(r);
            QDialog chooser(&d);
            chooser.setWindowTitle("Choose Tag");
            chooser.resize(300, 400);
            chooser.setStyleSheet(
                "QDialog { background-color: #1e222b; border: 1px solid #3f4452; border-radius: 8px; }"
                "QListWidget { background-color: #111318; color: #e2e8f0; border: 1px solid #2d313f; border-radius: 6px; padding: 5px; }"
                "QListWidget::item { padding: 6px; border-radius: 4px; }"
                "QListWidget::item:hover { background-color: #2e3440; color: #ffffff; }"
                "QListWidget::item:selected { background-color: #3b82f6; color: #ffffff; }"
            );

            QVBoxLayout *cl = new QVBoxLayout(&chooser);
            QListWidget *lw = new QListWidget(&chooser);

            QSettings settings(QCoreApplication::organizationName(), "Qwain");
            QStringList adifTags = settings.value("Station/AdifTags").toStringList();
            if (adifTags.isEmpty()) {
                adifTags << "<date> - Date of Contact"
                         << "<day> - Numerical (1 ... 31)"
                         << "<month> - Numerical (1 ... 12)"
                         << "<year> - Numerical"
                         << "<time_utc> - UTC Time Of Contact"
                         << "<band> - Band"
                         << "<mode> - Mode"
                         << "<tx_pwr> - Power"
                         << "<rst_rcvd> - Received Signal Report"
                         << "<rst_sent> - Sent Signal Report"
                         << "<call> - Callsign of Contact"
                         << "<name> - Name of Contact"
                         << "<qrp> - QSO <= 6Watts";
            }
            lw->addItems(adifTags);
            cl->addWidget(lw);

            connect(lw, &QListWidget::itemClicked, &chooser, [twFields, c, &chooser, updateLivePreview](QListWidgetItem *item) {
                if (item) {
                    QString tag = item->text();
                    QString bareTag = tag;
                    if (tag.startsWith("<") && tag.contains(">")) {
                        bareTag = tag.left(tag.indexOf(">") + 1);
                    }
                    if (twFields->item(0, c)) {
                        twFields->item(0, c)->setText(bareTag);
                    }
                    updateLivePreview();
                }
                chooser.accept();
            });
            chooser.exec();
        });

        updateColsInFields(spinCols->value());
        updateLivePreview();

        QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &d);
        vl->addWidget(box);
        connect(box, &QDialogButtonBox::accepted, &d, &QDialog::accept);
        connect(box, &QDialogButtonBox::rejected, &d, &QDialog::reject);

        if (d.exec() == QDialog::Accepted) {
            int contactRows = spinRows->value();
            int cols = spinCols->value();
            bool showHeaders = chkShowHeaders->isChecked();
            bool showBorders = chkShowBorders->isChecked();
            double thickness = spinThickness->value();

            int totalRows = contactRows + (showHeaders ? 1 : 0);

            m_qsoTableRows = totalRows;
            m_qsoTableCols = cols;
            m_qsoTableShowHeaders = showHeaders;
            m_qsoTableShowBorders = showBorders;
            m_qsoTableBorderThickness = thickness;
            m_qsoTableCellData.clear();

            for (int r = 0; r < totalRows; ++r) {
                for (int c = 0; c < cols; ++c) {
                    if (r == 0 && showHeaders) {
                        QString customHeader = (twHeaders->item(0, c)) ? twHeaders->item(0, c)->text().trimmed() : "";
                        m_qsoTableCellData << customHeader;
                    } else {
                        QString tag = twFields->item(0, c) ? twFields->item(0, c)->text() : QString("<field_%1>").arg(c+1);
                        m_qsoTableCellData << tag;
                    }
                }
            }

            QSettings settings(QCoreApplication::organizationName(), "Qwain");
            settings.setValue("QslUpdates/LastQsoBlockCols", cols);
            settings.setValue("QslUpdates/LastQsoBlockRows", contactRows);
            settings.setValue("QslUpdates/LastQsoBlockShowHeaders", showHeaders);
            settings.setValue("QslUpdates/LastQsoBlockShowBorders", showBorders);
            settings.setValue("QslUpdates/LastQsoBlockBorderThickness", thickness);
            settings.setValue("QslUpdates/LastQsoBlockCellData", m_qsoTableCellData);

            m_addMode = "TEXT";
            m_addArg = "TABLE";
            m_addType = 1; // Table
            m_addFieldMaxLines = 0;
            QApplication::setOverrideCursor(Qt::CrossCursor);
        }
    });

    QToolButton *btnAddCheck = new QToolButton(this);
    btnAddCheck->setText("Add Check");
    btnAddCheck->setIcon(createBtnIcon("check", "emblem-favorite"));
    btnAddCheck->setIconSize(QSize(16, 16));
    btnAddCheck->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnAddCheck, &QToolButton::clicked, this, [this]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }
        m_addMode = "CHK_CUSTOM";
        m_addArg = "";
        QApplication::setOverrideCursor(Qt::CrossCursor);
    });

    QToolButton *btnAddCircle = new QToolButton(this);
    btnAddCircle->setText("Add Circle");
    btnAddCircle->setIcon(createBtnIcon("circle"));
    btnAddCircle->setIconSize(QSize(16, 16));
    btnAddCircle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnAddCircle, &QToolButton::clicked, this, [this]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }
        m_addMode = "CIRC_CUSTOM";
        m_addArg = "";
        QApplication::setOverrideCursor(Qt::CrossCursor);
    });

    QToolButton *btnAddAddress = new QToolButton(this);
    btnAddAddress->setText("Add Address");
    btnAddAddress->setIcon(createBtnIcon("load", "mail-unread"));
    btnAddAddress->setIconSize(QSize(16, 16));
    btnAddAddress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnAddAddress, &QToolButton::clicked, this, [this]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }
        m_addMode = "TEXT";
        m_addArg = "ADDRESS_BOX";
        m_addType = 4; // Address Box
        m_addFieldMaxLines = 0;
        QApplication::setOverrideCursor(Qt::CrossCursor);
    });

    QToolButton *btnAddSr = new QToolButton(this);
    btnAddSr->setText("Add SR");
    btnAddSr->setIcon(createBtnIcon("configure", "window-restore"));
    btnAddSr->setIconSize(QSize(16, 16));
    btnAddSr->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnAddSr, &QToolButton::clicked, this, [this]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }
        for (QGraphicsItem *item : m_scene->items()) {
            PrintTableArea *area = dynamic_cast<PrintTableArea*>(item);
            if (area && area->areaId == "STAMP_REMINDER") {
                QMessageBox::warning(this, "Max Stamp Reminder", "You can only place 1 stamp reminder.");
                return;
            }
        }
        m_addMode = "STAMP_REMINDER";
        m_addArg = "STAMP_REMINDER";
        m_addType = 5; // Stamp Reminder
        m_addFieldMaxLines = 0;
        QApplication::setOverrideCursor(Qt::CrossCursor);
    });
    
    QToolButton *btnAddCustomChk = new QToolButton(this);
    btnAddCustomChk->setText("✔️ Add Custom Checkmark");
    btnAddCustomChk->setIcon(createBtnIcon("check"));
    btnAddCustomChk->setIconSize(QSize(48, 48));
    btnAddCustomChk->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnAddCustomChk, &QToolButton::clicked, this, [this]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }
        m_addMode = "CHK_CUSTOM";
        m_addArg = "";
        QApplication::setOverrideCursor(Qt::CrossCursor);
    });

    QToolButton *btnAddRefPoint = new QToolButton(this);
    btnAddRefPoint->setText("📍 Add Reference Point");
    btnAddRefPoint->setIcon(createBtnIcon("refpoint"));
    btnAddRefPoint->setIconSize(QSize(48, 48));
    btnAddRefPoint->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnAddRefPoint, &QToolButton::clicked, this, [this]() {
        // Enforce max 2 reference points
        int refCount = 0;
        for (QGraphicsItem *item : m_scene->items()) {
            PrintTableArea *area = dynamic_cast<PrintTableArea*>(item);
            if (area && area->areaId.startsWith("REF_POINT_")) {
                refCount++;
            }
        }
        if (refCount >= 2) {
            QMessageBox::warning(this, "Max Ref Points", "You can only place 2 reference points.");
            return;
        }
        
        m_addMode = "REF_POINT";
        m_addArg = "REF_POINT_" + QString::number(refCount + 1);
        QApplication::setOverrideCursor(Qt::CrossCursor);
    });
    
    QToolButton *btnRm = new QToolButton(this);
    btnRm->setText("🗑️ Remove Selected");
    btnRm->setIcon(createBtnIcon("remove"));
    btnRm->setIconSize(QSize(48, 48));
    btnRm->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnRm, &QToolButton::clicked, this, [this]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }
        removePrintArea();
    });

    QToolButton *btnChangeSize = new QToolButton(this);
    btnChangeSize->setText("📏 Change Size");
    btnChangeSize->setIcon(createBtnIcon("configure"));
    btnChangeSize->setIconSize(QSize(48, 48));
    btnChangeSize->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnChangeSize, &QToolButton::clicked, this, [this]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }
        
        QDialog dlg(this);
        dlg.setWindowTitle("Change Size");
        dlg.setMinimumWidth(380);
        dlg.setStyleSheet(
            "QDialog {"
            "  background-color: #1e222b;"
            "  border: 1px solid #3f4452;"
            "  border-radius: 8px;"
            "}"
            "QLabel {"
            "  color: #f1f5f9;"
            "  font-family: 'Segoe UI', Arial, sans-serif;"
            "  font-size: 10pt;"
            "  font-weight: bold;"
            "}"
            "QLineEdit {"
            "  background-color: #111318;"
            "  color: #e2e8f0;"
            "  border: 1px solid #2d313f;"
            "  border-radius: 4px;"
            "  padding: 6px;"
            "  font-size: 10pt;"
            "}"
            "QPushButton {"
            "  background-color: #3b82f6;"
            "  color: white;"
            "  border: none;"
            "  border-radius: 4px;"
            "  padding: 8px 16px;"
            "  font-weight: bold;"
            "  font-size: 10pt;"
            "}"
            "QPushButton:hover {"
            "  background-color: #2563eb;"
            "}"
            "QPushButton#btnCancel {"
            "  background-color: #4b5563;"
            "}"
            "QPushButton#btnStd {"
            "  background-color: #2d313f;"
            "  color: #f1f5f9;"
            "  border: 1px solid #3f4452;"
            "  font-size: 9pt;"
            "  font-weight: normal;"
            "  padding: 6px 12px;"
            "}"
            "QPushButton#btnStd:hover {"
            "  background-color: #3b82f6;"
            "}"
        );
        
        QVBoxLayout *vl = new QVBoxLayout(&dlg);
        vl->setSpacing(12);
        vl->setContentsMargins(16, 16, 16, 16);
        
        QHBoxLayout *inputLayout = new QHBoxLayout();
        inputLayout->setSpacing(8);
        
        QLineEdit *wEdit = new QLineEdit(QString::number(m_paperW / 96.0, 'f', 3), &dlg);
        wEdit->setPlaceholderText("Width (in)");
        QLineEdit *hEdit = new QLineEdit(QString::number(m_paperH / 96.0, 'f', 3), &dlg);
        hEdit->setPlaceholderText("Height (in)");
        
        inputLayout->addWidget(new QLabel("W (in):", &dlg));
        inputLayout->addWidget(wEdit);
        inputLayout->addWidget(new QLabel("H (in):", &dlg));
        inputLayout->addWidget(hEdit);
        vl->addLayout(inputLayout);
        
        vl->addWidget(new QLabel("Standard Presets:", &dlg));
        
        QPushButton *btnStdQsl = new QPushButton("Std QSL Card (5.500 x 3.500 in)", &dlg);
        btnStdQsl->setObjectName("btnStd");
        QPushButton *btnStdNo6 = new QPushButton("Std No 6 Envelope (6.500 x 3.375 in)", &dlg);
        btnStdNo6->setObjectName("btnStd");
        
        vl->addWidget(btnStdQsl);
        vl->addWidget(btnStdNo6);
        
        connect(btnStdQsl, &QPushButton::clicked, this, [wEdit, hEdit]() {
            wEdit->setText("5.500");
            hEdit->setText("3.500");
        });
        connect(btnStdNo6, &QPushButton::clicked, this, [wEdit, hEdit]() {
            wEdit->setText("6.500");
            hEdit->setText("3.375");
        });
        
        QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        QPushButton *okBtn = box->button(QDialogButtonBox::Ok);
        QPushButton *cancelBtn = box->button(QDialogButtonBox::Cancel);
        cancelBtn->setObjectName("btnCancel");
        
        connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        vl->addWidget(box);
        
        if (dlg.exec() == QDialog::Accepted) {
            double w = wEdit->text().toDouble();
            double h = hEdit->text().toDouble();
            if (w <= 0.0 || h <= 0.0) {
                QMessageBox::warning(this, "Invalid Dimensions", "Width and Height must be positive numbers.");
                return;
            }
            
            m_paperW = w * 96.0;
            m_paperH = h * 96.0;
            m_isModified = true;
            drawBackgroundRulers();
            m_scene->update();
            if (m_scene && m_view) {
                m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
            }
            
            // remove current alignment/reference points
            QList<QGraphicsItem*> itemsToRemove;
            for (QGraphicsItem *item : m_scene->items()) {
                PrintTableArea *area = dynamic_cast<PrintTableArea*>(item);
                if (area && area->areaId.startsWith("REF_POINT_")) {
                    itemsToRemove.append(item);
                }
            }
            for (QGraphicsItem *item : itemsToRemove) {
                m_scene->removeItem(item);
                delete item;
            }
            
            // begin adding reference points procedure
            showCalibrationMessage(this, "Calibration Instruction", "Please Place Your First Reference Point at the Upper Leftmost Corner of your template.");
            m_addMode = "REF_POINT";
            m_addArg = "REF_POINT_1";
            QApplication::setOverrideCursor(Qt::CrossCursor);
        }
    });

    QToolButton *btnAdvLayer = new QToolButton(this);
    btnAdvLayer->setText("Copy\n Layers");
    btnAdvLayer->setIcon(createBtnIcon("duplicate", "edit-copy"));
    btnAdvLayer->setIconSize(QSize(48, 48));
    btnAdvLayer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnAdvLayer, &QToolButton::clicked, this, [this, btnAdvLayer, createBtnIcon]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }
        
        if (!m_hasCopiedLayers) {
            // Copy layers to clipboard
            QVariantList itemsList;
            for (QGraphicsItem *item : m_scene->items()) {
                PrintTableArea *area = dynamic_cast<PrintTableArea*>(item);
                if (area) {
                    QVariantMap m;
                    m["id"] = area->areaId;
                    m["text"] = area->textContent;
                    m["x"] = area->pos().x();
                    m["y"] = area->pos().y();
                    m["font"] = area->m_font.family();
                    m["size"] = area->m_font.pointSize();
                    m["color"] = area->m_color.name();
                    m["rot"] = area->rotation();
                    m["w"] = area->rect().width();
                    m["h"] = area->rect().height();
                    m["maxLines"] = area->m_maxLines;
                    m["align"] = area->m_align;
                    m["rows"] = area->m_rows;
                    m["cols"] = area->m_cols;
                    m["showHeaders"] = area->m_showHeaders;
                    m["showBorders"] = area->m_showBorders;
                    m["borderThickness"] = area->m_borderThickness;
                    m["rowHeights"] = area->m_rowHeightsStr;
                    m["colWidths"] = area->m_colWidthsStr;
                    m["isImage"] = area->m_isImage;
                    m["imagePath"] = area->m_imagePath;
                    m["isQrCode"] = area->m_isQrCode;
                    m["qrPayload"] = area->m_qrPayload;
                    m["circleLineWidth"] = area->m_circleLineWidth;
                    m["zValue"] = area->zValue();
                    m["cellData"] = area->m_cellData;
                    
                    m["p0_norm_x"] = area->m_p0_norm.x();
                    m["p0_norm_y"] = area->m_p0_norm.y();
                    m["p1_norm_x"] = area->m_p1_norm.x();
                    m["p1_norm_y"] = area->m_p1_norm.y();
                    m["p2_norm_x"] = area->m_p2_norm.x();
                    m["p2_norm_y"] = area->m_p2_norm.y();
                    m["p3_norm_x"] = area->m_p3_norm.x();
                    m["p3_norm_y"] = area->m_p3_norm.y();

                    itemsList.append(m);
                }
            }

            QVariantMap root;
            root["signature"] = "Qwain_QSL_Layers";
            root["items"] = itemsList;

            QJsonDocument doc = QJsonDocument::fromVariant(root);
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(doc.toJson(QJsonDocument::Compact));

            m_hasCopiedLayers = true;
            btnAdvLayer->setText("Paste\n Layers");
            btnAdvLayer->setIcon(createBtnIcon("add", "edit-paste"));
        } else {
            // Paste layers from clipboard
            QClipboard *clipboard = QApplication::clipboard();
            QString jsonStr = clipboard->text();
            QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
            
            if (!doc.isNull() && doc.isObject()) {
                QVariantMap root = doc.toVariant().toMap();
                if (root["signature"].toString() == "Qwain_QSL_Layers") {
                    pushUndo(); // Safe undo step
                    
                    // Appending pasted layers to target template without clearing existing items

                    QVariantList itemsList = root["items"].toList();
                    for (const QVariant &v : itemsList) {
                        QVariantMap m = v.toMap();
                        PrintTableArea *area = new PrintTableArea(m["text"].toString());
                        area->areaId = m["id"].toString();
                        area->m_isAddressBox = (area->areaId == "ADDRESS_BOX");
                        area->setPos(m["x"].toDouble() + 10.0, m["y"].toDouble() + 10.0);
                        area->m_font = QFont(m["font"].toString(), m["size"].toInt());
                        area->m_color = QColor(m["color"].toString());
                        area->setRotation(m["rot"].toDouble());
                        area->setRect(0, 0, m["w"].toDouble(), m["h"].toDouble());
                        
                        area->m_maxLines = m["maxLines"].toInt();
                        if (m.contains("align")) area->m_align = m["align"].toInt();
                        else area->m_align = Qt::AlignCenter;
                        
                        area->m_isImage = m["isImage"].toBool();
                        area->m_imagePath = m["imagePath"].toString();
                        area->m_isQrCode = m.value("isQrCode", area->areaId.startsWith("QR_CODE") || area->m_imagePath.contains("qr_")).toBool();
                        area->m_qrPayload = m.value("qrPayload", "").toString();
                        if (area->m_isQrCode) area->ensureQrModules();
                        if (m.contains("circleLineWidth")) {
                            area->m_circleLineWidth = m["circleLineWidth"].toInt();
                        }
                        if (area->m_isImage && !area->m_imagePath.isEmpty()) {
                            area->m_pixmap.load(area->m_imagePath);
                        }
                        if (m.contains("zValue")) area->setZValue(m["zValue"].toDouble());
                        
                        area->m_rows = m["rows"].toInt();
                        area->m_cols = m["cols"].toInt();
                        if (m.contains("showHeaders")) area->m_showHeaders = m["showHeaders"].toBool();
                        if (m.contains("showBorders")) area->m_showBorders = m["showBorders"].toBool();
                        if (m.contains("borderThickness")) area->m_borderThickness = m["borderThickness"].toDouble();
                        area->m_rowHeightsStr = m["rowHeights"].toString();
                        area->m_colWidthsStr = m["colWidths"].toString();
                        
                        if (m.contains("p0_norm_x")) {
                            area->m_p0_norm = QPointF(m["p0_norm_x"].toDouble(), m["p0_norm_y"].toDouble());
                            area->m_p1_norm = QPointF(m["p1_norm_x"].toDouble(), m["p1_norm_y"].toDouble());
                            area->m_p2_norm = QPointF(m["p2_norm_x"].toDouble(), m["p2_norm_y"].toDouble());
                            area->m_p3_norm = QPointF(m["p3_norm_x"].toDouble(), m["p3_norm_y"].toDouble());
                        }

                        area->m_cellData = m["cellData"].toStringList();
                        m_scene->addItem(area);
                    }
                    
                    m_isModified = true;
                    drawBackgroundRulers();
                    updateBlockList();
                    m_scene->update();
                    m_view->update();
                } else {
                    QMessageBox::warning(this, "Clipboard Error", "Clipboard does not contain valid Qwain QSL Layers.");
                }
            } else {
                QMessageBox::warning(this, "Clipboard Error", "Clipboard is empty or does not contain valid template layout layer data.");
            }

            m_hasCopiedLayers = false;
            btnAdvLayer->setText("Copy\n Layers");
            btnAdvLayer->setIcon(createBtnIcon("duplicate", "edit-copy"));
        }
    });

    // QR Code generation button (below Copy Layers)
    QToolButton *btnQrCode = new QToolButton(this);
    btnQrCode->setText("QR Code");
    btnQrCode->setIcon(createBtnIcon("qr", "insert-image"));
    btnQrCode->setIconSize(QSize(48, 48));
    btnQrCode->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnQrCode, &QToolButton::clicked, this, [this]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template. Select One or New");
            return;
        }

        QDialog qrDlg(this);
        qrDlg.setWindowTitle("Generate QR Code");
        qrDlg.setMinimumWidth(380);
        qrDlg.setStyleSheet(
            "QDialog { background-color: #1e222b; border: 1px solid #3f4452; border-radius: 8px; }"
            "QLabel { color: #f1f5f9; font-family: 'Segoe UI', Arial, sans-serif; font-size: 10pt; font-weight: bold; }"
            "QLineEdit { background-color: #111318; color: #38bdf8; border: 1px solid #3b82f6; border-radius: 4px; padding: 6px; font-size: 10pt; }"
            "QPushButton { background-color: #3b82f6; color: white; border: none; border-radius: 4px; padding: 8px 16px; font-weight: bold; font-family: 'Segoe UI', Arial, sans-serif; font-size: 10pt; }"
            "QPushButton:hover { background-color: #2563eb; }"
            "QPushButton#btnCancel { background-color: #64748b; }"
            "QPushButton#btnCancel:hover { background-color: #475569; }"
        );

        QVBoxLayout *vl = new QVBoxLayout(&qrDlg);
        vl->setSpacing(12);
        vl->setContentsMargins(16, 16, 16, 16);

        QLabel *lbl = new QLabel("Enter URL or text to encode into QR code:", &qrDlg);
        vl->addWidget(lbl);

        QLineEdit *editUrl = new QLineEdit(&qrDlg);
        editUrl->setPlaceholderText("https://qrz.com/db/YOURCALL or any URL / text");
        vl->addWidget(editUrl);

        QHBoxLayout *hlBtns = new QHBoxLayout();
        QPushButton *btnOk = new QPushButton("OK", &qrDlg);
        QPushButton *btnCancel = new QPushButton("Cancel", &qrDlg);
        btnCancel->setObjectName("btnCancel");
        hlBtns->addStretch();
        hlBtns->addWidget(btnOk);
        hlBtns->addWidget(btnCancel);
        vl->addLayout(hlBtns);

        connect(btnOk, &QPushButton::clicked, &qrDlg, &QDialog::accept);
        connect(btnCancel, &QPushButton::clicked, &qrDlg, &QDialog::reject);

        if (qrDlg.exec() == QDialog::Accepted) {
            QString urlText = editUrl->text().trimmed();
            if (urlText.isEmpty()) {
                QMessageBox::warning(this, "Empty Input", "No URL or text was provided for the QR code.");
                return;
            }

            try {
                using qrcodegen::QrCode;
                QrCode qr = QrCode::encodeText(urlText.toUtf8().constData(), QrCode::Ecc::MEDIUM);
                int qrSize = qr.getSize();
                int border = 2; // Quiet zone border in modules
                int totalModules = qrSize + border * 2;
                int modulePixels = 8; // Crisp 8px per module for high-DPI rendering
                int imgSize = totalModules * modulePixels;

                QImage qrImg(imgSize, imgSize, QImage::Format_RGB32);
                qrImg.fill(Qt::white);
                QPainter p(&qrImg);
                p.setPen(Qt::NoPen);
                p.setBrush(Qt::black);

                for (int y = 0; y < qrSize; ++y) {
                    for (int x = 0; x < qrSize; ++x) {
                        if (qr.getModule(x, y)) {
                            p.drawRect((x + border) * modulePixels, (y + border) * modulePixels, modulePixels, modulePixels);
                        }
                    }
                }
                p.end();

                // Save to local template images directory for persistent template loading & printing
                QDir dataDir(getAppDataDir());
                dataDir.mkdir("tpl_images");
                QString filename = QString("qr_%1.png").arg(QDateTime::currentMSecsSinceEpoch());
                QString fullPath = dataDir.filePath("tpl_images/" + filename);
                qrImg.save(fullPath, "PNG");

                pushUndo(); // Record undo state before adding QR element

                PrintTableArea *area = new PrintTableArea("QR_CODE", true, fullPath);
                area->m_isImage = true;
                area->m_itemType = ITEM_IMAGE;
                area->m_imagePath = fullPath;
                area->m_pixmap = QPixmap::fromImage(qrImg);
                area->m_isQrCode = true;
                area->m_qrPayload = urlText;
                area->ensureQrModules();
                
                // Set default square size on template (90 x 90 pt)
                double defaultSize = 90.0;
                area->setRect(0, 0, defaultSize, defaultSize);
                area->setZValue(1.5); // Layered element above background and table for immediate click-ability

                // Place in clear visible area without completely overlapping another layer (e.g. QSO table)
                QPointF initPos = findInitialElementPos(defaultSize, defaultSize);
                area->setPos(initPos);

                m_scene->addItem(area);
                m_scene->clearSelection();
                area->setSelected(true);
                m_isModified = true;
                updateBlockList();
            } catch (const std::exception &ex) {
                QMessageBox::critical(this, "QR Generation Error", QString("Failed to generate QR code: %1").arg(ex.what()));
            }
        }
    });

    btnTestView = new QToolButton(this);
    btnTestView->setText("Test View");
    btnTestView->setIcon(createBtnIcon("fields", "view-preview"));
    btnTestView->setIconSize(QSize(48, 48));
    btnTestView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnTestView, &QToolButton::clicked, this, [this, createBtnIcon]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }
        
        m_isTestView = !m_isTestView;
        
        // Update the button text to show active mode
        if (m_isTestView) {
            btnTestView->setText("Design\nView");
            btnTestView->setIcon(createBtnIcon("configure", "view-preview"));
        } else {
            btnTestView->setText("Test View");
            btnTestView->setIcon(createBtnIcon("fields", "view-preview"));
        }
        
        m_scene->setProperty("isTestView", m_isTestView);
        
        // Disable/enable item movement and selection during Test View
        for (QGraphicsItem *item : m_scene->items()) {
            if (PrintTableArea *area = dynamic_cast<PrintTableArea*>(item)) {
                if (m_isTestView) {
                    area->setFlag(QGraphicsItem::ItemIsMovable, false);
                    area->setFlag(QGraphicsItem::ItemIsSelectable, false);
                    area->setFlag(QGraphicsItem::ItemIsFocusable, false);
                } else {
                    area->setFlag(QGraphicsItem::ItemIsMovable, true);
                    area->setFlag(QGraphicsItem::ItemIsSelectable, true);
                    area->setFlag(QGraphicsItem::ItemIsFocusable, true);
                }
            }
        }
        m_scene->clearSelection();
        m_scene->update();
    });

    QToolButton *btnBg = new QToolButton(this);
    btnBg->setText("Add Image");
    btnBg->setIcon(createBtnIcon("bgimage", "insert-image"));
    btnBg->setIconSize(QSize(48, 48));
    btnBg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnBg, &QToolButton::clicked, this, [this]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }
        ImageViewer dlg(true, this);
        QString fp;
        if (dlg.exec() == QDialog::Accepted) {
            fp = dlg.selectedImagePath();
        }
        if (!fp.isEmpty()) {
            pushUndo(); // Save before adding
            PrintTableArea *area = new PrintTableArea("IMAGE");
            area->m_isImage = true;
            area->m_imagePath = fp;
            area->m_pixmap.load(fp);
            if (!area->m_pixmap.isNull()) {
                area->setRect(0, 0, area->m_pixmap.width()/4.0, area->m_pixmap.height()/4.0); // scale it initially
            }
            
            QDialog layerDlg(this);
            layerDlg.setWindowTitle("Image Layering");
            layerDlg.setMinimumWidth(360);
            layerDlg.setStyleSheet(
                "QDialog {"
                "  background-color: #1e222b;"
                "  border: 1px solid #3f4452;"
                "  border-radius: 8px;"
                "}"
                "QLabel {"
                "  color: #f1f5f9;"
                "  font-family: 'Segoe UI', Arial, sans-serif;"
                "  font-size: 10pt;"
                "  font-weight: bold;"
                "}"
                "QPushButton {"
                "  background-color: #3b82f6;"
                "  color: white;"
                "  border: none;"
                "  border-radius: 4px;"
                "  padding: 8px 16px;"
                "  font-weight: bold;"
                "  font-family: 'Segoe UI', Arial, sans-serif;"
                "  font-size: 10pt;"
                "}"
                "QPushButton:hover {"
                "  background-color: #2563eb;"
                "}"
                "QPushButton#btnBg {"
                "  background-color: #10b981;"
                "}"
                "QPushButton#btnBg:hover {"
                "  background-color: #059669;"
                "}"
            );

            QPropertyAnimation *animLayer = new QPropertyAnimation(&layerDlg, "windowOpacity");
            animLayer->setDuration(200);
            animLayer->setStartValue(0.0);
            animLayer->setEndValue(1.0);
            animLayer->setEasingCurve(QEasingCurve::OutCubic);
            layerDlg.setWindowOpacity(0.0);
            animLayer->start(QAbstractAnimation::DeleteWhenStopped);

            QVBoxLayout *vlLayer = new QVBoxLayout(&layerDlg);
            QLabel *lblLayer = new QLabel("Is This A Layered Image or Background?", &layerDlg);
            lblLayer->setAlignment(Qt::AlignCenter);
            vlLayer->addWidget(lblLayer);
            vlLayer->addSpacing(15);
            
            QHBoxLayout *hlLayer = new QHBoxLayout();
            QPushButton *btnLayered = new QPushButton("Layered", &layerDlg);
            QPushButton *btnBackground = new QPushButton("Background", &layerDlg);
            btnBackground->setObjectName("btnBg");
            
            hlLayer->addWidget(btnLayered);
            hlLayer->addWidget(btnBackground);
            vlLayer->addLayout(hlLayer);

            int layerResult = 1; // 1 = Layered, 2 = Background
            connect(btnLayered, &QPushButton::clicked, [&layerDlg, &layerResult]() {
                layerResult = 1;
                layerDlg.accept();
            });
            connect(btnBackground, &QPushButton::clicked, [&layerDlg, &layerResult]() {
                layerResult = 2;
                layerDlg.accept();
            });

            if (layerDlg.exec() == QDialog::Accepted) {
                if (layerResult == 1) {
                    // ========================================================================
                    // FEATURE: Layered Image Click-ability & Non-Overlapping Initial Placement
                    // INTENDED FUNCTION:
                    //   Elevate layered images to zValue 1.5 (above background at -1.5 and
                    //   table/text at 1.0) so the newly added image immediately receives mouse
                    //   focus, click events, and drag operations without column/row resize
                    //   handles of underlying tables intercepting the cursor.
                    //   Position the image dynamically in a visible, clear area of the card
                    //   instead of dead-center on top of the QSO table.
                    // ========================================================================
                    area->setZValue(1.5);
                    QPointF initPos = findInitialElementPos(area->rect().width(), area->rect().height());
                    area->setPos(initPos);
                } else {
                    for (QGraphicsItem *item : m_scene->items()) {
                        if (auto *otherArea = dynamic_cast<PrintTableArea*>(item)) {
                            if (otherArea != area && otherArea->zValue() == -1.5 && otherArea->m_isImage) {
                                m_scene->removeItem(otherArea);
                                delete otherArea;
                            }
                        }
                    }
                    area->setZValue(-1.5); // Background: behind everything
                    area->setRect(0, 0, m_paperW, m_paperH);
                    area->setPos(0, 0);
                }
            } else {
                area->setZValue(1.5);
                QPointF initPos = findInitialElementPos(area->rect().width(), area->rect().height());
                area->setPos(initPos);
            }

            m_scene->addItem(area);
            m_scene->clearSelection();
            area->setSelected(true);
            m_isModified = true;
        }
    });

    QToolButton *btnToggleBg = new QToolButton(this);
    btnToggleBg->setText("👁️ Toggle BG Image");
    btnToggleBg->setIcon(createBtnIcon("toggle_bg"));
    btnToggleBg->setIconSize(QSize(48, 48));
    btnToggleBg->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnToggleBg, &QToolButton::clicked, this, [this](){
        if (m_bgImageItem) {
            m_bgImageItem->setVisible(!m_bgImageItem->isVisible());
        }
    });

    // Properties for active element
    QFrame *propFrame = new QFrame(this);
    propFrame->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout *propLayout = new QVBoxLayout(propFrame);
    propLayout->addWidget(new QLabel("<b>Table Properties</b>"));
    
    m_idEdit = new QLineEdit(this);
    connect(m_idEdit, &QLineEdit::textChanged, this, &QslDesigner::updateActiveItemProperties);
    propLayout->addWidget(new QLabel("ID / Text:"));
    propLayout->addWidget(m_idEdit);

    m_widthSpin = new QSpinBox(this); m_widthSpin->setRange(10, 2000);
    m_heightSpin = new QSpinBox(this); m_heightSpin->setRange(10, 2000);
    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &QslDesigner::updateActiveItemProperties);
    connect(m_heightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &QslDesigner::updateActiveItemProperties);
    propLayout->addWidget(new QLabel("Width px:")); propLayout->addWidget(m_widthSpin);
    propLayout->addWidget(new QLabel("Height px:")); propLayout->addWidget(m_heightSpin);

    m_fontCombo = new QComboBox(this);
    QFontDatabase fontDb;
    m_fontCombo->addItems(fontDb.families());
    connect(m_fontCombo, &QComboBox::currentTextChanged, this, &QslDesigner::updateActiveItemProperties);
    propLayout->addWidget(new QLabel("Font:"));
    propLayout->addWidget(m_fontCombo);

    m_sizeSpin = new QSpinBox(this);
    m_sizeSpin->setRange(6, 144);
    m_sizeSpin->setValue(12);
    connect(m_sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &QslDesigner::updateActiveItemProperties);
    propLayout->addWidget(new QLabel("Size pt:"));
    propLayout->addWidget(m_sizeSpin);

    m_colorBtn = new QPushButton("🎨 Change Color", this);
    m_activeColor = Qt::black;
    connect(m_colorBtn, &QPushButton::clicked, this, [this]() {
        QFont f; f.setFamily(m_fontCombo->currentText()); f.setPointSize(m_sizeSpin->value());
        StyleDialog dlg(f, m_activeColor, this);
        if (dlg.exec() == QDialog::Accepted) {
            m_activeColor = dlg.selectedColor();
            m_fontCombo->setCurrentText(dlg.selectedFont().family());
            m_sizeSpin->setValue(dlg.selectedFont().pointSize());
            updateActiveItemProperties();
        }
    });
    propLayout->addWidget(m_colorBtn);

    m_rotSpin = new QSpinBox(this);
    m_rotSpin->setRange(-360, 360);
    m_rotSpin->setValue(0);
    connect(m_rotSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &QslDesigner::updateActiveItemProperties);
    propLayout->addWidget(new QLabel("Rotation:"));
    propLayout->addWidget(m_rotSpin);

    // Template handling
    QFrame *tmFrame = new QFrame(this);
    tmFrame->setFixedSize(208, 480);
    tmFrame->setFrameShape(QFrame::StyledPanel);
    QVBoxLayout *tmLayout = new QVBoxLayout(tmFrame);
    
    QVBoxLayout *listsLayout = new QVBoxLayout();
    listsLayout->setSpacing(5);
    listsLayout->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *cardsLayout = new QVBoxLayout();
    cardsLayout->setSpacing(2);
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->addWidget(new QLabel("<b>Cards</b>"));
    m_cardTemplateList = new QListWidget(this);
    m_cardTemplateList->setFixedSize(205, 185);
    cardsLayout->addWidget(m_cardTemplateList);
    
    QVBoxLayout *envsLayout = new QVBoxLayout();
    envsLayout->setSpacing(2);
    envsLayout->setContentsMargins(0, 0, 0, 0);
    envsLayout->addWidget(new QLabel("<b>Envelopes</b>"));
    m_envTemplateList = new QListWidget(this);
    m_envTemplateList->setFixedSize(205, 185);
    envsLayout->addWidget(m_envTemplateList);
    
    // Diagnostics Inspector button moved to left side under envelope template pane
    QPushButton *btnInspector = new QPushButton("🔬 Diagnostics Inspector", this);
    btnInspector->setStyleSheet("QPushButton { background-color: #8b5cf6; color: white; border: none; border-radius: 4px; padding: 6px 12px; font-weight: bold; } QPushButton:hover { background-color: #7c3aed; }");
    connect(btnInspector, &QPushButton::clicked, this, [this]() {
        QslTemplateInspector dlg(m_currentTemplateName, this, this);
        dlg.exec();
    });
    envsLayout->addWidget(btnInspector);
    
    listsLayout->addLayout(cardsLayout);
    listsLayout->addLayout(envsLayout);
    
    m_cardTemplateList->setMouseTracking(true);
    m_envTemplateList->setMouseTracking(true);

    QToolButton *btnNew = new QToolButton(this);
    btnNew->setText("📄 New (Clear All)");
    btnNew->setIcon(createBtnIcon("new"));
    btnNew->setIconSize(QSize(48, 48));
    btnNew->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    connect(btnNew, &QToolButton::clicked, this, [this]() {
        if (m_isModified) {
            auto ret = QMessageBox::question(this, "Unsaved Changes", "You have unsaved changes. Do you want to save them before creating a new template?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            if (ret == QMessageBox::Cancel) return;
            if (ret == QMessageBox::Yes) {
                saveCurrentTemplate();
                if (m_isModified) return; // Cancelled or failed save
            }
        }
        if (QMessageBox::question(this, "Confirm New", "Clear all areas and start a new layout?") == QMessageBox::Yes) {
            QDialog typeDlg(this);
            typeDlg.setWindowTitle("Select Template Type");
            typeDlg.setMinimumWidth(320);
            typeDlg.setStyleSheet(
                "QDialog {"
                "  background-color: #1e222b;"
                "  border: 1px solid #3f4452;"
                "  border-radius: 8px;"
                "}"
                "QLabel {"
                "  color: #f1f5f9;"
                "  font-family: 'Segoe UI', Arial, sans-serif;"
                "  font-size: 11pt;"
                "  font-weight: bold;"
                "  padding: 10px;"
                "}"
                "QPushButton {"
                "  background-color: #3b82f6;"
                "  color: white;"
                "  border: none;"
                "  border-radius: 4px;"
                "  padding: 8px 16px;"
                "  font-weight: bold;"
                "  font-size: 10pt;"
                "  min-width: 100px;"
                "}"
                "QPushButton:hover {"
                "  background-color: #2563eb;"
                "}"
                "QPushButton#btnCancel {"
                "  background-color: #4b5563;"
                "}"
                "QPushButton#btnCancel:hover {"
                "  background-color: #374151;"
                "}"
            );

            QPropertyAnimation *anim = new QPropertyAnimation(&typeDlg, "windowOpacity");
            anim->setDuration(200);
            anim->setStartValue(0.0);
            anim->setEndValue(1.0);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            typeDlg.setWindowOpacity(0.0);
            anim->start(QAbstractAnimation::DeleteWhenStopped);

            QVBoxLayout *vl = new QVBoxLayout(&typeDlg);
            QLabel *lbl = new QLabel(&typeDlg);
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setTextInteractionFlags(Qt::TextBrowserInteraction);
            lbl->setOpenExternalLinks(false);
            lbl->setText("Is This Going To Be..<br><br>"
                         "<span style='font-size: 12pt;'>"
                         "<a href=\"card\" style=\"color: #3b82f6; text-decoration: underline; font-weight: bold;\">A Card</a>"
                         "&nbsp;&nbsp;&nbsp;or&nbsp;&nbsp;&nbsp;"
                         "<a href=\"envelope\" style=\"color: #10b981; text-decoration: underline; font-weight: bold;\">An Envelope</a>?"
                         "</span>");
            vl->addWidget(lbl);
            vl->addSpacing(15);

            int result = 0; // 1 = Card, 2 = Envelope, 0 = Cancel
            connect(lbl, &QLabel::linkActivated, [&typeDlg, &result](const QString &link) {
                if (link == "card") {
                    result = 1;
                } else if (link == "envelope") {
                    result = 2;
                }
                typeDlg.accept();
            });

            QPushButton *btnCancel = new QPushButton("Cancel", &typeDlg);
            btnCancel->setObjectName("btnCancel");
            vl->addWidget(btnCancel);

            connect(btnCancel, &QPushButton::clicked, &typeDlg, &QDialog::reject);

            if (typeDlg.exec() == QDialog::Accepted && result != 0) {
                m_isEnvelope = (result == 2);

                if (changePaperSize()) {
                    QList<QGraphicsItem*> items = m_scene->items();
                    for (QGraphicsItem *item : items) {
                        if (dynamic_cast<PrintTableArea*>(item)) {
                            m_scene->removeItem(item);
                            delete item;
                        }
                    }
                    if (m_bgImageItem) {
                        m_scene->removeItem(m_bgImageItem);
                        delete m_bgImageItem;
                        m_bgImageItem = nullptr;
                    }
                    m_bgImagePath.clear();
                    
                    m_templateActive = true;
                    m_currentTemplateName.clear();
                    m_cardTemplateList->clearSelection();
                    m_envTemplateList->clearSelection();
                    drawBackgroundRulers();
                    updateBlockList();
                    m_isModified = false;

                    QMessageBox msgBox(this);
                    msgBox.setWindowTitle("Qwain Cartesian Calibration Scheme");
                    msgBox.setText(
                        "Due to the incredible variations in Printer Manufacturers, and Drivers and Operating System, "
                        "even within .jpg, .png, or other image files orientation is difficult or impossible at best, "
                        "to help i developed a Qwain Cartesian Calibration Scheme.\n\n"
                        "Please Place Your First Reference Point at the Upper Leftmost Corner of your template, "
                        "Now place the Second Reference Point at the Lower Rightmost Corner. After 2 placements, continue."
                    );
                    msgBox.setIcon(QMessageBox::Information);
                    msgBox.setStyleSheet(
                        "QMessageBox {"
                        "  background-color: #1e222b;"
                        "  border: 1px solid #3f4452;"
                        "  border-radius: 8px;"
                        "}"
                        "QLabel {"
                        "  color: #f1f5f9;"
                        "  font-family: 'Segoe UI', Arial, sans-serif;"
                        "  font-size: 11pt;"
                        "  padding: 10px;"
                        "}"
                        "QPushButton {"
                        "  background-color: #3b82f6;"
                        "  color: white;"
                        "  border: none;"
                        "  border-radius: 4px;"
                        "  padding: 8px 16px;"
                        "  font-weight: bold;"
                        "  font-size: 10pt;"
                        "  min-width: 100px;"
                        "}"
                        "QPushButton:hover {"
                        "  background-color: #2563eb;"
                        "}"
                    );
                    msgBox.exec();

                    showCalibrationMessage(this, "Calibration Instruction", "Please Place Your First Reference Point at the Upper Leftmost Corner of your template.");

                    m_addMode = "REF_POINT";
                    m_addArg = "REF_POINT_1";
                    QApplication::setOverrideCursor(Qt::CrossCursor);
                }
            }
        }
    });

    // Handle exclusive selection
    connect(m_cardTemplateList, &QListWidget::itemSelectionChanged, this, [this](){
        if(m_cardTemplateList->selectedItems().count() > 0) {
            m_envTemplateList->clearSelection();
            m_envTemplateList->setCurrentItem(nullptr);
        }
    });
    connect(m_envTemplateList, &QListWidget::itemSelectionChanged, this, [this](){
        if(m_envTemplateList->selectedItems().count() > 0) {
            m_cardTemplateList->clearSelection();
            m_cardTemplateList->setCurrentItem(nullptr);
        }
    });

    QToolButton *btnSave = new QToolButton(this);
    btnSave->setText("💾 Save");
    btnSave->setIcon(createBtnIcon("save"));
    btnSave->setIconSize(QSize(48, 48));
    btnSave->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnSave, &QToolButton::clicked, this, &QslDesigner::saveCurrentTemplate);
    
    QToolButton *btnUndo = new QToolButton(this);
    btnUndo->setText("↩️ Undo");
    btnUndo->setIcon(createBtnIcon("undo"));
    btnUndo->setIconSize(QSize(48, 48));
    btnUndo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnUndo, &QToolButton::clicked, this, [this]() {
        if (!m_templateActive) {
            QMessageBox::warning(this, "No Template", "No Template.  Select One or New");
            return;
        }
        if (!m_undoStack.isEmpty()) {
            QVariantMap state = m_undoStack.takeLast();
            restoreState(state);
        }
    });

    connect(m_cardTemplateList, &QListWidget::itemDoubleClicked, this, &QslDesigner::loadSelectedTemplate);
    connect(m_envTemplateList, &QListWidget::itemDoubleClicked, this, &QslDesigner::loadSelectedTemplate);

    QToolButton *btnRename = new QToolButton(this);
    btnRename->setText("🏷️ Rename");
    btnRename->setIcon(createBtnIcon("rename"));
    btnRename->setIconSize(QSize(48, 48));
    btnRename->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnRename, &QToolButton::clicked, this, &QslDesigner::renameTemplate);

    QToolButton *btnRename2 = new QToolButton(this);
    btnRename2->setText("🏷️ Rename");
    btnRename2->setIcon(createBtnIcon("rename"));
    btnRename2->setIconSize(QSize(48, 48));
    btnRename2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnRename2, &QToolButton::clicked, this, &QslDesigner::renameTemplate);

    QToolButton *btnDelete = new QToolButton(this);
    btnDelete->setText("💣 Delete");
    btnDelete->setIcon(createBtnIcon("delete"));
    btnDelete->setIconSize(QSize(48, 48));
    btnDelete->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnDelete, &QToolButton::clicked, this, &QslDesigner::deleteTemplate);

    QToolButton *btnDuplicate = new QToolButton(this);
    btnDuplicate->setText("👯 Duplicate");
    btnDuplicate->setIcon(createBtnIcon("duplicate"));
    btnDuplicate->setIconSize(QSize(48, 48));
    btnDuplicate->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnDuplicate, &QToolButton::clicked, this, &QslDesigner::duplicateTemplate);
    
    tmLayout->addLayout(listsLayout);

    QToolButton *btnDefineFields = new QToolButton(this);
    btnDefineFields->setText("📋 Define Fields");
    btnDefineFields->setIcon(createBtnIcon("fields"));
    btnDefineFields->setIconSize(QSize(48, 48));
    btnDefineFields->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnDefineFields, &QToolButton::clicked, this, [this]() {
        bool ok;
        QString txt = QInputDialog::getText(this, "Define Fields", "Enter comma separated fields for the QSO table:", QLineEdit::Normal, "day, month, year, utc, MHz/Band, Mode, Watts, RST sent, RST rcvd, t", &ok);
        if (ok && !txt.isEmpty()) {
            QSettings settings(QCoreApplication::organizationName(), "Qwain");
            settings.setValue("QSOFields", txt);
        }
    });

    QToolButton *btnPrinterSettings = new QToolButton(this);
    btnPrinterSettings->setText("📄 Print Direction");
    btnPrinterSettings->setIcon(createBtnIcon("printer"));
    btnPrinterSettings->setIconSize(QSize(48, 48));
    btnPrinterSettings->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnPrinterSettings, &QToolButton::clicked, this, [this]() {
        QDialog d(this);
        d.setWindowTitle("Print Direction");
        QVBoxLayout *vl = new QVBoxLayout(&d);
        
        QLabel *lblArrow = new QLabel("⬆", &d);
        lblArrow->setAlignment(Qt::AlignCenter);
        QFont arrowFont = lblArrow->font();
        arrowFont.setPixelSize(48); // roughly 1" depending on DPI
        lblArrow->setFont(arrowFont);
        vl->addWidget(lblArrow);
        
        QLabel *lblThumb = new QLabel(&d);
        lblThumb->setAlignment(Qt::AlignCenter);
        lblThumb->setMinimumSize(400, 400);
        vl->addWidget(lblThumb);
        
        QPushButton *btnCorrection = new QPushButton(&d);
        vl->addWidget(btnCorrection);
        
        QGridLayout *offsetGrid = new QGridLayout();
        QPushButton *btnUp = new QPushButton("↑", &d);
        QPushButton *btnDown = new QPushButton("↓", &d);
        QPushButton *btnLeft = new QPushButton("←", &d);
        QPushButton *btnRight = new QPushButton("→", &d);
        QLabel *lblOffset = new QLabel("X: 0.00\"  Y: 0.00\"", &d);
        lblOffset->setAlignment(Qt::AlignCenter);
        
        offsetGrid->addWidget(btnUp, 0, 1);
        offsetGrid->addWidget(btnLeft, 1, 0);
        offsetGrid->addWidget(lblOffset, 1, 1);
        offsetGrid->addWidget(btnRight, 1, 2);
        offsetGrid->addWidget(btnDown, 2, 1);
        vl->addLayout(offsetGrid);
        
        int tempCorr = m_printCorrection;
        double tempOffsetX = m_printOffsetX;
        double tempOffsetY = m_printOffsetY;
        
        auto updateUi = [&]() {
            btnCorrection->setText(QString("Abs Correction: %1°").arg(tempCorr));
            lblOffset->setText(QString("X: %1\"  Y: %2\"").arg(tempOffsetX, 0, 'f', 2).arg(tempOffsetY, 0, 'f', 2));
            
            // thumbnail
            int w = m_paperW;
            int h = m_paperH;
            QPixmap pm(w, h);
            pm.fill(Qt::white);
            QPainter p(&pm);
            m_scene->render(&p, QRectF(0, 0, w, h), QRectF(0, 0, w, h));
            p.end();
            QTransform trans;
            
            double rot = 180.0 + tempCorr;
            while (rot >= 360.0) rot -= 360.0;
            while (rot < 0.0) rot += 360.0;
            trans.rotate(rot);
            
            QPixmap rotated = pm.transformed(trans, Qt::SmoothTransformation);
            QPixmap scaled = rotated.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            lblThumb->setPixmap(scaled);
        };
        
        connect(btnUp, &QPushButton::clicked, [&]() { tempOffsetY -= 0.01; updateUi(); });
        connect(btnDown, &QPushButton::clicked, [&]() { tempOffsetY += 0.01; updateUi(); });
        connect(btnLeft, &QPushButton::clicked, [&]() { tempOffsetX -= 0.01; updateUi(); });
        connect(btnRight, &QPushButton::clicked, [&]() { tempOffsetX += 0.01; updateUi(); });
        
        connect(btnCorrection, &QPushButton::clicked, [&]() {
            if (tempCorr == -180) tempCorr = -90;
            else if (tempCorr == -90) tempCorr = 0;
            else if (tempCorr == 0) tempCorr = 90;
            else if (tempCorr == 90) tempCorr = 180;
            else if (tempCorr == 180) tempCorr = -180;
            updateUi();
        });
        
        updateUi();
        
        QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &d);
        connect(bbox, &QDialogButtonBox::accepted, &d, &QDialog::accept);
        connect(bbox, &QDialogButtonBox::rejected, &d, &QDialog::reject);
        vl->addWidget(bbox);
        
        if (d.exec() == QDialog::Accepted) {
            m_printCorrection = tempCorr;
            m_printOffsetX = tempOffsetX;
            m_printOffsetY = tempOffsetY;
            m_isModified = true;
        }
    });

    QToolButton *btnExportTemplate = new QToolButton(this);
    btnExportTemplate->setText("📤 Export Template");
    btnExportTemplate->setIcon(createBtnIcon("duplicate"));
    btnExportTemplate->setIconSize(QSize(48, 48));
    btnExportTemplate->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnExportTemplate, &QToolButton::clicked, this, [this]() { QslTemplateIO::exportTemplate(this, this); });

    QToolButton *btnImportTemplate = new QToolButton(this);
    btnImportTemplate->setText("📤 Import Template");
    btnImportTemplate->setIcon(createBtnIcon("load"));
    btnImportTemplate->setIconSize(QSize(48, 48));
    btnImportTemplate->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    connect(btnImportTemplate, &QToolButton::clicked, this, [this]() { QslTemplateIO::importTemplate(this, this); });

    m_buttonConfigMode = 0;
    QPushButton *btnToggleConfig = new QPushButton("NEW?", this);
    auto updateButtonTexts = [=]() {
        QList<QToolButton*> keptBtns = {
            btnNew, btnSave, btnDelete, btnRename2, btnDuplicate,
            btnAdd, btnBg, btnAddQso, btnAddCheck, btnAddCircle,
            btnAddAddress, btnAddSr, btnRm, btnPrinterSettings, btnUndo,
            btnChangeSize, btnAdvLayer, btnQrCode, btnExportTemplate, btnImportTemplate,
            btnTestView
        };
        for (auto btn : keptBtns) {
            btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            btn->setIconSize(QSize(12, 12));
            btn->setFixedSize(77, 44);
            btn->setStyleSheet("QToolButton { font-size: 7pt; padding: 0px; margin: 0px; }");
        }
        btnNew->setText("New");
        btnNew->setIcon(createBtnIcon("new", "edit-clear-all"));
        
        btnSave->setText("Save");
        btnSave->setIcon(createBtnIcon("save", "document-save"));
        
        btnDelete->setText("Delete");
        btnDelete->setIcon(createBtnIcon("delete", "edit-delete"));
        
        btnRename2->setText("Rename");
        btnRename2->setIcon(createBtnIcon("rename", "system-file-manager"));
        
        btnDuplicate->setText("Duplicate");
        btnDuplicate->setIcon(createBtnIcon("duplicate", "edit-copy"));
        
        btnAdd->setText("Add\nText");
        btnAdd->setIcon(createBtnIcon("add", "insert-text"));
        
        btnBg->setText("Add\nImage");
        btnBg->setIcon(createBtnIcon("bgimage", "insert-image"));
        
        btnAddQso->setText("Add\nQSO");
        btnAddQso->setIcon(createBtnIcon("fields", "emblem-symbolic-link"));
        
        btnAddCheck->setText("Add\nCheck");
        btnAddCheck->setIcon(createBtnIcon("check", "emblem-favorite"));

        btnAddCircle->setText("Add\nCircle");
        btnAddCircle->setIcon(createBtnIcon("circle"));

        btnAddAddress->setText("Add\nAddress");
        btnAddAddress->setIcon(createBtnIcon("load", "mail-unread"));
        
        btnAddSr->setText("Add\nSR");
        btnAddSr->setIcon(createBtnIcon("configure", "window-restore"));
        
        btnRm->setText("Delete\nSelected");
        btnRm->setIcon(createBtnIcon("remove", "edit-cut"));
        
        btnPrinterSettings->setText("Tray\nPath");
        btnPrinterSettings->setIcon(createBtnIcon("printer", "help-contents"));
        
        btnUndo->setText("Undo");
        btnUndo->setIcon(createBtnIcon("undo", "edit-undo"));
 
        btnChangeSize->setText("Change\nSize");
        btnChangeSize->setIcon(createBtnIcon("configure", "configure"));
 
        if (m_hasCopiedLayers) {
            btnAdvLayer->setText("Paste\n Layers");
            btnAdvLayer->setIcon(createBtnIcon("add", "edit-paste"));
        } else {
            btnAdvLayer->setText("Copy\n Layers");
            btnAdvLayer->setIcon(createBtnIcon("duplicate", "edit-copy"));
        }
 
        btnExportTemplate->setText("Export\nTemplate");
        btnExportTemplate->setIcon(createBtnIcon("duplicate", "document-send"));

        btnImportTemplate->setText("Import\nTemplate");
        btnImportTemplate->setIcon(createBtnIcon("load", "document-open"));
        btnQrCode->setText("QR Code");
        btnQrCode->setIcon(createBtnIcon("qr", "insert-image"));

        if (m_isTestView) {
            btnTestView->setText("Design\nView");
            btnTestView->setIcon(createBtnIcon("configure", "view-preview"));
        } else {
            btnTestView->setText("Test View");
            btnTestView->setIcon(createBtnIcon("fields", "view-preview"));
        }
    };
    updateButtonTexts();
    btnToggleConfig->hide();

    // Hide discarded buttons
    btnSize->hide();
    btnAddCustomChk->hide();
    btnAddRefPoint->hide();
    btnDefineFields->hide();
    btnToggleBg->hide();
    btnRename->hide();

    // Hide XY labels
    m_cursorLabel->hide();
    m_unitCombo->hide();

    // Left controls setup
    leftCtrl->addWidget(tmFrame);
    leftCtrl->addStretch();
    
    QWidget *leftPanelWidget = new QWidget(this);
    leftPanelWidget->setLayout(leftCtrl);
    leftPanelWidget->setFixedSize(210, 560);

    // Center view setup
    m_view->setFixedSize(764, 560);

    // Right buttons panel layout (Centered Vertically and Horizontally)
    QWidget *rightButtonsWidget = new QWidget(this);
    rightButtonsWidget->setFixedSize(176, 560);
    
    QVBoxLayout *rightPanelLayout = new QVBoxLayout(rightButtonsWidget);
    rightPanelLayout->setContentsMargins(4, 4, 4, 4);
    rightPanelLayout->setSpacing(6);
    
    rightPanelLayout->addStretch(); // Top stretch to center vertically
    
    QGridLayout *btnGrid = new QGridLayout();
    btnGrid->setAlignment(Qt::AlignCenter);
    btnGrid->setSpacing(4);
    
    // Row 0: New, Save
    btnGrid->addWidget(btnNew, 0, 0);
    btnGrid->addWidget(btnSave, 0, 1);
    
    // Row 1: Rename, Duplicate
    btnGrid->addWidget(btnRename2, 1, 0);
    btnGrid->addWidget(btnDuplicate, 1, 1);
    
    // Row 2: Delete, Undo
    btnGrid->addWidget(btnDelete, 2, 0);
    btnGrid->addWidget(btnUndo, 2, 1);
    
    // Row 3: Export Template, Import Template (above the line)
    btnGrid->addWidget(btnExportTemplate, 3, 0);
    btnGrid->addWidget(btnImportTemplate, 3, 1);

    // Row 4: thin line spanning all columns
    QFrame *line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("QFrame { background-color: #cccccc; max-height: 1px; border: none; }");
    line->setFixedWidth(130);
    btnGrid->addWidget(line, 4, 0, 1, 2, Qt::AlignCenter);
    
    // Row 5: Add Text, Add Image
    btnGrid->addWidget(btnAdd, 5, 0);
    btnGrid->addWidget(btnBg, 5, 1);
    
    // Row 6: Add QSO Block, Add Check
    btnGrid->addWidget(btnAddQso, 6, 0);
    btnGrid->addWidget(btnAddCheck, 6, 1);
    
    // Row 7: Add Circle, Add Address
    btnGrid->addWidget(btnAddCircle, 7, 0);
    btnGrid->addWidget(btnAddAddress, 7, 1);
    
    // Row 8: Add SR, Delete Selected (Rm)
    btnGrid->addWidget(btnAddSr, 8, 0);
    btnGrid->addWidget(btnRm, 8, 1);
    
    // Row 9: Tray Path (PrinterSettings), Change Size
    btnGrid->addWidget(btnPrinterSettings, 9, 0);
    btnGrid->addWidget(btnChangeSize, 9, 1);
    
    // Row 10: Copy/Paste Layers (AdvLayer) and Test View side-by-side
    btnGrid->addWidget(btnAdvLayer, 10, 0);
    btnGrid->addWidget(btnTestView, 10, 1);

    // Row 11: QR Code button directly below Copy Layers (Row 10, Col 0)
    btnGrid->addWidget(btnQrCode, 11, 0);
    
    rightPanelLayout->addLayout(btnGrid);
    
    rightPanelLayout->addStretch(); // Bottom stretch to center vertically

    // Assembly of four-column layout in topLayout
    QWidget *telemetryWidget = new QWidget(this);
    telemetryWidget->setFixedSize(250, 560);
    QVBoxLayout *telemetryLayout = new QVBoxLayout(telemetryWidget);
    telemetryLayout->setContentsMargins(4, 4, 4, 4);
    telemetryLayout->setSpacing(6);

    QLabel *telemetryTitle = new QLabel("<b>DESIGNER TELEMETRY</b>", telemetryWidget);
    telemetryTitle->setStyleSheet("QLabel { color: #3b82f6; font-family: 'Segoe UI', Arial, sans-serif; font-size: 10pt; font-weight: bold; margin-bottom: 2px; }");
    telemetryTitle->setAlignment(Qt::AlignCenter);
    telemetryLayout->addWidget(telemetryTitle);

    m_telemetryBrowser = new QTextBrowser(telemetryWidget);
    m_telemetryBrowser->setStyleSheet(
        "QTextBrowser {"
        "  background-color: #111318;"
        "  color: #38bdf8;"
        "  border: 1px solid #2d313f;"
        "  border-radius: 6px;"
        "  font-family: 'JetBrains Mono', 'Fira Code', 'Courier New', monospace;"
        "  font-size: 8pt;"
        "  padding: 6px;"
        "}"
    );
    m_telemetryBrowser->setReadOnly(true);
    telemetryLayout->addWidget(m_telemetryBrowser);

    QLabel *copyNotice = new QLabel("Press <b>W</b> to copy telemetry to clipboard", telemetryWidget);
    copyNotice->setStyleSheet("QLabel { color: #64748b; font-family: 'Segoe UI', Arial, sans-serif; font-size: 7.5pt; }");
    copyNotice->setAlignment(Qt::AlignCenter);
    telemetryLayout->addWidget(copyNotice);

    QHBoxLayout *mainHorizontalLayout = new QHBoxLayout();
    mainHorizontalLayout->setContentsMargins(0, 0, 0, 0);
    mainHorizontalLayout->setSpacing(10);
    mainHorizontalLayout->addWidget(leftPanelWidget);
    mainHorizontalLayout->addWidget(m_view);
    mainHorizontalLayout->addWidget(rightButtonsWidget);
    // Hide the designer telemetry panel to restore standard layout
    telemetryWidget->hide();
    // mainHorizontalLayout->addWidget(telemetryWidget);
    
    topLayout->addLayout(mainHorizontalLayout, 1);
    
    mainLayout->addLayout(topLayout, 1);

    // In-situ appearance customizer toggle: corner checkbox placed in lowest right corner
    // (naked checkbox without bracketed text, defaulting to unchecked false on window open)
    QHBoxLayout *bottomCornerLayout = new QHBoxLayout();
    bottomCornerLayout->setContentsMargins(4, 0, 8, 4);
    bottomCornerLayout->addStretch(1);
    QCheckBox *cornerSchemeToggle = WindowSchemeManager::createCornerToggle(this, this);
    bottomCornerLayout->addWidget(cornerSchemeToggle, 0, Qt::AlignRight | Qt::AlignVCenter);
    mainLayout->addLayout(bottomCornerLayout);
    
    // We do not show the properties panel or the blocks window to save space
    propFrame->hide();
    m_blocksWindow = nullptr;
    m_blockList = new QTableWidget(); // create but do not show to avoid null pointer issues
    
    connect(m_blockList, &QTableWidget::cellChanged, this, [this](int row, int col) {
        QList<QGraphicsItem*> sel = m_scene->selectedItems();
        if (sel.isEmpty()) return;
        PrintTableArea *area = dynamic_cast<PrintTableArea*>(sel.first());
        if (area) {
            int idx = row * area->m_cols + col;
            if (idx < area->m_cellData.size() && m_blockList->item(row, col)) {
                area->m_cellData[idx] = m_blockList->item(row, col)->text();
                area->update();
                m_isModified = true;
            }
        }
    });

    QShortcut *ctrlA = new QShortcut(QKeySequence("Ctrl+A"), this);
    connect(ctrlA, &QShortcut::activated, this, [this]() {
        QDialog *d = new QDialog(this);
        d->setWindowTitle("All Template Settings");
        QVBoxLayout *vl = new QVBoxLayout(d);
        QScrollArea *scroll = new QScrollArea();
        QWidget *w = new QWidget();
        QFormLayout *form = new QFormLayout(w);
        scroll->setWidget(w);
        scroll->setWidgetResizable(true);
        vl->addWidget(scroll);
        
        auto addField = [this, form](const QString &name, const QString &val, std::function<void(const QString&)> onChange) {
            QLineEdit *le = new QLineEdit(val);
            form->addRow(name, le);
            connect(le, &QLineEdit::textEdited, onChange);
        };
        
        QPushButton *btnCopy = new QPushButton("📋 Copy to Clipboard", w);
        form->addRow("", btnCopy);
        connect(btnCopy, &QPushButton::clicked, [form]() {
            QString info;
            for (int i = 0; i < form->rowCount(); ++i) {
                QLayoutItem *labelItem = form->itemAt(i, QFormLayout::LabelRole);
                QLayoutItem *fieldItem = form->itemAt(i, QFormLayout::FieldRole);
                if (labelItem && fieldItem) {
                    QLabel *lbl = qobject_cast<QLabel*>(labelItem->widget());
                    QString textVal;
                    if (QLineEdit *le = qobject_cast<QLineEdit*>(fieldItem->widget())) textVal = le->text();
                    else if (QComboBox *cb = qobject_cast<QComboBox*>(fieldItem->widget())) textVal = cb->currentText();
                    if (lbl && !lbl->text().isEmpty()) info += lbl->text() + " " + textVal + "\n";
                }
            }
            QApplication::clipboard()->setText(info);
        });

        addField("paperW", QString::number(m_paperW), [this](const QString &v) {
            m_paperW = v.toDouble(); drawBackgroundRulers(); m_isModified = true;
        });
        addField("paperH", QString::number(m_paperH), [this](const QString &v) {
            m_paperH = v.toDouble(); drawBackgroundRulers(); m_isModified = true;
        });
        
        addField("isEnvelope", m_isEnvelope ? "true" : "false", [this](const QString &v) {
            m_isEnvelope = (v == "true" || v == "1"); m_isModified = true;
        });
        addField("printCorrection", QString::number(m_printCorrection), [this](const QString &v) {
            m_printCorrection = v.toInt(); m_isModified = true;
        });
        addField("printOffsetX", QString::number(m_printOffsetX), [this](const QString &v) {
            m_printOffsetX = v.toDouble(); m_isModified = true;
        });
        addField("printOffsetY", QString::number(m_printOffsetY), [this](const QString &v) {
            m_printOffsetY = v.toDouble(); m_isModified = true;
        });
        addField("bgImagePath", m_bgImagePath, [this](const QString &v) {
            m_bgImagePath = v; setBackgroundImage(v); m_isModified = true;
        });
        
        QComboBox *printerCombo = new QComboBox(w);
        printerCombo->setEditable(true);
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        QStringList knownPrinters = settings.value("KnownPrinters").toStringList();
        for (const QString &kp : knownPrinters) {
            QString dest = kp.split(" ").first().trimmed();
            QString display = dest;
            if (dest.startsWith("ipp://") || dest.startsWith("ipps://")) {
                display += " [Connect via IPP]";
            } else if (dest.startsWith("http://") || dest.startsWith("https://")) {
                display += " [Connect via HTTP]";
            } else if (dest.count('.') == 3) {
                display += " [Connect via IP]";
            } else {
                display += " [Connect via CUPS]";
            }
            printerCombo->addItem(display, dest);
        }
        if (printerCombo->findData("Print to PDF") < 0) {
            printerCombo->addItem("Print to PDF", "Print to PDF");
        }
        int piIdx = printerCombo->findData(m_printerName);
        if (piIdx >= 0) printerCombo->setCurrentIndex(piIdx);
        else printerCombo->setCurrentText(m_printerName);
        form->addRow("printerName", printerCombo);
        connect(printerCombo, &QComboBox::currentTextChanged, [this](const QString &v) {
            m_printerName = v; m_isModified = true;
        });

        int i = 0;
        for (QGraphicsItem *gi : m_scene->items()) {
            PrintTableArea *area = dynamic_cast<PrintTableArea*>(gi);
            if (area) {
                QString prefix = QString("item_%1_").arg(i);
                addField(prefix + "id", area->areaId, [area, this](const QString &v) { area->areaId = v; m_isModified = true; area->update(); });
                addField(prefix + "x", QString::number(area->scenePos().x()), [area, this](const QString &v) { area->setPos(v.toDouble(), area->scenePos().y()); m_isModified = true; });
                addField(prefix + "y", QString::number(area->scenePos().y()), [area, this](const QString &v) { area->setPos(area->scenePos().x(), v.toDouble()); m_isModified = true; });
                addField(prefix + "text", area->textContent, [area, this](const QString &v) { area->textContent = v; m_isModified = true; area->update(); });
                
                addField(prefix + "font", area->m_font.toString(), [area, this](const QString &v) {
                    area->m_font.fromString(v); m_isModified = true; area->update();
                });
                addField(prefix + "color", area->m_color.name(QColor::HexArgb), [area, this](const QString &v) {
                    QColor c(v); if (c.isValid()) area->m_color = c; m_isModified = true; area->update();
                });
                addField(prefix + "rot", QString::number(area->rotation()), [area, this](const QString &v) {
                    area->setRotation(v.toDouble()); m_isModified = true; area->update();
                });
                if (!area->m_cellData.isEmpty()) addField(prefix + "cellData", area->m_cellData.join("|"), [area, this](const QString &v) {
                    area->m_cellData = v.split("|"); m_isModified = true; area->update();
                });
                
                if (!area->m_colWidthsStr.isEmpty()) {
                    addField(prefix + "colWidths", area->m_colWidthsStr, [area, this](const QString &v) {
                        area->m_colWidthsStr = v; m_isModified = true; area->update();
                    });
                }
                if (!area->m_rowHeightsStr.isEmpty()) {
                    addField(prefix + "rowHeights", area->m_rowHeightsStr, [area, this](const QString &v) {
                        area->m_rowHeightsStr = v; m_isModified = true; area->update();
                    });
                }
                
                i++;
            }
        }
        
        d->resize(500, 700);
        d->setAttribute(Qt::WA_DeleteOnClose);
        d->show();
    });

    updateTemplateList();

    // Apply previously saved custom appearance overrides from Qwain.conf [WindowStyles/Card_Designer]
    WindowSchemeManager::applySavedStyles(this);
}

QslDesigner::~QslDesigner() {
    m_isDestroying = true;
    if (m_scene) {
        m_scene->disconnect(this);
    }
}

void QslDesigner::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    if (m_scene && m_view && event->size() != event->oldSize()) {
        QTimer::singleShot(10, this, [this]() {
            if (m_scene && m_view && !m_scene->sceneRect().isEmpty()) {
                m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
            }
        });
    }
}

void QslDesigner::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);
    if (m_scene && m_view) {
        QTimer::singleShot(10, this, [this]() {
            if (m_scene && m_view && !m_scene->sceneRect().isEmpty()) {
                m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
            }
        });
    }
}

void QslDesigner::closeEvent(QCloseEvent *event) {
    reject();
    if (m_isModified) {
        event->ignore();
    } else {
        event->accept();
    }
}

void QslDesigner::reject() {
    if (m_isModified) {
        auto ret = QMessageBox::question(this, "Unsaved Changes", "You have unsaved changes. Do you want to save the template?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel) {
            return;
        }
        if (ret == QMessageBox::Yes) {
            saveCurrentTemplate();
            if (m_isModified) {
                return; // Cancelled or failed save
            }
        } else if (ret == QMessageBox::No) {
            m_isModified = false;
        }
    }
    if (m_blocksWindow) m_blocksWindow->close();
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.setValue("QslDesigner/geometry", saveGeometry());
    QDialog::reject();
}

void QslDesigner::drawBackgroundRulers() {
    for (QGraphicsItem *item : m_scene->items()) {
        if (!dynamic_cast<PrintTableArea*>(item) && item != m_crosshairH && item != m_crosshairV && item != m_bgImageItem) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    
    if (!m_templateActive) {
        for (QGraphicsItem *item : m_scene->items()) {
            if (dynamic_cast<PrintTableArea*>(item)) {
                item->setVisible(false);
            }
        }
        if (m_crosshairH) m_crosshairH->setVisible(false);
        if (m_crosshairV) m_crosshairV->setVisible(false);
        if (m_bgImageItem) m_bgImageItem->setVisible(false);
        m_scene->setSceneRect(m_view->rect());
        return;
    } else {
        for (QGraphicsItem *item : m_scene->items()) {
            if (dynamic_cast<PrintTableArea*>(item)) {
                item->setVisible(true);
            }
        }
        if (m_crosshairH) m_crosshairH->setVisible(true);
        if (m_crosshairV) m_crosshairV->setVisible(true);
        if (m_bgImageItem) m_bgImageItem->setVisible(true);
    }
    
    QRectF envelopeRect(0, 0, m_paperW, m_paperH);
    PrintTableArea *ref1 = nullptr, *ref2 = nullptr;
    for (QGraphicsItem *item : m_scene->items()) {
        if (PrintTableArea *a = dynamic_cast<PrintTableArea*>(item)) {
            if (a->areaId.startsWith("REF_POINT_")) {
                if (!ref1) ref1 = a;
                else if (!ref2) ref2 = a;
            }
        }
    }
    if (!m_isEnvelope && ref1 && ref2) {
        QPointF p1 = ref1->mapToScene(ref1->rect().center());
        QPointF p2 = ref2->mapToScene(ref2->rect().center());
        double minX = qMin(p1.x(), p2.x());
        double minY = qMin(p1.y(), p2.y());
        double maxX = qMax(p1.x(), p2.x());
        double maxY = qMax(p1.y(), p2.y());
        envelopeRect = QRectF(minX, minY, maxX - minX, maxY - minY);
    }
    
    // draw paper outline
    QGraphicsRectItem *paper = m_scene->addRect(envelopeRect, QPen(Qt::blue, 1, Qt::DashLine), QBrush(m_bgImageItem ? Qt::transparent : Qt::white));
    paper->setZValue(-2);
    
    if (m_bgImageItem) {
        m_bgImageItem->setZValue(-1);
    }

    // draw rulers (every inch = 96 px)
    QPen rulerPen(Qt::lightGray, 1, Qt::DashLine);
    for (int x = 0; x <= m_paperW; x += 96) {
        m_scene->addLine(x, 0, x, m_paperH, rulerPen)->setZValue(-9);
        QGraphicsTextItem *txt = m_scene->addText(QString::number(x/96) + "in");
        txt->setPos(x, -20);
        txt->setDefaultTextColor(Qt::darkGray);
    }
    for (int y = 0; y <= m_paperH; y += 96) {
        m_scene->addLine(0, y, m_paperW, y, rulerPen)->setZValue(-9);
        QGraphicsTextItem *txt = m_scene->addText(QString::number(y/96) + "in");
        txt->setPos(-30, y);
        txt->setDefaultTextColor(Qt::darkGray);
    }
    m_scene->setSceneRect(-50, -50, m_paperW + 100, m_paperH + 100);
}

void QslDesigner::onMouseMoved(QPointF scenePos) {
    m_lastMouseScenePos = scenePos;
    m_crosshairH->setLine(-1000, scenePos.y(), 2000, scenePos.y());
    m_crosshairV->setLine(scenePos.x(), -1000, scenePos.x(), 2000);
    double valX = qAbs(scenePos.x()) / 96.0;
    double valY = qAbs(scenePos.y()) / 96.0;
    QString unit = m_unitCombo->currentText();
    if (unit == "cm") {
        valX *= 2.54;
        valY *= 2.54;
    }
    m_cursorLabel->setText(QString("<b>X: %1 %3 &nbsp;&nbsp; Y: %2 %3</b>").arg(valX, 0, 'f', 2).arg(valY, 0, 'f', 2).arg(unit));
    updateTelemetry(scenePos);
}

void QslDesigner::updateTelemetry(QPointF mousePos) {
    if (m_isDestroying || !m_scene || !m_telemetryBrowser) return;

    if (mousePos.isNull()) {
        mousePos = m_lastMouseScenePos;
    }

    QString html = "<html><body style='font-family:\"JetBrains Mono\",\"Fira Code\",monospace; font-size:10px; color:#38bdf8;'>";

    // 1. Template metrics
    html += "<b style='color:#f43f5e;'>[TEMPLATE METRICS]</b><br/>";
    html += "Name: <b>" + (m_currentTemplateName.isEmpty() ? "Untitled" : m_currentTemplateName) + "</b><br/>";
    html += "Type: <b>" + QString(m_isEnvelope ? "Envelope" : "Card") + "</b><br/>";
    html += QString("Dimensions: <b>%1 x %2 px</b><br/>").arg(m_paperW, 0, 'f', 1).arg(m_paperH, 0, 'f', 1);
    html += QString("Inches: <b>%1 x %2 in</b><br/>").arg(m_paperW / 96.0, 0, 'f', 2).arg(m_paperH / 96.0, 0, 'f', 2);
    html += QString("Metric: <b>%1 x %2 cm</b><br/>").arg((m_paperW / 96.0) * 2.54, 0, 'f', 2).arg((m_paperH / 96.0) * 2.54, 0, 'f', 2);
    html += QString("Offset: <b>X: %1, Y: %2</b><br/>").arg(m_printOffsetX, 0, 'f', 2).arg(m_printOffsetY, 0, 'f', 2);
    html += "Background: <b>" + (m_bgImagePath.isEmpty() ? "None" : QFileInfo(m_bgImagePath).fileName()) + "</b><br/>";
    html += "<hr style='border:0; border-top:1px solid #2d313f;'/>";

    // 2. Cursor coordinates
    html += "<b style='color:#10b981;'>[CURSOR POSITION]</b><br/>";
    html += QString("Scene Px: <b>X: %1, Y: %2</b><br/>").arg(mousePos.x(), 0, 'f', 1).arg(mousePos.y(), 0, 'f', 1);
    html += QString("Inches: <b>X: %1, Y: %2 in</b><br/>").arg(mousePos.x() / 96.0, 0, 'f', 3).arg(mousePos.y() / 96.0, 0, 'f', 3);
    html += QString("Metric: <b>X: %1, Y: %2 cm</b><br/>").arg((mousePos.x() / 96.0) * 2.54, 0, 'f', 3).arg((mousePos.y() / 96.0) * 2.54, 0, 'f', 3);
    html += "<hr style='border:0; border-top:1px solid #2d313f;'/>";

    // 3. Selected item details
    html += "<b style='color:#3b82f6;'>[ACTIVE ELEMENT]</b><br/>";
    QList<QGraphicsItem*> sel = m_scene->selectedItems();
    if (!sel.isEmpty()) {
        PrintTableArea *area = dynamic_cast<PrintTableArea*>(sel.first());
        if (area) {
            html += "ID: <b style='color:#ffffff;'>" + area->areaId + "</b><br/>";
            html += QString("Pos: <b>X: %1, Y: %2 px</b><br/>").arg(area->pos().x(), 0, 'f', 1).arg(area->pos().y(), 0, 'f', 1);
            html += QString("Bounds: <b>W: %1, H: %2 px</b><br/>").arg(area->rect().width(), 0, 'f', 1).arg(area->rect().height(), 0, 'f', 1);
            html += QString("Inches: <b>W: %1, H: %2 in</b><br/>").arg(area->rect().width() / 96.0, 0, 'f', 2).arg(area->rect().height() / 96.0, 0, 'f', 2);
            html += QString("Rotation: <b>%1&deg;</b><br/>").arg(area->rotation(), 0, 'f', 1);
            html += "Z-Value: <b>" + QString::number(area->zValue()) + "</b><br/>";
            
            // Type identification
            QString areaType = "Standard Text";
            if (area->m_isImage) {
                areaType = "Layered Image";
            } else if (area->areaId.startsWith("CHK_")) {
                areaType = "Checkmark";
            } else if (area->areaId.startsWith("CIRC_")) {
                areaType = "Circle";
            } else if (area->m_rows > 1 || area->m_cols > 1) {
                areaType = QString("QSO Grid (%1x%2)").arg(area->m_rows).arg(area->m_cols);
            }
            html += "Type: <b style='color:#eab308;'>" + areaType + "</b><br/>";

            // Font info
            if (!area->m_isImage && !area->areaId.startsWith("CHK_") && !area->areaId.startsWith("CIRC_")) {
                html += QString("Font: <b>%1, %2pt</b><br/>").arg(area->m_font.family()).arg(area->m_font.pointSize());
                html += QString("Color: <b>#%1%2%3</b><br/>")
                    .arg(area->m_color.red(), 2, 16, QChar('0'))
                    .arg(area->m_color.green(), 2, 16, QChar('0'))
                    .arg(area->m_color.blue(), 2, 16, QChar('0'));
            }

            // Bezier info
            html += "Bezier Curved: <b style='color:#a855f7;'>" + QString(area->m_useBezierPath ? "ENABLED" : "Disabled") + "</b><br/>";
            if (area->m_useBezierPath) {
                html += QString("&bull; P0 Norm: <b>(%1, %2)</b><br/>").arg(area->m_p0_norm.x(), 0, 'f', 2).arg(area->m_p0_norm.y(), 0, 'f', 2);
                html += QString("&bull; P1 Norm: <b>(%1, %2)</b><br/>").arg(area->m_p1_norm.x(), 0, 'f', 2).arg(area->m_p1_norm.y(), 0, 'f', 2);
                html += QString("&bull; P2 Norm: <b>(%1, %2)</b><br/>").arg(area->m_p2_norm.x(), 0, 'f', 2).arg(area->m_p2_norm.y(), 0, 'f', 2);
                html += QString("&bull; P3 Norm: <b>(%1, %2)</b><br/>").arg(area->m_p3_norm.x(), 0, 'f', 2).arg(area->m_p3_norm.y(), 0, 'f', 2);
                
                QPointF p0 = area->getP0();
                QPointF p1 = area->getP1();
                QPointF p2 = area->getP2();
                QPointF p3 = area->getP3();
                html += QString("&bull; P0 Abs: <b>(%1, %2)</b><br/>").arg(p0.x(), 0, 'f', 1).arg(p0.y(), 0, 'f', 1);
                html += QString("&bull; P1 Abs: <b>(%1, %2)</b><br/>").arg(p1.x(), 0, 'f', 1).arg(p1.y(), 0, 'f', 1);
                html += QString("&bull; P2 Abs: <b>(%1, %2)</b><br/>").arg(p2.x(), 0, 'f', 1).arg(p2.y(), 0, 'f', 1);
                html += QString("&bull; P3 Abs: <b>(%1, %2)</b><br/>").arg(p3.x(), 0, 'f', 1).arg(p3.y(), 0, 'f', 1);
            }

            // Interaction state
            if (area->m_isResizing) {
                html += QString("State: <b style='color:#f43f5e;'>RESIZING (Edge %1)</b><br/>").arg(area->m_resizeEdge);
            } else if (area->m_draggingColDivider != -1) {
                html += QString("State: <b style='color:#f43f5e;'>DRAGGING COL %1</b><br/>").arg(area->m_draggingColDivider);
            } else if (area->m_draggingRowDivider != -1) {
                html += QString("State: <b style='color:#f43f5e;'>DRAGGING ROW %1</b><br/>").arg(area->m_draggingRowDivider);
            } else if (area->m_draggingControlPoint != -1) {
                html += QString("State: <b style='color:#f43f5e;'>DRAGGING BEZIER PT %1</b><br/>").arg(area->m_draggingControlPoint);
            } else {
                html += "State: <b>Selected</b><br/>";
            }
        } else {
            html += "Type: <b>Non-design element</b><br/>";
        }
    } else {
        html += "<span style='color:#64748b;'>No element selected</span><br/>";
    }
    html += "<hr style='border:0; border-top:1px solid #2d313f;'/>";

    // 4. All elements on screen
    html += "<b style='color:#eab308;'>[SCENE INVENTORY]</b><br/>";
    QList<QGraphicsItem*> allItems = m_scene->items();
    int count = 0;
    for (QGraphicsItem *it : allItems) {
        PrintTableArea *area = dynamic_cast<PrintTableArea*>(it);
        if (area) {
            count++;
            QString bounds = QString("(%1,%2) %3x%4")
                .arg(area->pos().x(), 0, 'f', 0)
                .arg(area->pos().y(), 0, 'f', 0)
                .arg(area->rect().width(), 0, 'f', 0)
                .arg(area->rect().height(), 0, 'f', 0);
            html += QString("%1. <b>%2</b> <span style='color:#64748b; font-size:9px;'>%3</span><br/>")
                .arg(count)
                .arg(area->areaId)
                .arg(bounds);
        }
    }
    if (count == 0) {
        html += "<span style='color:#64748b;'>Empty canvas</span><br/>";
    }
    html += "</body></html>";
    m_telemetryBrowser->setHtml(html);
}

#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QClipboard>
#include <QApplication>
#include <QFileInfo>
#include <QToolTip>
#include <QPlainTextEdit>

class TelemetryDialog : public QDialog {
public:
    TelemetryDialog(QWidget *parent, QslDesigner *designer, QPointF mousePos) 
        : QDialog(parent), m_designer(designer), m_mousePos(mousePos) {
        setWindowTitle("Designer Telemetry - Feedback");
        resize(550, 450);
        
        // Apply sophisticated theme styling
        setStyleSheet(
            "QDialog { background-color: #1a1d24; color: #f1f5f9; }"
            "QTabWidget::pane { border: 1px solid #2d313f; background-color: #111318; border-radius: 6px; }"
            "QTabBar::tab { background-color: #1a1d24; color: #94a3b8; border: 1px solid #2d313f; border-bottom: none; padding: 8px 16px; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
            "QTabBar::tab:selected { background-color: #111318; color: #f59e0b; font-weight: bold; border-bottom: 2px solid #f59e0b; }"
            "QLabel { color: #cbd5e1; font-size: 10pt; }"
            "QTableWidget { background-color: #111318; color: #f1f5f9; border: none; gridline-color: #2d313f; }"
            "QHeaderView::section { background-color: #1a1d24; color: #f59e0b; font-weight: bold; border: 1px solid #2d313f; padding: 4px; }"
            "QPushButton { background-color: #2c3e50; color: white; border: 1px solid #34495e; border-radius: 4px; padding: 6px 12px; font-weight: bold; }"
            "QPushButton:hover { background-color: #34495e; }"
        );

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(12, 12, 12, 12);
        mainLayout->setSpacing(10);

        QLabel *hdr = new QLabel("<b>DESIGNER TELEMETRY FEEDBACK</b>", this);
        hdr->setStyleSheet("color: #3b82f6; font-size: 11pt; font-weight: bold;");
        hdr->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(hdr);

        QTabWidget *tabWidget = new QTabWidget(this);
        mainLayout->addWidget(tabWidget);

        auto addRow = [](QTableWidget *t, const QString &prop, const QString &val) {
            int r = t->rowCount();
            t->insertRow(r);
            QTableWidgetItem *item0 = new QTableWidgetItem(prop);
            QTableWidgetItem *item1 = new QTableWidgetItem(val);
            item0->setFlags(item0->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
            item1->setFlags(item1->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
            item0->setForeground(QBrush(QColor("#94a3b8")));
            item1->setForeground(QBrush(QColor("#f1f5f9")));
            t->setItem(r, 0, item0);
            t->setItem(r, 1, item1);
        };

        // --- Tab 1: Template & Canvas ---
        QWidget *t1 = new QWidget(this);
        QVBoxLayout *l1 = new QVBoxLayout(t1);
        QTableWidget *tbl1 = new QTableWidget(t1);
        tbl1->setColumnCount(2);
        tbl1->setHorizontalHeaderLabels(QStringList() << "Property" << "Value");
        tbl1->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tbl1->verticalHeader()->setVisible(false);
        tbl1->setShowGrid(false);

        addRow(tbl1, "Template Name", m_designer->m_currentTemplateName.isEmpty() ? "Untitled" : m_designer->m_currentTemplateName);
        addRow(tbl1, "Template Type", m_designer->m_isEnvelope ? "Envelope" : "Card");
        addRow(tbl1, "Dimensions (Pixels)", QString("%1 x %2 px").arg(m_designer->m_paperW, 0, 'f', 1).arg(m_designer->m_paperH, 0, 'f', 1));
        addRow(tbl1, "Dimensions (Inches)", QString("%1 x %2 in").arg(m_designer->m_paperW / 96.0, 0, 'f', 2).arg(m_designer->m_paperH / 96.0, 0, 'f', 2));
        addRow(tbl1, "Dimensions (Metric)", QString("%1 x %2 cm").arg((m_designer->m_paperW / 96.0) * 2.54, 0, 'f', 2).arg((m_designer->m_paperH / 96.0) * 2.54, 0, 'f', 2));
        addRow(tbl1, "Print Offsets", QString("X: %1, Y: %2").arg(m_designer->m_printOffsetX, 0, 'f', 2).arg(m_designer->m_printOffsetY, 0, 'f', 2));
        addRow(tbl1, "Background Image", m_designer->m_bgImagePath.isEmpty() ? "None" : QFileInfo(m_designer->m_bgImagePath).fileName());
        
        l1->addWidget(tbl1);
        tabWidget->addTab(t1, "Template Metrics");

        // --- Tab 2: Cursor Tracking ---
        QWidget *t2 = new QWidget(this);
        QVBoxLayout *l2 = new QVBoxLayout(t2);
        QTableWidget *tbl2 = new QTableWidget(t2);
        tbl2->setColumnCount(2);
        tbl2->setHorizontalHeaderLabels(QStringList() << "Coordinate System" << "Position");
        tbl2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tbl2->verticalHeader()->setVisible(false);
        tbl2->setShowGrid(false);

        addRow(tbl2, "Scene Pixels", QString("X: %1, Y: %2 px").arg(m_mousePos.x(), 0, 'f', 1).arg(m_mousePos.y(), 0, 'f', 1));
        addRow(tbl2, "Inches", QString("X: %1, Y: %2 in").arg(m_mousePos.x() / 96.0, 0, 'f', 3).arg(m_mousePos.y() / 96.0, 0, 'f', 3));
        addRow(tbl2, "Metric", QString("X: %1, Y: %2 cm").arg((m_mousePos.x() / 96.0) * 2.54, 0, 'f', 3).arg((m_mousePos.y() / 96.0) * 2.54, 0, 'f', 3));
        l2->addWidget(tbl2);
        tabWidget->addTab(t2, "Cursor Tracking");

        // --- Tab 3: Selected Active Element ---
        QWidget *t3 = new QWidget(this);
        QVBoxLayout *l3 = new QVBoxLayout(t3);
        QList<QGraphicsItem*> sel = m_designer->m_scene->selectedItems();
        if (!sel.isEmpty()) {
            PrintTableArea *area = dynamic_cast<PrintTableArea*>(sel.first());
            if (area) {
                QTableWidget *tbl3 = new QTableWidget(t3);
                tbl3->setColumnCount(2);
                tbl3->setHorizontalHeaderLabels(QStringList() << "Attribute" << "Value");
                tbl3->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
                tbl3->verticalHeader()->setVisible(false);
                tbl3->setShowGrid(false);

                addRow(tbl3, "ID", area->areaId);
                
                QString areaType = "Standard Text";
                if (area->m_isImage) areaType = "Layered Image";
                else if (area->areaId.startsWith("CHK_")) areaType = "Checkmark";
                else if (area->areaId.startsWith("CIRC_")) areaType = "Circle";
                else if (area->m_rows > 1 || area->m_cols > 1) areaType = QString("QSO Grid (%1x%2)").arg(area->m_rows).arg(area->m_cols);
                addRow(tbl3, "Element Type", areaType);

                addRow(tbl3, "Pos (Pixels)", QString("X: %1, Y: %2 px").arg(area->pos().x(), 0, 'f', 1).arg(area->pos().y(), 0, 'f', 1));
                addRow(tbl3, "Size (Pixels)", QString("W: %1, H: %2 px").arg(area->rect().width(), 0, 'f', 1).arg(area->rect().height(), 0, 'f', 1));
                addRow(tbl3, "Size (Inches)", QString("W: %1, H: %2 in").arg(area->rect().width() / 96.0, 0, 'f', 2).arg(area->rect().height() / 96.0, 0, 'f', 2));
                addRow(tbl3, "Rotation", QString("%1 degree").arg(area->rotation(), 0, 'f', 1));
                addRow(tbl3, "Z-Value", QString::number(area->zValue()));

                if (!area->m_isImage && !area->areaId.startsWith("CHK_") && !area->areaId.startsWith("CIRC_")) {
                    addRow(tbl3, "Font", QString("%1, %2pt").arg(area->m_font.family()).arg(area->m_font.pointSize()));
                    addRow(tbl3, "Color", QString("#%1%2%3")
                        .arg(area->m_color.red(), 2, 16, QChar('0'))
                        .arg(area->m_color.green(), 2, 16, QChar('0'))
                        .arg(area->m_color.blue(), 2, 16, QChar('0')));
                }

                addRow(tbl3, "Bezier Curved Status", area->m_useBezierPath ? "ENABLED" : "Disabled");
                if (area->m_useBezierPath) {
                    addRow(tbl3, "Bezier P0 Norm", QString("(%1, %2)").arg(area->m_p0_norm.x(), 0, 'f', 2).arg(area->m_p0_norm.y(), 0, 'f', 2));
                    addRow(tbl3, "Bezier P1 Norm", QString("(%1, %2)").arg(area->m_p1_norm.x(), 0, 'f', 2).arg(area->m_p1_norm.y(), 0, 'f', 2));
                    addRow(tbl3, "Bezier P2 Norm", QString("(%1, %2)").arg(area->m_p2_norm.x(), 0, 'f', 2).arg(area->m_p2_norm.y(), 0, 'f', 2));
                    addRow(tbl3, "Bezier P3 Norm", QString("(%1, %2)").arg(area->m_p3_norm.x(), 0, 'f', 2).arg(area->m_p3_norm.y(), 0, 'f', 2));
                    
                    QPointF p0 = area->getP0();
                    QPointF p1 = area->getP1();
                    QPointF p2 = area->getP2();
                    QPointF p3 = area->getP3();
                    addRow(tbl3, "Bezier P0 Abs", QString("(%1, %2)").arg(p0.x(), 0, 'f', 1).arg(p0.y(), 0, 'f', 1));
                    addRow(tbl3, "Bezier P1 Abs", QString("(%1, %2)").arg(p1.x(), 0, 'f', 1).arg(p1.y(), 0, 'f', 1));
                    addRow(tbl3, "Bezier P2 Abs", QString("(%1, %2)").arg(p2.x(), 0, 'f', 1).arg(p2.y(), 0, 'f', 1));
                    addRow(tbl3, "Bezier P3 Abs", QString("(%1, %2)").arg(p3.x(), 0, 'f', 1).arg(p3.y(), 0, 'f', 1));
                }

                l3->addWidget(tbl3);
            } else {
                QLabel *lbl = new QLabel("Non-design element selected.", t3);
                lbl->setAlignment(Qt::AlignCenter);
                l3->addWidget(lbl);
            }
        } else {
            QLabel *lbl = new QLabel("No active element currently selected on the canvas.", t3);
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setStyleSheet("color: #64748b; font-style: italic;");
            l3->addWidget(lbl);
        }
        tabWidget->addTab(t3, "Active Element");

        // --- Tab 4: Scene Inventory ---
        QWidget *t4 = new QWidget(this);
        QVBoxLayout *l4 = new QVBoxLayout(t4);
        QTableWidget *tbl4 = new QTableWidget(t4);
        tbl4->setColumnCount(4);
        tbl4->setHorizontalHeaderLabels(QStringList() << "Index" << "Element ID" << "Position (X,Y)" << "Size (WxH)");
        tbl4->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        tbl4->verticalHeader()->setVisible(false);
        tbl4->setShowGrid(false);

        QList<QGraphicsItem*> allItems = m_designer->m_scene->items();
        int idx = 0;
        for (QGraphicsItem *it : allItems) {
            PrintTableArea *area = dynamic_cast<PrintTableArea*>(it);
            if (area) {
                tbl4->insertRow(idx);
                tbl4->setItem(idx, 0, new QTableWidgetItem(QString::number(idx + 1)));
                tbl4->setItem(idx, 1, new QTableWidgetItem(area->areaId));
                tbl4->setItem(idx, 2, new QTableWidgetItem(QString("(%1, %2)").arg(area->pos().x(), 0, 'f', 0).arg(area->pos().y(), 0, 'f', 0)));
                tbl4->setItem(idx, 3, new QTableWidgetItem(QString("%1 x %2").arg(area->rect().width(), 0, 'f', 0).arg(area->rect().height(), 0, 'f', 0)));
                for (int c = 0; c < 4; ++c) {
                    tbl4->item(idx, c)->setFlags(tbl4->item(idx, c)->flags() & ~Qt::ItemIsEditable & ~Qt::ItemIsSelectable);
                    tbl4->item(idx, c)->setForeground(QBrush(QColor("#f1f5f9")));
                }
                idx++;
            }
        }
        l4->addWidget(tbl4);
        tabWidget->addTab(t4, "Scene Inventory");

        // Buttons row
        QHBoxLayout *btnLayout = new QHBoxLayout();
        QPushButton *btnCopy = new QPushButton("Copy Raw Telemetry", this);
        QPushButton *btnClose = new QPushButton("Close", this);

        btnLayout->addWidget(btnCopy);
        btnLayout->addStretch();
        btnLayout->addWidget(btnClose);
        mainLayout->addLayout(btnLayout);

        connect(btnCopy, &QPushButton::clicked, this, [this]() {
            if (m_designer && m_designer->m_telemetryBrowser) {
                QString text = m_designer->m_telemetryBrowser->toPlainText();
                QApplication::clipboard()->setText(text);
                QToolTip::showText(QCursor::pos(), "Raw telemetry copied to clipboard!");
            }
        });

        connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    }

private:
    QslDesigner *m_designer;
    QPointF m_mousePos;
};

void QslDesigner::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape && !m_addMode.isEmpty()) {
        QApplication::restoreOverrideCursor();
        m_addMode.clear();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_W) {
        QWidget *focused = focusWidget();
        bool typingInWidget = false;
        if (focused) {
            if (qobject_cast<QLineEdit*>(focused) || 
                qobject_cast<QTextEdit*>(focused) || 
                qobject_cast<QPlainTextEdit*>(focused) || 
                qobject_cast<QTableWidget*>(focused)) {
                typingInWidget = true;
            }
        }
        if (!typingInWidget) {
            if (m_telemetryBrowser) {
                QString text = m_telemetryBrowser->toPlainText();
                QApplication::clipboard()->setText(text);
                QToolTip::showText(QCursor::pos(), "Telemetry copied to clipboard!");

                TelemetryDialog dlg(this, this, m_lastMouseScenePos);
                dlg.exec();
            }
            event->accept();
            return;
        }
    }
    QDialog::keyPressEvent(event);
}

#include "qslpreviewdialog.h"
#include <QDesktopServices>
#include <QUrl>
#include <QFileInfo>

void QslDesigner::duplicateTemplate() {
    if (m_currentTemplateName.isEmpty()) {
        QMessageBox::information(this, "Empty", "Please load or save a template to duplicate.");
        return;
    }
    bool ok;
    QString newName = QInputDialog::getText(this, "Duplicate Template", "New template name:", QLineEdit::Normal, m_currentTemplateName + "_Copy", &ok);
    if (!ok || newName.isEmpty()) return;
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");
    if (settings.childGroups().contains(newName)) {
        if (QMessageBox::question(this, "Overwrite", "Template already exists. Overwrite?") != QMessageBox::Yes) {
            settings.endGroup();
            return;
        }
    }
    
    saveTemplateWithName(newName);
}


void QslDesigner::testPrintWifi() {
    QVariantMap state;
    state["paperW"] = m_paperW;
    state["paperH"] = m_paperH;
    state["printerName"] = m_printerName;
    state["printCorrection"] = m_printCorrection;
    state["printOffsetX"] = m_printOffsetX;
    state["printOffsetY"] = m_printOffsetY;
    state["bgImagePath"] = m_bgImagePath;
    
    QVariantList items;
    for (QGraphicsItem *item : m_scene->items()) {
        PrintTableArea *area = dynamic_cast<PrintTableArea*>(item);
        if (area) {
            QVariantMap itemMap;
            itemMap["id"] = area->areaId;
            itemMap["text"] = area->textContent;
            itemMap["x"] = area->x();
            itemMap["y"] = area->y();
            itemMap["font"] = area->m_font.family();
            itemMap["size"] = area->m_font.pointSize();
            itemMap["color"] = area->m_color.name();
            itemMap["rot"] = area->rotation();
            itemMap["w"] = area->rect().width();
            itemMap["h"] = area->rect().height();
            itemMap["zValue"] = area->zValue();
            itemMap["maxLines"] = area->m_maxLines;
            itemMap["align"] = area->m_align;
            itemMap["isImage"] = area->m_isImage;
            itemMap["imagePath"] = area->m_imagePath;
            itemMap["rows"] = area->m_rows;
            itemMap["cols"] = area->m_cols;
            itemMap["rowHeights"] = area->m_rowHeightsStr;
            itemMap["colWidths"] = area->m_colWidthsStr;
            
            QStringList cells;
            for (int j=0; j<area->m_cellData.size(); ++j) {
                cells.append(area->m_cellData[j]);
            }
            itemMap["cellData"] = cells;
            items.append(itemMap);
        }
    }
    state["items"] = items;

    QsoDetail m1; m1.call="W4DJF"; m1.date="20260527"; m1.time="143000"; m1.freq="14.250"; m1.band="20m"; m1.mode="J3E";
    m1.name = "John Doe"; m1.address = "123 Main St"; m1.city = "Anytown, US"; m1.zip = "12345"; m1.country = "UNITED STATES";
    QsoDetail m2; m2.call="W4DJF"; m2.date="20260528"; m2.time="154500"; m2.freq="7.050"; m2.band="40m"; m2.mode="CW";  
    m2.name = "John Doe"; m2.address = "123 Main St"; m2.city = "Anytown, US"; m2.zip = "12345"; m2.country = "UNITED STATES";
    QsoDetail m3; m3.call="W4DJF"; m3.date="20260529"; m3.time="182000"; m3.freq="21.074"; m3.band="15m"; m3.mode="FT8";
    m3.name = "John Doe"; m3.address = "123 Main St"; m3.city = "Anytown, US"; m3.zip = "12345"; m3.country = "UNITED STATES";
    QList<QsoDetail> dummyQsos = {m1, m2, m3};

    QslPreviewDialog *dlg = new QslPreviewDialog("W4DJF", "Name: John Doe<br>Address: 123 Main St<br>City: Anytown, US 12345<br>Country: UNITED STATES<br>Mode: Mock Data Preview<br>Grid: EM73", dummyQsos, this);
    dlg->setPreviewState(state);
    dlg->showPreviewWindowOnly();
}

bool QslDesigner::changePaperSize() {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QStringList customSizes = settings.value("CustomPaperSizes").toStringList();
    
    QDialog d(this);
    d.setWindowTitle("Select Envelope / Card Size");
    d.setMinimumWidth(400);
    d.setMinimumHeight(400);
    d.setStyleSheet(
        "QDialog {"
        "  background-color: #1e222b;"
        "  border: 1px solid #3f4452;"
        "  border-radius: 8px;"
        "}"
        "QLabel {"
        "  color: #f1f5f9;"
        "  font-family: 'Segoe UI', Arial, sans-serif;"
        "  font-size: 10pt;"
        "  font-weight: bold;"
        "}"
        "QListWidget {"
        "  background-color: #111318;"
        "  color: #e2e8f0;"
        "  border: 1px solid #2d313f;"
        "  border-radius: 6px;"
        "  font-family: 'Segoe UI', Arial, sans-serif;"
        "  font-size: 10pt;"
        "  padding: 5px;"
        "}"
        "QListWidget::item {"
        "  padding: 6px;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #2e3440;"
        "  color: #ffffff;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #3b82f6;"
        "  color: #ffffff;"
        "}"
        "QDoubleSpinBox, QLineEdit {"
        "  background-color: #111318;"
        "  color: #e2e8f0;"
        "  border: 1px solid #2d313f;"
        "  border-radius: 4px;"
        "  padding: 4px;"
        "}"
        "QPushButton {"
        "  background-color: #3b82f6;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2563eb;"
        "}"
        "QPushButton:disabled {"
        "  background-color: #4b5563;"
        "  color: #9ca3af;"
        "}"
    );

    QPropertyAnimation *anim = new QPropertyAnimation(&d, "windowOpacity");
    anim->setDuration(200);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    d.setWindowOpacity(0.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    QVBoxLayout *vl = new QVBoxLayout(&d);
    
    QListWidget *lw = new QListWidget(&d);
    lw->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // scrollable w/o elevators
    lw->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    struct PaperSize { QString name; double w, h; bool isCustom; };
    QList<PaperSize> sizes = {
        {"US No 6 3/4 Envelope (6.5 x 3.625 in)", 6.5, 3.625, false},
        {"US No 9 Envelope (8.875 x 3.875 in)", 8.875, 3.875, false},
        {"US No 10 Envelope (9.5 x 4.125 in)", 9.5, 4.125, false},
        {"US A2 Envelope (5.75 x 4.375 in)", 5.75, 4.375, false},
        {"US A6 Envelope (6.5 x 4.75 in)", 6.5, 4.75, false},
        {"EU C4 Envelope (12.75 x 9.01 in)", 12.75, 9.01, false},
        {"EU C5 Envelope (9.01 x 6.38 in)", 9.01, 6.38, false},
        {"EU C6 Envelope (6.38 x 4.49 in)", 6.38, 4.49, false},
        {"EU DL Envelope (8.66 x 4.33 in)", 8.66, 4.33, false},
        {"Card A4 (11.69 x 8.27 in)", 11.69, 8.27, false},
        {"Card Letter (11.0 x 8.5 in)", 11.0, 8.5, false},
        {"Card Postcard (6.0 x 4.0 in)", 6.0, 4.0, false}
    };
    
    for (const QString &cs : customSizes) {
        QStringList parts = cs.split("|");
        if (parts.size() == 3) {
            sizes.append({parts[0] + " (Custom)", parts[1].toDouble(), parts[2].toDouble(), true});
        }
    }
    
    sizes.append({"Custom...", 0, 0, false});
    
    for (int i = 0; i < sizes.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(sizes[i].name, lw);
        item->setData(Qt::UserRole, QSizeF(sizes[i].w, sizes[i].h));
        item->setData(Qt::UserRole + 1, sizes[i].isCustom);
    }
    
    QPushButton *btnDeleteCustom = new QPushButton("Delete Custom Size", &d);
    btnDeleteCustom->setEnabled(false);
    
    vl->addWidget(new QLabel("Select Envelope/Card Size:"));
    vl->addWidget(lw);
    vl->addWidget(btnDeleteCustom);
    
    QWidget *customWidget = new QWidget(&d);
    QFormLayout *fl = new QFormLayout(customWidget);
    QDoubleSpinBox *wSpin = new QDoubleSpinBox(&d);
    wSpin->setRange(1.0, 50.0);
    wSpin->setDecimals(3);
    wSpin->setValue(m_paperW / 96.0);
    QDoubleSpinBox *hSpin = new QDoubleSpinBox(&d);
    hSpin->setRange(1.0, 50.0);
    hSpin->setDecimals(3);
    hSpin->setValue(m_paperH / 96.0);
    QLineEdit *nameEdit = new QLineEdit(&d);
    nameEdit->setPlaceholderText("Leave blank to not save in list");
    
    QHBoxLayout *spinLayout = new QHBoxLayout();
    spinLayout->addWidget(new QLabel("W:"));
    spinLayout->addWidget(wSpin);
    spinLayout->addWidget(new QLabel("H:"));
    spinLayout->addWidget(hSpin);
    
    fl->addRow("Dimensions (in):", spinLayout);
    fl->addRow("Save as Name:", nameEdit);
    vl->addWidget(customWidget);
    
    customWidget->setVisible(false);
    
    QObject::connect(lw, &QListWidget::currentRowChanged, [&customWidget, btnDeleteCustom, lw](int idx){
        if (idx < 0) return;
        QListWidgetItem *it = lw->item(idx);
        bool isCustomMode = (it->text() == "Custom...");
        customWidget->setVisible(isCustomMode);
        btnDeleteCustom->setEnabled(it->data(Qt::UserRole + 1).toBool());
    });
    
    QObject::connect(btnDeleteCustom, &QPushButton::clicked, [&](){
        int idx = lw->currentRow();
        if (idx < 0) return;
        QListWidgetItem *it = lw->item(idx);
        if (!it->data(Qt::UserRole + 1).toBool()) return;
        
        QString nm = it->text().replace(" (Custom)", "");
        for (int i = 0; i < customSizes.size(); ++i) {
            if (customSizes[i].startsWith(nm + "|")) {
                customSizes.removeAt(i);
                settings.setValue("CustomPaperSizes", customSizes);
                break;
            }
        }
        delete lw->takeItem(idx);
    });
    
    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &d);
    QObject::connect(box, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    QObject::connect(box, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    vl->addWidget(box);
    
    if (d.exec() == QDialog::Accepted) {
        QListWidgetItem *it = lw->currentItem();
        if (!it) return false;
        
        if (it->text() == "Custom...") {
            double w = wSpin->value();
            double h = hSpin->value();
            m_paperW = w * 96.0;
            m_paperH = h * 96.0;
            
            if (!nameEdit->text().trimmed().isEmpty()) {
                QString customName = nameEdit->text().trimmed();
                QString entry = customName + "|" + QString::number(w) + "|" + QString::number(h);
                customSizes.append(entry);
                settings.setValue("CustomPaperSizes", customSizes);
            }
        } else {
            QSizeF sz = it->data(Qt::UserRole).toSizeF();
            m_paperW = sz.width() * 96.0;
            m_paperH = sz.height() * 96.0;
        }
        m_isModified = true;
        drawBackgroundRulers();
        m_scene->update();
        if (m_scene && m_view) {
            m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
        }
        return true;
    }
    return false;
}

void QslDesigner::updateBlockList() {
    m_blockList->blockSignals(true);
    m_blockList->clear();
    
    QList<QGraphicsItem*> sel = m_scene->selectedItems();
    PrintTableArea *selectedArea = sel.isEmpty() ? nullptr : dynamic_cast<PrintTableArea*>(sel.first());
    
    if (selectedArea) {
        m_blockList->setRowCount(selectedArea->m_rows);
        m_blockList->setColumnCount(selectedArea->m_cols);
        m_blockList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
        m_blockList->setSelectionBehavior(QAbstractItemView::SelectItems);
        
        QStringList headers;
        for (int c=0; c<selectedArea->m_cols; ++c) headers << QString("Col %1").arg(c+1);
        m_blockList->setHorizontalHeaderLabels(headers);
        m_blockList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        
        for (int r = 0; r < selectedArea->m_rows; ++r) {
            for (int c = 0; c < selectedArea->m_cols; ++c) {
                int idx = r * selectedArea->m_cols + c;
                QString text = (idx < selectedArea->m_cellData.size()) ? selectedArea->m_cellData[idx] : "";
                QTableWidgetItem *item = new QTableWidgetItem(text);
                item->setTextAlignment(Qt::AlignCenter);
                m_blockList->setItem(r, c, item);
            }
        }
    } else {
        m_blockList->setRowCount(0);
        m_blockList->setColumnCount(4);
        m_blockList->setHorizontalHeaderLabels({"Area ID", "X (px)", "Y (px)", "Dimensions"});
        m_blockList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_blockList->setSelectionBehavior(QAbstractItemView::SelectRows);
        
        int row = 0;
        for (QGraphicsItem *item : m_scene->items()) {
            PrintTableArea *area = dynamic_cast<PrintTableArea*>(item);
            if (area) {
                m_blockList->insertRow(row);
                m_blockList->setItem(row, 0, new QTableWidgetItem(area->areaId));
                m_blockList->setItem(row, 1, new QTableWidgetItem(QString::number(area->x())));
                m_blockList->setItem(row, 2, new QTableWidgetItem(QString::number(area->y())));
                m_blockList->setItem(row, 3, new QTableWidgetItem(QString("%1 x %2").arg(area->rect().width()).arg(area->rect().height())));
                row++;
            }
        }
    }
    m_blockList->blockSignals(false);
}

void QslDesigner::setBackgroundImage(const QString &forcePath) {
    QString file = forcePath;
    if (file.isEmpty()) {
        ImageViewer dlg(true, this);
        if (dlg.exec() == QDialog::Accepted) {
            file = dlg.selectedImagePath();
        }
    }
    if (file == ":REMOVE_BG:") {
        if (m_bgImageItem) {
            m_scene->removeItem(m_bgImageItem);
            delete m_bgImageItem;
            m_bgImageItem = nullptr;
        }
        m_bgImagePath.clear();
        drawBackgroundRulers();
        m_isModified = true;
        return;
    }
    if (file.isEmpty()) return;

    if (!m_bgImageItem) {
        m_bgImageItem = m_scene->addPixmap(QPixmap());
        m_bgImageItem->setZValue(-1);
    }
    m_bgImagePath = file;
    QPixmap pm(file);
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain_EXIF");
    QString calibDims = settings.value(file + "_CalibDims", "").toString();
    
    m_bgImageItem->setPixmap(pm);
    m_bgImageItem->setPos(0, 0); // default top left

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
                
                m_bgImageItem->setPixmap(pm.scaled(targetW, targetH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                m_bgImageItem->setPos(m_paperW/2.0 - targetW/2.0, m_paperH/2.0 - targetH/2.0);
            }
        }
    } else {
        // Fit to paper by default and center
        QPixmap scaledPm = pm.scaled(m_paperW, m_paperH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_bgImageItem->setPixmap(scaledPm);
        m_bgImageItem->setPos(m_paperW/2.0 - scaledPm.width()/2.0, m_paperH/2.0 - scaledPm.height()/2.0);
    }
    m_isModified = true;
}

// ============================================================================
// FEATURE: Intelligent Non-Overlapping Initial Element Placement
// INTENDED FUNCTION:
//   When a new element (image, logo, badge, QR code) is added to the template,
//   place it visibly on the canvas in an open area that does not completely
//   overlap existing layers (such as the QSO table or existing graphics).
//   This ensures the new element is immediately visible, selectable, and
//   draggable without mouse clicks and hover events being intercepted by
//   underlying table resize handles.
// ============================================================================
QPointF QslDesigner::findInitialElementPos(double itemW, double itemH)
{
    const double margin = 20.0;
    const double maxX = qMax(margin, m_paperW - itemW - margin);
    const double maxY = qMax(margin, m_paperH - itemH - margin);

    // Collect scene bounding rects of all visible, non-background items
    QList<QRectF> occupiedRects;
    if (m_scene) {
        for (QGraphicsItem *item : m_scene->items()) {
            PrintTableArea *area = dynamic_cast<PrintTableArea*>(item);
            if (area && area->isVisible() && area->zValue() > -1.0) {
                occupiedRects.append(area->sceneBoundingRect());
            }
        }
    }

    // If canvas has no non-background items, place in standard upper-right corner
    if (occupiedRects.isEmpty()) {
        return QPointF(maxX, margin);
    }

    // Helper to calculate total overlapping area with existing elements
    auto calcOverlap = [&](const QRectF &candRect) -> double {
        double totalOverlap = 0.0;
        for (const QRectF &occ : occupiedRects) {
            QRectF inter = candRect.intersected(occ);
            if (!inter.isEmpty()) {
                totalOverlap += (inter.width() * inter.height());
            }
        }
        return totalOverlap;
    };

    // Candidate positions: prioritized by typical QSL card layout aesthetics
    // 1. Upper-right (traditional badge/crest location)
    // 2. Upper-left
    // 3. Lower-right
    // 4. Lower-left
    // 5. Right-center
    // 6. Top-center
    QList<QPointF> primaryCandidates;
    primaryCandidates << QPointF(maxX, margin)
                      << QPointF(margin, margin)
                      << QPointF(maxX, maxY)
                      << QPointF(margin, maxY)
                      << QPointF(maxX, qBound(margin, (m_paperH - itemH) / 2.0, maxY))
                      << QPointF(qBound(margin, (m_paperW - itemW) / 2.0, maxX), margin);

    for (const QPointF &pt : primaryCandidates) {
        QRectF r(pt.x(), pt.y(), itemW, itemH);
        if (calcOverlap(r) < 1.0) {
            return pt; // Completely clear spot found!
        }
    }

    // Grid search across visible bounds for zero or minimal overlap
    double bestOverlap = 1e12;
    QPointF bestPos = primaryCandidates.first();

    const double stepX = 25.0;
    const double stepY = 20.0;

    for (double y = margin; y <= maxY + 0.1; y += stepY) {
        // Scan right-to-left as the right side is most commonly open on QSL cards
        for (double x = maxX; x >= margin - 0.1; x -= stepX) {
            QRectF candRect(x, y, itemW, itemH);
            double overlap = calcOverlap(candRect);
            if (overlap < 1.0) {
                return QPointF(x, y); // Clean non-overlapping position
            }
            if (overlap < bestOverlap) {
                bestOverlap = overlap;
                bestPos = QPointF(x, y);
            }
        }
    }

    // Fallback: best minimal-overlap position found
    return bestPos;
}

bool QslDesigner::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_view->viewport() && event->type() == QEvent::MouseButtonPress) {
        if (!m_addMode.isEmpty()) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                pushUndo(); // Save before adding
                
                QPointF scenePos = m_view->mapToScene(me->pos());
                
                QString idStr;
                if (m_addMode == "TEXT") {
                    idStr = m_addArg;
                } else if (m_addMode == "CHK_CUSTOM") {
                    idStr = "CHK_CUSTOM";
                } else if (m_addMode == "CIRC_CUSTOM") {
                    idStr = "CIRC_CUSTOM";
                } else if (m_addMode == "REF_POINT") {
                    idStr = m_addArg;
                } else if (m_addMode == "STAMP_REMINDER") {
                    idStr = m_addArg;
                }
                
                if (idStr == "ADDRESS_BOX") {
                    PrintTableArea *area1 = new PrintTableArea("ADDRESS_BOX");
                    area1->m_maxLines = m_addFieldMaxLines;
                    area1->setRect(0, 0, 250, 90);
                    area1->setPos(scenePos.x() - area1->rect().width() / 2.0, scenePos.y() - area1->rect().height() / 2.0);
                    m_scene->addItem(area1);

                    PrintTableArea *area2 = new PrintTableArea("<COUNTRY>");
                    area2->m_maxLines = m_addFieldMaxLines;
                    area2->setRect(0, 0, 250, 30);
                    area2->setPos(area1->pos().x(), area1->pos().y() + area1->rect().height() + 5);
                    m_scene->addItem(area2);

                    m_scene->clearSelection();
                    area1->setSelected(true);
                    m_isModified = true;

                    QApplication::restoreOverrideCursor();
                    m_addMode.clear();
                    return true;
                }
                
                PrintTableArea *area = new PrintTableArea(idStr);
                
                if (m_addMode == "TEXT" && m_addType == 3) { // Image
                    ImageViewer dlg(true, this);
                    QString fp;
                    if (dlg.exec() == QDialog::Accepted) {
                        fp = dlg.selectedImagePath();
                    }
                    if (!fp.isEmpty()) {
                        area->m_isImage = true;
                        area->m_imagePath = fp;
                        area->m_pixmap.load(fp);
                        if (!area->m_pixmap.isNull()) {
                            area->setRect(0, 0, area->m_pixmap.width()/4.0, area->m_pixmap.height()/4.0); // scale it initially
                        }
                        
                        QDialog layerDlg(this);
                        layerDlg.setWindowTitle("Image Layering");
                        layerDlg.setMinimumWidth(360);
                        layerDlg.setStyleSheet(
                            "QDialog {"
                            "  background-color: #1e222b;"
                            "  border: 1px solid #3f4452;"
                            "  border-radius: 8px;"
                            "}"
                            "QLabel {"
                            "  color: #f1f5f9;"
                            "  font-family: 'Segoe UI', Arial, sans-serif;"
                            "  font-size: 10pt;"
                            "  font-weight: bold;"
                            "}"
                            "QPushButton {"
                            "  background-color: #3b82f6;"
                            "  color: white;"
                            "  border: none;"
                            "  border-radius: 4px;"
                            "  padding: 8px 16px;"
                            "  font-weight: bold;"
                            "  font-family: 'Segoe UI', Arial, sans-serif;"
                            "  font-size: 10pt;"
                            "}"
                            "QPushButton:hover {"
                            "  background-color: #2563eb;"
                            "}"
                            "QPushButton#btnBg {"
                            "  background-color: #10b981;"
                            "}"
                            "QPushButton#btnBg:hover {"
                            "  background-color: #059669;"
                            "}"
                        );

                        QPropertyAnimation *animLayer = new QPropertyAnimation(&layerDlg, "windowOpacity");
                        animLayer->setDuration(200);
                        animLayer->setStartValue(0.0);
                        animLayer->setEndValue(1.0);
                        animLayer->setEasingCurve(QEasingCurve::OutCubic);
                        layerDlg.setWindowOpacity(0.0);
                        animLayer->start(QAbstractAnimation::DeleteWhenStopped);

                        QVBoxLayout *vlLayer = new QVBoxLayout(&layerDlg);
                        
                        QLabel *lblLayer = new QLabel("Is This A Layered Image or Background?", &layerDlg);
                        lblLayer->setAlignment(Qt::AlignCenter);
                        
                        vlLayer->addWidget(lblLayer);
                        vlLayer->addSpacing(15);
                        
                        QHBoxLayout *hlLayer = new QHBoxLayout();
                        QPushButton *btnLayered = new QPushButton("Layered", &layerDlg);
                        QPushButton *btnBackground = new QPushButton("Background", &layerDlg);
                        btnBackground->setObjectName("btnBg");
                        
                        hlLayer->addWidget(btnLayered);
                        hlLayer->addWidget(btnBackground);
                        vlLayer->addLayout(hlLayer);

                        int layerResult = 1; // 1 = Layered, 2 = Background
                        connect(btnLayered, &QPushButton::clicked, [&layerDlg, &layerResult]() {
                            layerResult = 1;
                            layerDlg.accept();
                        });
                        connect(btnBackground, &QPushButton::clicked, [&layerDlg, &layerResult]() {
                            layerResult = 2;
                            layerDlg.accept();
                        });

                        if (layerDlg.exec() == QDialog::Accepted) {
                            if (layerResult == 1) {
                                area->setZValue(1.5); // Layered: above background and table for immediate click-ability
                            } else {
                                for (QGraphicsItem *item : m_scene->items()) {
                                    if (auto *otherArea = dynamic_cast<PrintTableArea*>(item)) {
                                        if (otherArea != area && otherArea->zValue() == -1.5 && otherArea->m_isImage) {
                                            m_scene->removeItem(otherArea);
                                            delete otherArea;
                                        }
                                    }
                                }
                                area->setZValue(-1.5); // Background: behind everything
                            }
                        } else {
                            area->setZValue(1.5);
                        }
                    } else {
                        delete area;
                        QApplication::restoreOverrideCursor();
                        m_addMode.clear();
                        return true;
                    }
                } else if (m_addMode == "TEXT") {
                    area->m_maxLines = m_addFieldMaxLines;
                    if (m_addType == 1) { // Table
                        area->m_rows = m_qsoTableRows;
                        area->m_cols = m_qsoTableCols;
                        area->m_cellData = m_qsoTableCellData;
                        area->m_showHeaders = m_qsoTableShowHeaders;
                        area->m_showBorders = m_qsoTableShowBorders;
                        area->m_borderThickness = m_qsoTableBorderThickness;
                        area->setRect(0, 0, qMax(100.0, m_qsoTableCols * 60.0), qMax(50.0, m_qsoTableRows * 25.0));
                    } else if (m_addType == 0) { // Text Box
                        QDialog textChoiceDlg(this);
                        textChoiceDlg.setWindowTitle("Add Text Element");
                        textChoiceDlg.setFixedWidth(340);
                        textChoiceDlg.setStyleSheet(
                            "QDialog { background-color: #1e222b; border: 1px solid #3f4452; border-radius: 8px; }"
                            "QLabel { color: #f1f5f9; font-family: 'Segoe UI', Arial; font-size: 10pt; font-weight: bold; }"
                            "QPushButton { background-color: #3b82f6; color: white; border: none; border-radius: 6px; padding: 8px 14px; font-size: 10pt; font-weight: bold; text-align: left; }"
                            "QPushButton:hover { background-color: #2563eb; }"
                        );

                        QVBoxLayout *vl = new QVBoxLayout(&textChoiceDlg);
                        vl->setSpacing(10);
                        vl->setContentsMargins(16, 16, 16, 16);

                        QLabel *lblMsg = new QLabel("Select text element type to place:", &textChoiceDlg);
                        vl->addWidget(lblMsg);

                        QPushButton *btnCallsign = new QPushButton("🏷️ Callsign Field (<call>)", &textChoiceDlg);
                        QPushButton *btnComment = new QPushButton("💬 Comment Box (<comment>)", &textChoiceDlg);
                        QPushButton *btnCustom = new QPushButton("✏️ Custom Text...", &textChoiceDlg);

                        vl->addWidget(btnCallsign);
                        vl->addWidget(btnComment);
                        vl->addWidget(btnCustom);

                        int selectedChoice = 0; // 1=Callsign, 2=Comment, 3=Custom
                        connect(btnCallsign, &QPushButton::clicked, [&textChoiceDlg, &selectedChoice]() {
                            selectedChoice = 1;
                            textChoiceDlg.accept();
                        });
                        connect(btnComment, &QPushButton::clicked, [&textChoiceDlg, &selectedChoice]() {
                            selectedChoice = 2;
                            textChoiceDlg.accept();
                        });
                        connect(btnCustom, &QPushButton::clicked, [&textChoiceDlg, &selectedChoice]() {
                            selectedChoice = 3;
                            textChoiceDlg.accept();
                        });

                        if (textChoiceDlg.exec() != QDialog::Accepted || selectedChoice == 0) {
                            delete area;
                            m_addMode.clear();
                            QApplication::restoreOverrideCursor();
                            return true;
                        }

                        if (selectedChoice == 1) {
                            area->textContent = "<call>";
                            area->areaId = "Call";
                        } else if (selectedChoice == 2) {
                            area->textContent = "CommentBox";
                            area->areaId = "Comment Box";
                        } else if (selectedChoice == 3) {
                            bool ok = false;
                            QString customText = QInputDialog::getText(this, "User Text",
                                "Enter User Text to display:", QLineEdit::Normal, "", &ok);
                            if (!ok || customText.trimmed().isEmpty()) {
                                delete area;
                                m_addMode.clear();
                                QApplication::restoreOverrideCursor();
                                return true;
                            }
                            area->textContent = customText.trimmed();
                            area->areaId = customText.trimmed();
                        }
                    }
                }

                // If it's a check mark or ref point, we want it small. For REF_POINT, make it a fixed small box like 20x20
                if (idStr.startsWith("REF_POINT") || idStr == "CIRC_CUSTOM") {
                    area->setRect(0, 0, 20, 20);
                } else if (idStr == "CHK_CUSTOM") {
                    area->setRect(0, 0, 5, 5);
                } else if (idStr == "STAMP_REMINDER") {
                    area->setRect(0, 0, 40, 40);
                } else if (idStr == "ADDRESS_BOX") {
                    area->setRect(0, 0, 250, 120);
                }
                
                // If it's a check mark, center it there. PrintTableArea is a QGraphicsRectItem.
                // Default rect is (0,0, 100, 50). We want the center to be scenePos.
                area->setPos(scenePos.x() - area->rect().width()/2.0, scenePos.y() - area->rect().height()/2.0);
                
                m_scene->addItem(area);
                m_scene->clearSelection();
                area->setSelected(true);
                m_isModified = true;
                
                QApplication::restoreOverrideCursor();

                if (idStr == "CHK_CUSTOM") {
                    // ========================================================================
                    // FEATURE: "Assign to:" Checkmark Selection Dialog
                    // INTENDED FUNCTION:
                    //   When placing a check element on the QSL canvas, prompt the user with
                    //   an "Assign to:" selection list containing all defined station checkmarks
                    //   (from QSettings "Station/Checks", with standard fallback defaults).
                    //   Any checkmarks already placed on the active template are displayed as
                    //   non-selectable and grayed out with an "(Already used in template)" badge
                    //   to prevent duplicate assignments. An inline input allows defining and
                    //   assigning a new checkmark immediately without leaving the dialog.
                    // ========================================================================
                    QSettings settings;
                    QStringList definedChecks = settings.value("Station/Checks").toStringList();
                    if (definedChecks.isEmpty()) {
                        definedChecks << "PSE" << "TNX" << "QSL" << "Direct" << "Bureau";
                    }

                    // Discover all checkmarks already placed in the active template
                    QSet<QString> usedChecks;
                    if (m_scene) {
                        for (QGraphicsItem *item : m_scene->items()) {
                            PrintTableArea *existingArea = dynamic_cast<PrintTableArea*>(item);
                            if (existingArea && existingArea != area && existingArea->areaId.startsWith("CHK_")) {
                                QString usedName = existingArea->areaId.mid(4).trimmed();
                                if (!usedName.isEmpty()) {
                                    usedChecks.insert(usedName);
                                }
                            }
                        }
                    }

                    // Ensure any checkmark already in the template also appears in the display list
                    QStringList allChecks = definedChecks;
                    for (const QString &u : usedChecks) {
                        bool found = false;
                        for (const QString &c : allChecks) {
                            if (c.compare(u, Qt::CaseInsensitive) == 0) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            allChecks.append(u);
                        }
                    }

                    QDialog dlg(this);
                    dlg.setWindowTitle("Assign Checkmark");
                    dlg.setFixedWidth(420);
                    dlg.setStyleSheet(
                        "QDialog { background-color: #1e222b; border: 1px solid #3f4452; border-radius: 8px; }"
                        "QLabel { color: #f1f5f9; font-family: 'Segoe UI', Arial; font-size: 10pt; }"
                        "QListWidget { background-color: #111318; color: #f1f5f9; border: 1px solid #2d313f; border-radius: 6px; padding: 4px; font-size: 10pt; outline: 0; }"
                        "QListWidget::item { padding: 6px 10px; border-radius: 4px; margin: 1px 0px; }"
                        "QListWidget::item:hover:enabled { background-color: #252b3b; color: #ffffff; }"
                        "QListWidget::item:selected:enabled { background-color: #2563eb; color: #ffffff; font-weight: bold; }"
                        "QListWidget::item:disabled { color: #64748b; background-color: #161922; }"
                        "QLineEdit { background-color: #111318; color: #f1f5f9; border: 1px solid #2d313f; border-radius: 4px; padding: 6px 10px; font-size: 10pt; }"
                        "QLineEdit:focus { border: 1px solid #3b82f6; }"
                        "QPushButton { background-color: #3b82f6; color: white; border: none; border-radius: 4px; padding: 6px 14px; font-weight: bold; }"
                        "QPushButton:hover { background-color: #2563eb; }"
                        "QPushButton:disabled { background-color: #374151; color: #9ca3af; }"
                        "QToolButton { background-color: #374151; color: #f3f4f6; border: 1px solid #4b5563; border-radius: 10px; font-weight: bold; }"
                    );

                    QVBoxLayout *vl = new QVBoxLayout(&dlg);
                    vl->setSpacing(10);
                    vl->setContentsMargins(16, 16, 16, 16);

                    // Header: "Assign to:" label with help tooltip button
                    QHBoxLayout *hlHeader = new QHBoxLayout();
                    hlHeader->setSpacing(6);
                    QLabel *lblTitle = new QLabel("Assign to:", &dlg);
                    lblTitle->setStyleSheet("font-size: 11pt; font-weight: bold; color: #f8fafc;");
                    QToolButton *btnHelp = new QToolButton(&dlg);
                    btnHelp->setText("?");
                    btnHelp->setFixedSize(20, 20);
                    btnHelp->setToolTip("Select a defined checkmark for this checkbox element.\nCheckmarks already placed on the template are disabled.");
                    connect(btnHelp, &QToolButton::clicked, this, [&dlg]() {
                        QMessageBox::information(&dlg, "Checkmark Assignment Help",
                            "Select which defined checkmark to assign to this checkbox element.\n\n"
                            "Checkmarks already used in the current template are shown as non-selectable to ensure unique identification during printing.");
                    });
                    hlHeader->addWidget(lblTitle);
                    hlHeader->addWidget(btnHelp);
                    hlHeader->addStretch();
                    vl->addLayout(hlHeader);

                    // Clickable list widget displaying defined and template checkmarks
                    QListWidget *listWidget = new QListWidget(&dlg);
                    listWidget->setMinimumHeight(180);
                    vl->addWidget(listWidget);

                    auto populateItem = [&](const QString &chkName, bool isAlreadyUsed) {
                        QListWidgetItem *item = new QListWidgetItem(listWidget);
                        item->setData(Qt::UserRole, chkName);
                        if (isAlreadyUsed) {
                            item->setText(chkName + "  (Already used in template)");
                            item->setFlags(Qt::NoItemFlags); // Non-selectable and disabled
                            item->setForeground(QBrush(QColor("#64748b")));
                            QFont f = item->font();
                            f.setItalic(true);
                            item->setFont(f);
                        } else {
                            item->setText(chkName);
                            item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                            item->setForeground(QBrush(QColor("#f8fafc")));
                        }
                    };

                    for (const QString &chk : allChecks) {
                        bool isUsed = false;
                        for (const QString &u : usedChecks) {
                            if (u.compare(chk, Qt::CaseInsensitive) == 0) {
                                isUsed = true;
                                break;
                            }
                        }
                        populateItem(chk, isUsed);
                    }

                    // Pre-select first available checkmark
                    for (int i = 0; i < listWidget->count(); ++i) {
                        if (listWidget->item(i)->flags() & Qt::ItemIsEnabled) {
                            listWidget->setCurrentRow(i);
                            break;
                        }
                    }

                    // Inline row to define and add a new checkmark on the fly
                    QHBoxLayout *hlNew = new QHBoxLayout();
                    hlNew->setSpacing(6);
                    QLineEdit *txtNewCheck = new QLineEdit(&dlg);
                    txtNewCheck->setPlaceholderText("Or define new checkmark...");
                    txtNewCheck->setMaxLength(20);
                    QPushButton *btnAddNew = new QPushButton("+ Add", &dlg);
                    btnAddNew->setStyleSheet("background-color: #2563eb; color: white; padding: 6px 12px; font-weight: bold; border-radius: 4px;");
                    hlNew->addWidget(txtNewCheck);
                    hlNew->addWidget(btnAddNew);
                    vl->addLayout(hlNew);

                    // Dialog Button Box (Assign / Cancel)
                    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
                    QPushButton *btnAssign = box->button(QDialogButtonBox::Ok);
                    if (btnAssign) {
                        btnAssign->setText("Assign");
                    }
                    vl->addWidget(box);

                    auto updateAssignBtn = [&]() {
                        QListWidgetItem *cur = listWidget->currentItem();
                        bool valid = cur && (cur->flags() & Qt::ItemIsEnabled);
                        if (btnAssign) {
                            btnAssign->setEnabled(valid);
                        }
                    };
                    updateAssignBtn();

                    connect(listWidget, &QListWidget::currentItemChanged, &dlg, [updateAssignBtn](QListWidgetItem*, QListWidgetItem*) {
                        updateAssignBtn();
                    });

                    // Double-clicking an enabled item accepts immediately
                    connect(listWidget, &QListWidget::itemDoubleClicked, &dlg, [&](QListWidgetItem *item) {
                        if (item && (item->flags() & Qt::ItemIsEnabled)) {
                            dlg.accept();
                        }
                    });

                    // Handler for adding a custom/new checkmark
                    auto handleAddNew = [&]() {
                        QString newName = txtNewCheck->text().trimmed();
                        if (newName.isEmpty()) return;

                        // Check if already used in template
                        for (const QString &u : usedChecks) {
                            if (u.compare(newName, Qt::CaseInsensitive) == 0) {
                                QMessageBox::warning(&dlg, "Already Used", QString("'%1' is already used in this template.").arg(newName));
                                return;
                            }
                        }

                        // Check if already present in the list
                        for (int i = 0; i < listWidget->count(); ++i) {
                            if (listWidget->item(i)->data(Qt::UserRole).toString().compare(newName, Qt::CaseInsensitive) == 0) {
                                if (listWidget->item(i)->flags() & Qt::ItemIsEnabled) {
                                    listWidget->setCurrentRow(i);
                                    txtNewCheck->clear();
                                    return;
                                }
                            }
                        }

                        // Add new selectable item to the list
                        populateItem(newName, false);
                        listWidget->setCurrentRow(listWidget->count() - 1);
                        txtNewCheck->clear();

                        // Persist to Station/Checks settings
                        QStringList curChecks = settings.value("Station/Checks").toStringList();
                        if (!curChecks.contains(newName, Qt::CaseInsensitive)) {
                            curChecks.append(newName);
                            settings.setValue("Station/Checks", curChecks);
                        }
                    };
                    connect(btnAddNew, &QPushButton::clicked, &dlg, handleAddNew);
                    connect(txtNewCheck, &QLineEdit::returnPressed, &dlg, handleAddNew);

                    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
                    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

                    if (dlg.exec() != QDialog::Accepted) {
                        m_scene->removeItem(area);
                        delete area;
                        m_addMode.clear();
                        return true;
                    }

                    QListWidgetItem *chosen = listWidget->currentItem();
                    if (!chosen || !(chosen->flags() & Qt::ItemIsEnabled)) {
                        m_scene->removeItem(area);
                        delete area;
                        m_addMode.clear();
                        return true;
                    }

                    QString txt = chosen->data(Qt::UserRole).toString().trimmed();
                    area->areaId = "CHK_" + txt;
                    area->textContent = "CHK_CUSTOM";
                    area->m_cellData.clear();
                    area->m_cellData.append("CHK_CUSTOM");
                    area->setZValue(2);
                    area->update();
                }

                if (idStr == "CIRC_CUSTOM") {
                    while (true) {
                        QDialog dlg(this);
                        dlg.setWindowTitle("New Circle");
                        dlg.setFixedWidth(380);
                        dlg.setStyleSheet(
                            "QDialog { background-color: #1e222b; border: 1px solid #3f4452; border-radius: 8px; }"
                            "QLabel { color: #f1f5f9; font-family: 'Segoe UI', Arial; font-size: 10pt; }"
                            "QLineEdit { background-color: #111318; color: #f1f5f9; border: 1px solid #2d313f; border-radius: 4px; padding: 6px; font-size: 10pt; }"
                            "QPushButton { background-color: #3b82f6; color: white; border: none; border-radius: 4px; padding: 6px 14px; font-weight: bold; }"
                            "QPushButton:hover { background-color: #2563eb; }"
                            "QToolButton { background-color: #374151; color: #f3f4f6; border: 1px solid #4b5563; border-radius: 10px; font-weight: bold; }"
                        );

                        QVBoxLayout *vl = new QVBoxLayout(&dlg);
                        vl->setSpacing(10);
                        vl->setContentsMargins(16, 16, 16, 16);

                        QHBoxLayout *hlLabel = new QHBoxLayout();
                        hlLabel->setSpacing(6);

                        QLabel *lblTitle = new QLabel("Enter circle name:", &dlg);
                        lblTitle->setStyleSheet("font-weight: bold;");

                        QToolButton *btnHelp = new QToolButton(&dlg);
                        btnHelp->setText("?");
                        btnHelp->setFixedSize(20, 20);
                        btnHelp->setToolTip("(This identifies the circle when printing.)");
                        connect(btnHelp, &QToolButton::clicked, this, [&dlg]() {
                            QMessageBox::information(&dlg, "Circle Name Help", "? (This identifies the circle when printing.)");
                        });

                        hlLabel->addWidget(lblTitle);
                        hlLabel->addWidget(btnHelp);
                        hlLabel->addStretch();
                        vl->addLayout(hlLabel);

                        QLineEdit *txtEdit = new QLineEdit(&dlg);
                        txtEdit->setPlaceholderText("Text");
                        txtEdit->setMaxLength(20);
                        vl->addWidget(txtEdit);

                        QLabel *lblCount = new QLabel("20 characters remaining", &dlg);
                        lblCount->setStyleSheet("color: #94a3b8; font-size: 9pt;");
                        vl->addWidget(lblCount);

                        connect(txtEdit, &QLineEdit::textChanged, [lblCount](const QString &text) {
                            int rem = 20 - text.length();
                            lblCount->setText(QString("%1 characters remaining").arg(rem));
                        });

                        QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
                        vl->addWidget(box);
                        connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
                        connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

                        if (dlg.exec() != QDialog::Accepted) {
                            m_scene->removeItem(area);
                            delete area;
                            m_addMode.clear();
                            return true;
                        }

                        QString txt = txtEdit->text().trimmed();
                        if (txt.isEmpty()) {
                            int num = 1;
                            while (true) {
                                QString candidate = QString("Circle %1").arg(num);
                                bool found = false;
                                if (m_scene) {
                                    for (QGraphicsItem *item : m_scene->items()) {
                                        if (auto *eArea = dynamic_cast<PrintTableArea*>(item)) {
                                            if (eArea != area && eArea->areaId == ("CIRC_" + candidate)) {
                                                found = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                                if (!found) {
                                    txt = candidate;
                                    break;
                                }
                                num++;
                            }
                        } else {
                            bool alreadyExists = false;
                            if (m_scene) {
                                for (QGraphicsItem *item : m_scene->items()) {
                                    PrintTableArea *existingArea = dynamic_cast<PrintTableArea*>(item);
                                    if (existingArea && existingArea != area && existingArea->areaId == ("CIRC_" + txt)) {
                                        alreadyExists = true;
                                        break;
                                    }
                                }
                            }
                            if (alreadyExists) {
                                QMessageBox::warning(this, "Duplicate Name", "Name must be unique. This name has already been used before.");
                                continue;
                            }
                        }

                        area->areaId = "CIRC_" + txt;
                        area->textContent = area->areaId;
                        area->setZValue(2);
                        area->update();
                        break;
                    }
                }
                
                if (idStr == "STAMP_REMINDER" && m_isOpenedByQslProcessor) {
                    if (QMessageBox::question(this, "Confirm Stamp Reminder", "Is this placement OK?") == QMessageBox::Yes) {
                        saveTemplateWithName(m_currentTemplateName);
                        accept(); // Close the dialog
                    } else {
                        m_scene->removeItem(area);
                        delete area;
                        m_addMode = "STAMP_REMINDER";
                        QApplication::setOverrideCursor(Qt::CrossCursor);
                        return true;
                    }
                }

                if (idStr == "REF_POINT_1") {
                    showCalibrationMessage(this, "Calibration Instruction", "First Reference Point placed successfully!\n\nPlease Place Your Second Reference Point at the Lower Rightmost Corner of your template.");
                    m_addMode = "REF_POINT";
                    m_addArg = "REF_POINT_2";
                    QApplication::setOverrideCursor(Qt::CrossCursor);
                    return true;
                } else if (idStr == "REF_POINT_2") {
                    showCalibrationMessage(this, "Calibration Completed", "Both Reference Points placed successfully!\n\nCalibration Scheme completed.");
                }
                
                m_addMode.clear();
                return true;
            }
        }
    } else if (event->type() == QEvent::KeyPress && !m_addMode.isEmpty()) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape) {
            QApplication::restoreOverrideCursor();
            m_addMode.clear();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void QslDesigner::removePrintArea() {
    pushUndo(); // Save before removing
    for (QGraphicsItem *item : m_scene->selectedItems()) {
        if (dynamic_cast<PrintTableArea*>(item)) {
            m_scene->removeItem(item);
            delete item;
            m_isModified = true;
        }
    }
    updateBlockList();
}

void QslDesigner::onSelectionChanged() {
    if (m_isDestroying || !m_scene) return;
    QList<QGraphicsItem*> sel = m_scene->selectedItems();
    if (sel.isEmpty()) {
        updateBlockList();
        updateTelemetry();
        return;
    }
    PrintTableArea *area = dynamic_cast<PrintTableArea*>(sel.first());
    if (area) {
        m_idEdit->blockSignals(true);
        m_fontCombo->blockSignals(true);
        m_sizeSpin->blockSignals(true);
        m_rotSpin->blockSignals(true);
        m_widthSpin->blockSignals(true);
        m_heightSpin->blockSignals(true);
        
        m_idEdit->setText(area->textContent);
        m_fontCombo->setCurrentText(area->m_font.family());
        m_sizeSpin->setValue(area->m_font.pointSize());
        m_activeColor = area->m_color;
        m_rotSpin->setValue((int)area->rotation());
        m_widthSpin->setValue(area->rect().width());
        m_heightSpin->setValue(area->rect().height());
        
        m_idEdit->blockSignals(false);
        m_fontCombo->blockSignals(false);
        m_sizeSpin->blockSignals(false);
        m_rotSpin->blockSignals(false);
        m_widthSpin->blockSignals(false);
        m_heightSpin->blockSignals(false);
        updateBlockList();
    } else {
        updateBlockList();
    }
    updateTelemetry();
}



void QslDesigner::updateActiveItemProperties() {
    QList<QGraphicsItem*> sel = m_scene->selectedItems();
    if (sel.isEmpty()) return;
    PrintTableArea *area = dynamic_cast<PrintTableArea*>(sel.first());
    if (area) {
        if (area->m_isAddressBox) {
            QString textVal = m_idEdit->text().trimmed();
            if (textVal.startsWith("<") && textVal.endsWith(">") && textVal.toLower() != "<address>") {
                area->m_isAddressBox = false;
                area->areaId = textVal;
                area->textContent = textVal;
            } else {
                area->areaId = "ADDRESS_BOX";
                area->textContent = m_idEdit->text();
            }
        } else {
            QString newId = m_idEdit->text();
            area->areaId = newId;
            if (newId.startsWith("CHK_")) {
                area->textContent = "CHK_CUSTOM";
            } else {
                area->textContent = newId;
            }
        }
        // ONLY update cell data if this is NOT a table block with multiple cells
        // For 1x1 blocks, it's fine to keep them synced as they are basically Text Blocks.
        if (area->m_rows <= 1 && area->m_cols <= 1) {
            if (area->areaId.startsWith("CHK_")) {
                area->m_cellData.clear();
                area->m_cellData.append("CHK_CUSTOM");
            } else {
                if(!area->m_cellData.isEmpty()) area->m_cellData[0] = area->textContent;
                else area->m_cellData.append(area->textContent);
            }
        }
        area->m_font = QFont(m_fontCombo->currentText(), m_sizeSpin->value());
        area->m_color = m_activeColor;
        area->setRotation(m_rotSpin->value());
        area->setRect(0, 0, m_widthSpin->value(), m_heightSpin->value());
        area->update();
        m_isModified = true;
        updateBlockList();
    }
    updateTelemetry();
}

void QslDesigner::updateTemplateList() {
    m_cardTemplateList->clear();
    m_envTemplateList->clear();
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");
    for (const QString &name : settings.childGroups()) {
        settings.beginGroup(name);
        bool isEnv = settings.value("isEnvelope", false).toBool();
        settings.endGroup();
        
        if (isEnv) {
            m_envTemplateList->addItem(name);
        } else {
            m_cardTemplateList->addItem(name);
        }
    }
    settings.endGroup();
    m_cardTemplateList->addItem("Load from local file...");
}

void QslDesigner::saveCurrentTemplate() {
    QDialog d(this);
    d.setWindowTitle("Save Template");
    d.setMinimumWidth(320);
    d.setStyleSheet(
        "QDialog {"
        "  background-color: #1e222b;"
        "  border: 1px solid #3f4452;"
        "  border-radius: 8px;"
        "}"
        "QLabel {"
        "  color: #f1f5f9;"
        "  font-family: 'Segoe UI', Arial, sans-serif;"
        "  font-size: 10pt;"
        "  font-weight: bold;"
        "}"
        "QLineEdit {"
        "  background-color: #111318;"
        "  color: #e2e8f0;"
        "  border: 1px solid #2d313f;"
        "  border-radius: 4px;"
        "  padding: 6px;"
        "  font-family: 'Segoe UI', Arial, sans-serif;"
        "  font-size: 10pt;"
        "}"
        "QPushButton {"
        "  background-color: #3b82f6;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 4px;"
        "  padding: 6px 12px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: #2563eb;"
        "}"
    );

    QPropertyAnimation *anim = new QPropertyAnimation(&d, "windowOpacity");
    anim->setDuration(200);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    d.setWindowOpacity(0.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    QVBoxLayout *vl = new QVBoxLayout(&d);
    
    QLineEdit *nameEdit = new QLineEdit(&d);
    nameEdit->setText(m_currentTemplateName);
    
    vl->addWidget(new QLabel("Template Name:"));
    vl->addWidget(nameEdit);
    vl->addSpacing(10);
    
    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &d);
    connect(box, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    vl->addWidget(box);
    
    if (d.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        if (name.isEmpty()) return;
        saveTemplateWithName(name);
    }
}

void QslDesigner::saveTemplateWithName(const QString &name) {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");
    settings.beginGroup(name);
    
    settings.setValue("paperW", m_paperW);
    settings.setValue("paperH", m_paperH);
    settings.setValue("printerName", m_printerName);
    settings.setValue("printCorrection", m_printCorrection);
    settings.setValue("printOffsetX", m_printOffsetX);
    settings.setValue("printOffsetY", m_printOffsetY);
    settings.setValue("isEnvelope", m_isEnvelope);
    settings.setValue("bgImagePath", m_bgImagePath);
    
    int i = 0;
    for (QGraphicsItem *item : m_scene->items()) {
        PrintTableArea *area = dynamic_cast<PrintTableArea*>(item);
        if (area) {
            settings.beginGroup(QString("Area_%1").arg(i++));
            settings.setValue("id", area->areaId);
            settings.setValue("text", area->textContent);
            settings.setValue("x", area->x());
            settings.setValue("y", area->y());
            settings.setValue("font", area->m_font.family());
            settings.setValue("size", area->m_font.pointSize());
            settings.setValue("color", area->m_color.name());
            settings.setValue("rot", area->rotation());
            settings.setValue("w", area->rect().width());
            settings.setValue("h", area->rect().height());
            settings.setValue("zValue", area->zValue());
            settings.setValue("maxLines", area->m_maxLines);
            settings.setValue("align", area->m_align);
            settings.setValue("isImage", area->m_isImage);
            settings.setValue("itemType", area->m_itemType);
            settings.setValue("imagePath", area->m_imagePath);
            settings.setValue("isQrCode", area->m_isQrCode);
            settings.setValue("qrPayload", area->m_qrPayload);
            settings.setValue("circleLineWidth", area->m_circleLineWidth);
            settings.setValue("rows", area->m_rows);
            settings.setValue("cols", area->m_cols);
            settings.setValue("showHeaders", area->m_showHeaders);
            settings.setValue("showBorders", area->m_showBorders);
            settings.setValue("borderThickness", area->m_borderThickness);
            settings.setValue("rowHeights", area->m_rowHeightsStr);
            settings.setValue("colWidths", area->m_colWidthsStr);
            settings.setValue("useBezierPath", false);
            settings.setValue("p0_norm_x", area->m_p0_norm.x());
            settings.setValue("p0_norm_y", area->m_p0_norm.y());
            settings.setValue("p1_norm_x", area->m_p1_norm.x());
            settings.setValue("p1_norm_y", area->m_p1_norm.y());
            settings.setValue("p2_norm_x", area->m_p2_norm.x());
            settings.setValue("p2_norm_y", area->m_p2_norm.y());
            settings.setValue("p3_norm_x", area->m_p3_norm.x());
            settings.setValue("p3_norm_y", area->m_p3_norm.y());
            
            settings.beginWriteArray("CellData", area->m_cellData.size());
            for (int j=0; j<area->m_cellData.size(); ++j) {
                settings.setArrayIndex(j);
                settings.setValue("data", area->m_cellData[j]);
            }
            settings.endArray();
            
            settings.endGroup();
        }
    }
    settings.setValue("count", i);
    
    m_scene->clearSelection();
    QImage img(m_paperW, m_paperH, QImage::Format_ARGB32);
    img.fill(Qt::white);
    QPainter p(&img);
    bool chv = m_crosshairV->isVisible();
    bool chh = m_crosshairH->isVisible();
    m_crosshairV->setVisible(false);
    m_crosshairH->setVisible(false);
    m_scene->render(&p, QRectF(), QRectF(0, 0, m_paperW, m_paperH));
    p.end();
    m_crosshairV->setVisible(chv);
    m_crosshairH->setVisible(chh);
    
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    img.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(&buffer, "PNG");
    settings.setValue("thumbnail", ba);
    
    settings.endGroup();
    settings.endGroup();
    updateTemplateList();
    m_currentTemplateName = name;
    m_isModified = false;

    // Determine if the saved template contains a high-capacity QSO table
    bool hasQsoTable = false;
    int maxRows = 0;
    for (QGraphicsItem *item : m_scene->items()) {
        PrintTableArea *area = dynamic_cast<PrintTableArea*>(item);
        if (area) {
            hasQsoTable = true;
            if (area->m_rows > maxRows) {
                maxRows = area->m_rows;
            }
        }
    }

    if (hasQsoTable && maxRows >= 5) {
        QSettings assocSettings(QCoreApplication::organizationName(), "Qwain");
        assocSettings.beginGroup("QslTemplates");
        QStringList allTemplates = assocSettings.childGroups();
        allTemplates.removeAll(name); // Don't associate with itself

        if (!allTemplates.isEmpty()) {
            QDialog assocDlg(this);
            assocDlg.setWindowTitle("Associate Overflow Template");
            assocDlg.setMinimumWidth(420);
            assocDlg.setStyleSheet(
                "QDialog { background-color: #1e222b; border: 1px solid #3f4452; border-radius: 8px; }"
                "QLabel { color: #f1f5f9; font-family: 'Segoe UI', Arial, sans-serif; font-size: 10pt; }"
                "QComboBox { background-color: #111318; color: #e2e8f0; border: 1px solid #2d313f; border-radius: 4px; padding: 6px; }"
                "QPushButton { background-color: #3b82f6; color: white; border: none; border-radius: 4px; padding: 6px 12px; font-weight: bold; }"
                "QPushButton:hover { background-color: #2563eb; }"
            );

            QVBoxLayout *vl = new QVBoxLayout(&assocDlg);
            QLabel *lbl = new QLabel(QString("This template contains a QSO table with %1 rows.\n\n"
                                             "Is this high-capacity (multi-QSO) card designed as an overflow template for a standard card?")
                                             .arg(maxRows), &assocDlg);
            lbl->setWordWrap(true);
            vl->addWidget(lbl);
            vl->addSpacing(10);

            vl->addWidget(new QLabel("Select the standard card template this is for:", &assocDlg));
            QComboBox *combo = new QComboBox(&assocDlg);
            combo->addItem("-- Do not associate / Not an overflow template --", "");
            for (const QString &tempName : allTemplates) {
                assocSettings.beginGroup(tempName);
                bool isEnv = assocSettings.value("isEnvelope", false).toBool();
                assocSettings.endGroup();
                if (!isEnv) {
                    combo->addItem(tempName, tempName);
                }
            }
            vl->addWidget(combo);
            vl->addSpacing(15);

            QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &assocDlg);
            connect(box, &QDialogButtonBox::accepted, &assocDlg, &QDialog::accept);
            connect(box, &QDialogButtonBox::rejected, &assocDlg, &QDialog::reject);
            vl->addWidget(box);

            if (assocDlg.exec() == QDialog::Accepted) {
                QString targetNormalTemplate = combo->currentData().toString();
                if (!targetNormalTemplate.isEmpty()) {
                    assocSettings.beginGroup(targetNormalTemplate);
                    assocSettings.setValue("OverflowTemplate", name);
                    assocSettings.endGroup();

                    QMessageBox::information(this, "Association Saved",
                        QString("Successfully associated overflow template '%1' with standard template '%2'.\n\n"
                                "When printing with '%2', the system will automatically handle overflow using '%1'.")
                        .arg(name).arg(targetNormalTemplate));
                }
            }
        }
    }
}

void QslDesigner::renameTemplate() {
    QListWidgetItem *itm = getSelectedTemplateItem();
    if (!itm) return;
    QString oldName = itm->text();
    if (oldName.startsWith("🏷️ ")) oldName = oldName.mid(3).trimmed();
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename Template", "New Template Name:", QLineEdit::Normal, oldName, &ok);
    if (!ok || newName.isEmpty() || newName == oldName) return;

    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");
    
    QStringList keys = settings.childGroups();
    if (keys.contains(newName)) {
        QMessageBox::warning(this, "Error", "A template with that name already exists.");
        return;
    }

    // Qt's QSettings has no rename. Copy keys.
    settings.beginGroup(oldName);
    QStringList allKeys = settings.allKeys();
    QHash<QString, QVariant> data;
    for (const QString &k : allKeys) data[k] = settings.value(k);
    settings.remove(""); // clear whole group
    settings.endGroup();

    settings.beginGroup(newName);
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        settings.setValue(it.key(), it.value());
    }
    settings.endGroup();
    settings.endGroup();

    if (m_currentTemplateName == oldName) m_currentTemplateName = newName;
    updateTemplateList();
}

void QslDesigner::deleteTemplate() {
    QListWidgetItem *itm = getSelectedTemplateItem();
    if (!itm) return;
    QString name = itm->text();
    if (name.startsWith("🏷️ ")) name = name.mid(3).trimmed();
    if (QMessageBox::question(this, "Confirm Delete", "Delete template: " + name + "?") != QMessageBox::Yes) return;
    if (QMessageBox::question(this, "Warning", "This cannot be undone. Do you really want to proceed?") != QMessageBox::Yes) return;
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");
    settings.remove(name);
    settings.endGroup();
    
    // Intended function: When deleting the template currently open on canvas,
    // synchronize document state by clearing canvas elements, resetting the modified
    // dirty flag, clearing the undo stack, and wiping tracking variables. This prevents
    // spurious 'Unsaved Changes' prompts when the user subsequently creates a 'New'
    // template, loads another template, or closes the designer.
    if (m_currentTemplateName.trimmed().compare(name.trimmed(), Qt::CaseInsensitive) == 0) {
        QList<QGraphicsItem*> items = m_scene->items();
        for (QGraphicsItem *item : items) {
            if (dynamic_cast<PrintTableArea*>(item)) {
                m_scene->removeItem(item);
                delete item;
            }
        }
        if (m_bgImageItem) {
            m_scene->removeItem(m_bgImageItem);
            delete m_bgImageItem;
            m_bgImageItem = nullptr;
        }
        m_bgImagePath.clear();

        m_currentTemplateName.clear();
        m_templateActive = false;
        m_isModified = false;
        m_undoStack.clear();
        m_cardTemplateList->clearSelection();
        m_envTemplateList->clearSelection();
        updateBlockList();
        drawBackgroundRulers();
    }
    updateTemplateList();
}

void QslDesigner::loadSelectedTemplate() {
    QListWidgetItem *itm = getSelectedTemplateItem();
    if (!itm) return;
    QString templateName = itm->text();
    
    if (m_isModified) {
        auto ret = QMessageBox::question(this, "Unsaved Changes", "You have unsaved changes. Do you want to save them before loading another template?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel) {
            updateTemplateList();
            return;
        }
        if (ret == QMessageBox::Yes) {
            saveCurrentTemplate();
            if (m_isModified) {
                updateTemplateList();
                return; // Cancelled or failed save
            }
        }
    }
    loadTemplate(templateName);
}

void QslDesigner::loadTemplate(const QString &rawName) {
    if (rawName.isEmpty()) return;
    QString name = rawName;
    if (name.startsWith("🏷️ ")) name = name.mid(3).trimmed();
    m_templateActive = true;

    std::unique_ptr<QSettings> settings;
    bool isJson = false;
    QJsonObject jsonDoc;
    QString loadedFileName;
    
    if (name == "Load from local file...") {
        loadedFileName = QFileDialog::getOpenFileName(this, "Open Template File", "", "Template Files (*.json *.ini);;JSON Files (*.json);;INI Files (*.ini);;All Files (*)");
        if (loadedFileName.isEmpty()) {
            m_cardTemplateList->clearSelection();
            m_envTemplateList->clearSelection();
            return;
        }
        
        if (loadedFileName.endsWith(".json", Qt::CaseInsensitive)) {
            QFile file(loadedFileName);
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray data = file.readAll();
                QJsonDocument doc = QJsonDocument::fromJson(data);
                if (!doc.isNull()) {
                    isJson = true;
                    jsonDoc = doc.object();
                    
                    QSettings s(QCoreApplication::organizationName(), "Qwain");
                    QString sysLogPath = s.value("Files/systemLogfile", "").toString();
                    if (!sysLogPath.isEmpty()) {
                        QFile sysfile(sysLogPath);
                        if (sysfile.open(QIODevice::Append | QIODevice::Text)) {
                            QTextStream out(&sysfile);
                            int elements = jsonDoc.keys().count() + (jsonDoc.contains("elements") ? jsonDoc["elements"].toArray().size() : 0);
                            out << QDateTime::currentDateTimeUtc().toString("dd-MM-yy HH:mm:ss.zzz UTC") << " JSON size: " << data.size() << " bytes, # of elements: " << elements << "\n";
                        }
                    }
                }
            }
        } else {
            settings = std::make_unique<QSettings>(loadedFileName, QSettings::IniFormat);
            if (settings->childGroups().contains("QslTemplates")) {
               settings->beginGroup("QslTemplates");
               QStringList templates = settings->childGroups();
               if (!templates.isEmpty()) {
                   settings->beginGroup(templates.first());
               }
            }
        }
    } else {
        settings = std::make_unique<QSettings>(QCoreApplication::organizationName(), "Qwain");
        settings->beginGroup("QslTemplates");
        settings->beginGroup(name);
    }
    
    // Clear all existing items from canvas before loading new template
    if (m_scene) {
        QList<QGraphicsItem*> items = m_scene->items();
        for (QGraphicsItem *item : items) {
            if (dynamic_cast<PrintTableArea*>(item)) {
                m_scene->removeItem(item);
                delete item;
            }
        }
        if (m_bgImageItem) {
            m_scene->removeItem(m_bgImageItem);
            delete m_bgImageItem;
            m_bgImageItem = nullptr;
        }
        m_bgImagePath.clear();
    }

    if (isJson) {
        m_paperW = jsonDoc["paperW"].toDouble(6.5) * 96.0;
        m_paperH = jsonDoc["paperH"].toDouble(3.625) * 96.0;
        m_printerName = jsonDoc["printerName"].toString();
        m_printCorrection = jsonDoc["printCorrection"].toInt();
        m_printOffsetX = jsonDoc["printOffsetX"].toDouble(0.0);
        m_printOffsetY = jsonDoc["printOffsetY"].toDouble(0.0);
        m_isEnvelope = jsonDoc["isEnvelope"].toBool(false);
        drawBackgroundRulers();
        
        QString bgPath = jsonDoc["bgImage"].toString();
        if (!bgPath.isEmpty() && QFile::exists(bgPath)) {
            setBackgroundImage(bgPath);
        } else if (m_bgImageItem) {
            m_scene->removeItem(m_bgImageItem);
            delete m_bgImageItem;
            m_bgImageItem = nullptr;
        }
        
        QJsonArray itemsArray = jsonDoc["items"].toArray();
        for (int i = 0; i < itemsArray.size(); ++i) {
            QJsonObject obj = itemsArray[i].toObject();
            int type = obj["type"].toInt();
            QString id = obj["id"].toString();
            
            PrintTableArea *area = nullptr;
            
            if (type == 1) { // TextElement
                QString txt = obj["text"].toString();
                area = new PrintTableArea(txt);
                area->areaId = id.isEmpty() ? txt : id;
                area->m_isAddressBox = (area->areaId == "ADDRESS_BOX");
                if (area->m_isAddressBox) {
                    area->textContent = txt;
                } else {
                    area->textContent = area->areaId;
                }
                area->setPos(obj["x"].toDouble(), obj["y"].toDouble());
                area->m_font = QFont(obj["fontFamily"].toString("Arial"), obj["fontSize"].toInt(12));
                area->m_color = QColor(obj["color"].toString("#000000"));
                area->setRect(0, 0, obj["w"].toDouble(100), obj["h"].toDouble(50));
            } else if (type == 2) { // TableElement
                area = new PrintTableArea(id.isEmpty() ? "TABLE" : id);
                area->areaId = id.isEmpty() ? "TABLE" : id;
                area->textContent = area->areaId;
                area->setPos(obj["x"].toDouble(), obj["y"].toDouble());
                area->m_font = QFont(obj["fontFamily"].toString("Arial"), obj["fontSize"].toInt(12));
                area->m_color = QColor(obj["color"].toString("#000000"));
                area->setRect(0, 0, obj["w"].toDouble(100), obj["h"].toDouble(50));
                
                area->m_rows = obj["rows"].toInt(1);
                area->m_cols = obj["cols"].toInt(1);
                area->m_rowHeightsStr = obj["rowHeights"].toString();
                area->m_colWidthsStr = obj["colWidths"].toString();
                
                area->m_cellData.clear();
                QJsonArray cells = obj["cellData"].toArray();
                for (int j = 0; j < cells.size(); ++j) {
                    area->m_cellData.append(cells[j].toString());
                }
            } else if (type == 3) { // RefElement
                area = new PrintTableArea(id.isEmpty() ? "REF_POINT" : id);
                area->areaId = id.isEmpty() ? "REF_POINT" : id;
                area->textContent = area->areaId;
                area->setPos(obj["x"].toDouble(), obj["y"].toDouble());
                area->setRect(0, 0, obj["w"].toDouble(30), obj["h"].toDouble(30));
                area->m_color = Qt::red;
            }
            
            if (area) {
                m_scene->addItem(area);
            }
        }
    } else {
        m_paperW = settings->value("paperW", 6.5*96).toDouble();
        m_paperH = settings->value("paperH", 3.625*96).toDouble();
        m_printerName = settings->value("printerName").toString();
        m_printCorrection = settings->value("printCorrection", 0).toInt();
        m_printOffsetX = settings->value("printOffsetX", 0.0).toDouble();
        m_printOffsetY = settings->value("printOffsetY", 0.0).toDouble();
        m_isEnvelope = settings->value("isEnvelope", false).toBool();
        drawBackgroundRulers();
        
        QString bgPath = settings->value("bgImagePath", "").toString();
        if (!bgPath.isEmpty() && QFile::exists(bgPath)) {
            setBackgroundImage(bgPath);
        } else if (m_bgImageItem) {
            m_scene->removeItem(m_bgImageItem);
            delete m_bgImageItem;
            m_bgImageItem = nullptr;
        }
        
        int count = settings->value("count", 0).toInt();
        for (int i=0; i<count; ++i) {
            settings->beginGroup(QString("Area_%1").arg(i));
            QString txt = settings->value("text").toString();
            PrintTableArea *area = new PrintTableArea(txt);
            area->areaId = settings->value("id").toString();
            area->m_isAddressBox = (area->areaId == "ADDRESS_BOX");
            area->setPos(settings->value("x").toDouble(), settings->value("y").toDouble());
            area->m_font = QFont(settings->value("font").toString(), settings->value("size", 12).toInt());
            area->m_color = QColor(settings->value("color", "#000000").toString());
            area->setRotation(settings->value("rot", 0).toDouble());
            area->setRect(0, 0, settings->value("w", 100).toDouble(), settings->value("h", 50).toDouble());
            
            area->m_maxLines = settings->value("maxLines", 0).toInt();
            area->m_align = settings->value("align", Qt::AlignCenter).toInt();
            area->m_rows = settings->value("rows", 1).toInt();
            area->m_cols = settings->value("cols", 1).toInt();
            area->m_showHeaders = settings->value("showHeaders", true).toBool();
            area->m_showBorders = settings->value("showBorders", true).toBool();
            area->m_borderThickness = settings->value("borderThickness", 1.0).toDouble();
            area->m_rowHeightsStr = settings->value("rowHeights", "").toString();
            area->m_colWidthsStr = settings->value("colWidths", "").toString();
            area->m_useBezierPath = false;
            if (settings->contains("p0_norm_x")) {
                area->m_p0_norm = QPointF(settings->value("p0_norm_x").toDouble(), settings->value("p0_norm_y").toDouble());
                area->m_p1_norm = QPointF(settings->value("p1_norm_x").toDouble(), settings->value("p1_norm_y").toDouble());
                area->m_p2_norm = QPointF(settings->value("p2_norm_x").toDouble(), settings->value("p2_norm_y").toDouble());
                area->m_p3_norm = QPointF(settings->value("p3_norm_x").toDouble(), settings->value("p3_norm_y").toDouble());
            }
            area->m_isImage = settings->value("isImage", false).toBool();
            if (settings->contains("itemType")) {
                area->m_itemType = settings->value("itemType").toInt();
            } else {
                // Auto-infer legacy templates for 100% backward compatibility
                if (area->areaId.startsWith("CHK_")) area->m_itemType = ITEM_CHECKMARK;
                else if (area->areaId.startsWith("CIRC_")) area->m_itemType = ITEM_CIRCLE;
                else if (area->areaId == "ADDRESS_BOX" || area->areaId.contains("ADDRESS")) area->m_itemType = ITEM_ADDRESS;
                else if (area->areaId == "STAMP_REMINDER") area->m_itemType = ITEM_STAMP;
                else if (area->areaId.contains("REF_POINT")) area->m_itemType = ITEM_REF_POINT;
                else if (area->m_isImage) area->m_itemType = ITEM_IMAGE;
                else if (area->m_rows > 1 || area->m_cols > 1 || area->areaId.contains("TABLE")) area->m_itemType = ITEM_TABLE;
                else area->m_itemType = ITEM_TEXT;
            }
            area->m_imagePath = settings->value("imagePath", "").toString();
            area->m_circleLineWidth = settings->value("circleLineWidth", 2).toInt();
            area->m_isQrCode = settings->value("isQrCode", area->areaId.startsWith("QR_CODE") || area->m_imagePath.contains("qr_")).toBool();
            area->m_qrPayload = settings->value("qrPayload", "").toString();
            if (area->m_isQrCode) area->ensureQrModules();
            if(area->m_isImage && !area->m_imagePath.isEmpty()) area->m_pixmap.load(area->m_imagePath);
            if (area->m_isImage || (area->m_rows == 1 && area->m_cols == 1)) {
                area->m_useBezierPath = false;
            }
            if (settings->contains("zValue")) area->setZValue(settings->value("zValue").toDouble());
            
            int cdCount = settings->beginReadArray("CellData");
            area->m_cellData.clear();
            for (int j=0; j<cdCount; ++j) {
                settings->setArrayIndex(j);
                area->m_cellData.append(settings->value("data").toString());
            }
            settings->endArray();
            
            m_scene->addItem(area);
            settings->endGroup();
        }
    }
    
    m_isModified = false;
    updateBlockList();
    if (name != "Load from local file...") {
        m_currentTemplateName = name;
    } else {
        if (isJson) {
            QFileInfo fi(loadedFileName);
            m_currentTemplateName = fi.completeBaseName();
            m_isModified = true;
        } else {
            m_currentTemplateName.clear();
        }
    }
    
    // Fit to view without resizing the window.
    if (m_scene && m_view) {
        m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }

    // Check if the template has any reference points defined
    int loadedRefPoints = 0;
    for (QGraphicsItem *item : m_scene->items()) {
        PrintTableArea *area = dynamic_cast<PrintTableArea*>(item);
        if (area && area->areaId.startsWith("REF_POINT_")) {
            loadedRefPoints++;
        }
    }
    
    if (loadedRefPoints == 0) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Qwain Cartesian Calibration Scheme Required");
        msgBox.setText(
            "This template does not have any reference marks defined (possibly from an older version or external import).\n\n"
            "Due to the incredible variations in Printer Manufacturers, and Drivers and Operating System, "
            "even within .jpg, .png, or other image files orientation is difficult or impossible at best, "
            "to help i developed a Qwain Cartesian Calibration Scheme.\n\n"
            "Please Place Your First Reference Point at the Upper Leftmost Corner of your template, "
            "Now place the Second Reference Point at the Lower Rightmost Corner. After 2 placements, continue."
        );
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setStyleSheet(
            "QMessageBox {"
            "  background-color: #1e222b;"
            "  border: 1px solid #3f4452;"
            "  border-radius: 8px;"
            "}"
            "QLabel {"
            "  color: #f1f5f9;"
            "  font-family: 'Segoe UI', Arial, sans-serif;"
            "  font-size: 11pt;"
            "  padding: 10px;"
            "}"
            "QPushButton {"
            "  background-color: #3b82f6;"
            "  color: white;"
            "  border: none;"
            "  border-radius: 4px;"
            "  padding: 8px 16px;"
            "  font-weight: bold;"
            "  font-size: 10pt;"
            "  min-width: 100px;"
            "}"
            "QPushButton:hover {"
            "  background-color: #2563eb;"
            "}"
        );
        msgBox.exec();

        showCalibrationMessage(this, "Calibration Instruction", "Please Place Your First Reference Point at the Upper Leftmost Corner of your template.");

        m_addMode = "REF_POINT";
        m_addArg = "REF_POINT_1";
        QApplication::setOverrideCursor(Qt::CrossCursor);
    }
}

QVariantMap QslDesigner::serializeState() {
    QVariantMap state;
    state["paperW"] = m_paperW;
    state["paperH"] = m_paperH;
    state["printerName"] = m_printerName;
    state["printCorrection"] = m_printCorrection;
    state["printOffsetX"] = m_printOffsetX;
    state["printOffsetY"] = m_printOffsetY;
    state["isEnvelope"] = m_isEnvelope;
    state["bgImagePath"] = m_bgImagePath;
    
    QVariantList items;
    for (QGraphicsItem *item : m_scene->items()) {
        PrintTableArea *area = dynamic_cast<PrintTableArea*>(item);
        if (area) {
            QVariantMap m;
            m["id"] = area->areaId;
            m["text"] = area->textContent;
            m["x"] = area->pos().x();
            m["y"] = area->pos().y();
            m["font"] = area->m_font.family();
            m["size"] = area->m_font.pointSize();
            m["color"] = area->m_color.name();
            m["rot"] = area->rotation();
            m["w"] = area->rect().width();
            m["h"] = area->rect().height();
            m["maxLines"] = area->m_maxLines;
            m["align"] = area->m_align;
            m["rows"] = area->m_rows;
            m["cols"] = area->m_cols;
            m["showHeaders"] = area->m_showHeaders;
            m["showBorders"] = area->m_showBorders;
            m["borderThickness"] = area->m_borderThickness;
            m["rowHeights"] = area->m_rowHeightsStr;
            m["colWidths"] = area->m_colWidthsStr;
            m["isImage"] = area->m_isImage;
            m["imagePath"] = area->m_imagePath;
            m["circleLineWidth"] = area->m_circleLineWidth;
            m["zValue"] = area->zValue();
            m["cellData"] = area->m_cellData;
            items.append(m);
        }
    }
    state["items"] = items;
    return state;
}

void QslDesigner::restoreState(const QVariantMap &state) {
    m_templateActive = true;
    for (QGraphicsItem *item : m_scene->items()) {
        if (dynamic_cast<PrintTableArea*>(item)) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    m_paperW = state["paperW"].toDouble();
    m_paperH = state["paperH"].toDouble();
    m_printerName = state["printerName"].toString();
    m_printCorrection = state["printCorrection"].toInt();
    m_printOffsetX = state["printOffsetX"].toDouble();
    m_printOffsetY = state["printOffsetY"].toDouble();
    m_isEnvelope = state["isEnvelope"].toBool();
    drawBackgroundRulers();
    
    QString bgPath = state["bgImagePath"].toString();
    if (!bgPath.isEmpty() && QFile::exists(bgPath)) {
        setBackgroundImage(bgPath);
    } else if (m_bgImageItem) {
        m_scene->removeItem(m_bgImageItem);
        delete m_bgImageItem;
        m_bgImageItem = nullptr;
    }
    
    QVariantList items = state["items"].toList();
    for (const QVariant &v : items) {
        QVariantMap m = v.toMap();
        PrintTableArea *area = new PrintTableArea(m["text"].toString());
        area->areaId = m["id"].toString();
        area->m_isAddressBox = (area->areaId == "ADDRESS_BOX");
        area->setPos(m["x"].toDouble(), m["y"].toDouble());
        area->m_font = QFont(m["font"].toString(), m["size"].toInt());
        area->m_color = QColor(m["color"].toString());
        area->setRotation(m["rot"].toDouble());
        area->setRect(0, 0, m["w"].toDouble(), m["h"].toDouble());
        
        area->m_maxLines = m["maxLines"].toInt();
        if (m.contains("align")) area->m_align = m["align"].toInt();
        else area->m_align = Qt::AlignCenter;
        
        area->m_isImage = m["isImage"].toBool();
        area->m_imagePath = m["imagePath"].toString();
        if (m.contains("circleLineWidth")) {
            area->m_circleLineWidth = m["circleLineWidth"].toInt();
        }
        if (area->m_isImage && !area->m_imagePath.isEmpty()) {
            area->m_pixmap.load(area->m_imagePath);
        }
        if (m.contains("zValue")) area->setZValue(m["zValue"].toDouble());
        
        area->m_rows = m["rows"].toInt();
        area->m_cols = m["cols"].toInt();
        if (m.contains("showHeaders")) area->m_showHeaders = m["showHeaders"].toBool();
        if (m.contains("showBorders")) area->m_showBorders = m["showBorders"].toBool();
        if (m.contains("borderThickness")) area->m_borderThickness = m["borderThickness"].toDouble();
        area->m_rowHeightsStr = m["rowHeights"].toString();
        area->m_colWidthsStr = m["colWidths"].toString();
        
        area->m_cellData = m["cellData"].toStringList();
        m_scene->addItem(area);
    }
    updateBlockList();
    m_scene->update();
}

void QslDesigner::pushUndo() {
    QVariantMap newState = serializeState();
    if (!m_undoStack.isEmpty() && m_undoStack.last() == newState) {
        return; // Don't push duplicate states
    }
    m_undoStack.append(newState);
    if (m_undoStack.size() > 4) { // Keep up to 4, so we have 3 undos
        m_undoStack.removeFirst();
    }
}

QListWidgetItem* QslDesigner::getSelectedTemplateItem() {
    if (m_cardTemplateList && m_cardTemplateList->selectedItems().count() > 0) return m_cardTemplateList->currentItem();
    if (m_envTemplateList && m_envTemplateList->selectedItems().count() > 0) return m_envTemplateList->currentItem();
    return nullptr;
}

void QslDesigner::newTemplate() {
    // Stub implementation to satisfy linker
}

void QslDesigner::undoAction() {
    // Stub implementation to satisfy linker
}

void QslDesigner::addCustomCheckmark() {
    // Stub implementation to satisfy linker
}

void QslDesigner::addReferencePoint() {
    // Stub implementation to satisfy linker
}

void QslDesigner::defineFields() {
    // Stub implementation to satisfy linker
}

void QslDesigner::toggleBackground() {
    // Stub implementation to satisfy linker
}

void QslDesigner::printerSettings() {
    // Stub implementation to satisfy linker
}

void QslDesigner::addNewField() {
    // Stub implementation to satisfy linker
}

void QslDesigner::triggerDebugDump() {
    // Stub implementation to satisfy linker
}

// [ENTRY POINT: Canvas Drag-Panning Mouse Move Tracker]
// Translates view coordinates into scene deltas for inertia-free center shifts.
void MyGraphicsView::mouseMoveEvent(QMouseEvent *event) {
    if (m_isPanning) {
        QPoint delta = event->pos() - m_panStartPos;
        m_panStartPos = event->pos();
        
        QPointF p1 = mapToScene(0, 0);
        QPointF p2 = mapToScene(delta);
        QPointF sceneDelta = p2 - p1;
        
        QPointF center = mapToScene(viewport()->rect().center());
        QPointF targetCenter = center - sceneDelta;
        centerOn(targetCenter);
        
        if (auto designer = qobject_cast<QslDesigner*>(window())) {
            designer->updateTelemetry(mapToScene(event->pos()));
        }
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
    emit mouseMoved(mapToScene(event->pos()));
}

// [ENTRY POINT: Click Selection & Intercept Bypass Gateway]
// Intercepts selections, bypasses non-interactive guidelines (Z-value < 0 or Z-value = 100),
// and initiates undo snapshot states or left-click-drag canvas panning.
void MyGraphicsView::mousePressEvent(QMouseEvent *event) {
    bool isTestView = scene() && scene()->property("isTestView").toBool();
    if (!isTestView) {
        if (auto designer = qobject_cast<QslDesigner*>(window())) {
           designer->pushUndo();
        }
    }
    QGraphicsItem *item = itemAt(event->pos());
    
    // Crosshair bypass: If the clicked item is a cursor-tracking crosshair line (Z-value of 100), click "through" it
    // to find the actual design element or background underneath.
    if (auto designer = qobject_cast<QslDesigner*>(window())) {
        if (item && (item == designer->m_crosshairH || item == designer->m_crosshairV)) {
            QList<QGraphicsItem*> itemsAtPos = scene()->items(mapToScene(event->pos()));
            item = nullptr;
            for (QGraphicsItem *it : itemsAtPos) {
                if (it != designer->m_crosshairH && it != designer->m_crosshairV) {
                    item = it;
                    break;
                }
            }
        }
    }

    // If the clicked item is a background image, the paper rect, grid lines, or any non-interactive background item (Z-value < 0),
    // treat it as empty canvas space to trigger template panning.
    if (item && item->zValue() < 0) {
        item = nullptr;
    }
    if (!item && event->button() == Qt::LeftButton) {
        m_isPanning = true;
        m_panStartPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        if (auto designer = qobject_cast<QslDesigner*>(window())) {
            designer->updateTelemetry(mapToScene(event->pos()));
        }
        event->accept();
        return;
    }
    if (auto designer = qobject_cast<QslDesigner*>(window())) {
        designer->updateTelemetry(mapToScene(event->pos()));
    }
    QGraphicsView::mousePressEvent(event);
}

void MyGraphicsView::mouseReleaseEvent(QMouseEvent *event) {
    if (m_isPanning && event->button() == Qt::LeftButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        if (auto designer = qobject_cast<QslDesigner*>(window())) {
            designer->updateTelemetry(mapToScene(event->pos()));
        }
        event->accept();
        return;
    }
    if (auto designer = qobject_cast<QslDesigner*>(window())) {
        designer->updateTelemetry(mapToScene(event->pos()));
    }
    QGraphicsView::mouseReleaseEvent(event);
    emit mouseReleased();
}
