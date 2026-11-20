#include "properties_panel.h"
#include "elements.h"
#include <QFontDialog>
#include <QColorDialog>
#include <QFormLayout>
#include <QGroupBox>

#include <QTableWidget>
#include <QHeaderView>

PropertiesPanel::PropertiesPanel(QWidget *parent)
    : QWidget(parent), m_currentElement(nullptr)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setAlignment(Qt::AlignTop);
    
    // Create cached widgets
    m_noElementLabel = new QLabel("No element selected");
    m_layout->addWidget(m_noElementLabel);

    m_baseGroup = new QGroupBox("General");
    QFormLayout *baseFl = new QFormLayout(m_baseGroup);
    m_idEdit = new QLineEdit();
    m_xBox = new QSpinBox(); m_xBox->setRange(-9999, 9999);
    m_yBox = new QSpinBox(); m_yBox->setRange(-9999, 9999);
    m_wBox = new QSpinBox(); m_wBox->setRange(10, 9999);
    m_hBox = new QSpinBox(); m_hBox->setRange(10, 9999);
    baseFl->addRow("Identifier:", m_idEdit);
    baseFl->addRow("X Position:", m_xBox);
    baseFl->addRow("Y Position:", m_yBox);
    baseFl->addRow("Width:", m_wBox);
    baseFl->addRow("Height:", m_hBox);
    m_layout->addWidget(m_baseGroup);
    
    m_fontBtn = new QPushButton("Choose Font...");
    m_colorBtn = new QPushButton("Choose Color...");
    
    m_textGroup = new QGroupBox("Text Settings");
    QFormLayout *textFl = new QFormLayout(m_textGroup);
    m_textEdit = new QLineEdit();
    textFl->addRow("Text / Tag:", m_textEdit);
    textFl->addRow("Font:", m_fontBtn);
    textFl->addRow("Color:", m_colorBtn);
    QLabel *textHint = new QLabel("Use tags like <CALL>, <DATE> for variables.");
    textHint->setWordWrap(true);
    textHint->setStyleSheet("color: gray; font-size: 10px;");
    textFl->addRow(textHint);
    m_layout->addWidget(m_textGroup);

    m_tableGroup = new QGroupBox("Table Settings");
    QFormLayout *tableFl = new QFormLayout(m_tableGroup);
    m_rowsBox = new QSpinBox(); m_rowsBox->setRange(1, 100);
    m_colsBox = new QSpinBox(); m_colsBox->setRange(1, 20);
    m_rhEdit = new QLineEdit();
    m_rhEdit->setToolTip("Comma separated row heights (e.g. 50,50)");
    m_cwEdit = new QLineEdit();
    m_cwEdit->setToolTip("Comma separated col widths (e.g. 30,70)");
    
    m_cellTable = new QTableWidget();
    m_cellTable->horizontalHeader()->setVisible(false);
    m_cellTable->verticalHeader()->setVisible(false);
    m_cellTable->setMinimumHeight(120);

    tableFl->addRow("Rows:", m_rowsBox);
    tableFl->addRow("Cols:", m_colsBox);
    tableFl->addRow("Row Heights:", m_rhEdit);
    tableFl->addRow("Col Widths:", m_cwEdit);
    tableFl->addRow("Cell Assignment:", (QWidget*)nullptr);
    tableFl->addRow(m_cellTable);
    
    QPushButton *tableFontBtn = new QPushButton("Choose Font...");
    QPushButton *tableColorBtn = new QPushButton("Choose Color...");
    tableFl->addRow("Font:", tableFontBtn);
    tableFl->addRow("Color:", tableColorBtn);
    
    QLabel *tableHint = new QLabel("Use <ADIF_TAG> (e.g. <CALL>). Cell values correspond to rows left-to-right.");
    tableHint->setWordWrap(true);
    tableHint->setStyleSheet("color: gray; font-size: 10px;");
    tableFl->addRow(tableHint);
    m_layout->addWidget(m_tableGroup);

    m_layout->addStretch();

    // Hide initially
    m_baseGroup->hide();
    m_textGroup->hide();
    m_tableGroup->hide();

    // Connections
    connect(m_idEdit, &QLineEdit::editingFinished, this, &PropertiesPanel::applyChanges);
    connect(m_xBox, &QSpinBox::editingFinished, this, &PropertiesPanel::applyChanges);
    connect(m_yBox, &QSpinBox::editingFinished, this, &PropertiesPanel::applyChanges);
    connect(m_wBox, &QSpinBox::editingFinished, this, &PropertiesPanel::applyChanges);
    connect(m_hBox, &QSpinBox::editingFinished, this, &PropertiesPanel::applyChanges);
    
    connect(m_textEdit, &QLineEdit::editingFinished, this, &PropertiesPanel::applyChanges);
    
    auto fontAction = [this]() {
        if (!m_currentElement) return;
        bool ok;
        QFont initialFont;
        if (m_currentElement->type() == BaseElement::TextType) initialFont = static_cast<TextElement*>(m_currentElement)->font();
        else if (m_currentElement->type() == BaseElement::TableType) initialFont = static_cast<TableElement*>(m_currentElement)->font();
        
        QFont font = QFontDialog::getFont(&ok, initialFont, this);
        if (ok) {
            if (m_currentElement->type() == BaseElement::TextType) static_cast<TextElement*>(m_currentElement)->setFont(font);
            else if (m_currentElement->type() == BaseElement::TableType) static_cast<TableElement*>(m_currentElement)->setFont(font);
            emit elementChanged();
        }
    };

    auto colorAction = [this]() {
        if (!m_currentElement) return;
        QColor initialColor;
        if (m_currentElement->type() == BaseElement::TextType) initialColor = static_cast<TextElement*>(m_currentElement)->color();
        else if (m_currentElement->type() == BaseElement::TableType) initialColor = static_cast<TableElement*>(m_currentElement)->color();
        
        QColor color = QColorDialog::getColor(initialColor, this);
        if (color.isValid()) {
            if (m_currentElement->type() == BaseElement::TextType) static_cast<TextElement*>(m_currentElement)->setColor(color);
            else if (m_currentElement->type() == BaseElement::TableType) static_cast<TableElement*>(m_currentElement)->setColor(color);
            emit elementChanged();
        }
    };

    connect(m_fontBtn, &QPushButton::clicked, this, fontAction);
    connect(tableFontBtn, &QPushButton::clicked, this, fontAction);
    
    connect(m_colorBtn, &QPushButton::clicked, this, colorAction);
    connect(tableColorBtn, &QPushButton::clicked, this, colorAction);

    connect(m_rowsBox, &QSpinBox::editingFinished, this, &PropertiesPanel::applyChanges);
    connect(m_colsBox, &QSpinBox::editingFinished, this, &PropertiesPanel::applyChanges);
    connect(m_rhEdit, &QLineEdit::editingFinished, this, &PropertiesPanel::applyChanges);
    connect(m_cwEdit, &QLineEdit::editingFinished, this, &PropertiesPanel::applyChanges);
    connect(m_cellTable, &QTableWidget::cellChanged, this, &PropertiesPanel::applyChanges);
}

