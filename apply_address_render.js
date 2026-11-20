const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

let search = `        bool isEnvelopeAddress = m_previewingEnvelope && 
                (lowerTxt.contains("address") || lowerTxt.contains("name") || lowerTxt == "city" || lowerTxt.contains("country") || lowerTxt == "<state>" || lowerTxt == "<zip>" || lowerTxt.contains("mailing"));
            
            if (!isEnvelopeAddress) {`;

if (!cppCode.includes(search)) {
    // try shorter
    search = `            bool isEnvField = m_previewingEnvelope && (lowerTxt.contains("address") || lowerTxt.contains("name") || lowerTxt == "city" || lowerTxt.contains("country") || lowerTxt == "<state>" || lowerTxt == "<zip>");
            if (!isEnvField) ti->setTextWidth(w);`;
    let replace = `            bool isEnvField = m_previewingEnvelope && (lowerTxt.contains("address") || lowerTxt.contains("name") || lowerTxt == "city" || lowerTxt.contains("country") || lowerTxt == "<state>" || lowerTxt == "<zip>");
            if (isEnvField) hasEnvelopeAddress = true;
            if (!isEnvField) ti->setTextWidth(w);`;
    cppCode = cppCode.replace(search, replace);
}

let topDecl = `    bool potaRendered = false;`;
let topDeclReplace = `    bool potaRendered = false;
    bool hasEnvelopeAddress = false;`;
cppCode = cppCode.replace(topDecl, topDeclReplace);

let botDecl = `    m_potaWidget->setVisible(hasPota);`;
let botDeclReplace = `    m_potaWidget->setVisible(hasPota);
    if (m_btnAddressFont) m_btnAddressFont->setVisible(m_previewingEnvelope && hasEnvelopeAddress);`;
cppCode = cppCode.replace(botDecl, botDeclReplace);

let searchFont = `            if (!m_currentTemplate.isEmpty()) {
                if (idStr.toLower() == "call" || idStr.toLower() == "callsign" || idStr.toLower() == "<call>") {
                    QString cf = settings.value(QString("QslTemplateOverrides/%1/CallFont").arg(m_currentTemplate)).toString();
                    QString cc = settings.value(QString("QslTemplateOverrides/%1/CallColor").arg(m_currentTemplate)).toString();
                    if (!cf.isEmpty()) font.fromString(cf);
                    if (!cc.isEmpty()) color = QColor(cc);
                } else if (idStr.toLower() == "<pota_ref>" || idStr.toLower() == "<pota>" || idStr.toLower() == "pota" || idStr.toLower().contains("pota string")) {
                    QString pf = settings.value(QString("QslTemplateOverrides/%1/PotaFont").arg(m_currentTemplate)).toString();
                    QString pc = settings.value(QString("QslTemplateOverrides/%1/PotaColor").arg(m_currentTemplate)).toString();
                    if (!pf.isEmpty()) font.fromString(pf);
                    if (!pc.isEmpty()) color = QColor(pc);
                }
            }`;

let replaceFont = `            if (!m_currentTemplate.isEmpty()) {
                QString low = idStr.toLower();
                if (low == "call" || low == "callsign" || low == "<call>") {
                    QString cf = settings.value(QString("QslTemplateOverrides/%1/CallFont").arg(m_currentTemplate)).toString();
                    QString cc = settings.value(QString("QslTemplateOverrides/%1/CallColor").arg(m_currentTemplate)).toString();
                    if (!cf.isEmpty()) font.fromString(cf);
                    if (!cc.isEmpty()) color = QColor(cc);
                } else if (low == "<pota_ref>" || low == "<pota>" || low == "pota" || low.contains("pota string")) {
                    QString pf = settings.value(QString("QslTemplateOverrides/%1/PotaFont").arg(m_currentTemplate)).toString();
                    QString pc = settings.value(QString("QslTemplateOverrides/%1/PotaColor").arg(m_currentTemplate)).toString();
                    if (!pf.isEmpty()) font.fromString(pf);
                    if (!pc.isEmpty()) color = QColor(pc);
                } else if (m_previewingEnvelope && (low.contains("address") || low.contains("name") || low == "city" || low.contains("country") || low == "<state>" || low == "<zip>")) {
                    QString af = settings.value(QString("QslTemplateOverrides/%1/AddressFont").arg(m_currentTemplate)).toString();
                    QString ac = settings.value(QString("QslTemplateOverrides/%1/AddressColor").arg(m_currentTemplate)).toString();
                    if (!af.isEmpty()) font.fromString(af);
                    if (!ac.isEmpty()) color = QColor(ac);
                }
            }`;
cppCode = cppCode.replace(searchFont, replaceFont);

fs.writeFileSync(cppPath, cppCode);
