// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

#include "qsltemplateinspector.h"
#include "qsldesigner.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTabWidget>
#include <QTextBrowser>
#include <QLabel>
#include <QSettings>
#include <QCoreApplication>
#include <QGraphicsItem>
#include <QDateTime>

// ============================================================================
// MODULE: qsltemplateinspector
// ROLE: Real-time side-by-side forensic inspection window monitoring template
//       creation on canvas, QSettings disk serialization, and preview population.
// ============================================================================

QslTemplateInspector::QslTemplateInspector(const QString &templateName, QslDesigner *designer, QWidget *parent)
    : QDialog(parent), m_templateName(templateName), m_designer(designer)
{
    setWindowTitle(QString("🔬 Template Forensic Inspector - [%1]").arg(templateName.isEmpty() ? "Active Canvas" : templateName));
    resize(820, 600);
    setStyleSheet("QDialog { background-color: #0f172a; color: #f8fafc; font-family: 'Segoe UI', Arial, sans-serif; }"
                  "QTabWidget::pane { border: 1px solid #334155; background-color: #0f172a; }"
                  "QTabBar::tab { background-color: #1e293b; color: #94a3b8; padding: 8px 16px; font-weight: bold; border-top-left-radius: 6px; border-top-right-radius: 6px; }"
                  "QTabBar::tab:selected { background-color: #3b82f6; color: white; }"
                  "QTextBrowser { background-color: #020617; color: #38bdf8; font-family: 'Consolas', 'Courier New', monospace; font-size: 9.5pt; border: 1px solid #1e293b; border-radius: 4px; padding: 8px; }"
                  "QPushButton { background-color: #2563eb; color: white; border: none; border-radius: 4px; padding: 8px 16px; font-weight: bold; }"
                  "QPushButton:hover { background-color: #1d4ed8; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(10);

    QHBoxLayout *topBar = new QHBoxLayout();
    QLabel *lblTitle = new QLabel(QString("<b>🔬 Live Forensic Analysis:</b> %1").arg(templateName.isEmpty() ? "Active Designer Canvas" : templateName), this);
    lblTitle->setStyleSheet("font-size: 11pt; color: #f1f5f9;");
    QPushButton *btnRefresh = new QPushButton("🔄 Refresh Diagnostics", this);
    connect(btnRefresh, &QPushButton::clicked, this, &QslTemplateInspector::refreshDiagnostics);

    topBar->addWidget(lblTitle);
    topBar->addStretch();
    topBar->addWidget(btnRefresh);
    mainLayout->addLayout(topBar);

    m_tabs = new QTabWidget(this);
    m_canvasBrowser = new QTextBrowser(this);
    m_diskBrowser = new QTextBrowser(this);
    m_previewBrowser = new QTextBrowser(this);

    m_tabs->addTab(m_canvasBrowser, "1. Designer Canvas Items");
    m_tabs->addTab(m_diskBrowser, "2. QSettings Disk Group Dump");
    m_tabs->addTab(m_previewBrowser, "3. Preview Population Trace");

    mainLayout->addWidget(m_tabs, 1);

    refreshDiagnostics();
}

void QslTemplateInspector::refreshDiagnostics() {
    inspectCanvasState();
    inspectDiskSettings();
    inspectPreviewPopulation();
}

void QslTemplateInspector::inspectCanvasState() {
    if (!m_canvasBrowser) return;
    QString html = QString("<h3>🎨 Designer Canvas Active Items (Timestamp: %1)</h3>")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    if (!m_designer || !m_designer->scene()) {
        html += "<p style='color:#f87171;'>No active QslDesigner canvas connected.</p>";
        m_canvasBrowser->setHtml(html);
        return;
    }

    QList<QGraphicsItem*> items = m_designer->scene()->items();
    int count = 0;
    for (QGraphicsItem *item : items) {
        if (dynamic_cast<QGraphicsRectItem*>(item) && item->zValue() >= 0) count++;
    }

    html += QString("<p><b>Total Canvas Items:</b> <span style='color:#4ade80;'>%1</span></p><hr>").arg(count);

    int idx = 1;
    for (QGraphicsItem *item : items) {
        QGraphicsRectItem *rectItem = dynamic_cast<QGraphicsRectItem*>(item);
        if (!rectItem || item->zValue() < 0) continue; // Skip grid background / lines

        QString areaId = item->data(0).toString();
        if (areaId.isEmpty()) areaId = QString("Item_%1").arg(idx);

        html += QString("<div style='margin-bottom:10px; border-bottom:1px solid #1e293b; padding-bottom:6px;'>");
        html += QString("<b>[Item #%1] Item ID:</b> <span style='color:#facc15;'>%2</span><br>")
            .arg(idx++).arg(areaId);
        html += QString("&nbsp;&nbsp;📍 <b>Pos:</b> (%1, %2) | <b>Size:</b> (%3 x %4) | <b>Z-Order:</b> %5<br>")
            .arg(rectItem->x(), 0, 'f', 1).arg(rectItem->y(), 0, 'f', 1)
            .arg(rectItem->rect().width(), 0, 'f', 1).arg(rectItem->rect().height(), 0, 'f', 1)
            .arg(rectItem->zValue());
        html += "</div>";
    }

    m_canvasBrowser->setHtml(html);
}

void QslTemplateInspector::inspectDiskSettings() {
    if (!m_diskBrowser) return;
    QString html = QString("<h3>💾 QSettings Disk Group Dump (Timestamp: %1)</h3>")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    if (m_templateName.isEmpty() && m_designer) {
        m_templateName = m_designer->currentTemplateName();
    }

    if (m_templateName.isEmpty()) {
        html += "<p style='color:#f87171;'>No template name provided for disk group inspection.</p>";
        m_diskBrowser->setHtml(html);
        return;
    }

    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");

    if (!settings.childGroups().contains(m_templateName)) {
        html += QString("<p style='color:#f87171;'>Group 'QslTemplates/%1' DOES NOT EXIST on disk QSettings!</p>").arg(m_templateName);
        m_diskBrowser->setHtml(html);
        return;
    }

    settings.beginGroup(m_templateName);
    int count = settings.value("count", 0).toInt();
    double paperW = settings.value("paperW", 0).toDouble();
    double paperH = settings.value("paperH", 0).toDouble();

    html += QString("<p><b>Group:</b> QslTemplates/%1 | <b>Saved Items Count:</b> <span style='color:#4ade80;'>%2</span> | <b>Paper:</b> %3 x %4 px</p><hr>")
        .arg(m_templateName).arg(count).arg(paperW).arg(paperH);

    for (int i = 0; i < count; ++i) {
        settings.beginGroup(QString("Area_%1").arg(i));
        html += QString("<div style='margin-bottom:10px; border-bottom:1px solid #1e293b; padding-bottom:6px;'>");
        html += QString("<b>[Area_%1] ID:</b> <span style='color:#facc15;'>%2</span> | <b>Text:</b> '%3'<br>")
            .arg(i).arg(settings.value("id").toString()).arg(settings.value("text").toString());
        html += QString("&nbsp;&nbsp;📍 <b>Pos:</b> (%1, %2) | <b>Size:</b> (%3 x %4) | <b>Z:</b> %5<br>")
            .arg(settings.value("x").toDouble(), 0, 'f', 1).arg(settings.value("y").toDouble(), 0, 'f', 1)
            .arg(settings.value("w").toDouble(), 0, 'f', 1).arg(settings.value("h").toDouble(), 0, 'f', 1)
            .arg(settings.value("zValue").toDouble());
        html += QString("&nbsp;&nbsp;📊 <b>Rows:</b> %1 | <b>Cols:</b> %2 | <b>IsImage:</b> %3 (%4)<br>")
            .arg(settings.value("rows").toInt()).arg(settings.value("cols").toInt())
            .arg(settings.value("isImage").toBool() ? "TRUE" : "FALSE")
            .arg(settings.value("isImage").toString());

        int cdCount = settings.beginReadArray("CellData");
        QStringList cdList;
        for (int j = 0; j < cdCount; ++j) {
            settings.setArrayIndex(j);
            cdList.append(settings.value("data").toString());
        }
        settings.endArray();

        if (!cdList.isEmpty()) {
            html += QString("&nbsp;&nbsp;🏷️ <b>CellData Tags (%1):</b> [%2]<br>")
                .arg(cdList.size()).arg(cdList.join(", "));
        }
        html += "</div>";
        settings.endGroup();
    }

    settings.endGroup();
    settings.endGroup();

    m_diskBrowser->setHtml(html);
}

void QslTemplateInspector::inspectPreviewPopulation() {
    if (!m_previewBrowser) return;
    QString html = QString("<h3>🔬 MAXIMUM INTENSITY FORENSIC DIAGNOSTICS (Timestamp: %1)</h3>")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    if (m_templateName.isEmpty() && m_designer) {
        m_templateName = m_designer->currentTemplateName();
    }

    if (m_templateName.isEmpty()) {
        html += "<p style='color:#f87171;'>No active template selected for forensic trace.</p>";
        m_previewBrowser->setHtml(html);
        return;
    }

    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");
    settings.beginGroup(m_templateName);

    int count = settings.value("count", 0).toInt();
    double paperW = settings.value("paperW", 0).toDouble();
    double paperH = settings.value("paperH", 0).toDouble();

    html += QString("<p style='font-weight:bold; color:#38bdf8;'>Tracing Population & Engine Bounds for '%1' (%2 items, Canvas: %3 x %4 px):</p><hr>")
        .arg(m_templateName).arg(count).arg(paperW).arg(paperH);

    for (int i = 0; i < count; ++i) {
        settings.beginGroup(QString("Area_%1").arg(i));
        QString idStr = settings.value("id").toString();
        QString textStr = settings.value("text").toString();
        double x = settings.value("x").toDouble();
        double y = settings.value("y").toDouble();
        double w = settings.value("w").toDouble();
        double h = settings.value("h").toDouble();
        int rows = settings.value("rows", 1).toInt();
        int cols = settings.value("cols", 1).toInt();
        bool isImage = settings.value("isImage", false).toBool();
        double zVal = settings.value("zValue", 0.0).toDouble();
        int itemType = settings.value("itemType", 0).toInt();
        QString fontStr = settings.value("font").toString();
        int fontSize = settings.value("size", 10).toInt();

        int cdCount = settings.beginReadArray("CellData");
        QStringList cellData;
        for (int j = 0; j < cdCount; ++j) {
            settings.setArrayIndex(j);
            cellData.append(settings.value("data").toString());
        }
        settings.endArray();

        QString lowerId = idStr.toLower();
        QString lowerText = textStr.toLower();

        html += QString("<div style='margin-bottom:12px; border:1px solid #334155; border-radius:6px; padding:8px; background-color:#0f172a;'>");
        html += QString("<b style='color:#facc15;'>Item #%1 [%2]</b> (Type ID: %3)<br>").arg(i + 1).arg(idStr.isEmpty() ? "No_ID" : idStr).arg(itemType);
        
        // Geometry, Z-Value & Font Analysis
        html += QString("<span style='color:#94a3b8;'>📐 <b>Geometry:</b> Pos(%1, %2) | Size(%3 x %4 px) | <b>Z-Value:</b> %5 | <b>Font:</b> %6 (%7pt)</span><br>")
            .arg(x, 0, 'f', 1).arg(y, 0, 'f', 1).arg(w, 0, 'f', 1).arg(h, 0, 'f', 1).arg(zVal, 0, 'f', 2).arg(fontStr.isEmpty() ? "Default" : fontStr).arg(fontSize);

        if (itemType == 2 || lowerId == "address_box" || lowerId.contains("address") || lowerText.contains("full contact name")) {
            html += "<span style='color:#38bdf8; font-weight:bold;'>📍 MATCHED RULE: ITEM_ADDRESS (Multiline Recipient Address Box)</span><br>";
            html += "&nbsp;&nbsp;• <b>Evaluation Branch:</b> Branch #2 (Multiline Address Assembly)<br>";
            html += "&nbsp;&nbsp;• <b>Pre-Substitution Raw Text:</b><br>";
            html += QString("<pre style='background:#1e293b; color:#e2e8f0; padding:4px; border-radius:4px;'>%1</pre>").arg(textStr);
            html += "&nbsp;&nbsp;• <b>Target Field Binding:</b> &lt;NAME&gt; \n &lt;STREET&gt; \n &lt;CITY, STATE ZIP&gt; \n &lt;COUNTRY&gt;<br>";
            html += "&nbsp;&nbsp;• <b>Rendering Pipeline:</b> Reads ADIF log fields (NAME, ADDRESS, CITY, STATE, ZIP, COUNTRY) with QRZ cache fallback<br>";
        } else if (itemType == 4 || idStr == "STAMP_REMINDER") {
            html += "<span style='color:#f59e0b; font-weight:bold;'>✉️ MATCHED RULE: ITEM_STAMP (Stamp Reminder Box)</span><br>";
            html += "&nbsp;&nbsp;• <b>Evaluation Branch:</b> Branch #4 (PSE/TNX QSL Stamp Rendering)<br>";
        } else if (itemType == 3 || isImage) {
            QString imgDesc = (zVal < -0.5) ? "Background Card Artwork (Backdrop)" : "Layered Image / Badge / Sticker (Foreground)";
            html += QString("<span style='color:#ec4899; font-weight:bold;'>🖼️ MATCHED RULE: ITEM_IMAGE (%1, Z-Value: %2)</span><br>")
                .arg(imgDesc).arg(zVal, 0, 'f', 2);
        } else if (itemType == 5 || idStr.contains("REF_POINT")) {
            html += "<span style='color:#64748b; font-weight:bold;'>🎯 MATCHED RULE: ITEM_REF_POINT (Alignment Anchor - Omitted at Print)</span><br>";
        } else if (itemType == 1 || rows > 1 || cols > 1) {
            html += QString("<span style='color:#4ade80; font-weight:bold;'>📊 MATCHED RULE: ITEM_TABLE (QSO Table Grid %1x%2)</span><br>").arg(rows).arg(cols);
            html += QString("&nbsp;&nbsp;• <b>Cell Tags:</b> [%1]<br>").arg(cellData.join(", "));
        } else {
            html += QString("<span style='color:#e2e8f0;'>🔤 MATCHED RULE: ITEM_TEXT (Static Slogan / Tag Field)</span><br>");
            html += QString("&nbsp;&nbsp;• <b>Raw Value:</b> '%1'<br>").arg(textStr);
        }

        html += "</div>";
        settings.endGroup();
    }

    settings.endGroup();
    settings.endGroup();

    m_previewBrowser->setHtml(html);
}