const fs = require('fs');
let code = fs.readFileSync('cpp_source/configdialog.cpp', 'utf8');

// 1. Initial fields assignment
const initMatch = `m_selectedColor = settings.value("SkinColor", "#221e1f").toString();`;
const initRepl = `m_selectedColor = settings.value("SkinColor", "#221e1f").toString();
    m_initialSelectedColor = m_selectedColor;`;
code = code.replace(initMatch, initRepl);

const fcolorMatch = `        f = qvariant_cast<QFont>(settings.value(fkey, defFont));
        c = qvariant_cast<QColor>(settings.value(ckey, defColor));`;
const fcolorRepl = `        f = qvariant_cast<QFont>(settings.value(fkey, defFont));
        c = qvariant_cast<QColor>(settings.value(ckey, defColor));
        if (&f == &m_clockFont) { m_initialClockFont = f; m_initialClockColor = c; }
        else if (&f == &m_feed1Font) { m_initialFeed1Font = f; m_initialFeed1Color = c; }
        else if (&f == &m_feed2Font) { m_initialFeed2Font = f; m_initialFeed2Color = c; }
        else if (&f == &m_feed3Font) { m_initialFeed3Font = f; m_initialFeed3Color = c; }
        else if (&f == &m_callInfoFont) { m_initialCallInfoFont = f; m_initialCallInfoColor = c; }
        else if (&f == &m_qsoListFont) { m_initialQsoListFont = f; m_initialQsoListColor = c; }
`;
code = code.replace(fcolorMatch, fcolorRepl);

// 2. Remove old buttons logic and add side scroll for Preview Pane
const addFontColorMatch = `        QPushButton *fBtn = new QPushButton("Font...");
        QPushButton *cBtn = new QPushButton("Color...");
        QPushButton *rBtn = new QPushButton("Reset");
        connect(fBtn, &QPushButton::clicked, this, fontSlot);
        connect(cBtn, &QPushButton::clicked, this, colorSlot);
        connect(rBtn, &QPushButton::clicked, this, [&f, &c, defFont, defColor]() {
            f = defFont;
            c = defColor;
        });
        
        QHBoxLayout *hl = new QHBoxLayout;
        hl->addWidget(fBtn);
        hl->addWidget(cBtn);
        hl->addWidget(rBtn);
        layout->addRow(label, hl);
    };`;
code = code.replace(addFontColorMatch, `        // Replaced by StyleDialog
    };`);

const previewMatch = `    layout->addRow(new QLabel("<b>UI Preview (Click text to edit style):</b>"));
    layout->addRow(m_uiPreview);`;
const previewRepl = `    QHBoxLayout *navLayout = new QHBoxLayout;
    QPushButton *btnLeft = new QPushButton("<");
    btnLeft->setFixedWidth(30);
    QPushButton *btnRight = new QPushButton(">");
    btnRight->setFixedWidth(30);
    QLabel *lblTitle = new QLabel("<b>Main Window Preview (Click text to edit style)</b>");
    lblTitle->setAlignment(Qt::AlignCenter);
    navLayout->addWidget(btnLeft);
    navLayout->addWidget(lblTitle, 1);
    navLayout->addWidget(btnRight);
    
    connect(btnLeft, &QPushButton::clicked, this, [this, lblTitle]() {
        m_currentPreviewPane = (m_currentPreviewPane - 1 + 3) % 3;
        lblTitle->setText(m_currentPreviewPane == 0 ? "<b>Main Window Preview (Click text to edit style)</b>" : (m_currentPreviewPane == 1 ? "<b>Print QSL Preview</b>" : "<b>Design QSLs Preview</b>"));
        updateUIPreview();
    });
    connect(btnRight, &QPushButton::clicked, this, [this, lblTitle]() {
        m_currentPreviewPane = (m_currentPreviewPane + 1) % 3;
        lblTitle->setText(m_currentPreviewPane == 0 ? "<b>Main Window Preview (Click text to edit style)</b>" : (m_currentPreviewPane == 1 ? "<b>Print QSL Preview</b>" : "<b>Design QSLs Preview</b>"));
        updateUIPreview();
    });
    
    layout->addRow(navLayout);
    layout->addRow(m_uiPreview);`;
code = code.replace(previewMatch, previewRepl);

const anchorMatch = `        // Since we cannot easily know if they want font or color, we can ask them via a small message box
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Edit Style");
        msgBox.setText("What would you like to edit?");
        QPushButton *fontBtn = msgBox.addButton("Font", QMessageBox::ActionRole);
        QPushButton *colorBtn = msgBox.addButton("Color", QMessageBox::ActionRole);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.exec();
        
        if (msgBox.clickedButton() == fontBtn) {
            if (action == "clock") chooseClockFont();
            else if (action == "feed1") chooseFeed1Font();
            else if (action == "feed2") chooseFeed2Font();
            else if (action == "feed3") chooseFeed3Font();
            else if (action == "callInfo") chooseCallInfoFont();
            else if (action == "qsoList") chooseQsoListFont();
        } else if (msgBox.clickedButton() == colorBtn) {
            if (action == "clock") chooseClockColor();
            else if (action == "feed1") chooseFeed1Color();
            else if (action == "feed2") chooseFeed2Color();
            else if (action == "feed3") chooseFeed3Color();
            else if (action == "callInfo") chooseCallInfoColor();
            else if (action == "qsoList") chooseQsoListColor();
        }`;
