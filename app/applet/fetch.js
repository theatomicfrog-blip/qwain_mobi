const https = require('https');
const fs = require('fs');

function fetchMap(url) {
  https.get(url, (res) => {
    if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
        let newUrl = res.headers.location;
        if (!newUrl.startsWith('http')) newUrl = 'https://upload.wikimedia.org' + newUrl;
        return fetchMap(newUrl);
    }
    let data = '';
    res.on('data', d => data += d);
    res.on('end', () => {
      fs.writeFileSync('cpp_source/world-states-provinces.svg.h', '#ifndef WORLD_PROV_H\n#define WORLD_PROV_H\nconst char* world_states_provinces_svg = R"raw(' + data + ')raw";\n#endif\n');
      console.log('done length: ' + data.length);
    });
  }).on('error', (e) => {
    console.error(e);
  });
}
fetchMap('https://upload.wikimedia.org/wikipedia/commons/8/80/World_map_-_low_resolution.svg');
