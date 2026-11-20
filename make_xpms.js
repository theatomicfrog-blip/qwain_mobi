const fs = require('fs');
const w = 154;
const h = 117;
function make(name) {
    let out = `const char *${name}_xpm[] = {\n"${w} ${h} 2 1",\n"  c None",\n". c #000000",\n`;
    for(let y=0; y<h; y++) {
        let line = "";
        for(let x=0; x<w; x++) {
            if (x < 10 && y < 10) line += ".";
            else if (x > w-10 && y > h-10) line += ".";
            else line += " ";
        }
        out += `"${line}"${y===h-1 ? "" : ","}\n`;
    }
    out += "};\n";
    return out;
}
fs.writeFileSync('cpp_source/print_qsl.c', make('print_qsl'));
fs.writeFileSync('cpp_source/design_qsl.c', make('design_qsl'));
fs.writeFileSync('cpp_source/image_tracker.c', make('image_tracker'));
