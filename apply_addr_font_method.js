const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

let search = `void QslPreviewDialog::undo() {`;
let replace = `void QslPreviewDialog::changeAddressFont() {
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

void QslPreviewDialog::undo() {`;

cppCode = cppCode.replace(search, replace);

fs.writeFileSync(cppPath, cppCode);
