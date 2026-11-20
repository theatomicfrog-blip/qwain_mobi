#ifndef TEMPLATE_VIEW_H
#define TEMPLATE_VIEW_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QSize>
#include <QString>

class TemplateView : public QGraphicsView {
    Q_OBJECT
public:
    explicit TemplateView(QWidget *parent = nullptr);
    void setScene(QGraphicsScene *scene);
    
    QSizeF paperSizeInInches() const { return m_paperInc; }
    void setPaperSizeInInches(const QSizeF &size);
    
    QString backgroundImage() const { return m_bgImage; }
    void setBackgroundImage(const QString &path);
    
    double bgOpacity() const { return m_bgOpacity; }
    void setBgOpacity(double opacity);

signals:
    void opacityChanged(double opacity);
    void mouseMoved(QPointF scenePos);
    void mouseClicked(QPointF scenePos);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void updateSceneRect();

    QSizeF m_paperInc; // Paper size in inches
    QString m_bgImage;
    double m_bgOpacity;
    double m_dpi;
};

#endif // TEMPLATE_VIEW_H
