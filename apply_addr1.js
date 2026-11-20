const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

let searchField = `else if (lowerTxt == "address" || lowerTxt == "addr" || lowerTxt == "<address>") txt = m_addrStreet.isEmpty() ? textStr : m_addrStreet;`;
let replaceField = `else if (lowerTxt == "address" || lowerTxt == "addr" || lowerTxt == "<address>" || lowerTxt == "<addr1>" || lowerTxt == "addr1") txt = m_addrStreet.isEmpty() ? textStr : m_addrStreet;`;

cppCode = cppCode.replace(searchField, replaceField);

let searchEnvEnv = `bool isEnvField = m_previewingEnvelope && (lowerTxt.contains("address") || lowerTxt.contains("name") || lowerTxt == "city" || lowerTxt.contains("country") || lowerTxt == "<state>" || lowerTxt == "<zip>");`;
let replaceEnvEnv = `bool isEnvField = m_previewingEnvelope && (lowerTxt.contains("address") || lowerTxt.contains("addr") || lowerTxt.contains("name") || lowerTxt == "city" || lowerTxt.contains("country") || lowerTxt == "<state>" || lowerTxt == "<zip>");`;

cppCode = cppCode.replace(searchEnvEnv, replaceEnvEnv);

let searchEnvFont = `if (idStr == "address" || idStr == "name" || idStr == "city" || idStr == "country" || idStr == "<state>" || idStr == "<zip>" || idStr == "full address" || idStr == "mailing address" || idStr == "<full address>") {`;
let replaceEnvFont = `if (idStr == "address" || idStr.contains("addr") || idStr == "name" || idStr == "city" || idStr == "country" || idStr == "<state>" || idStr == "<zip>" || idStr == "full address" || idStr == "mailing address" || idStr == "<full address>") {`;

cppCode = cppCode.replace(searchEnvFont, replaceEnvFont);

fs.writeFileSync(cppPath, cppCode);
