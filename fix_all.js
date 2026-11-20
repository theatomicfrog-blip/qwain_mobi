const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

// Fix 1: Move hasEnvelopeAddress
let search1 = `    m_potaWidget->setVisible(hasPota);
    if (m_btnAddressFont) m_btnAddressFont->setVisible(m_previewingEnvelope && hasEnvelopeAddress);
    m_chkOptionsGroup->hide();
    QStringList blankFields;
    bool potaRendered = false;
    bool hasEnvelopeAddress = false;`;

let replace1 = `    bool hasEnvelopeAddress = false;
    m_potaWidget->setVisible(hasPota);
    // Address font button visibility will be handled at the end, after checking all fields
    m_chkOptionsGroup->hide();
    QStringList blankFields;
    bool potaRendered = false;`;

cppCode = cppCode.replace(search1, replace1);

// Fix 2: isEnvField
let search2 = `            bool hasHumanFont = false;
            if (!m_currentTemplate.isEmpty()) {
                if (lowerTxt == "call" || lowerTxt == "callsign" || lowerTxt == "<call>") {`;

let replace2 = `            bool isEnvField = m_previewingEnvelope && (lowerTxt.contains("address") || lowerTxt.contains("name") || lowerTxt == "city" || lowerTxt.contains("country") || lowerTxt == "<state>" || lowerTxt == "<zip>");
            
            bool hasHumanFont = false;
            if (!m_currentTemplate.isEmpty()) {
                if (lowerTxt == "call" || lowerTxt == "callsign" || lowerTxt == "<call>") {`;

cppCode = cppCode.replace(search2, replace2);

// Remove the old isEnvField declaration
let search3 = `            // Allow text wrapping
            bool isEnvField = m_previewingEnvelope && (lowerTxt.contains("address") || lowerTxt.contains("name") || lowerTxt == "city" || lowerTxt.contains("country") || lowerTxt == "<state>" || lowerTxt == "<zip>");
            if (isEnvField) hasEnvelopeAddress = true;`;

let replace3 = `            // Allow text wrapping
            if (isEnvField) hasEnvelopeAddress = true;`;

cppCode = cppCode.replace(search3, replace3);

// Fix 3: move changeAddressFont
let search4 = `void QslPreviewDialog::changeAddressFont() {
    if (m_currentTemplate.isEmpty()) return;
    
    QFont f("Arial", 14);
    QColor c(Qt::black);
    for (const QVariantMap &m : m_templateItems) {
        QString idStr = m["id"].toString().toLower();
        if (idStr == "address" || idStr == "name" || idStr == "city" || idStr == "country" || idStr == "<state>" || idStr == "<zip>" || idStr == "full address" || idStr == "mailing address" || idStr == "<full address>") {
            f = QFont(m["font"].toString(), qMax(6, m["size"].toInt()));
            c = QColor(m["color"].toString());
            break;
        }
    }
    
    QSettings settings(QCoreApplication::organizationName(), "AdifMonitor");
    QString cf = settings.value(QString("QslTemplateOverrides/%1/AddressFont").arg(m_currentTemplate)).toString();
    QString cc = settings.value(QString("QslTemplateOverrides/%1/AddressColor").arg(m_currentTemplate)).toString();
    if (!cf.isEmpty()) f.fromString(cf);
    if (!cc.isEmpty()) c = QColor(cc);

    FontColorChooserDialog dlg(f, c, this);
    dlg.onApply = [&](const QFont &newF, const QColor &newC) {
        saveStateForUndo();
        QSettings s(QCoreApplication::organizationName(), "AdifMonitor");
        s.setValue(QString("QslTemplateOverrides/%1/AddressFont").arg(m_currentTemplate), newF.toString());
        s.setValue(QString("QslTemplateOverrides/%1/AddressColor").arg(m_currentTemplate), newC.name());
        renderTemplate();
    };
    dlg.exec();
}
`;

let replace4 = ``;
cppCode = cppCode.replace(search4, replace4);

let search5 = `void QslPreviewDialog::changeCallFont() {`;
let replace5 = `void QslPreviewDialog::changeAddressFont() {
    if (m_currentTemplate.isEmpty()) return;
    
    QFont f("Arial", 14);
    QColor c(Qt::black);
    for (const QVariantMap &m : m_templateItems) {
        QString idStr = m["id"].toString().toLower();
        if (idStr == "address" || idStr == "name" || idStr == "city" || idStr == "country" || idStr == "<state>" || idStr == "<zip>" || idStr == "full address" || idStr == "mailing address" || idStr == "<full address>") {
            f = QFont(m["font"].toString(), qMax(6, m["size"].toInt()));
            c = QColor(m["color"].toString());
            break;
        }
    }
    
    QSettings settings(QCoreApplication::organizationName(), "AdifMonitor");
    QString cf = settings.value(QString("QslTemplateOverrides/%1/AddressFont").arg(m_currentTemplate)).toString();
    QString cc = settings.value(QString("QslTemplateOverrides/%1/AddressColor").arg(m_currentTemplate)).toString();
    if (!cf.isEmpty()) f.fromString(cf);
    if (!cc.isEmpty()) c = QColor(cc);

    saveStateForUndo();
    bool changed = false;

    FontColorChooserDialog dlg(f, c, this);
    dlg.onApply = [this, &changed](const QFont &newF, const QColor &newC) {
        changed = true;
        QSettings s(QCoreApplication::organizationName(), "AdifMonitor");
        s.setValue(QString("QslTemplateOverrides/%1/AddressFont").arg(m_currentTemplate), newF.toString());
        s.setValue(QString("QslTemplateOverrides/%1/AddressColor").arg(m_currentTemplate), newC.name());
        renderTemplate();
    };
    dlg.exec();
    
    if (!changed) m_undoStack.takeLast();
}

void QslPreviewDialog::changeCallFont() {`;

cppCode = cppCode.replace(search5, replace5);

fs.writeFileSync(cppPath, cppCode);
