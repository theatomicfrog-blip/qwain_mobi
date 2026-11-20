#ifndef ELEMENTS_H
#define ELEMENTS_H

#include <QGraphicsRectItem>
#include <QGraphicsObject>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QJsonObject>
#include <QStringList>

class BaseElement : public QGraphicsObject {
    Q_OBJECT
public:
    enum ElementType { TextType = UserType + 1, TableType, ImageType, RefType };

    explicit BaseElement(QGraphicsItem *parent = nullptr);
    virtual ~BaseElement() = default;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    virtual QJsonObject toJson() const;
    virtual void fromJson(const QJsonObject &json);
    virtual int type() const = 0;

    QString elementId() const { return m_id; }
    void setElementId(const QString &id) { m_id = id; }

    QRectF rect() const { return m_rect; }
    void setRect(const QRectF &rect);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;

    QRectF m_rect;
    QString m_id;
    bool m_isResizing;
    QPointF m_resizeStart;
    QRectF m_rectStart;
};

class TextElement : public BaseElement {
    Q_OBJECT
public:
    explicit TextElement(QGraphicsItem *parent = nullptr);
    
    int type() const override { return TextType; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &json) override;

    QString text() const { return m_text; }
    void setText(const QString &text);

    QFont font() const { return m_font; }
    void setFont(const QFont &font);

    QColor color() const { return m_color; }
    void setColor(const QColor &color);

private:
    QString m_text;
    QFont m_font;
    QColor m_color;
};

class TableElement : public BaseElement {
    Q_OBJECT
public:
    explicit TableElement(QGraphicsItem *parent = nullptr);
    
    int type() const override { return TableType; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &json) override;

    int rows() const { return m_rows; }
    void setRows(int rows);

    int columns() const { return m_cols; }
    void setColumns(int cols);

    QStringList cellData() const { return m_cellData; }
    void setCellData(const QStringList &data);

    QString rowHeightsStr() const { return m_rowHeightsStr; }
    void setRowHeightsStr(const QString &str);
    
    QString colWidthsStr() const { return m_colWidthsStr; }
    void setColWidthsStr(const QString &str);

    QFont font() const { return m_font; }
    void setFont(const QFont &font);

    QColor color() const { return m_color; }
    void setColor(const QColor &color);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    int m_rows;
    int m_cols;
    QStringList m_cellData;
    QString m_rowHeightsStr;
    QString m_colWidthsStr;
    QFont m_font;
    QColor m_color;
    
    int m_draggingColDivider;
    int m_draggingRowDivider;
    QList<double> m_currentCw;
    QList<double> m_currentRh;
};

class RefElement : public BaseElement {
    Q_OBJECT
public:
    explicit RefElement(QGraphicsItem *parent = nullptr);
    
    int type() const override { return RefType; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &json) override;
};

#endif // ELEMENTS_H
