const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

let search = `m_thumbnailView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);`;

let replace = `m_thumbnailView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_thumbnailView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);`;

cppCode = cppCode.replace(search, replace);

fs.writeFileSync(cppPath, cppCode);
