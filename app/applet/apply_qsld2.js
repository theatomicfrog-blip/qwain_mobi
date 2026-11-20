const fs = require("fs");
let mCode = fs.readFileSync("cpp_source/mainwindow.cpp", "utf8");
mCode = mCode.replace(`QAction *qslDesigner2Action = qslMenu->addAction(tr("QSL Designer"));
    connect(qslDesigner2Action, &QAction::triggered, this, [this]() {
        QslDesigner2 *designer2 = new QslDesigner2(this);
        designer2->setAttribute(Qt::WA_DeleteOnClose);
        designer2->show();
    
    });`, `QAction *qslDesignerActionWYSIWYG = qslMenu->addAction(tr("QSL Designer"));
    connect(qslDesignerActionWYSIWYG, &QAction::triggered, this, [this]() {
        QslDesigner *designer = new QslDesigner(this);
        designer->setAttribute(Qt::WA_DeleteOnClose);
        designer->show();
    });`);
fs.writeFileSync("cpp_source/mainwindow.cpp", mCode);
