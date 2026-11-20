const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

let search = `    m_btnTogglePreview = new QPushButton("QSL Card/Envelope", this);
    m_btnTogglePreview->setSizePolicy(sp);
    contactsBtnLayout->addWidget(m_btnTogglePreview);

    connect(m_btnTogglePreview, &QPushButton::clicked, this, [this]() {`;

let replace = `    m_btnTogglePreview = new QPushButton("QSL Card/Envelope", this);
    m_btnTogglePreview->setSizePolicy(sp);
    contactsBtnLayout->addWidget(m_btnTogglePreview);
    
    QPushButton *btnTogglePreviewWin = new QPushButton("Toggle Thumbnail", this);
    btnTogglePreviewWin->setSizePolicy(sp);
    contactsBtnLayout->addWidget(btnTogglePreviewWin);
    
    connect(btnTogglePreviewWin, &QPushButton::clicked, this, [this]() {
        if (m_previewDialog) {
            if (m_previewDialog->isVisible()) m_previewDialog->hide();
            else m_previewDialog->show();
        }
    });

    connect(m_btnTogglePreview, &QPushButton::clicked, this, [this]() {`;

cppCode = cppCode.replace(search, replace);

fs.writeFileSync(cppPath, cppCode);
