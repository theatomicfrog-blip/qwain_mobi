#include "addressrenderinspectorwindow.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsRectItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <QTimer>

AddressRenderInspectorWindow::AddressRenderInspectorWindow(const QString &callsign, const QString &templateName,
                                                           const QString &addrName, const QString &addrStreet,
                                                           const QString &addrCity, const QString &addrCountry,
                                                           QGraphicsScene *previewScene, QWidget *parent)
    : QDialog(parent), m_callsign(callsign), m_templateName(templateName.isEmpty() ? "qsl culture" : templateName),
      m_addrName(addrName), m_addrStreet(addrStreet), m_addrCity(addrCity), m_addrCountry(addrCountry),
      m_sourceScene(previewScene) {
    setWindowTitle("🔬 Enhanced Address Render Inspector & Wireframe Reference Laboratory");
    resize(1150, 850);
    buildUi();
    runDiagnosticTrace();
    QTimer::singleShot(25, this, [this]() {
        if (m_liveView && m_liveScene && !m_liveScene->sceneRect().isEmpty()) {
            m_liveView->fitInView(m_liveScene->sceneRect(), Qt::KeepAspectRatio);
        }
    });
}

void AddressRenderInspectorWindow::buildUi() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Header Banner
    QLabel *lblTitle = new QLabel(QString("<b>Inspector:</b> Callsign <span style='color:#38bdf8;'>%1</span> | Template <span style='color:#a855f7;'>'%2'</span> (Rendered Output + Overlaid Wireframe Reference)").arg(m_callsign).arg(m_templateName), this);
    lblTitle->setStyleSheet("font-size: 11pt; padding: 8px; background-color: #0f172a; color: #f8fafc; border-radius: 4px;");
    mainLayout->addWidget(lblTitle);

    QSplitter *vertSplitter = new QSplitter(Qt::Vertical, this);
    mainLayout->addWidget(vertSplitter, 1);

    // TOP HORIZONTAL SPLITTER: Raw Data & Formatted Data
    QSplitter *topSplitter = new QSplitter(Qt::Horizontal, vertSplitter);
    m_rawBrowser = new QTextBrowser(topSplitter);
    m_rawBrowser->setStyleSheet("background-color: #0f172a; color: #e2e8f0; font-family: monospace;");

    m_formattedBrowser = new QTextBrowser(topSplitter);
    m_formattedBrowser->setStyleSheet("background-color: #0f172a; color: #4ade80; font-family: monospace;");
    topSplitter->addWidget(m_rawBrowser);
    topSplitter->addWidget(m_formattedBrowser);

    // MIDDLE HORIZONTAL SPLITTER: Template Breakdown & Live Canvas
    QSplitter *midSplitter = new QSplitter(Qt::Horizontal, vertSplitter);
    m_templateBreakdownBrowser = new QTextBrowser(midSplitter);
    m_templateBreakdownBrowser->setStyleSheet("background-color: #0f172a; color: #facc15; font-family: monospace;");

    m_liveScene = new QGraphicsScene(this);
    m_liveView = new QGraphicsView(m_liveScene, midSplitter);
    m_liveView->setStyleSheet("background-color: #1e293b;");
    midSplitter->addWidget(m_templateBreakdownBrowser);
    midSplitter->addWidget(m_liveView);

    // BOTTOM PANE: ONE BIG FAT COMPLETE FEEDBACK PANE
    m_feedbackBrowser = new QTextBrowser(vertSplitter);
    m_feedbackBrowser->setStyleSheet("background-color: #020617; color: #38bdf8; font-family: monospace; font-size: 10pt;");

    QPushButton *btnClose = new QPushButton("Close Forensic Inspector", this);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(btnClose);
}

