const fs = require('fs');
const path = require('path');

const rootDir = __dirname;
const targetDir = path.join(rootDir, 'cpp_source');

// Ensure target dir exists
if (!fs.existsSync(targetDir)) {
    fs.mkdirSync(targetDir);
}

// File extensions to move
const extensions = ['.cpp', '.h', '.c', '.qrc', '.xsd', '.csv'];

// List of specific files to move
const specificFiles = ['CMakeLists.txt', 'CMakeLists_test.txt'];

const files = fs.readdirSync(rootDir);

for (const file of files) {
    const fullPath = path.join(rootDir, file);
    if (fs.statSync(fullPath).isDirectory()) continue;

    const ext = path.extname(file);
    if (extensions.includes(ext) || specificFiles.includes(file)) {
        const destPath = path.join(targetDir, file);
        // Copy to overwrite existing in cpp_source if it exists, then delete from root
        fs.copyFileSync(fullPath, destPath);
        fs.unlinkSync(fullPath);
        console.log(`Moved ${file} to cpp_source/`);
    }
}
