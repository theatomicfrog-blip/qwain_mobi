const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

// remove auto show
cppCode = cppCode.replace("if(m_previewDialog && !m_previewDialog->isVisible()) m_previewDialog->show();", "// No auto show");

fs.writeFileSync(cppPath, cppCode);
