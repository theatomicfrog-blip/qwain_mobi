const fs = require('fs');
let svg = fs.readFileSync('cpp_source/world-states-provinces.svg.h', 'utf8');
let style = svg.match(/style="([^"]+)"/);
console.log(style ? style[0] : "no style attr");
let fill = svg.match(/fill="([^"]+)"/);
console.log(fill ? fill[0] : "no fill attr");
