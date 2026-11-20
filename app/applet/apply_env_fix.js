const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

let searchTxt = `ti->setTextWidth(w);
            ti->document()->setDocumentMargin(0);`;

let replaceTxt = `bool isEnvelopeAddress = m_previewingEnvelope && 
                (lowerTxt.contains("address") || lowerTxt.contains("name") || lowerTxt == "city" || lowerTxt.contains("country") || lowerTxt == "<state>" || lowerTxt == "<zip>");
            
            if (!isEnvelopeAddress) {
                ti->setTextWidth(w);
            }
            ti->document()->setDocumentMargin(0);`;

cppCode = cppCode.replace(searchTxt, replaceTxt);

let searchHtmlSetting = `            } else {
                ti->setHtml(QString("<div align='center'>%1</div>").arg(txt.replace("\\n", "<br>")));
            }`;
let replaceHtmlSetting = `            } else if (isEnvelopeAddress) {
                QString newHtml = "<div align='center'>";
                QStringList lines = txt.split("\\n");
                for(int l=0; l<lines.size(); ++l) {
                    newHtml += "<span style='white-space:nowrap;'>" + lines[l] + "</span>";
                    if(l < lines.size()-1) newHtml += "<br>";
                }
                newHtml += "</div>";
                ti->setHtml(newHtml);
            } else {
                ti->setHtml(QString("<div align='center'>%1</div>").arg(txt.replace("\\n", "<br>")));
            }`;

cppCode = cppCode.replace(searchHtmlSetting, replaceHtmlSetting);

fs.writeFileSync(cppPath, cppCode);
