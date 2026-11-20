const fs = require('fs');

let cppFiles = fs.readdirSync('cpp_source').filter(f => f.endsWith('.cpp'));
let totalBytes = 0;
let totalLines = 0;

cppFiles.forEach(file => {
    let content = fs.readFileSync('cpp_source/' + file, 'utf8');
    totalBytes += Buffer.byteLength(content, 'utf8');
    totalLines += content.split('\n').length;
});

let binarySize = 0;
if (fs.existsSync('build/QSLManager')) {
    binarySize = fs.statSync('build/QSLManager').size;
} else if (fs.existsSync('QSLManager')) {
    binarySize = fs.statSync('QSLManager').size;
}

console.log('Total CPP files: ' + cppFiles.length);
console.log('Total Lines: ' + totalLines);
console.log('Total Bytes: ' + totalBytes);
console.log('Binary Size: ' + binarySize);
