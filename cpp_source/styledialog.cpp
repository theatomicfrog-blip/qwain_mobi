#include "styledialog.h"
#include <QDialogButtonBox>

// ============================================================================
// COMPONENT: StyleDialog
// ROLE: Typography, color theme, and orientation transformation picker.
//       Presents font families, sizes, styles (bold/italic), palette dialogues,
//       and rotation angle parameters with real-time on-the-fly CSS previews.
// INVOKED BY: QslDesigner (when editing template fields).
// PRODUCES: Selected fonts, colors, rotations, and interactive property signals.
// ============================================================================

// [ENTRY POINT: Style Selection Constructor]
// Configures layouts, connects comboboxes to live CSS update loops, and registers 
// persistent undo buttons to revert properties back to original settings.
StyleDialog::StyleDialog(const QFont &initialFont, const QColor &initialColor, QWidget *parent, bool colorOnly, bool showRotation, double initialRotation, bool showSaveToTemplate)
    : QDialog(parent), m_initialFont(initialFont), m_initialColor(initialColor),
      m_showRotation(showRotation), m_currentRotation(initialRotation), m_initialRotation(initialRotation)
{
    if (colorOnly) {
        setWindowTitle("Select Color");
    } else {
        setWindowTitle("Select Font & Color");
    }
    
    m_currentFont = initialFont;
    m_currentColor = initialColor;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *fontGroup = new QGroupBox("Font Settings");
    QFormLayout *flayout = new QFormLayout(fontGroup);

    m_fontCombo = new QFontComboBox;
    m_fontCombo->setCurrentFont(m_currentFont);
    
    m_sizeSpin = new QSpinBox;
    m_sizeSpin->setRange(6, 72);
    m_sizeSpin->setValue(m_currentFont.pointSize() > 0 ? m_currentFont.pointSize() : 10);
    
    m_boldCheck = new QCheckBox("Bold");
    m_boldCheck->setChecked(m_currentFont.bold());
    
    m_italicCheck = new QCheckBox("Italic");
    m_italicCheck->setChecked(m_currentFont.italic());

    flayout->addRow("Family:", m_fontCombo);
    flayout->addRow("Size:", m_sizeSpin);
    
    QHBoxLayout *styleLayout = new QHBoxLayout;
    styleLayout->addWidget(m_boldCheck);
    styleLayout->addWidget(m_italicCheck);
    flayout->addRow("Style:", styleLayout);

    m_colorBtn = new QPushButton("Change Color...");
    flayout->addRow("Color:", m_colorBtn);

    m_rotSpin = nullptr;
    if (m_showRotation) {
        m_rotSpin = new QSpinBox(this);
        m_rotSpin->setRange(-360, 360);
        m_rotSpin->setValue((int)m_currentRotation);
        flayout->addRow("Rotation (deg):", m_rotSpin);
        
        connect(m_rotSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val){
            m_currentRotation = val;
            updatePreview();
        });
    }

    if (!colorOnly) {
        mainLayout->addWidget(fontGroup);
    } else {
        QGroupBox *colorGroup = new QGroupBox("Color Settings");
        QFormLayout *clayout = new QFormLayout(colorGroup);
        clayout->addRow("Color:", m_colorBtn);
        mainLayout->addWidget(colorGroup);
    }

    QGroupBox *previewGroup = new QGroupBox("Preview");
    QVBoxLayout *playout = new QVBoxLayout(previewGroup);
    m_previewLabel = new QLabel("The quick brown fox jumps over the lazy dog\n0123456789");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(60);
    playout->addWidget(m_previewLabel);
    
    mainLayout->addWidget(previewGroup);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QPushButton *undoBtn = new QPushButton("Undo");
    btnBox->addButton(undoBtn, QDialogButtonBox::ResetRole);
    if (showSaveToTemplate) {
        QPushButton *saveBtn = new QPushButton("OK/Save to Template");
        btnBox->addButton(saveBtn, QDialogButtonBox::ApplyRole);
        connect(saveBtn, &QPushButton::clicked, this, [this](){
            done(2); // Custom result code for Saving to Template
        });
    }
    mainLayout->addWidget(btnBox);

    connect(m_fontCombo, &QFontComboBox::currentFontChanged, this, [this](const QFont &f){
        m_currentFont.setFamily(f.family());
        updatePreview();
    });
    connect(m_sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val){
        m_currentFont.setPointSize(val);
        updatePreview();
    });
    connect(m_boldCheck, &QCheckBox::toggled, this, [this](bool b){
        m_currentFont.setBold(b);
        updatePreview();
    });
    connect(m_italicCheck, &QCheckBox::toggled, this, [this](bool b){
        m_currentFont.setItalic(b);
        updatePreview();
    });
    connect(m_colorBtn, &QPushButton::clicked, this, &StyleDialog::pickColor);

    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(undoBtn, &QPushButton::clicked, this, [this](){
        m_currentFont = m_initialFont;
        m_currentColor = m_initialColor;
        m_currentRotation = m_initialRotation;
        m_fontCombo->setCurrentFont(m_currentFont);
        m_sizeSpin->setValue(m_currentFont.pointSize() > 0 ? m_currentFont.pointSize() : 10);
        m_boldCheck->setChecked(m_currentFont.bold());
        m_italicCheck->setChecked(m_currentFont.italic());
        if (m_rotSpin) {
            m_rotSpin->setValue((int)m_currentRotation);
        }
        updatePreview();
    });

    updatePreview();
}

void StyleDialog::pickColor() {
    QColor c = QColorDialog::getColor(m_currentColor, this, "Select Color");
    if (c.isValid()) {
        m_currentColor = c;
        updatePreview();
    }
}

// [ENTRY POINT: Real-time UI Stylesheet Invalidator]
// Compiles font, weight, alignment, and color choices into a live stylesheet string, 
// updating the preview box and firing real-time signals to the visual designer canvas.
void StyleDialog::updatePreview() {
    m_previewLabel->setFont(m_currentFont);
    m_previewLabel->setAutoFillBackground(true);
    
    QString fontStyle = QString("font-family: '%1'; font-size: %2pt; font-weight: %3; font-style: %4;")
        .arg(m_currentFont.family())
        .arg(m_currentFont.pointSize() > 0 ? m_currentFont.pointSize() : 10)
        .arg(m_currentFont.bold() ? "bold" : "normal")
        .arg(m_currentFont.italic() ? "italic" : "normal");

    m_previewLabel->setStyleSheet(QString("QLabel { %1 color: %2; background-color: #333333; border: 1px solid gray; border-radius: 4px; padding: 10px; }")
        .arg(fontStyle)
        .arg(m_currentColor.name()));
    emit styleChanged(m_currentFont, m_currentColor);
    emit styleChangedWithRotation(m_currentFont, m_currentColor, m_currentRotation);
}

QFont StyleDialog::selectedFont() const {
    return m_currentFont;
}

QColor StyleDialog::selectedColor() const {
    return m_currentColor;
}

double StyleDialog::selectedRotation() const {
    return m_currentRotation;
}
