// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

#include "qsldesigner_tpl.h"
#include "qsldesigner.h"
#include "paths.h"
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QVariantList>
#include <QSettings>
#include <QCoreApplication>
#include <QGraphicsItem>
#include <QBuffer>
#include <QPixmap>
#include <QDateTime>

// ============================================================================
// MODULE: qsldesigner_tpl
// ROLE: Encapsulates all-inclusive Qwain QSL template JSON export options,
//       explicit .tpl signature encoding ("QWAIN-TPL-V1"), base64 image asset
//       embedding/restoration, and template import validation.
// ============================================================================

void QslTemplateIO::exportTemplate(QslDesigner *designer, QWidget *parent) {
    if (!designer) return;

    // Modal dialog requesting export choices (Formatting and/or Images)
    QDialog exportDlg(parent);
    exportDlg.setWindowTitle("Export Qwain Template Options");
    exportDlg.setMinimumWidth(380);

    QVBoxLayout *dlgLayout = new QVBoxLayout(&exportDlg);
    dlgLayout->setSpacing(12);

    QLabel *infoLbl = new QLabel("Select content options to include in template export:", &exportDlg);
    infoLbl->setWordWrap(true);
    dlgLayout->addWidget(infoLbl);

    QCheckBox *chkFormatting = new QCheckBox("Export Formatting (Layout, positions, fonts, colors, dimensions, grids)", &exportDlg);
    chkFormatting->setChecked(true);
    dlgLayout->addWidget(chkFormatting);

    QCheckBox *chkImages = new QCheckBox("Export Images & Background Graphics (Embed image files into .tpl)", &exportDlg);
    chkImages->setChecked(true);
    dlgLayout->addWidget(chkImages);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &exportDlg);
    if (btnBox->button(QDialogButtonBox::Ok)) {
        btnBox->button(QDialogButtonBox::Ok)->setText("Export...");
    }
    QObject::connect(btnBox, &QDialogButtonBox::accepted, &exportDlg, &QDialog::accept);
    QObject::connect(btnBox, &QDialogButtonBox::rejected, &exportDlg, &QDialog::reject);
    dlgLayout->addWidget(btnBox);

    if (exportDlg.exec() != QDialog::Accepted) return;

    bool exportFormatting = chkFormatting->isChecked();
    bool exportImages = chkImages->isChecked();

    if (!exportFormatting && !exportImages) {
        QMessageBox::warning(parent, "Export Error", "Please select at least Formatting or Images to export.");
        return;
    }

    QVariantMap root;
    // Encode explicit Qwain signature and metadata
    root["qwain_signature"] = "QWAIN-TPL-V1";
    root["format"] = "QwainTemplate";
    root["app"] = "Qwain QSL Designer";
    root["created_at"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    root["exportFormatting"] = exportFormatting;
    root["exportImages"] = exportImages;

    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString currentName = designer->getCurrentTemplateName();
    if (currentName.isEmpty()) {
        currentName = "UntitledTemplate";
    }
    root["templateName"] = currentName;

    if (exportFormatting) {
        settings.beginGroup("QslTemplates");
        settings.beginGroup(currentName);

        root["paperW"] = settings.value("paperW", 6.5*96);
        root["paperH"] = settings.value("paperH", 3.625*96);
        root["printerName"] = settings.value("printerName");
        root["printCorrection"] = settings.value("printCorrection", 0);
        root["printOffsetX"] = settings.value("printOffsetX", 0.0);
        root["printOffsetY"] = settings.value("printOffsetY", 0.0);
        root["isEnvelope"] = settings.value("isEnvelope", false);
        QString bgPath = settings.value("bgImagePath").toString();
        root["bgImagePath"] = bgPath;

        if (exportImages && !bgPath.isEmpty() && QFile::exists(bgPath)) {
            QFile bgFile(bgPath);
            if (bgFile.open(QIODevice::ReadOnly)) {
                root["bgImageData"] = QString(bgFile.readAll().toBase64());
                bgFile.close();
            }
        }

        int count = settings.value("count", 0).toInt();
        QVariantList itemsList;
        for (int i = 0; i < count; ++i) {
            settings.beginGroup(QString("Area_%1").arg(i));
            QVariantMap itemMap;
            for (const QString &k : settings.childKeys()) {
                itemMap[k] = settings.value(k);
            }

            int cdCount = settings.beginReadArray("CellData");
            QVariantList cellData;
            for (int j = 0; j < cdCount; ++j) {
                settings.setArrayIndex(j);
                cellData.append(settings.value("data").toString());
            }
            settings.endArray();
            itemMap["cellData"] = cellData;

            if (exportImages && itemMap["isImage"].toBool()) {
                QString imgPath = itemMap["imagePath"].toString();
                if (!imgPath.isEmpty() && QFile::exists(imgPath)) {
                    QFile imgFile(imgPath);
                    if (imgFile.open(QIODevice::ReadOnly)) {
                        itemMap["imageData"] = QString(imgFile.readAll().toBase64());
                        imgFile.close();
                    }
                }
            }

            // Force native JSON types for numbers, booleans, and arrays
            QVariantMap cleanMap;
            cleanMap["id"] = itemMap["id"].toString();
            cleanMap["text"] = itemMap["text"].toString();
            cleanMap["x"] = itemMap["x"].toDouble();
            cleanMap["y"] = itemMap["y"].toDouble();
            cleanMap["w"] = itemMap["w"].toDouble();
            cleanMap["h"] = itemMap["h"].toDouble();
            cleanMap["rot"] = itemMap["rot"].toDouble();
            cleanMap["font"] = itemMap["font"].toString();
            cleanMap["size"] = itemMap["size"].toInt();
            cleanMap["color"] = itemMap["color"].toString();
            cleanMap["align"] = itemMap["align"].toInt();
            cleanMap["maxLines"] = itemMap["maxLines"].toInt();
            cleanMap["zValue"] = itemMap["zValue"].toDouble();
            
            QVariant isImgVar = itemMap["isImage"];
            cleanMap["isImage"] = (isImgVar.type() == QVariant::Bool) ? isImgVar.toBool() : (isImgVar.toString().toLower() == "true" || isImgVar.toString() == "1");
            cleanMap["imagePath"] = itemMap["imagePath"].toString();
            cleanMap["circleLineWidth"] = itemMap["circleLineWidth"].toInt();
            
            int rVal = itemMap["rows"].toInt();
            cleanMap["rows"] = (rVal > 0) ? rVal : 1;
            
            int cVal = itemMap["cols"].toInt();
            cleanMap["cols"] = (cVal > 0) ? cVal : 1;
            
            QVariant shVar = itemMap["showHeaders"];
            cleanMap["showHeaders"] = (shVar.type() == QVariant::Bool) ? shVar.toBool() : (shVar.toString().toLower() == "true" || shVar.toString() == "1");
            
            QVariant sbVar = itemMap["showBorders"];
            cleanMap["showBorders"] = (sbVar.type() == QVariant::Bool) ? sbVar.toBool() : (sbVar.toString().toLower() == "true" || sbVar.toString() == "1");
            
            double btVal = itemMap["borderThickness"].toDouble();
            cleanMap["borderThickness"] = (btVal > 0.0) ? btVal : 1.0;
            cleanMap["rowHeights"] = itemMap["rowHeights"].toString();
            cleanMap["colWidths"] = itemMap["colWidths"].toString();
            cleanMap["cellData"] = itemMap["cellData"];

            if (exportImages && cleanMap["isImage"].toBool()) {
                QString imgPath = cleanMap["imagePath"].toString();
                if (!imgPath.isEmpty() && QFile::exists(imgPath)) {
                    QFile imgFile(imgPath);
                    if (imgFile.open(QIODevice::ReadOnly)) {
                        cleanMap["imageData"] = QString(imgFile.readAll().toBase64());
                        imgFile.close();
                    }
                }
            }

            itemsList.append(cleanMap);
            settings.endGroup();
        }
        root["items"] = itemsList;
        settings.endGroup();
        settings.endGroup();
    }

    QString defUserDir = settings.value("userDirectory", QDir::homePath()).toString();
    QString savePath = QDir(defUserDir).filePath(currentName + ".tpl");

    QString fileName = QFileDialog::getSaveFileName(parent, "Export Qwain Template File", savePath, "Qwain Template Files (*.tpl);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc = QJsonDocument::fromVariant(root);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        QMessageBox::information(parent, "Template Exported", QString("Qwain template '%1' successfully exported to '%2'!").arg(currentName, QFileInfo(fileName).fileName()));
    } else {
        QMessageBox::critical(parent, "Export Error", "Failed to open file for writing.");
    }
}

