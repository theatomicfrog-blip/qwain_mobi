const fs = require("fs");
let p = "cpp_source/mainwindow.cpp";
let c = fs.readFileSync(p, "utf8");

let search = `    QString name = extractAdifField(record, "NAME");
    QString addr2 = extractAdifField(record, "ADDR2");
    QString city = extractAdifField(record, "CITY");`;

let replace = `    QString name = extractAdifField(record, "NAME");
    QString addr2 = extractAdifField(record, "ADDRESS");
    if (addr2.isEmpty()) addr2 = extractAdifField(record, "ADDR2");
    QString city = extractAdifField(record, "CITY");`;

c = c.replace(search, replace);

fs.writeFileSync(p, c);
