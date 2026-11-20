// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "imageviewer.h"
#include "imageeditordialog.h"
#include "paths.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QInputDialog>
#include <QDir>
#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QMessageBox>
#include <QSettings>
#include <QCoreApplication>
#include <QLabel>
#include <QGridLayout>
#include <QPixmapCache>
#include <QListView>
#include <QFileIconProvider>
#include <QImageReader>
#include <QBuffer>
#include <QSet>
#include <QDateTime>

// ============================================================================
// COMPONENT: ImageViewer
// ROLE: Visual media repository, asset importer, and template background picker.
//       This component indexes downloaded raster and vector background assets,
//       facilitates local import with real-time grid preview, supports dynamic
//       format detection for URL downloading and local disk imports, and provides
//       picker/picker-bypass pathways to configure QSL layout designs.
// INVOKED BY: QslDesigner (when configuring backgrounds), or MainWindow.
// PRODUCES: Selected image paths, unlinked items, or spooled image editor instances.
// ============================================================================

// [HELPER: Dynamic Image Format Discovery]
// Dynamically discovers all image formats supported by the runtime Qt framework
// and its installed image format plugins (via QImageReader::supportedImageFormats()),
// normalizes alias extensions (e.g. jpeg/jpg, tiff/tif, svg/svgz), and merges a
// comprehensive baseline of common graphic formats to ensure maximum file compatibility.
QStringList ImageViewer::getSupportedImageExtensions()
{
    QSet<QString> extensions;

    // Discover all formats supported by installed Qt image format plugins at runtime
    const QByteArrayList formats = QImageReader::supportedImageFormats();
    for (const QByteArray &fmt : formats) {
        QString ext = QString::fromLatin1(fmt).toLower().trimmed();
        if (!ext.isEmpty()) {
            extensions.insert(ext);
            if (ext == "jpeg") extensions.insert("jpg");
            if (ext == "jpg") extensions.insert("jpeg");
            if (ext == "tiff") extensions.insert("tif");
            if (ext == "tif") extensions.insert("tiff");
            if (ext == "svg") extensions.insert("svgz");
        }
    }

    // Ensure universal baseline common formats are always supported
    static const QStringList baseline = {
        "png", "jpg", "jpeg", "bmp", "webp", "gif", "svg", "svgz",
        "tiff", "tif", "ico", "pbm", "pgm", "ppm", "xbm", "xpm"
    };
    for (const QString &b : baseline) {
        extensions.insert(b);
    }

    QStringList result = extensions.values();
    result.sort();
    return result;
}

// [HELPER: Dynamic File Dialog Filter Generator]
// Constructs a comprehensive filter string for QFileDialog based on the dynamically
// detected image formats supported by the runtime environment.
QString ImageViewer::getSupportedFilterString()
{
    QStringList exts = getSupportedImageExtensions();
    QStringList wildcards;
    for (const QString &ext : exts) {
        wildcards << "*." + ext;
    }

    return QString("All Supported Images (%1);;All Files (*)").arg(wildcards.join(" "));
}

