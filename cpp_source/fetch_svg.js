const fs = require('fs');
const lines = fs.readFileSync('./cpp_source/fips.csv', 'utf8').split('\n');
let out = '#ifndef FIPS_DATA_H\n#define FIPS_DATA_H\n\n#include <QMap>\n#include <QString>\n\ninline QMap<QString, QString> getFipsMap() {\n    QMap<QString, QString> map;\n';
for (let i = 1; i < lines.length; i++) {
  const parts = lines[i].split(',');
  if (parts.length >= 3) {
    let fips = parts[0].trim();
    if (fips.length === 4) fips = '0' + fips;
    let name = parts[1].toUpperCase().replace(' COUNTY', '').replace(' PARISH', '').trim();
    const state = parts[2].trim();
    out += `    map["${state},${name}"] = "c${fips}";\n`;
  }
}
out += '    return map;\n}\n\n#endif\n';
fs.writeFileSync('./cpp_source/fips_data.h', out);
console.log('done');



