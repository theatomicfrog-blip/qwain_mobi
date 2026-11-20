const fs = require('fs');
let p = 'cpp_source/qslpreviewdialog.cpp';
let c = fs.readFileSync(p, 'utf8');

c = c.replace('ti->setTextWidth(w);\n            ti->document()->setDocumentMargin(0);', 
    'bool isEnvField = m_previewingEnvelope && (lowerTxt.contains("address") || lowerTxt.contains("name") || lowerTxt == "city" || lowerTxt.contains("country") || lowerTxt == "<state>" || lowerTxt == "<zip>");\n            if (!isEnvField) ti->setTextWidth(w);\n            ti->document()->setDocumentMargin(0);');

let s = '} else {\n                ti->setHtml(QString("<div align=\'center\'>%1</div>").arg(txt.replace("\\n", "<br>")));\n            }';
let r = '} else if (isEnvField) {\n                QString nh = "<div align=\'center\'>";\n                QStringList lines = txt.split("\\n");\n                for(int l=0; l<lines.size(); ++l) {\n                    nh += "<span style=\'white-space:nowrap;\'>" + lines[l] + "</span>";\n                    if(l < lines.size()-1) nh += "<br>";\n                }\n                nh += "</div>";\n                ti->setHtml(nh);\n            }' + s.substring(6);

c = c.replace(s, r);
fs.writeFileSync(p, c);
