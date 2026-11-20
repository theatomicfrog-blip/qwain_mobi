#ifndef GTKFONTCHOOSERWIDGET_H
#define GTKFONTCHOOSERWIDGET_H

#include <QDialog>
#include <QFont>
#include <QColor>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

class GtkFontChooserWidget : public QDialog {
    Q_OBJECT
public:
    explicit GtkFontChooserWidget(const QFont &initialFont, const QColor &initialColor, QWidget *parent = nullptr);
    ~GtkFontChooserWidget();

    QFont selectedFont() const;
    QColor selectedColor() const;

private slots:
    void filterFamilies(const QString &text);
    void onFamilySelected(const QString &family);
    void onStyleSelected(const QString &style);
    void onSizeSelected(int size);
    void onSizeListSelected(QListWidgetItem *item);
    void chooseColor();
    void updatePreview();

private:
    QFont m_currentFont;
    QColor m_currentColor;

    QLineEdit *m_searchEdit;
    QListWidget *m_familyList;
    QListWidget *m_styleList;
    QListWidget *m_sizeList;
    QSpinBox *m_sizeSpin;
    QSlider *m_sizeSlider;
    QPushButton *m_colorBtn;
    QLabel *m_previewLabel;
    
    QStringList m_allFamilies;
    
    void populateFamilies();
    void selectNearestSize(int size);
};

#endif // GTKFONTCHOOSERWIDGET_H
