const fs = require("fs");
let hPath = "cpp_source/qslpreviewdialog.h";
let hCode = fs.readFileSync(hPath, "utf8");

if (!hCode.includes("QListWidget *m_envTemplateList")) {
    hCode = hCode.replace("QListWidget *m_templateList = nullptr;", `QListWidget *m_templateList = nullptr;
    QListWidget *m_envTemplateList = nullptr;
    QPushButton *m_btnTogglePreview = nullptr;
    bool m_previewingEnvelope = false;
    QDialog *m_previewDialog = nullptr;`);
    fs.writeFileSync(hPath, hCode);
}
