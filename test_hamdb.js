const https = require('https');
const http = require('http');

http.get("http://api.hamdb.org/v1/W1AW/xml/testapp", (res) => {
    let data = '';
    res.on('data', chunk => data += chunk);
    res.on('end', () => console.log(data));
});
