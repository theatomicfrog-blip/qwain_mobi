const fs = require('fs');
let p = 'cpp_source/qslpreviewdialog.cpp';
let c = fs.readFileSync(p, 'utf8');

c = c.replace('m_previewDialog->setWindowFlags(Qt::Tool);\n    m_previewDialog->show();', 'm_previewDialog->setWindowFlags(Qt::Tool);');

if (c.indexOf('m_previewDialog->show();') === -1) {
    // Modify updateThumbnail to show the preview dialog if it is populated.
    let utFind = 'm_thumbnailView->fitInView(m_thumbnailScene->sceneRect(), Qt::KeepAspectRatio);';
    let utReplace = utFind + '\n    if(m_previewDialog && !m_previewDialog->isVisible()) m_previewDialog->show();';
    c = c.replace(utFind, utReplace);
}

fs.writeFileSync(p, c);