// [CUSTOM ICON PROVIDER: Dynamic Grid Thumbnail Renderer]
// Dynamically renders thumbnail icons in the QFileDialog file grid for any image format
// supported by Qt or the dynamic formats list, with smooth scaling and fallback mechanisms.
class ThumbnailIconProvider : public QFileIconProvider {
public:
    QIcon icon(const QFileInfo &info) const override {
        QString ext = info.suffix().toLower();
        // Dynamically match against all runtime supported image formats
        if (info.isFile() && ImageViewer::getSupportedImageExtensions().contains(ext)) {
            QImageReader reader(info.absoluteFilePath());
            reader.setScaledSize(QSize(128, 128));
            QImage img = reader.read();
            if (!img.isNull()) {
                return QIcon(QPixmap::fromImage(img));
            }
            // Fallback for vector or plugin-managed formats (e.g. SVG)
            QPixmap pix(info.absoluteFilePath());
            if (!pix.isNull()) {
                return QIcon(pix.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
        return QFileIconProvider::icon(info);
    }
    
    QIcon icon(QFileIconProvider::IconType type) const override {
        return QFileIconProvider::icon(type);
    }
};

// [ENTRY POINT: Image Viewer Constructor]
// Configures dialog dimensions, prepares directory structures, and instantiates 
// grid/list layouts based on picker mode settings before refreshing the visual gallery.
ImageViewer::ImageViewer(bool pickerMode, QWidget *parent) : QDialog(parent), m_pickerMode(pickerMode)
{
    setWindowTitle(pickerMode ? "Select Image" : "Image Viewer");
    resize(800, 600);
    
    m_imagesDir = getAppDataDir() + "/images";
    QDir().mkpath(m_imagesDir);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    QHBoxLayout *topLayout = new QHBoxLayout();
    QPushButton *btnLocal = new QPushButton("Import");
    QPushButton *btnUrl = new QPushButton("Import from URL");
    QPushButton *btnRemove = new QPushButton("Remove Selected");
    QPushButton *btnRefresh = new QPushButton("Refresh");
    topLayout->addWidget(btnLocal);
    topLayout->addWidget(btnUrl);
    topLayout->addWidget(btnRemove);
    topLayout->addWidget(btnRefresh);
    if (pickerMode) {
        QPushButton *btnRemoveBg = new QPushButton("Remove Current BG");
        topLayout->addWidget(btnRemoveBg);
        connect(btnRemoveBg, &QPushButton::clicked, this, [this]() {
            m_selectedPath = ":REMOVE_BG:";
            accept();
        });
    }
    topLayout->addStretch();
    
    mainLayout->addLayout(topLayout);
    
    QHBoxLayout *contentLayout = new QHBoxLayout();
    
    m_galleryList = new QListWidget(this);
    m_galleryList->setViewMode(QListView::IconMode);
    m_galleryList->setIconSize(QSize(200, 200));
    m_galleryList->setGridSize(QSize(220, 240));
    m_galleryList->setResizeMode(QListView::Adjust);
    m_galleryList->setSpacing(10);
    contentLayout->addWidget(m_galleryList, 1);
    
    m_previewLabel = nullptr;

    mainLayout->addLayout(contentLayout);
    
    connect(btnLocal, &QPushButton::clicked, this, &ImageViewer::uploadLocalImage);
    connect(btnUrl, &QPushButton::clicked, this, &ImageViewer::uploadUrlImage);
    connect(btnRemove, &QPushButton::clicked, this, &ImageViewer::removeSelectedImage);
    connect(btnRefresh, &QPushButton::clicked, this, &ImageViewer::refreshGallery);
    connect(m_galleryList, &QListWidget::itemDoubleClicked, this, &ImageViewer::onItemDoubleClicked);
    connect(m_galleryList, &QListWidget::itemClicked, this, &ImageViewer::onItemEntered);
    
    refreshGallery();
}

// [ENTRY POINT: Disk Directory Gallery Indexer]
// Clears list views, crawls the local cached images subdirectory for all dynamically
// supported formats (PNG, JPG, BMP, WEBP, GIF, SVG, TIFF, etc.), scales items to thumbnail
// icons with fallback renderers, and binds absolute paths.
void ImageViewer::refreshGallery()
{
    m_galleryList->clear();
    QDir dir(m_imagesDir);
    QStringList filters;
    const QStringList exts = getSupportedImageExtensions();
    for (const QString &ext : exts) {
        filters << "*." + ext;
    }
    
    QFileInfoList list = dir.entryInfoList(filters, QDir::Files, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo &f : list) {
        QPixmap pixmap(f.absoluteFilePath());
        if (pixmap.isNull()) {
            // Fallback via QImageReader for specific raster formats
            QImageReader reader(f.absoluteFilePath());
            QImage img = reader.read();
            if (!img.isNull()) {
                pixmap = QPixmap::fromImage(img);
            }
        }
        if (!pixmap.isNull()) {
            QPixmap thumbnail = pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QListWidgetItem *item = new QListWidgetItem(QIcon(thumbnail), f.fileName());
            item->setData(Qt::UserRole, f.absoluteFilePath());
            m_galleryList->addItem(item);
        }
    }
}

// [ENTRY POINT: Local File Dialog Import Gateway]
// Configures and spools a custom non-native QFileDialog with a custom grid view 
// icon provider and dynamic format filter string, copying selected targets into the app directory.
void ImageViewer::uploadLocalImage()
{
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString lastDir = settings.value("LastImageUploadDir", QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)).toString();
    QStringList history = settings.value("ImageUploadHistory").toStringList();

    // Dynamically build filter string accommodating all supported formats
    QFileDialog dialog(this, "Select Image", lastDir, getSupportedFilterString());
    dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setHistory(history);
    
    ThumbnailIconProvider provider;
    dialog.setIconProvider(&provider);
    
    QLabel *previewLabel = new QLabel(&dialog);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setFixedSize(250, 250);
    previewLabel->setStyleSheet("border: 1px solid #444; background-color: #222; color: #aaa;");
    previewLabel->setText("No Preview");

    if (QGridLayout *grid = qobject_cast<QGridLayout*>(dialog.layout())) {
        grid->addWidget(previewLabel, 1, 3, 3, 1);
    }
    
    QListView *listView = dialog.findChild<QListView *>("listView");
    if (listView) {
        listView->setViewMode(QListView::IconMode);
        listView->setIconSize(QSize(128, 128));
        listView->setGridSize(QSize(140, 140));
        listView->setWordWrap(true);
    }

    connect(&dialog, &QFileDialog::currentChanged, [previewLabel](const QString &path) {
        QPixmap pixmap(path);
        if (pixmap.isNull()) {
            QImageReader reader(path);
            QImage img = reader.read();
            if (!img.isNull()) {
                pixmap = QPixmap::fromImage(img);
            }
        }
        if (!pixmap.isNull()) {
            previewLabel->setPixmap(pixmap.scaled(250, 250, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            previewLabel->clear();
            previewLabel->setText("No Preview");
        }
    });

    if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty()) {
        QString file = dialog.selectedFiles().first();
        QFileInfo fi(file);
        
        settings.setValue("LastImageUploadDir", fi.absolutePath());
        QStringList currHist = dialog.history();
        settings.setValue("ImageUploadHistory", currHist);
        
        QString dest = m_imagesDir + "/" + fi.fileName();
        if (QFile::exists(dest)) QFile::remove(dest); // clear old file
        QFile::copy(file, dest);
        
        QPixmapCache::clear(); // fix caching issue in the pane
        refreshGallery();
    }
}

// [ENTRY POINT: Remote URL Asset Downloader with Dynamic Format Detection]
// Prompts the user for an image URL, configures HTTP redirect following and a standard User-Agent,
// fetches the payload synchronously, dynamically detects format from binary content / magic bytes
// or content headers, resolves clean filenames even with complex query strings, and updates the gallery.
void ImageViewer::uploadUrlImage()
{
    bool ok;
    QString urlStr = QInputDialog::getText(this, "Import from URL", "Image URL:", QLineEdit::Normal, "", &ok);
    if (!ok || urlStr.trimmed().isEmpty()) return;
    
    QUrl url(urlStr.trimmed());
    if (!url.isValid() || (url.scheme().toLower() != "http" && url.scheme().toLower() != "https")) {
        QMessageBox::warning(this, "Invalid URL", "Please enter a valid HTTP or HTTPS image URL.");
        return;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    
    // Enable HTTP redirect following (crucial for modern CDNs, Imgur, Unsplash, Wikimedia, GitHub raw)
    // In Qt 5.14.0+, QNetworkRequest::FollowRedirectsAttribute was deprecated in favor of RedirectPolicyAttribute.
    // We check Qt version to ensure clean compilation without deprecation warnings on Qt 5.14/5.15 and Qt 6+.
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif

    // Supply a browser-like User-Agent to prevent anti-bot rejections from image CDNs
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) Qwain/1.0 ImageFetcher");

    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, "Error", "Failed to download image: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    // Inspect HTTP status code if available
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode >= 400) {
        QMessageBox::warning(this, "Error", QString("HTTP error %1 received while downloading image.").arg(statusCode));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        QMessageBox::warning(this, "Error", "Downloaded image payload is empty.");
        reply->deleteLater();
        return;
    }

    // Dynamic format detection using magic bytes from the downloaded byte stream
    QBuffer buffer(&data);
    buffer.open(QIODevice::ReadOnly);
    QByteArray detectedFormat = QImageReader::imageFormat(&buffer);
    QString detectedExt = QString::fromLatin1(detectedFormat).toLower();

    // If QImageReader didn't recognize format directly, inspect Content-Type header
    if (detectedExt.isEmpty()) {
        QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString().toLower();
        if (contentType.contains("image/png")) detectedExt = "png";
        else if (contentType.contains("image/jpeg") || contentType.contains("image/jpg")) detectedExt = "jpg";
        else if (contentType.contains("image/webp")) detectedExt = "webp";
        else if (contentType.contains("image/gif")) detectedExt = "gif";
        else if (contentType.contains("image/bmp")) detectedExt = "bmp";
        else if (contentType.contains("image/svg")) detectedExt = "svg";
        else if (contentType.contains("image/tiff")) detectedExt = "tiff";
    }

    // Also check if payload contains XML/SVG header if not detected
    if (detectedExt.isEmpty()) {
        QByteArray trimmedData = data.trimmed();
        if (trimmedData.startsWith("<?xml") || trimmedData.startsWith("<svg") || trimmedData.contains("<svg")) {
            detectedExt = "svg";
        }
    }

    // Extract clean file name from URL path (stripping query strings and fragment)
    QString rawFileName = QFileInfo(url.path()).fileName();
    QString baseName;
    QString urlExt;
    
    if (!rawFileName.isEmpty()) {
        QFileInfo fi(rawFileName);
        baseName = fi.completeBaseName();
        urlExt = fi.suffix().toLower();
    }
    
    if (baseName.isEmpty()) {
        baseName = QString("web_image_%1").arg(QDateTime::currentMSecsSinceEpoch());
    }

    // Resolve final file extension: prefer detected extension if valid, or urlExt if supported
    QString finalExt;
    QStringList supportedExts = getSupportedImageExtensions();
    if (!detectedExt.isEmpty() && supportedExts.contains(detectedExt)) {
        finalExt = detectedExt;
    } else if (!urlExt.isEmpty() && supportedExts.contains(urlExt)) {
        finalExt = urlExt;
    } else if (!detectedExt.isEmpty()) {
        finalExt = detectedExt;
    } else {
        // Fallback: check if QImage can load the data directly
        QImage testImg;
        if (testImg.loadFromData(data)) {
            finalExt = "png"; // default safe format
        } else {
            QMessageBox::warning(this, "Unrecognized Format", 
                                 "The downloaded content was not recognized as a valid supported image format.");
            reply->deleteLater();
            return;
        }
    }

    QString finalFileName = baseName + "." + finalExt;
    QString dest = m_imagesDir + "/" + finalFileName;
    if (QFile::exists(dest)) QFile::remove(dest);

    QFile f(dest);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(data);
        f.close();
        QPixmapCache::clear();
        refreshGallery();
        QMessageBox::information(this, "Success", QString("Successfully imported '%1'.").arg(finalFileName));
    } else {
        QMessageBox::warning(this, "Save Error", "Could not save downloaded image to local gallery folder.");
    }

    reply->deleteLater();
}

// [ENTRY POINT: Gallery Selection Callback]
// Triggered on double click; either returns the selected path to the picker caller,
// or spools an interactive crop-and-rotate ImageEditorDialog for asset modification.
void ImageViewer::onItemDoubleClicked(QListWidgetItem *item)
{
    QString filePath = item->data(Qt::UserRole).toString();
    if (m_pickerMode) {
        m_selectedPath = filePath;
        accept();
    } else {
        ImageEditorDialog dlg(filePath, this);
        dlg.exec();
    }
}

void ImageViewer::onItemEntered(QListWidgetItem *item)
{
    if (item && m_previewLabel) {
        QString filePath = item->data(Qt::UserRole).toString();
        QPixmap pix(filePath);
        if (!pix.isNull()) {
            m_previewLabel->setPixmap(pix.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            m_previewLabel->clear();
            m_previewLabel->setText("No Preview");
        }
    }
}

void ImageViewer::removeSelectedImage()
{
    QListWidgetItem *item = m_galleryList->currentItem();
    if (item) {
        QString filePath = item->data(Qt::UserRole).toString();
        if (QMessageBox::question(this, "Remove Image", "Are you sure you want to remove " + item->text() + "?") == QMessageBox::Yes) {
            QFile::remove(filePath);
            QPixmapCache::clear();
            refreshGallery();
            if (m_previewLabel) {
                m_previewLabel->clear();
                m_previewLabel->setText("Select to preview");
            }
        }
    } else {
        QMessageBox::information(this, "Remove Image", "Please select an image to remove.");
    }
}

void ImageViewer::updateColorPreview(const QColor &col)
{
    // Stub implementation to satisfy linker
}

void ImageViewer::saveColorToHistory(const QColor &col)
{
    // Stub implementation to satisfy linker
}
