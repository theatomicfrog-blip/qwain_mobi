const fs = require('fs');
let svg = fs.readFileSync('cpp_source/world-states-provinces.svg.h', 'utf8');
let idMatch = svg.match(/id="usa"[\s\S]*?(<\/g>|\/>)/);
console.log(idMatch ? idMatch[0] : "not found");