void PropertiesPanel::setElement(BaseElement *element) {
    m_currentElement = element;
    
    m_noElementLabel->setVisible(!element);
    m_baseGroup->setVisible(element != nullptr);
    m_textGroup->setVisible(element && element->type() == BaseElement::TextType);
    m_tableGroup->setVisible(element && element->type() == BaseElement::TableType);
    
    if (!element) {
        return;
    }
    
    populateBaseProperties(element);
    
    if (element->type() == BaseElement::TextType) {
        populateTextProperties(static_cast<TextElement*>(element));
    } else if (element->type() == BaseElement::TableType) {
        populateTableProperties(static_cast<TableElement*>(element));
    }
}

void PropertiesPanel::applyChanges() {
    if (!m_currentElement) return;
    
    m_currentElement->setElementId(m_idEdit->text());
    m_currentElement->setPos(m_xBox->value(), m_yBox->value());
    m_currentElement->setRect(QRectF(0, 0, m_wBox->value(), m_hBox->value()));
    
    if (m_currentElement->type() == BaseElement::TextType) {
        static_cast<TextElement*>(m_currentElement)->setText(m_textEdit->text());
    } else if (m_currentElement->type() == BaseElement::TableType) {
        TableElement *te = static_cast<TableElement*>(m_currentElement);
        // Disconnect to prevent recursive calls
        m_cellTable->blockSignals(true);
        if (m_rowsBox->value() != te->rows() || m_colsBox->value() != te->columns()) {
            te->setRows(m_rowsBox->value());
            te->setColumns(m_colsBox->value());
            // Need to rebuild UI because size changed
            m_cellTable->setRowCount(te->rows());
            m_cellTable->setColumnCount(te->columns());
        }
        
        QStringList cells;
        for (int r = 0; r < te->rows(); ++r) {
            for (int c = 0; c < te->columns(); ++c) {
                QTableWidgetItem *item = m_cellTable->item(r, c);
                cells << (item ? item->text() : "");
            }
        }
        te->setCellData(cells);
        te->setRowHeightsStr(m_rhEdit->text());
        te->setColWidthsStr(m_cwEdit->text());
        m_cellTable->blockSignals(false);
    }
    
    emit elementChanged();
}

void PropertiesPanel::populateBaseProperties(BaseElement *el) {
    m_idEdit->setText(el->elementId());
    m_xBox->setValue(el->pos().x());
    m_yBox->setValue(el->pos().y());
    m_wBox->setValue(el->rect().width());
    m_hBox->setValue(el->rect().height());
}

void PropertiesPanel::populateTextProperties(TextElement *el) {
    m_textEdit->setText(el->text());
}

void PropertiesPanel::populateTableProperties(TableElement *el) {
    m_rowsBox->blockSignals(true);
    m_colsBox->blockSignals(true);
    m_cellTable->blockSignals(true);
    m_rhEdit->blockSignals(true);
    m_cwEdit->blockSignals(true);
    
    m_rowsBox->setValue(el->rows());
    m_colsBox->setValue(el->columns());
    
    m_rhEdit->setText(el->rowHeightsStr());
    m_cwEdit->setText(el->colWidthsStr());
    
    m_cellTable->setRowCount(el->rows());
    m_cellTable->setColumnCount(el->columns());
    
    QStringList cells = el->cellData();
    for (int r = 0; r < el->rows(); ++r) {
        for (int c = 0; c < el->columns(); ++c) {
            int idx = r * el->columns() + c;
            QString txt = (idx < cells.size()) ? cells[idx] : "";
            QTableWidgetItem *item = new QTableWidgetItem(txt);
            m_cellTable->setItem(r, c, item);
        }
    }
    
    m_rowsBox->blockSignals(false);
    m_colsBox->blockSignals(false);
    m_cellTable->blockSignals(false);
    m_rhEdit->blockSignals(false);
    m_cwEdit->blockSignals(false);
}
