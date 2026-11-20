import re

with open('cpp_source/qslpreviewdialog.cpp', 'r') as f:
    text = f.read()

target = """    connect(btnBox, &QDialogButtonBox::rejected, &confirmDlg, &QDialog::reject);
    layout->addWidget(btnBox);
    
    bool skipConfirm = false;
    if (m_batchContinuousPrinting) {
        if (m_batchSelectedPrinter.isEmpty()) {
            // First time in continuous mode, show dialog
        } else {
            // Subsequent times, skip dialog
            skipConfirm = true;
        }
    }
    if (!skipConfirm) {
        if (confirmDlg.exec() != QDialog::Accepted) {
            return false;
        }
        
        m_batchSelectedPrinter = printerCombo->currentData().toString();
    }"""

replacement = """    connect(btnBox, &QDialogButtonBox::rejected, &confirmDlg, &QDialog::reject);
    
    QPushButton *btnReprintCard = nullptr;
    QPushButton *btnMakeChanges = nullptr;
    if (isEnvelope) {
        btnMakeChanges = new QPushButton("Make Changes");
        btnBox->addButton(btnMakeChanges, QDialogButtonBox::ActionRole);
        btnReprintCard = new QPushButton("Reprint Card");
        btnBox->addButton(btnReprintCard, QDialogButtonBox::ActionRole);
        
        connect(btnReprintCard, &QPushButton::clicked, [&](){
            confirmDlg.done(2);
        });
        connect(btnMakeChanges, &QPushButton::clicked, [&](){
            confirmDlg.done(3);
        });
    }

    layout->addWidget(btnBox);
    
    bool skipConfirm = false;
    if (m_batchContinuousPrinting) {
        if (m_batchSelectedPrinter.isEmpty()) {
            // First time in continuous mode, show dialog
        } else {
            // Subsequent times, skip dialog
            skipConfirm = true;
        }
    }
    m_reprintCardRequested = false;
    if (!skipConfirm) {
        int res = confirmDlg.exec();
        if (res == 3) {
            // Make Changes -> close preview dialog completely, keep queue intact
            this->reject();
            return false;
        } else if (res == 2) {
            // Reprint Card -> return back and reprint
            m_reprintCardRequested = true;
            return false;
        } else if (res != QDialog::Accepted) {
            return false;
        }
        
        m_batchSelectedPrinter = printerCombo->currentData().toString();
    }"""

if target in text:
    print("Found target!")
    with open('cpp_source/qslpreviewdialog.cpp', 'w') as f:
        f.write(text.replace(target, replacement))
else:
    print("Target not found.")

