// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#include <QCoreApplication>
#include "imageeditordialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QGraphicsRectItem>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>

ImageEditorDialog::ImageEditorDialog(const QString &filePath, QWidget *parent)
    : QDialog(parent), m_filePath(filePath), m_clickCount(0), m_selRect(nullptr)
{
    setWindowTitle("Image Editor & Fields");
    resize(1000, 700);
    
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    
    // Left: Image View
    QVBoxLayout *leftLayout = new QVBoxLayout();
    m_view = new QGraphicsView(this);
    m_scene = new QGraphicsScene(this);
    m_view->setScene(m_scene);
    
    QPixmap pm(m_filePath);
    m_pixmapItem = m_scene->addPixmap(pm);
    m_view->fitInView(m_pixmapItem, Qt::KeepAspectRatio);
    m_view->scale(0.75, 0.75);
    
    m_view->viewport()->installEventFilter(this);
    
    leftLayout->addWidget(m_view);
    
    // Right: Exif/Data fields
    QVBoxLayout *rightLayout = new QVBoxLayout();
    
    // Size Controls
    rightLayout->addWidget(new QLabel("<b>Physical Size (for Designer):</b>"));
    QHBoxLayout *sizeLayout = new QHBoxLayout();
    m_widthEdit = new QLineEdit();
    m_widthEdit->setPlaceholderText("Width");
    m_heightEdit = new QLineEdit();
    m_heightEdit->setPlaceholderText("Height");
    m_unitCombo = new QComboBox();
    m_unitCombo->addItems({"in", "cm"});
    sizeLayout->addWidget(m_widthEdit);
    sizeLayout->addWidget(new QLabel("x"));
    sizeLayout->addWidget(m_heightEdit);
    sizeLayout->addWidget(m_unitCombo);
    rightLayout->addLayout(sizeLayout);
    
    QPushButton *btnSaveSize = new QPushButton("Save Size");
    rightLayout->addWidget(btnSaveSize);
    
    rightLayout->addWidget(new QLabel("<b>EXIF / Tag Properties</b>"));
    
    m_exifTable = new QTableWidget(this);
    m_exifTable->setColumnCount(2);
    m_exifTable->setHorizontalHeaderLabels({"Property", "Value"});
    m_exifTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    rightLayout->addWidget(m_exifTable);
    
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnAdd = new QPushButton("Add Field");
    QPushButton *btnSave = new QPushButton("Save All");
    btnLayout->addWidget(btnAdd);
    btnLayout->addWidget(btnSave);
    rightLayout->addLayout(btnLayout);
    
    mainLayout->addLayout(leftLayout, 2);
    mainLayout->addLayout(rightLayout, 1);
    
    connect(btnSaveSize, &QPushButton::clicked, this, &ImageEditorDialog::saveSize);
    
    connect(btnAdd, &QPushButton::clicked, this, &ImageEditorDialog::addField);
    connect(btnSave, &QPushButton::clicked, this, &ImageEditorDialog::saveFields);
    
    loadFields();
    
    QSettings settings(QCoreApplication::organizationName(), "Qwain_EXIF");
    
    QString calibDims = settings.value(m_filePath + "_CalibDims", "").toString();
    if (!calibDims.isEmpty()) {
        QString d = calibDims.toLower();
        if (d.contains("cm")) m_unitCombo->setCurrentText("cm");
        else m_unitCombo->setCurrentText("in");
        d.remove("in").remove("ches").remove("cm").remove(" ");
        QStringList parts = d.split("x");
        if (parts.size() == 2) {
            m_widthEdit->setText(parts[0]);
            m_heightEdit->setText(parts[1]);
        }
    }
}

bool ImageEditorDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_view->viewport()) {
        if (event->type() == QEvent::Wheel) {
            QWheelEvent *we = static_cast<QWheelEvent*>(event);
            if (we->angleDelta().y() > 0) m_view->scale(1.1, 1.1);
            else m_view->scale(1.0/1.1, 1.0/1.1);
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void ImageEditorDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    m_view->fitInView(m_pixmapItem, Qt::KeepAspectRatio);
}

void ImageEditorDialog::saveSize()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain_EXIF");
    
    double w = m_widthEdit->text().toDouble();
    double h = m_heightEdit->text().toDouble();
    if (w > 0 && h > 0) {
        QString unit = m_unitCombo->currentText();
        settings.setValue(m_filePath + "_CalibDims", QString("%1%2x%3%4").arg(w).arg(unit).arg(h).arg(unit));
        QMessageBox::information(this, "Saved", "Size saved.");
    } else {
        settings.remove(m_filePath + "_CalibDims");
        QMessageBox::information(this, "Saved", "Physical size was cleared/invalid.");
    }
}

void ImageEditorDialog::loadFields()
{
    m_exifTable->setRowCount(0);
    QSettings settings(QCoreApplication::organizationName(), "Qwain_EXIF");
    settings.beginGroup(m_filePath);
    QStringList keys = settings.childKeys();
    
    if (keys.isEmpty()) {
        // Prepopulate some default generic fields
        keys << "Date" << "Camera" << "Location";
        settings.setValue("Date", "2023-01-01");
        settings.setValue("Camera", "Unknown");
        settings.setValue("Location", "Unknown");
    }
    
    for (const QString &k : keys) {
        if (k.endsWith("_Orientation") || k.endsWith("_CalibRect") || k.endsWith("_CalibDims")) continue;
        int row = m_exifTable->rowCount();
        m_exifTable->insertRow(row);
        m_exifTable->setItem(row, 0, new QTableWidgetItem(k));
        m_exifTable->setItem(row, 1, new QTableWidgetItem(settings.value(k).toString()));
    }
    settings.endGroup();
}

void ImageEditorDialog::addField()
{
    int row = m_exifTable->rowCount();
    m_exifTable->insertRow(row);
    m_exifTable->setItem(row, 0, new QTableWidgetItem("New_Property"));
    m_exifTable->setItem(row, 1, new QTableWidgetItem(""));
}

void ImageEditorDialog::saveFields()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain_EXIF");
    settings.beginGroup(m_filePath);
    for (int i = 0; i < m_exifTable->rowCount(); ++i) {
        QTableWidgetItem *itemK = m_exifTable->item(i, 0);
        QTableWidgetItem *itemV = m_exifTable->item(i, 1);
        if (itemK && itemV) {
            settings.setValue(itemK->text(), itemV->text());
        }
    }
    settings.endGroup();
    QMessageBox::information(this, "Saved", "Saved Exif / properties.");
}
