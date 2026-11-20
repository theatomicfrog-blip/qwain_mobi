const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

let search1 = `    contactsBtnLayout->addWidget(btnTableFont);
    contactsBtnLayout->addWidget(btnCallFont);
    contactsBtnLayout->addWidget(m_btnUndo);`;

let replace1 = `    contactsBtnLayout->addWidget(btnTableFont);
    contactsBtnLayout->addWidget(btnCallFont);
    m_btnAddressFont = new QPushButton("Address Font", this);
    m_btnAddressFont->setSizePolicy(sp);
    m_btnAddressFont->hide();
    contactsBtnLayout->addWidget(m_btnAddressFont);
    contactsBtnLayout->addWidget(m_btnUndo);`;

cppCode = cppCode.replace(search1, replace1);

let search2 = `    connect(btnCallFont, &QPushButton::clicked, this, &QslPreviewDialog::changeCallFont);
    connect(m_btnUndo, &QPushButton::clicked, this, &QslPreviewDialog::undo);`;

let replace2 = `    connect(btnCallFont, &QPushButton::clicked, this, &QslPreviewDialog::changeCallFont);
    connect(m_btnAddressFont, &QPushButton::clicked, this, &QslPreviewDialog::changeAddressFont);
    connect(m_btnUndo, &QPushButton::clicked, this, &QslPreviewDialog::undo);`;

cppCode = cppCode.replace(search2, replace2);

fs.writeFileSync(cppPath, cppCode);
