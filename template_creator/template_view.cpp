#include "template_view.h"
#include "elements.h"
#include <QPainter>
#include <QPixmap>
#include <QGraphicsItem>
#include <QMouseEvent>

TemplateView::TemplateView(QWidget *parent)
    : QGraphicsView(parent), m_paperInc(5.5, 3.5), m_bgOpacity(1.0), m_dpi(96.0)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    setBackgroundBrush(Qt::darkGray);
    setDragMode(QGraphicsView::RubberBandDrag);
    setMouseTracking(true);
}

void TemplateView::setScene(QGraphicsScene *scene) {
    QGraphicsView::setScene(scene);
    updateSceneRect();
}

void TemplateView::setPaperSizeInInches(const QSizeF &size) {
    m_paperInc = size;
    updateSceneRect();
    viewport()->update();
}

void TemplateView::setBackgroundImage(const QString &path) {
    m_bgImage = path;
    viewport()->update();
}

void TemplateView::setBgOpacity(double opacity) {
    if (m_bgOpacity != opacity) {
        m_bgOpacity = opacity;
        emit opacityChanged(opacity);
        viewport()->update();
    }
}

void TemplateView::updateSceneRect() {
    if (scene()) {
        QRectF sr(0, 0, m_paperInc.width() * m_dpi, m_paperInc.height() * m_dpi);
        scene()->setSceneRect(sr);
    }
}

void TemplateView::drawBackground(QPainter *painter, const QRectF &rect) {
    // Draw the dark background outside paper
    QGraphicsView::drawBackground(painter, rect);

    if (!scene()) return;

    QRectF paperRect = scene()->sceneRect();
    
    // Check for envelope rect from RefElements
    RefElement *ref1 = nullptr, *ref2 = nullptr;
    for (QGraphicsItem *item : scene()->items()) {
        if (RefElement *a = dynamic_cast<RefElement*>(item)) {
            if (!ref1) ref1 = a;
            else if (!ref2) ref2 = a;
        }
    }
    
    if (ref1 && ref2) {
        QPointF p1 = ref1->mapToScene(ref1->rect().center());
        QPointF p2 = ref2->mapToScene(ref2->rect().center());
        double minX = qMin(p1.x(), p2.x());
        double minY = qMin(p1.y(), p2.y());
        double maxX = qMax(p1.x(), p2.x());
        double maxY = qMax(p1.y(), p2.y());
        paperRect = QRectF(minX, minY, maxX - minX, maxY - minY);
    }
    
    // Draw white paper base
    painter->fillRect(paperRect, Qt::white);
    
    // Draw background image if set
    if (!m_bgImage.isEmpty()) {
        QPixmap pm(m_bgImage);
        if (!pm.isNull()) {
            painter->save();
            painter->setOpacity(m_bgOpacity);
            // Draw scaled to fit the paper rect exactly
            painter->drawPixmap(paperRect.toRect(), pm);
            painter->restore();
        }
    }
    
    // Draw crosshair or grid maybe?
    painter->setPen(QPen(Qt::lightGray, 1, Qt::DotLine));
    int gridSpacing = 10;
    for (int x = 0; x < paperRect.width(); x += gridSpacing) painter->drawLine(x, 0, x, paperRect.height());
    for (int y = 0; y < paperRect.height(); y += gridSpacing) painter->drawLine(0, y, paperRect.width(), y);

    // Draw paper border
    painter->setPen(QPen(Qt::blue, 1, Qt::DashLine));
    painter->drawRect(paperRect);
}

void TemplateView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    if (scene()) {
        fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }
}

void TemplateView::mouseMoveEvent(QMouseEvent *event) {
    QGraphicsView::mouseMoveEvent(event);
    emit mouseMoved(mapToScene(event->pos()));
}

void TemplateView::mousePressEvent(QMouseEvent *event) {
    QGraphicsView::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        emit mouseClicked(mapToScene(event->pos()));
    }
}
