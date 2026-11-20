const fs = require('fs');
const path = require('path');

function findBinary(dir) {
    let files = fs.readdirSync(dir);
    for (let file of files) {
        if (file === 'node_modules' || file === '.git') continue;
        let p = path.join(dir, file);
        if (fs.statSync(p).isDirectory()) {
            findBinary(p);
        } else {
            if ((file === 'QSLManager' || file === 'AdifMonitor') && fs.statSync(p).size > 100000) {
                console.log('Found binary: ' + p + ' size: ' + fs.statSync(p).size);
            }
        }
    }
}
findBinary('.');
