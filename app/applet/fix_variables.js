const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

cppCode = cppCode.replace("leftWidget = new", "QWidget *leftWidget = new");
cppCode = cppCode.replace("contactsBtnLayout = new", "QHBoxLayout *contactsBtnLayout = new");
cppCode = cppCode.replace("rightWidget = new", "QWidget *rightWidget = new");

if (!cppCode.includes("qsldesigner.h")) {
    cppCode = cppCode.replace('#include "mainwindow.h"', '#include "mainwindow.h"\n#include "qsldesigner.h"');
}

fs.writeFileSync(cppPath, cppCode);
