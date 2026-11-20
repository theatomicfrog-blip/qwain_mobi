#include "gtkfontchooserwidget.h"
#include <QFontDatabase>
#include <QColorDialog>
#include <QDialogButtonBox>
#include <QStyleOption>
#include <QPainter>

GtkFontChooserWidget::GtkFontChooserWidget(const QFont &initialFont, const QColor &initialColor, QWidget *parent)
    : QDialog(parent), m_currentFont(initialFont), m_currentColor(initialColor)
{
    setWindowTitle(tr("Select Font"));
    setMinimumSize(550, 480);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);
    
    // Top Search / Filter
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(tr("Search font name..."));
    m_searchEdit->setClearButtonEnabled(true);
    mainLayout->addWidget(m_searchEdit);
    
    // Lists horizontal layout
    QHBoxLayout *listsLayout = new QHBoxLayout;
    listsLayout->setSpacing(8);
    
    // Family list container
    QVBoxLayout *familyLayout = new QVBoxLayout;
    familyLayout->addWidget(new QLabel(tr("Family")));
    m_familyList = new QListWidget;
    m_familyList->setMinimumHeight(150);
    familyLayout->addWidget(m_familyList, 1);
    listsLayout->addLayout(familyLayout, 3); // takes more space
    
    // Style list container
    QVBoxLayout *styleLayout = new QVBoxLayout;
    styleLayout->addWidget(new QLabel(tr("Style")));
    m_styleList = new QListWidget;
    m_styleList->setMinimumHeight(150);
    styleLayout->addWidget(m_styleList, 1.5);
    listsLayout->addLayout(styleLayout, 1.5);
    
    // Size list container
    QVBoxLayout *sizeLayout = new QVBoxLayout;
    sizeLayout->addWidget(new QLabel(tr("Size")));
    m_sizeList = new QListWidget;
    m_sizeList->setMinimumHeight(150);
    sizeLayout->addWidget(m_sizeList, 1);
    listsLayout->addLayout(sizeLayout, 1);
    
    mainLayout->addLayout(listsLayout, 1);
    
    // Size Slider and Spinbox layout
    QHBoxLayout *sizeControlLayout = new QHBoxLayout;
    sizeControlLayout->addWidget(new QLabel(tr("Size:")));
    m_sizeSlider = new QSlider(Qt::Horizontal);
    m_sizeSlider->setRange(6, 72);
    m_sizeSlider->setValue(m_currentFont.pointSize() > 0 ? m_currentFont.pointSize() : 10);
    sizeControlLayout->addWidget(m_sizeSlider, 1);
    
    m_sizeSpin = new QSpinBox;
    m_sizeSpin->setRange(6, 72);
    m_sizeSpin->setValue(m_currentFont.pointSize() > 0 ? m_currentFont.pointSize() : 10);
    sizeControlLayout->addWidget(m_sizeSpin);
    mainLayout->addLayout(sizeControlLayout);
    
    // Color selector layout (family/style/size/color)
    QHBoxLayout *colorLayout = new QHBoxLayout;
    colorLayout->addWidget(new QLabel(tr("Color:")));
    
    // Quick color palette
    QStringList paletteColors = {
        "#ffffff", "#000000", "#7f7f7f", "#ff0000", "#00ff00", 
        "#0000ff", "#ffff00", "#ff00ff", "#00ffff", "#ff5500"
    };
    for (const QString &colName : paletteColors) {
        QPushButton *colBtn = new QPushButton;
        colBtn->setFixedSize(20, 20);
        colBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #555; border-radius: 3px;").arg(colName));
        colBtn->setCursor(Qt::PointingHandCursor);
        connect(colBtn, &QPushButton::clicked, this, [this, colName]() {
            m_currentColor = QColor(colName);
            updatePreview();
        });
        colorLayout->addWidget(colBtn);
    }
    
    m_colorBtn = new QPushButton(tr("Custom Color..."));
    connect(m_colorBtn, &QPushButton::clicked, this, &GtkFontChooserWidget::chooseColor);
    colorLayout->addWidget(m_colorBtn);
    colorLayout->addStretch();
    mainLayout->addLayout(colorLayout);
    
    // Preview label
    m_previewLabel = new QLabel(tr("The quick brown fox jumps over the lazy dog. 0123456789"));
    m_previewLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(60);
    m_previewLabel->setWordWrap(true);
    mainLayout->addWidget(m_previewLabel);
    
    // Dialog buttons
    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(btnBox);
    
    // Populate standard sizes
    QStringList standardSizes = {
        "6", "7", "8", "9", "10", "11", "12", "13", "14", "16", "18", "20", "24", "28", "32", "36", "40", "48", "56", "64", "72"
    };
    m_sizeList->addItems(standardSizes);
    
    // Load all families
    populateFamilies();
    
    // Set initial values
    // Find family in list and select it
    QString initialFamily = m_currentFont.family();
    QList<QListWidgetItem*> foundFamilies = m_familyList->findItems(initialFamily, Qt::MatchExactly);
    if (!foundFamilies.isEmpty()) {
        m_familyList->setCurrentItem(foundFamilies.first());
        onFamilySelected(initialFamily);
    } else {
        if (m_familyList->count() > 0) {
            m_familyList->setCurrentRow(0);
            onFamilySelected(m_familyList->item(0)->text());
        }
    }
    
    // Select size
    selectNearestSize(m_currentFont.pointSize() > 0 ? m_currentFont.pointSize() : 10);
    
    // Connect signals
    connect(m_searchEdit, &QLineEdit::textChanged, this, &GtkFontChooserWidget::filterFamilies);
    
    connect(m_familyList, &QListWidget::currentTextChanged, this, &GtkFontChooserWidget::onFamilySelected);
    connect(m_styleList, &QListWidget::currentTextChanged, this, &GtkFontChooserWidget::onStyleSelected);
    connect(m_sizeList, &QListWidget::itemClicked, this, &GtkFontChooserWidget::onSizeListSelected);
    
    connect(m_sizeSlider, &QSlider::valueChanged, this, &GtkFontChooserWidget::onSizeSelected);
    connect(m_sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &GtkFontChooserWidget::onSizeSelected);
    
    updatePreview();
}

