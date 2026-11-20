const fs = require('fs');
let svg = fs.readFileSync('cpp_source/world-states-provinces.svg.h', 'utf8');
let ids = svg.match(/id=\"([^\"]+)\"/g) || [];
console.log(ids.slice(0, 50).join('\n'));
