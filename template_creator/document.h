#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QObject>
#include <QGraphicsScene>
#include <QJsonObject>

class TemplateView;

class Document : public QObject {
    Q_OBJECT
public:
    explicit Document(QGraphicsScene *scene, TemplateView *view, QObject *parent = nullptr);
    
    bool save(const QString &filename);
    bool load(const QString &filename);
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);

private:
    QGraphicsScene *m_scene;
    TemplateView *m_view;
};

#endif // DOCUMENT_H
