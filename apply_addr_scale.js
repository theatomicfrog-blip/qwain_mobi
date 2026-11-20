const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

let search = `            bool hasHumanCallFont = false;
            if (!m_currentTemplate.isEmpty() && (lowerTxt == "call" || lowerTxt == "callsign" || lowerTxt == "<call>")) {
                hasHumanCallFont = !settings.value(QString("QslTemplateOverrides/%1/CallFont").arg(m_currentTemplate)).toString().isEmpty();
            }
            
            if (!hasHumanCallFont && (br.width() > w || br.height() > h)) {`;

let replace = `            bool hasHumanCallFont = false;
            bool hasHumanAddrFont = false;
            if (!m_currentTemplate.isEmpty()) {
                if (lowerTxt == "call" || lowerTxt == "callsign" || lowerTxt == "<call>") {
                    hasHumanCallFont = !settings.value(QString("QslTemplateOverrides/%1/CallFont").arg(m_currentTemplate)).toString().isEmpty();
                } else if (isEnvField) {
                    hasHumanAddrFont = !settings.value(QString("QslTemplateOverrides/%1/AddressFont").arg(m_currentTemplate)).toString().isEmpty();
                }
            }
            
            if (!hasHumanCallFont && !hasHumanAddrFont && (br.width() > w || br.height() > h)) {`;

cppCode = cppCode.replace(search, replace);

let search2 = `            bool hasHumanFont = false;
            if (!m_currentTemplate.isEmpty()) {
                if (lowerTxt == "call" || lowerTxt == "callsign" || lowerTxt == "<call>") {
                    hasHumanFont = !settings.value(QString("QslTemplateOverrides/%1/CallFont").arg(m_currentTemplate)).toString().isEmpty();
                } else if (lowerTxt.contains("pota string") || lowerTxt == "<pota_ref>" || lowerTxt == "<pota>" || lowerTxt == "pota") {
                    hasHumanFont = !settings.value(QString("QslTemplateOverrides/%1/PotaFont").arg(m_currentTemplate)).toString().isEmpty();
                }`;

let replace2 = `            bool hasHumanFont = false;
            if (!m_currentTemplate.isEmpty()) {
                if (lowerTxt == "call" || lowerTxt == "callsign" || lowerTxt == "<call>") {
                    hasHumanFont = !settings.value(QString("QslTemplateOverrides/%1/CallFont").arg(m_currentTemplate)).toString().isEmpty();
                } else if (lowerTxt.contains("pota string") || lowerTxt == "<pota_ref>" || lowerTxt == "<pota>" || lowerTxt == "pota") {
                    hasHumanFont = !settings.value(QString("QslTemplateOverrides/%1/PotaFont").arg(m_currentTemplate)).toString().isEmpty();
                } else if (isEnvField) {
                    hasHumanFont = !settings.value(QString("QslTemplateOverrides/%1/AddressFont").arg(m_currentTemplate)).toString().isEmpty();
                }`;

cppCode = cppCode.replace(search2, replace2);

fs.writeFileSync(cppPath, cppCode);
