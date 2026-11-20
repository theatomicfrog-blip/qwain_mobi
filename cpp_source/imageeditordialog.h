// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef IMAGEEDITORDIALOG_H
#define IMAGEEDITORDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

class QSpinBox;
class QLineEdit;
class QComboBox;

class ImageEditorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ImageEditorDialog(const QString &filePath, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void addField();
    void saveFields();
    void saveSize();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void loadFields();
    
    QString m_filePath;
    QTableWidget *m_exifTable;
    QGraphicsView *m_view;
    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_pixmapItem;
    
    QLineEdit *m_widthEdit;
    QLineEdit *m_heightEdit;
    QComboBox *m_unitCombo;
    
    QPointF m_click1;
    QPointF m_click2;
    int m_clickCount;
    QGraphicsRectItem *m_selRect;
};

#endif // IMAGEEDITORDIALOG_H