void QslTemplateIO::importTemplate(QslDesigner *designer, QWidget *parent) {
    if (!designer) return;

    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    QString defUserDir = settings.value("userDirectory", QDir::homePath()).toString();

    QString fileName = QFileDialog::getOpenFileName(parent, "Import Qwain Template File", defUserDir, "Qwain Template Files (*.tpl);;All Files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(parent, "Import Error", "Failed to open template file for reading.");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(parent, "Import Error", "Invalid template file format.");
        return;
    }

    QVariantMap root = doc.toVariant().toMap();

    // Verify valid Qwain template signature / structure
    bool isValidQwainTpl = (root.contains("qwain_signature") && root["qwain_signature"].toString() == "QWAIN-TPL-V1")
                        || (root.contains("format") && root["format"].toString() == "QwainTemplate")
                        || (root.contains("paperW") && root.contains("paperH"));

    if (!isValidQwainTpl) {
        QMessageBox::critical(parent, "Invalid Qwain Template", "The selected file is not a valid Qwain template (.tpl). Import aborted.");
        return;
    }

    QString tName = root["templateName"].toString();
    if (tName.isEmpty()) {
        tName = QFileInfo(fileName).baseName();
    }

    auto choice = QMessageBox::question(parent, "Import Template", QString("Import Qwain template '%1' (%2)?").arg(tName, QFileInfo(fileName).fileName()), QMessageBox::Ok | QMessageBox::Cancel);
    if (choice != QMessageBox::Ok) return;

    // Restore embedded background image if present
    QString bgPath = root["bgImagePath"].toString();
    if (root.contains("bgImageData") && !root["bgImageData"].toString().isEmpty()) {
        QByteArray bgBytes = QByteArray::fromBase64(root["bgImageData"].toString().toLatin1());
        if (!bgBytes.isEmpty()) {
            QDir imgDir(getAppDataDir());
            imgDir.mkdir("tpl_images");
            QString saveBgPath = imgDir.filePath(QString("tpl_images/bg_%1.png").arg(tName));
            QFile bgOut(saveBgPath);
            if (bgOut.open(QIODevice::WriteOnly)) {
                bgOut.write(bgBytes);
                bgOut.close();
                bgPath = saveBgPath;
            }
        }
    }

    QSettings s(QCoreApplication::organizationName(), "Qwain");
    s.beginGroup("QslTemplates");
    s.beginGroup(tName);

    s.setValue("paperW", root["paperW"]);
    s.setValue("paperH", root["paperH"]);
    s.setValue("printerName", root["printerName"]);
    s.setValue("printCorrection", root["printCorrection"]);
    s.setValue("printOffsetX", root["printOffsetX"]);
    s.setValue("printOffsetY", root["printOffsetY"]);
    s.setValue("isEnvelope", root["isEnvelope"]);
    s.setValue("bgImagePath", bgPath);

    QVariantList itemsList = root["items"].toList();
    int idx = 0;
    for (const QVariant &itemVar : itemsList) {
        QVariantMap m = itemVar.toMap();
        
        QString itemImgPath = m["imagePath"].toString();
        if (m.contains("imageData") && !m["imageData"].toString().isEmpty()) {
            QByteArray imgBytes = QByteArray::fromBase64(m["imageData"].toString().toLatin1());
            if (!imgBytes.isEmpty()) {
                QDir imgDir(getAppDataDir());
                imgDir.mkdir("tpl_images");
                QString saveImgPath = imgDir.filePath(QString("tpl_images/img_%1_%2.png").arg(tName).arg(idx));
                QFile imgOut(saveImgPath);
                if (imgOut.open(QIODevice::WriteOnly)) {
                    imgOut.write(imgBytes);
                    imgOut.close();
                    itemImgPath = saveImgPath;
                }
            }
        }

        s.beginGroup(QString("Area_%1").arg(idx++));
        s.setValue("id", m["id"]);
        s.setValue("text", m["text"]);
        s.setValue("x", m["x"]);
        s.setValue("y", m["y"]);
        s.setValue("font", m["font"]);
        s.setValue("size", m["size"]);
        s.setValue("color", m["color"]);
        s.setValue("rot", m["rot"]);
        s.setValue("w", m["w"]);
        s.setValue("h", m["h"]);
        s.setValue("zValue", m["zValue"]);
        s.setValue("maxLines", m["maxLines"]);
        s.setValue("align", m["align"]);
        QVariant isImgV = m["isImage"];
        bool parsedIsImg = (isImgV.type() == QVariant::Bool) ? isImgV.toBool() : (isImgV.toString().toLower() == "true" || isImgV.toString() == "1");
        s.setValue("isImage", parsedIsImg);
        s.setValue("imagePath", itemImgPath);
        s.setValue("circleLineWidth", m["circleLineWidth"]);
        s.setValue("rows", m["rows"]);
        s.setValue("cols", m["cols"]);
        s.setValue("rowHeights", m["rowHeights"]);
        s.setValue("colWidths", m["colWidths"]);
        s.setValue("p0_norm_x", m["p0_norm_x"]);
        s.setValue("p0_norm_y", m["p0_norm_y"]);
        s.setValue("p1_norm_x", m["p1_norm_x"]);
        s.setValue("p1_norm_y", m["p1_norm_y"]);
        s.setValue("p2_norm_x", m["p2_norm_x"]);
        s.setValue("p2_norm_y", m["p2_norm_y"]);
        s.setValue("p3_norm_x", m["p3_norm_x"]);
        s.setValue("p3_norm_y", m["p3_norm_y"]);

        QVariantList cd = m["cellData"].toList();
        s.beginWriteArray("CellData", cd.size());
        for (int j = 0; j < cd.size(); ++j) {
            s.setArrayIndex(j);
            s.setValue("data", cd[j]);
        }
        s.endArray();

        s.endGroup();
    }
    s.setValue("count", idx);
    s.endGroup();
    s.endGroup();

    designer->updateTemplateList();
    designer->loadTemplate(tName);
    QMessageBox::information(parent, "Success", QString("Qwain template '%1' imported and loaded successfully!").arg(tName));
}
