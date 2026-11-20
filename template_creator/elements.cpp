#include "elements.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QCursor>
#include <QStyleOptionGraphicsItem>
#include <QJsonArray>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

// BaseElement
BaseElement::BaseElement(QGraphicsItem *parent)
    : QGraphicsObject(parent), m_rect(0, 0, 100, 50), m_isResizing(false) {
    setFlags(ItemIsSelectable | ItemIsMovable | ItemIsFocusable);
    setAcceptHoverEvents(true);
}

QRectF BaseElement::boundingRect() const {
    return m_rect;
}

void BaseElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(widget);
    if (option->state & QStyle::State_Selected) {
        painter->setPen(QPen(Qt::blue, 1, Qt::DashLine));
        painter->drawRect(m_rect);
        
        // Draw resize handle
        painter->setBrush(Qt::white);
        painter->setPen(Qt::black);
        painter->drawRect(QRectF(m_rect.right() - 6, m_rect.bottom() - 6, 6, 6));
    }
}

void BaseElement::setRect(const QRectF &rect) {
    prepareGeometryChange();
    m_rect = rect;
    update();
}

QJsonObject BaseElement::toJson() const {
    QJsonObject json;
    json["id"] = m_id;
    json["x"] = pos().x();
    json["y"] = pos().y();
    json["w"] = m_rect.width();
    json["h"] = m_rect.height();
    json["type"] = static_cast<int>(type());
    return json;
}

void BaseElement::fromJson(const QJsonObject &json) {
    m_id = json["id"].toString();
    setPos(json["x"].toDouble(), json["y"].toDouble());
    setRect(QRectF(0, 0, json["w"].toDouble(), json["h"].toDouble()));
}

void BaseElement::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    QRectF resizeHandle(m_rect.right() - 8, m_rect.bottom() - 8, 8, 8);
    if (resizeHandle.contains(event->pos())) {
        m_isResizing = true;
        m_resizeStart = event->scenePos();
        m_rectStart = m_rect;
        event->accept();
    } else {
        QGraphicsObject::mousePressEvent(event);
    }
}

void BaseElement::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (m_isResizing) {
        QPointF diff = event->scenePos() - m_resizeStart;
        double newW = qMax(20.0, m_rectStart.width() + diff.x());
        double newH = qMax(10.0, m_rectStart.height() + diff.y());
        setRect(QRectF(0, 0, newW, newH));
    } else {
        QGraphicsObject::mouseMoveEvent(event);
    }
}

void BaseElement::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (m_isResizing) {
        m_isResizing = false;
    } else {
        QGraphicsObject::mouseReleaseEvent(event);
    }
}

void BaseElement::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    QRectF resizeHandle(m_rect.right() - 8, m_rect.bottom() - 8, 8, 8);
    if (resizeHandle.contains(event->pos())) {
        setCursor(Qt::SizeFDiagCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsObject::hoverMoveEvent(event);
}

// TextElement
TextElement::TextElement(QGraphicsItem *parent) : BaseElement(parent) {
    m_font = QFont("Arial", 12);
    m_color = Qt::black;
    m_text = "[Text]";
}

void TextElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    BaseElement::paint(painter, option, widget);
    if (m_text.contains("<div") || m_text.contains("<span>")) {
        painter->save();
        QTextDocument doc;
        doc.setDefaultFont(m_font);
        // We set document width to m_rect width so right align works.
        doc.setTextWidth(m_rect.width());
        QString txt = m_text;
        doc.setHtml(QString("<div style='color:%2;'>%1</div>").arg(txt.replace("\n", "<br>"), m_color.name()));
        painter->translate(m_rect.topLeft());
        doc.drawContents(painter);
        painter->restore();
    } else {
        painter->setFont(m_font);
        painter->setPen(m_color);
        painter->drawText(m_rect, Qt::AlignCenter | Qt::TextWordWrap, m_text);
    }
}

QJsonObject TextElement::toJson() const {
    QJsonObject json = BaseElement::toJson();
    json["text"] = m_text;
    json["fontFamily"] = m_font.family();
    json["fontSize"] = m_font.pointSize();
    json["color"] = m_color.name();
    return json;
}

void TextElement::fromJson(const QJsonObject &json) {
    BaseElement::fromJson(json);
    m_text = json["text"].toString();
    m_font.setFamily(json["fontFamily"].toString());
    m_font.setPointSize(json["fontSize"].toInt());
    m_color = QColor(json["color"].toString());
}

void TextElement::setText(const QString &text) { m_text = text; update(); }
void TextElement::setFont(const QFont &font) { m_font = font; update(); }
void TextElement::setColor(const QColor &color) { m_color = color; update(); }

// TableElement
TableElement::TableElement(QGraphicsItem *parent) : BaseElement(parent) {
    m_rows = 4;
    m_cols = 4;
    m_cellData << "<DATE>" << "<TIME>" << "<BAND>" << "<MODE>";
    m_font = QFont("Arial", 10);
    m_color = Qt::black;
    m_draggingColDivider = -1;
    m_draggingRowDivider = -1;
    setRect(QRectF(0, 0, 300, 150));
    setAcceptHoverEvents(true);
}

void TableElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    BaseElement::paint(painter, option, widget);
    painter->setFont(m_font);
    painter->setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    
    // We can use individual row/col sizes if provided, or fallback to exact division
    QStringList rhStr = m_rowHeightsStr.split(",", Qt::SkipEmptyParts);
    QStringList cwStr = m_colWidthsStr.split(",", Qt::SkipEmptyParts);
    
    QList<double> rh, cw;
    double totalRh = 0, totalCw = 0;
    for (int i=0; i<m_rows; ++i) {
        double v = (i < rhStr.size()) ? rhStr[i].toDouble() : 100.0;
        if (v <= 0) v = 100;
        rh << v;
        totalRh += v;
    }
    for (int i=0; i<m_cols; ++i) {
        double v = (i < cwStr.size()) ? cwStr[i].toDouble() : 100.0;
        if (v <= 0) v = 100;
        cw << v;
        totalCw += v;
    }
    
    if (totalRh > 0) {
        for (int i=0; i<m_rows; ++i) rh[i] = (rh[i] / totalRh) * m_rect.height();
    } else {
        for (int i=0; i<m_rows; ++i) rh << (m_rect.height() / m_rows);
    }
    
    if (totalCw > 0) {
        for (int i=0; i<m_cols; ++i) cw[i] = (cw[i] / totalCw) * m_rect.width();
    } else {
        for (int i=0; i<m_cols; ++i) cw << (m_rect.width() / m_cols);
    }

    m_currentCw = cw;
    m_currentRh = rh;

    QPen gridPen(Qt::black, 2, Qt::SolidLine);
    painter->setPen(gridPen);

    // Grid lines
    double currentY = 0;
    for (int r = 0; r <= m_rows; ++r) {
        painter->drawLine(0, currentY, m_rect.width(), currentY);
        if (r < m_rows) currentY += rh[r];
    }
    
    double currentX = 0;
    for (int c = 0; c <= m_cols; ++c) {
        painter->drawLine(currentX, 0, currentX, m_rect.height());
        if (c < m_cols) currentX += cw[c];
    }

    // Placeholders
    currentY = 0;
    painter->setPen(m_color);
    for (int r = 0; r < m_rows; ++r) {
        currentX = 0;
        for (int c = 0; c < m_cols; ++c) {
            QRectF cell(currentX, currentY, cw[c], rh[r]);
            int idx = r * m_cols + c;
            QString txt = (idx < m_cellData.size()) ? m_cellData[idx] : "";
            painter->drawText(cell, Qt::AlignCenter | Qt::TextWordWrap, txt);
            currentX += cw[c];
        }
        currentY += rh[r];
    }
}

QJsonObject TableElement::toJson() const {
    QJsonObject json = BaseElement::toJson();
    json["rows"] = m_rows;
    json["cols"] = m_cols;
    
    QJsonArray cells;
    for (const QString &f : m_cellData) cells.append(f);
    json["cellData"] = cells;
    json["rowHeights"] = m_rowHeightsStr;
    json["colWidths"] = m_colWidthsStr;
    
    json["fontFamily"] = m_font.family();
    json["fontSize"] = m_font.pointSize();
    json["color"] = m_color.name();
    return json;
}

void TableElement::fromJson(const QJsonObject &json) {
    BaseElement::fromJson(json);
    m_rows = json["rows"].toInt();
    m_cols = json["cols"].toInt();
    
    m_cellData.clear();
    QJsonArray cells = json["cellData"].toArray();
    for (int i=0; i<cells.size(); ++i) m_cellData.append(cells[i].toString());
    
    m_rowHeightsStr = json["rowHeights"].toString();
    m_colWidthsStr = json["colWidths"].toString();
    
    m_font.setFamily(json["fontFamily"].toString());
    m_font.setPointSize(json["fontSize"].toInt());
    m_color = QColor(json["color"].toString());
}

void TableElement::setRows(int rows) { m_rows = rows; update(); }
void TableElement::setColumns(int cols) { m_cols = cols; update(); }
void TableElement::setCellData(const QStringList &data) { m_cellData = data; update(); }
void TableElement::setRowHeightsStr(const QString &str) { m_rowHeightsStr = str; update(); }
void TableElement::setColWidthsStr(const QString &str) { m_colWidthsStr = str; update(); }
void TableElement::setFont(const QFont &font) { m_font = font; update(); }
void TableElement::setColor(const QColor &color) { m_color = color; update(); }

