const fs = require('fs');
let svg = fs.readFileSync('cpp_source/world-states-provinces.svg.h', 'utf8');
let styleMatch = svg.match(/<style[^>]*>([\s\S]*?)<\/style>/i);
console.log(styleMatch ? styleMatch[1] : "no style");
