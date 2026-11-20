const fs = require('fs');

let code = fs.readFileSync('cpp_source/qslpreviewdialog.cpp', 'utf8');

// Remove FontColorChooserDialog class entirely
const fcClassRegex = /class FontColorChooserDialog : public QDialog \{[\s\S]*?\n    \}\n\nprivate:\n    void updateColorBtn\(\) \{[\s\S]*?\n    \}\n\n    QFont m_font;\n    QColor m_color;\n    QFontComboBox \*m_fontCombo;\n    QSpinBox \*m_sizeSpin;\n    QCheckBox \*m_boldCheck;\n    QCheckBox \*m_italicCheck;\n    QPushButton \*m_colorBtn;\n\};\n/m;
code = code.replace(fcClassRegex, '');

// include styledialog.h
code = code.replace('#include <QFontDialog>', '#include <QFontDialog>\n#include "styledialog.h"');

// Replace usages
// Note: We need to match the instantiation and connection of onApply
// Example:
/*
    FontColorChooserDialog dlg(f, c, this);
    dlg.onApply = [this, &changed](const QFont &rf, const QColor &rc) {
        changed = true;
        QSettings s(QCoreApplication::organizationName(), "AdifMonitor");
        s.setValue(QString("QslTemplateOverrides/%1/TableFont").arg(m_currentTemplate), rf.toString());
        s.setValue(QString("QslTemplateOverrides/%1/TableColor").arg(m_currentTemplate), rc.name());
        renderTemplate();
    };

    if (dlg.exec() == QDialog::Accepted) {
        dlg.onApply(dlg.font(), dlg.color());
    } else if (changed) {
        undo(); // Rolled back cancelled changes!
    }
*/

const replaceDialogBlock = (targetName) => {
    const regex = new RegExp(`    FontColorChooserDialog dlg\\(f, c, this\\);\n    dlg\\.onApply = \\[this, &changed\\]\\(const QFont &([^,]+), const QColor &([^)]+)\\) \\{\n([\\s\\S]*?)\n    \\};\n\n    if \\(dlg\\.exec\\(\\) == QDialog::Accepted\\) \\{\n        dlg\\.onApply\\(dlg\\.font\\(\\), dlg\\.color\\(\\)\\);\n    \\} else if \\(changed\\) \\{\n        undo\\(\\); // Rolled back cancelled changes!\n    \\}`, 'm');
    
    code = code.replace(regex, (match, rf, rc, body) => {
        return `    StyleDialog dlg(f, c, this);
    auto applyFn = [this, &changed](const QFont &${rf}, const QColor &${rc}) {
${body}
    };
    connect(&dlg, &StyleDialog::styleChanged, this, applyFn);

    if (dlg.exec() == QDialog::Accepted) {
        applyFn(dlg.selectedFont(), dlg.selectedColor());
    } else if (changed) {
        undo(); // Rolled back cancelled changes!
    }`;
    });
};

replaceDialogBlock();
replaceDialogBlock();
replaceDialogBlock();
replaceDialogBlock();

fs.writeFileSync('cpp_source/qslpreviewdialog.cpp', code);
