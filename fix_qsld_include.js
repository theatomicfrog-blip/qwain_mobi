const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

if (!cppCode.includes("qsldesigner.h")) {
    cppCode = cppCode.replace("#include \\"qslpreviewdialog.h\\"", "#include \\"qslpreviewdialog.h\\"\\n#include \\"qsldesigner.h\\"");
    fs.writeFileSync(cppPath, cppCode);
}
