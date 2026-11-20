const fs = require("fs");
let p = "cpp_source/mainwindow.cpp";
let c = fs.readFileSync(p, "utf8");

let search = `        QString addressLine = qrzAddr2;`;
let replace = `        QString addressLine = qrzAddr1.isEmpty() ? qrzAddr2 : qrzAddr1;`;

c = c.replace(search, replace);

fs.writeFileSync(p, c);
