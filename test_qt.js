const fs = require("fs");
let cppCode = `
#include <QCoreApplication>
#include <QTextDocument>
#include <QDebug>
#include <QString>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QTextDocument doc;
    doc.setHtml("<h2>W4DJF</h2><b>Name:</b> John Doe<br><b>Address:</b> 123 Main St<br><b>City:</b> Any, ST<br><b>Country:</b> USA");
    qDebug().noquote() << doc.toPlainText();
    return 0;
}
`;
fs.writeFileSync("test.cpp", cppCode);