void TableElement::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    if (m_currentCw.size() != m_cols || m_currentRh.size() != m_rows) {
        BaseElement::hoverMoveEvent(event);
        return;
    }
    
    QPointF pos = event->pos();
    const double TOL = 5.0;
    
    bool overDivider = false;
    double currentX = 0;
    for (int c = 0; c < m_cols - 1; ++c) {
        currentX += m_currentCw[c];
        if (qAbs(pos.x() - currentX) < TOL && pos.y() < m_rect.height()) {
            setCursor(Qt::SplitHCursor);
            overDivider = true;
            break;
        }
    }
    
    if (!overDivider) {
        double currentY = 0;
        for (int r = 0; r < m_rows - 1; ++r) {
            currentY += m_currentRh[r];
            if (qAbs(pos.y() - currentY) < TOL && pos.x() < m_rect.width()) {
                setCursor(Qt::SplitVCursor);
                overDivider = true;
                break;
            }
        }
    }
    
    if (!overDivider) {
        QRectF resizeHandle(m_rect.right() - 8, m_rect.bottom() - 8, 8, 8);
        if (resizeHandle.contains(pos)) {
            setCursor(Qt::SizeFDiagCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }
}

void TableElement::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (m_currentCw.size() != m_cols || m_currentRh.size() != m_rows) {
        BaseElement::mousePressEvent(event);
        return;
    }

    QPointF pos = event->pos();
    const double TOL = 5.0;
    
    m_draggingColDivider = -1;
    m_draggingRowDivider = -1;
    
    double currentX = 0;
    for (int c = 0; c < m_cols - 1; ++c) {
        currentX += m_currentCw[c];
        if (qAbs(pos.x() - currentX) < TOL) {
            m_draggingColDivider = c;
            event->accept();
            return;
        }
    }
    
    double currentY = 0;
    for (int r = 0; r < m_rows - 1; ++r) {
        currentY += m_currentRh[r];
        if (qAbs(pos.y() - currentY) < TOL) {
            m_draggingRowDivider = r;
            event->accept();
            return;
        }
    }
    
    BaseElement::mousePressEvent(event);
}

void TableElement::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (m_draggingColDivider >= 0) {
        // update relative strings
        double currentX = 0;
        for (int c = 0; c < m_draggingColDivider; ++c) currentX += m_currentCw[c];
        
        double diff = event->pos().x() - (currentX + m_currentCw[m_draggingColDivider]);
        double newLeft = qMax(10.0, m_currentCw[m_draggingColDivider] + diff);
        double newRight = qMax(10.0, m_currentCw[m_draggingColDivider+1] - diff);
        diff = newLeft - m_currentCw[m_draggingColDivider]; // actual diff applied
        
        m_currentCw[m_draggingColDivider] += diff;
        m_currentCw[m_draggingColDivider+1] -= diff;
        
        QStringList wStrs;
        for (double w : m_currentCw) wStrs << QString::number(w, 'f', 1);
        m_colWidthsStr = wStrs.join(",");
        
        update();
        return;
    }
    
    if (m_draggingRowDivider >= 0) {
        double currentY = 0;
        for (int r = 0; r < m_draggingRowDivider; ++r) currentY += m_currentRh[r];
        
        double diff = event->pos().y() - (currentY + m_currentRh[m_draggingRowDivider]);
        double newTop = qMax(10.0, m_currentRh[m_draggingRowDivider] + diff);
        double newBottom = qMax(10.0, m_currentRh[m_draggingRowDivider+1] - diff);
        diff = newTop - m_currentRh[m_draggingRowDivider]; // actual diff
        
        m_currentRh[m_draggingRowDivider] += diff;
        m_currentRh[m_draggingRowDivider+1] -= diff;
        
        QStringList hStrs;
        for (double h : m_currentRh) hStrs << QString::number(h, 'f', 1);
        m_rowHeightsStr = hStrs.join(",");
        
        update();
        return;
    }
    
    BaseElement::mouseMoveEvent(event);
}

void TableElement::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (m_draggingColDivider >= 0 || m_draggingRowDivider >= 0) {
        m_draggingColDivider = -1;
        m_draggingRowDivider = -1;
        
        // Let properties panel know it might need to update the height/width strings
        // Unfortunately we do not emit signals directly from QGraphicsItem to the specific properties panel because it's selected.
        // Instead the properties panel polls, which is fine when 'elementChanged' is not necessary since dragging doesn't emit. But if we click away it updates.
        // I won't worry too much about syncing the side panel perfectly while dragging divider.
        return;
    }
    BaseElement::mouseReleaseEvent(event);
}

// RefElement
RefElement::RefElement(QGraphicsItem *parent) : BaseElement(parent) {
    setRect(QRectF(0, 0, 20, 20));
    m_id = "REF_MARK";
}

void RefElement::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    BaseElement::paint(painter, option, widget);
    
    painter->setPen(QPen(Qt::red, 2));
    
    // Draw crosshair
    QPointF center = m_rect.center();
    painter->drawLine(center.x(), m_rect.top(), center.x(), m_rect.bottom());
    painter->drawLine(m_rect.left(), center.y(), m_rect.right(), center.y());
    
    // Draw outer circle
    painter->drawEllipse(m_rect);
}

QJsonObject RefElement::toJson() const {
    return BaseElement::toJson();
}

void RefElement::fromJson(const QJsonObject &json) {
    BaseElement::fromJson(json);
}
