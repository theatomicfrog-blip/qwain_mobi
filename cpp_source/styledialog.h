#ifndef STYLEDIALOG_H
#define STYLEDIALOG_H

#include <QDialog>
#include <QFont>
#include <QColor>
#include <QFontComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QColorDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

class StyleDialog : public QDialog {
    Q_OBJECT
public:
    explicit StyleDialog(const QFont &initialFont, const QColor &initialColor, QWidget *parent = nullptr, bool colorOnly = false, bool showRotation = false, double initialRotation = 0.0, bool showSaveToTemplate = false);
    
    QFont selectedFont() const;
    QColor selectedColor() const;
    double selectedRotation() const;

signals:
    void styleChanged(const QFont &font, const QColor &color);
    void styleChangedWithRotation(const QFont &font, const QColor &color, double rotation);

private:
    QFont m_currentFont;
    QColor m_currentColor;
    QFont m_initialFont;
    QColor m_initialColor;
    bool m_showRotation;
    double m_currentRotation;
    double m_initialRotation;

    QFontComboBox *m_fontCombo;
    QSpinBox *m_sizeSpin;
    QCheckBox *m_boldCheck;
    QCheckBox *m_italicCheck;
    QPushButton *m_colorBtn;
    QSpinBox *m_rotSpin;
    QLabel *m_previewLabel;

    void updatePreview();
    void pickColor();
};

#endif // STYLEDIALOG_H
