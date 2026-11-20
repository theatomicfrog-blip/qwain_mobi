const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

let search = `m_previewDialog->setWindowTitle("Preview");
    m_previewDialog->resize(300, 200);`;

let replace = `m_previewDialog->setWindowTitle("Preview");
    m_previewDialog->resize(300, 200);
    // do not force resize when content changes, let user resize it
    m_previewDialog->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);`;

cppCode = cppCode.replace(search, replace);

fs.writeFileSync(cppPath, cppCode);
