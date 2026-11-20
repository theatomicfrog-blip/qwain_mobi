const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

let searchBot = `    m_potaWidget->setVisible(hasPota);
    if (m_btnAddressFont) m_btnAddressFont->setVisible(m_previewingEnvelope && hasEnvelopeAddress);`;
let replaceBot = `    m_potaWidget->setVisible(hasPota);`;
cppCode = cppCode.replace(searchBot, replaceBot);

let searchLoopEnd = `            if (idStr.toLower() == "<comment>") {
                // If it's the comment block, make it visible to user tweaks if present
                hasPota = true;
            }
        }
    }
    
    // Scale view if scene is smaller
    m_view->setAlignment(Qt::AlignCenter);`;

let replaceLoopEnd = `            if (idStr.toLower() == "<comment>") {
                // If it's the comment block, make it visible to user tweaks if present
                hasPota = true;
            }
        }
    }
    
    if (m_btnAddressFont) m_btnAddressFont->setVisible(m_previewingEnvelope && hasEnvelopeAddress);

    // Scale view if scene is smaller
    m_view->setAlignment(Qt::AlignCenter);`;

cppCode = cppCode.replace(searchLoopEnd, replaceLoopEnd);
fs.writeFileSync(cppPath, cppCode);
