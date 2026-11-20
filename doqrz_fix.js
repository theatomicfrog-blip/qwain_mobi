const fs = require("fs");
let p = "cpp_source/mainwindow.cpp";
let c = fs.readFileSync(p, "utf8");

let search = `        QString htmlInfo = QString("<h2>%1</h2>%2<br>%3<br>%4")
                               .arg(callsign, finalName, cityState, finalCountry);`;

let replace = `        QString addressLine = qrzAddr2;
        if (!adifAddr2.isEmpty()) addressLine = adifAddr2;
        QString htmlInfo = QString("<h2>%1</h2><b>Name:</b> %2<br><b>Address:</b> %3<br><b>City:</b> %4<br><b>Country:</b> %5")
                               .arg(callsign, finalName, addressLine, cityState, finalCountry);`;

c = c.replace(search, replace);

fs.writeFileSync(p, c);
