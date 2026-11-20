const fs = require('fs');
let svg = fs.readFileSync('world.svg', 'utf8');
fs.writeFileSync('cpp_source/world-states-provinces.svg.h', '#ifndef WOL_H\n#define WOL_H\nconst char* world_states_provinces_svg = R"raw(' + svg + ')raw";\n#endif\n');
