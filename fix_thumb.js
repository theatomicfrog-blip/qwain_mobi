const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

let startIdx = cppCode.indexOf("void QslPreviewDialog::updateThumbnail(const QString& name)");
if (startIdx !== -1) {
    let nextIdx = cppCode.indexOf("void QslPreviewDialog::saveStateForUndo", startIdx);
    if (nextIdx !== -1) {
        let replacement = `void QslPreviewDialog::updateThumbnail(const QString& name) {
    if (name.isEmpty() || !m_thumbnailScene || !m_thumbnailView) return;
    
    QSettings settings(QCoreApplication::organizationName(), "AdifMonitor");
    settings.beginGroup("QslTemplates");
    if (!settings.childGroups().contains(name)) {
        settings.endGroup();
        return;
    }
    settings.beginGroup(name);
    
    double paperW = settings.value("paperW", 6.25*96).toDouble();
    double paperH = settings.value("paperH", 3.5*96).toDouble();
    QString bgImagePath = settings.value("bgImagePath").toString();
    
    settings.endGroup();
    settings.endGroup();

    m_thumbnailScene->clear();
    
    QGraphicsRectItem *paper = m_thumbnailScene->addRect(0, 0, paperW, paperH, Qt::NoPen, Qt::white);
    paper->setZValue(-2);
    
    if (!bgImagePath.isEmpty() && QFile::exists(bgImagePath)) {
        QPixmap pm(bgImagePath);
        QGraphicsPixmapItem *bg = m_thumbnailScene->addPixmap(pm.scaled(paperW, paperH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        bg->setPos(0, 0);
        bg->setZValue(-1);
    }
    
    m_thumbnailView->fitInView(m_thumbnailScene->sceneRect(), Qt::KeepAspectRatio);
}

`;
        cppCode = cppCode.substring(0, startIdx) + replacement + cppCode.substring(nextIdx);
        fs.writeFileSync(cppPath, cppCode);
    }
}
