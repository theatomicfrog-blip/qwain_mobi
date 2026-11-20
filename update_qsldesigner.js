const fs = require('fs');

let code = fs.readFileSync('cpp_source/qsldesigner.cpp', 'utf8');

// Include styledialog.h
code = code.replace('#include <QColorDialog>', '#include <QColorDialog>\n#include "styledialog.h"');

// Replace QColorDialog::getColor in m_colorBtn->clicked lambda inside QslDesigner properties panel (around line 1454)
/*
    connect(m_colorBtn, &QPushButton::clicked, this, [this]() {
        QColor c = QColorDialog::getColor(m_activeColor, this, "Select Font Color");
        if(c.isValid()) {
            m_activeColor = c;
            updateActiveItemProperties();
        }
    });
*/
code = code.replace(/    connect\(m_colorBtn, &QPushButton::clicked, this, \[this\]\(\) \{\n        QColor c = QColorDialog::getColor\(m_activeColor, this, "Select Font Color"\);\n        if\(c\.isValid\(\)\) \{\n            m_activeColor = c;\n            updateActiveItemProperties\(\);\n        \}\n    \}\);/, 
    `    connect(m_colorBtn, &QPushButton::clicked, this, [this]() {
        QFont f; f.setFamily(m_fontCombo->currentText()); f.setPointSize(m_sizeSpin->value());
        StyleDialog dlg(f, m_activeColor, this);
        if (dlg.exec() == QDialog::Accepted) {
            m_activeColor = dlg.selectedColor();
            m_fontCombo->setCurrentText(dlg.selectedFont().family());
            m_sizeSpin->setValue(dlg.selectedFont().pointSize());
            updateActiveItemProperties();
        }
    });`);

// Replace QColorDialog::getColor inside double-click dialog (around line 490)
// The colorBtn click
code = code.replace(/        QObject::connect\(colorBtn, &QPushButton::clicked, \[\&\]\(\) \{\n            QColor c = QColorDialog::getColor\(activeColor, &d, "Select Font Color"\);\n            if \(c\.isValid\(\)\) \{\n                activeColor = c;\n                updateArea\(\);\n            \}\n        \}\);/g,
    `        QObject::connect(colorBtn, &QPushButton::clicked, [&]() {
            QFont f = fontCombo->currentFont(); f.setPointSize(sizeSpin->value());
            StyleDialog dlg(f, activeColor, &d);
            if (dlg.exec() == QDialog::Accepted) {
                activeColor = dlg.selectedColor();
                fontCombo->setCurrentFont(dlg.selectedFont());
                sizeSpin->setValue(dlg.selectedFont().pointSize());
                updateArea();
            }
        });`);

fs.writeFileSync('cpp_source/qsldesigner.cpp', code);
