const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

let search = `    m_view->setRenderHint(QPainter::Antialiasing);`;
let replace = `    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_view->setMinimumSize(400, 300);`;

cppCode = cppCode.replace(search, replace);

fs.writeFileSync(cppPath, cppCode);