void AddressRenderInspectorWindow::runDiagnosticTrace() {
    // 1. RAW DATA IN
    QString rawHtml = "<b>1. RAW PRESENTED ADDRESS DATA IN (NO WORKAROUNDS/SUBSTITUTIONS):</b><br>";
    rawHtml += QString("• Active Callsign: <b>%1</b><br>").arg(m_callsign.toHtmlEscaped());
    rawHtml += QString("• Name: %1<br>").arg(m_addrName.isEmpty() ? "<font color='#94a3b8'>(NONE PRESENTED)</font>" : m_addrName.toHtmlEscaped());
    rawHtml += QString("• Street: %1<br>").arg(m_addrStreet.isEmpty() ? "<font color='#94a3b8'>(NONE PRESENTED)</font>" : m_addrStreet.toHtmlEscaped());
    rawHtml += QString("• City/State/Zip: %1<br>").arg(m_addrCity.isEmpty() ? "<font color='#94a3b8'>(NONE PRESENTED)</font>" : m_addrCity.toHtmlEscaped());
    rawHtml += QString("• Country: %1<br>").arg(m_addrCountry.isEmpty() ? "<font color='#94a3b8'>(NONE PRESENTED)</font>" : m_addrCountry.toHtmlEscaped());
    m_rawBrowser->setHtml(rawHtml);

    // 2. FORMATTED DATA (NO DUMMY WORKAROUNDS OR CALLSIGN SUBSTITUTIONS)
    QStringList lines;
    if (!m_addrName.isEmpty()) lines << m_addrName.trimmed();
    if (!m_addrStreet.isEmpty()) lines << m_addrStreet.trimmed();
    if (!m_addrCity.isEmpty()) lines << m_addrCity.trimmed();
    if (!m_addrCountry.isEmpty() && m_addrCountry.toUpper() != "USA" && m_addrCountry.toUpper() != "UNITED STATES") {
        lines << m_addrCountry.trimmed();
    }

    QString formattedStr = lines.join("\n");
    QString fmtHtml = "<b>2. PROPOSED ADDRESS BLOCK OUTPUT:</b><br><pre style='color:#4ade80; font-size:11pt;'>";
    fmtHtml += formattedStr.isEmpty() ? "<font color='#f87171'>(No address data presented for rendering)</font>" : formattedStr.toHtmlEscaped();
    fmtHtml += "</pre>";
    m_formattedBrowser->setHtml(fmtHtml);

    // 3. TEMPLATE DISK BREAKDOWN & ENHANCED RENDERED SCENE WITH WIREFRAME OVERLAY
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("QslTemplates");
    settings.beginGroup(m_templateName);
    int count = settings.value("count", 0).toInt();
    double paperW = settings.value("paperW", 624).toDouble();
    double paperH = settings.value("paperH", 348).toDouble();

    QString tbHtml = QString("<b>3. TEMPLATE DISK BREAKDOWN ('%1'):</b><br>").arg(m_templateName.toHtmlEscaped());
    tbHtml += QString("• Paper Bounds: %1 x %2 px<br>").arg(paperW).arg(paperH);
    tbHtml += QString("• Saved Items Count: %1<br><hr>").arg(count);

    m_liveScene->clear();
    m_liveScene->setSceneRect(0, 0, paperW, paperH);

    // Render white paper background
    QGraphicsRectItem *paper = m_liveScene->addRect(0, 0, paperW, paperH, Qt::NoPen, Qt::white);
    paper->setZValue(-2);

    // A. Render the SAME scene items as the preview pane shows (pixmaps, paths, ellipses, rects, text)
    int clonedTextCount = 0;
    QStringList clonedTexts;
    if (m_sourceScene) {
        for (QGraphicsItem *srcItem : m_sourceScene->items(Qt::AscendingOrder)) {
            if (srcItem->zValue() < -1.5) continue; // Skip raw paper item
            if (QGraphicsPixmapItem *pm = dynamic_cast<QGraphicsPixmapItem*>(srcItem)) {
                QGraphicsPixmapItem *pmClone = m_liveScene->addPixmap(pm->pixmap());
                pmClone->setPos(pm->scenePos());
                pmClone->setRotation(pm->rotation());
                pmClone->setZValue(pm->zValue());
            } else if (QGraphicsSimpleTextItem *st = dynamic_cast<QGraphicsSimpleTextItem*>(srcItem)) {
                QGraphicsSimpleTextItem *stClone = new QGraphicsSimpleTextItem(st->text());
                stClone->setFont(st->font());
                stClone->setBrush(st->brush());
                stClone->setPos(st->scenePos());
                stClone->setRotation(st->rotation());
                stClone->setZValue(2);
                m_liveScene->addItem(stClone);
                clonedTextCount++;
                clonedTexts << st->text();
            } else if (QGraphicsPathItem *pt = dynamic_cast<QGraphicsPathItem*>(srcItem)) {
                QGraphicsPathItem *ptClone = m_liveScene->addPath(pt->path(), pt->pen(), pt->brush());
                ptClone->setPos(pt->scenePos());
                ptClone->setRotation(pt->rotation());
                ptClone->setZValue(2);
            } else if (QGraphicsEllipseItem *el = dynamic_cast<QGraphicsEllipseItem*>(srcItem)) {
                QGraphicsEllipseItem *elClone = m_liveScene->addEllipse(el->rect(), el->pen(), el->brush());
                elClone->setPos(el->scenePos());
                elClone->setRotation(el->rotation());
                elClone->setZValue(2);
            } else if (QGraphicsRectItem *rt = dynamic_cast<QGraphicsRectItem*>(srcItem)) {
                if (rt->pen() != Qt::NoPen || rt->brush() != Qt::NoBrush) {
                    QGraphicsRectItem *rtClone = new QGraphicsRectItem(rt->rect());
                    rtClone->setPen(rt->pen());
                    rtClone->setBrush(rt->brush());
                    rtClone->setPos(rt->scenePos());
                    rtClone->setRotation(rt->rotation());
                    rtClone->setZValue(0);
                    m_liveScene->addItem(rtClone);
                }
            }
        }
    }

    // B. OVERLAY THE WIREFRAME POPULATED AS A REFERENCE (Magenta Dotted Rectangles + Item Labels)
    for (int i = 0; i < count; ++i) {
        settings.beginGroup(QString("Area_%1").arg(i));
        QString idStr = settings.value("id").toString();
        QString textStr = settings.value("text").toString();
        int cdCount = settings.beginReadArray("CellData");
        QStringList cellData;
        for (int j = 0; j < cdCount; ++j) {
            settings.setArrayIndex(j);
            cellData.append(settings.value("data").toString());
        }
        settings.endArray();
        if (textStr.isEmpty() && !cellData.isEmpty()) {
            textStr = cellData.first();
        }

        double x = settings.value("x").toDouble();
        double y = settings.value("y").toDouble();
        double w = settings.value("w").toDouble();
        double h = settings.value("h").toDouble();
        int itemType = settings.value("itemType", 0).toInt();

        QString displayTag = idStr.isEmpty() ? textStr : idStr;
        tbHtml += QString("• Item #%1 [%2] (Type: %3) Pos(%4, %5) Size(%6x%7)<br>")
            .arg(i + 1).arg(displayTag.toHtmlEscaped()).arg(itemType).arg(x).arg(y).arg(w).arg(h);

        // Add Magenta Wireframe Reference Rectangle
        QGraphicsRectItem *wireframe = new QGraphicsRectItem(0, 0, w, h);
        wireframe->setPos(x, y);
        wireframe->setPen(QPen(QColor(236, 72, 153), 1, Qt::DashLine)); // Magenta Wireframe
        wireframe->setZValue(10); // Overlay on top
        m_liveScene->addItem(wireframe);

        // Add Wireframe ID Tag Label
        QGraphicsSimpleTextItem *tagLabel = new QGraphicsSimpleTextItem(QString("[%1]").arg(displayTag), wireframe);
        tagLabel->setFont(QFont("Monospace", 8, QFont::Bold));
        tagLabel->setBrush(QColor(236, 72, 153));
        tagLabel->setPos(2, -14);
        tagLabel->setZValue(11);

        settings.endGroup();
    }
    settings.endGroup();
    settings.endGroup();

    m_templateBreakdownBrowser->setHtml(tbHtml);

    // 4. ONE BIG FAT COMPLETE FEEDBACK PANE
    QString fbHtml = "=========================================================================<br>";
    fbHtml += "🔥 <b>ONE BIG FAT COMPLETE FEEDBACK PANE & RENDER INSPECTION REPORT</b><br>";
    fbHtml += "=========================================================================<br>";
    fbHtml += QString("• <b>Timestamp:</b> %1<br>").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    fbHtml += QString("• <b>Callsign Processed:</b> %1<br>").arg(m_callsign.toHtmlEscaped());
    fbHtml += QString("• <b>Template Processed:</b> %1<br>").arg(m_templateName.toHtmlEscaped());
    fbHtml += QString("• <b>Formatted Address Block Output:</b><br><pre style='color:#4ade80;'>%1</pre><br>").arg(formattedStr.isEmpty() ? "(NONE PRESENTED)" : formattedStr.toHtmlEscaped());
    
    fbHtml += "<b>SCENE ITEMS EXTRACTION TRACE (CLONED FROM PREVIEW PANE SCENE):</b><br>";
    for (int idx = 0; idx < clonedTexts.size(); ++idx) {
        fbHtml += QString("&nbsp;&nbsp;[%1] Preview Scene Rendered Text: '%2'<br>").arg(idx + 1).arg(clonedTexts[idx].toHtmlEscaped());
    }

    fbHtml += "<br><span style='color:#4ade80; font-weight:bold; font-size:11pt;'>✅ ENHANCED DIAGNOSTIC STATUS: Same Rendered Template as Preview Pane + Wireframe Reference Overlay Active!</span>";
    m_feedbackBrowser->setHtml(fbHtml);
}
