#ifndef PROPERTIES_PANEL_H
#define PROPERTIES_PANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>

class BaseElement;
class TextElement;
class TableElement;

class PropertiesPanel : public QWidget {
    Q_OBJECT
public:
    explicit PropertiesPanel(QWidget *parent = nullptr);
    void setElement(BaseElement *element);

signals:
    void elementChanged();

private slots:
    void applyChanges();

private:
    void populateBaseProperties(BaseElement *el);
    void populateTextProperties(TextElement *el);
    void populateTableProperties(TableElement *el);

    QVBoxLayout *m_layout;
    BaseElement *m_currentElement;

    QLabel *m_noElementLabel;

    QGroupBox *m_baseGroup;
    QLineEdit *m_idEdit;
    QSpinBox *m_xBox;
    QSpinBox *m_yBox;
    QSpinBox *m_wBox;
    QSpinBox *m_hBox;

    QGroupBox *m_textGroup;
    QLineEdit *m_textEdit;

    QGroupBox *m_tableGroup;
    QSpinBox *m_rowsBox;
    QSpinBox *m_colsBox;
    QLineEdit *m_rhEdit;
    QLineEdit *m_cwEdit;
    class QTableWidget *m_cellTable;

    QPushButton *m_fontBtn;
    QPushButton *m_colorBtn;
};

#endif // PROPERTIES_PANEL_H