const anchorRepl = `        QFont *f = nullptr;
        QColor *c = nullptr;
        if (action == "clock") { f = &m_clockFont; c = &m_clockColor; }
        else if (action == "feed1") { f = &m_feed1Font; c = &m_feed1Color; }
        else if (action == "feed2") { f = &m_feed2Font; c = &m_feed2Color; }
        else if (action == "feed3") { f = &m_feed3Font; c = &m_feed3Color; }
        else if (action == "callInfo") { f = &m_callInfoFont; c = &m_callInfoColor; }
        else if (action == "qsoList") { f = &m_qsoListFont; c = &m_qsoListColor; }
        
        if (f && c) {
            StyleDialog sd(*f, *c, this);
            if (sd.exec() == QDialog::Accepted) {
                *f = sd.selectedFont();
                *c = sd.selectedColor();
                updateUIPreview();
            }
        }`;
code = code.replace(anchorMatch, anchorRepl);

// 3. Update preview switch
const updatePreviewMatch = `void ConfigDialog::updateUIPreview()
{
    if (!m_uiPreview) return;
    
    QString html = QString("<div style='background-color: %1; padding: 10px;'>").arg(m_selectedColor);`;

const updatePreviewRepl = `void ConfigDialog::updateUIPreview()
{
    if (!m_uiPreview) return;
    
    if (m_currentPreviewPane == 1) {
        // Print QSL Preview mock
        QString techStyle = "font-family: 'Courier New'; font-size: 11pt; font-weight: bold;";
        QString html = "<div style='background-color: #fdfdfd; padding: 10px; color: #333;'>";
        html += "<table width='100%' border='1' cellspacing='0' cellpadding='5' style='border-collapse: collapse;'>";
        html += "<tr style='background-color: #e0e0e0; " + techStyle + "'><th>Callsign</th><th>Date</th><th>Band</th></tr>";
        html += "<tr style='" + techStyle + "'><td align='center'>W4DJF</td><td align='center'>2026-06-27</td><td align='center'>20m</td></tr>";
        html += "</table>";
        html += "</div>";
        m_uiPreview->setHtml(html);
        return;
    } else if (m_currentPreviewPane == 2) {
        // Design QSLs Preview mock
        QString html = "<div style='background-color: #333; padding: 10px; color: white;'>";
        html += "<div style='background-color: white; color: black; padding: 20px; border: 2px dashed gray; width: 300px; margin: 0 auto; text-align: center;'>";
        html += "<h2 style='color: navy;'>Mock QSL Card</h2>";
        html += "<p style='font-family: Arial; font-size: 12pt;'>Confirming QSO with W4DJF</p>";
        html += "</div></div>";
        m_uiPreview->setHtml(html);
        return;
    }

    QString html = QString("<div style='background-color: %1; padding: 10px;'>").arg(m_selectedColor);`;
code = code.replace(updatePreviewMatch, updatePreviewRepl);

// Add #include "styledialog.h"
code = code.replace(`#include <QFontDialog>`, `#include <QFontDialog>\n#include "styledialog.h"`);

// Add Revert button to the buttonBox
const bbMatch = `    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &ConfigDialog::applySettings);`;

const bbRepl = `    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &ConfigDialog::applySettings);
    
    QPushButton *revertBtn = new QPushButton(tr("Revert"));
    buttonBox->addButton(revertBtn, QDialogButtonBox::ResetRole);
    connect(revertBtn, &QPushButton::clicked, this, [this](){
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Revert Settings");
        msgBox.setText("How would you like to revert colors and fonts?");
        QPushButton *startBtn = msgBox.addButton("Revert to Start", QMessageBox::ActionRole);
        QPushButton *defBtn = msgBox.addButton("Revert to Defaults", QMessageBox::ActionRole);
        msgBox.addButton(QMessageBox::Cancel);
        msgBox.exec();

        if (msgBox.clickedButton() == startBtn) {
            m_clockFont = m_initialClockFont; m_clockColor = m_initialClockColor;
            m_feed1Font = m_initialFeed1Font; m_feed1Color = m_initialFeed1Color;
            m_feed2Font = m_initialFeed2Font; m_feed2Color = m_initialFeed2Color;
            m_feed3Font = m_initialFeed3Font; m_feed3Color = m_initialFeed3Color;
            m_callInfoFont = m_initialCallInfoFont; m_callInfoColor = m_initialCallInfoColor;
            m_qsoListFont = m_initialQsoListFont; m_qsoListColor = m_initialQsoListColor;
            m_selectedColor = m_initialSelectedColor;
            if(m_currentColorLabel) m_currentColorLabel->setText(m_selectedColor);
            updateUIPreview();
        } else if (msgBox.clickedButton() == defBtn) {
            m_clockFont = QFont("Arial", 10); m_clockColor = Qt::white;
            m_feed1Font = QFont("Arial", 10); m_feed1Color = Qt::white;
            m_feed2Font = QFont("Arial", 10); m_feed2Color = Qt::white;
            m_feed3Font = QFont("Arial", 10); m_feed3Color = Qt::white;
            m_callInfoFont = QFont("Arial", 10); m_callInfoColor = Qt::white;
            m_qsoListFont = QFont("Arial", 10); m_qsoListColor = Qt::white;
            m_selectedColor = "#221e1f";
            if(m_currentColorLabel) m_currentColorLabel->setText(m_selectedColor);
            updateUIPreview();
        }
    });`;
code = code.replace(bbMatch, bbRepl);

fs.writeFileSync('cpp_source/configdialog.cpp', code);
