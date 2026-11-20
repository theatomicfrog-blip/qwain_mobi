const https = require('https');

https.get("https://callook.info/W1AW/json", (res) => {
    let data = '';
    res.on('data', chunk => data += chunk);
    res.on('end', () => console.log(data));
});