GtkFontChooserWidget::~GtkFontChooserWidget() {}

void GtkFontChooserWidget::populateFamilies() {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_allFamilies = QFontDatabase::families();
#else
    QFontDatabase db;
    m_allFamilies = db.families();
#endif
    m_familyList->clear();
    m_familyList->addItems(m_allFamilies);
}

void GtkFontChooserWidget::filterFamilies(const QString &text) {
    m_familyList->clear();
    if (text.isEmpty()) {
        m_familyList->addItems(m_allFamilies);
    } else {
        QStringList filtered;
        for (const QString &family : m_allFamilies) {
            if (family.contains(text, Qt::CaseInsensitive)) {
                filtered.append(family);
            }
        }
        m_familyList->addItems(filtered);
    }
    
    if (m_familyList->count() > 0) {
        m_familyList->setCurrentRow(0);
    }
}

void GtkFontChooserWidget::onFamilySelected(const QString &family) {
    if (family.isEmpty()) return;
    
    m_currentFont.setFamily(family);
    
    // Get styles for family
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QStringList styles = QFontDatabase::styles(family);
#else
    QFontDatabase db;
    QStringList styles = db.styles(family);
#endif

    QString oldStyle = "";
    if (m_styleList->currentItem()) {
        oldStyle = m_styleList->currentItem()->text();
    }
    
    m_styleList->clear();
    m_styleList->addItems(styles);
    
    // Select nearest style or default to first
    if (!oldStyle.isEmpty() && styles.contains(oldStyle)) {
        QList<QListWidgetItem*> found = m_styleList->findItems(oldStyle, Qt::MatchExactly);
        if (!found.isEmpty()) {
            m_styleList->setCurrentItem(found.first());
        }
    } else {
        // Try standard styles like "Regular" or "Normal"
        int defaultIdx = 0;
        for (int i = 0; i < styles.size(); ++i) {
            if (styles[i].contains("Regular", Qt::CaseInsensitive) || styles[i].contains("Normal", Qt::CaseInsensitive)) {
                defaultIdx = i;
                break;
            }
        }
        if (m_styleList->count() > 0) {
            m_styleList->setCurrentRow(defaultIdx);
        }
    }
    
    updatePreview();
}

void GtkFontChooserWidget::onStyleSelected(const QString &style) {
    if (style.isEmpty()) return;
    
    // Apply style to font using QFontDatabase
    QString family = m_currentFont.family();
    int size = m_currentFont.pointSize() > 0 ? m_currentFont.pointSize() : 10;
    
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_currentFont = QFontDatabase::font(family, style, size);
#else
    QFontDatabase db;
    m_currentFont = db.font(family, style, size);
#endif

    updatePreview();
}

void GtkFontChooserWidget::onSizeSelected(int size) {
    m_currentFont.setPointSize(size);
    
    // Update slider and spinbox without infinite loops
    if (m_sizeSlider->value() != size) {
        m_sizeSlider->blockSignals(true);
        m_sizeSlider->setValue(size);
        m_sizeSlider->blockSignals(false);
    }
    if (m_sizeSpin->value() != size) {
        m_sizeSpin->blockSignals(true);
        m_sizeSpin->setValue(size);
        m_sizeSpin->blockSignals(false);
    }
    
    // Select item in sizes list if matched
    m_sizeList->blockSignals(true);
    m_sizeList->clearSelection();
    QList<QListWidgetItem*> found = m_sizeList->findItems(QString::number(size), Qt::MatchExactly);
    if (!found.isEmpty()) {
        found.first()->setSelected(true);
        m_sizeList->scrollToItem(found.first());
    }
    m_sizeList->blockSignals(false);
    
    updatePreview();
}

void GtkFontChooserWidget::onSizeListSelected(QListWidgetItem *item) {
    if (!item) return;
    int size = item->text().toInt();
    onSizeSelected(size);
}

void GtkFontChooserWidget::selectNearestSize(int size) {
    onSizeSelected(size);
}

void GtkFontChooserWidget::chooseColor() {
    QColor c = QColorDialog::getColor(m_currentColor, this, tr("Select Color"));
    if (c.isValid()) {
        m_currentColor = c;
        updatePreview();
    }
}

void GtkFontChooserWidget::updatePreview() {
    m_previewLabel->setFont(m_currentFont);
    
    // Set preview text style
    QString style = QString("color: %1; background-color: #333333; border: 1px solid #555; border-radius: 4px; padding: 10px;")
                        .arg(m_currentColor.name());
    m_previewLabel->setStyleSheet(style);
}

QFont GtkFontChooserWidget::selectedFont() const {
    return m_currentFont;
}

QColor GtkFontChooserWidget::selectedColor() const {
    return m_currentColor;
}
