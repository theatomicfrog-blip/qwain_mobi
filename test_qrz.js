const https = require('https');
https.get("https://xmldata.qrz.com/xml/current/?callsign=W1AW", (res) => {
    let data = '';
    res.on('data', chunk => data += chunk);
    res.on('end', () => console.log(data));
});
