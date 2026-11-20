const fs = require("fs");
let mCode = fs.readFileSync("cpp_source/mainwindow.cpp", "utf8");
mCode = mCode.replace(`QAction *qslDesignerAction = qslMenu->addAction(tr("Print Manager"));
    connect(qslDesignerAction, &QAction::triggered, this, [this]() {
        QslDesigner dlg(this);
        dlg.exec();
    });`, `QAction *qslDesignerAction = qslMenu->addAction(tr("Print Manager"));
    connect(qslDesignerAction, &QAction::triggered, this, [this]() {
        QslPreviewDialog *dlg = new QslPreviewDialog("", "", QList<QsoDetail>(), this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    });`);
fs.writeFileSync("cpp_source/mainwindow.cpp", mCode);
