// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QDialog>
#include <QListWidget>

class ImageViewer : public QDialog
{
    Q_OBJECT
public:
    explicit ImageViewer(bool pickerMode = false, QWidget *parent = nullptr);
    QString selectedImagePath() const { return m_selectedPath; }

    // Dynamic format detection utilities
    static QStringList getSupportedImageExtensions();
    static QString getSupportedFilterString();

private slots:
    void uploadLocalImage();
    void uploadUrlImage();
    void removeSelectedImage();
    void onItemDoubleClicked(QListWidgetItem *item);
    void onItemEntered(QListWidgetItem *item);
    void refreshGallery();
    void updateColorPreview(const QColor &col);
    void saveColorToHistory(const QColor &col);

private:
    QListWidget *m_galleryList;
    class QLabel *m_previewLabel;
    QString m_imagesDir;
    bool m_pickerMode;
    QString m_selectedPath;
    class QCheckBox *m_useSolidColorChk = nullptr;
    class QColorDialog *m_colorWheel = nullptr;
    class QHBoxLayout *m_historyLayout = nullptr;
};

#endif // IMAGEVIEWER_H
