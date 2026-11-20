#include "document.h"
#include "elements.h"
#include "template_view.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

Document::Document(QGraphicsScene *scene, TemplateView *view, QObject *parent)
    : QObject(parent), m_scene(scene), m_view(view) {}

bool Document::save(const QString &filename) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) return false;

    QJsonDocument doc(toJson());
    file.write(doc.toJson());
    return true;
}

bool Document::load(const QString &filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) return false;

    fromJson(doc.object());
    return true;
}

QJsonObject Document::toJson() const {
    QJsonObject root;
    QJsonArray items;

    for (QGraphicsItem *item : m_scene->items()) {
        BaseElement *el = dynamic_cast<BaseElement*>(item);
        if (el) {
            items.append(el->toJson());
        }
    }
    root["items"] = items;

    // Save paper size and background image
    root["paperW"] = m_view->paperSizeInInches().width();
    root["paperH"] = m_view->paperSizeInInches().height();
    root["bgImage"] = m_view->backgroundImage();
    root["bgOpacity"] = m_view->bgOpacity();
    
    return root;
}

void Document::fromJson(const QJsonObject &json) {
    for (QGraphicsItem *item : m_scene->items()) {
        if (dynamic_cast<BaseElement*>(item)) {
            m_scene->removeItem(item);
            delete item;
        }
    }

    if (json.contains("paperW") && json.contains("paperH")) {
        m_view->setPaperSizeInInches(QSizeF(json["paperW"].toDouble(), json["paperH"].toDouble()));
    }
    if (json.contains("bgImage")) {
        m_view->setBackgroundImage(json["bgImage"].toString());
    }
    if (json.contains("bgOpacity")) {
        m_view->setBgOpacity(json["bgOpacity"].toDouble());
    } else {
        m_view->setBgOpacity(1.0);
    }

    QJsonArray items = json["items"].toArray();
    for (int i=0; i<items.size(); ++i) {
        QJsonObject o = items[i].toObject();
        BaseElement::ElementType type = static_cast<BaseElement::ElementType>(o["type"].toInt());

        BaseElement *el = nullptr;
        if (type == BaseElement::TextType) el = new TextElement();
        else if (type == BaseElement::TableType) el = new TableElement();
        else if (type == BaseElement::RefType) el = new RefElement();

        if (el) {
            el->fromJson(o);
            m_scene->addItem(el);
        }
    }
}
