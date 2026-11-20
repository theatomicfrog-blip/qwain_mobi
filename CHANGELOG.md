# ADIF Monitor - Version History

**Version 1.01 rc1 Beta**
- Initial build requested: Base window and core UI framework.
- ADIF log parsing and real-time network monitoring over TCP implemented.
- Added UI tracking features for DXCC and generic grids.
- Added interactive graphical grid map layout (`map.svg`).
- Integrated QRZ credentials to provide online queries for Callsign meta-data.
- Implemented secure API token session caching mechanism.
- Extended robustness of ADIF ingestion for arbitrary line lengths and missing standard fields.
- Hardened HTTP engine by transitioning from scripts to QNetworkAccessManager for in-app downloads.
- Re-styled Leaderboard as a structured QTableWidget.
- Added "SPISPOPD" (Doom) embedded payload in the Misc menu.
- Implemented exhaustive graphical map layers tracking Worked All Japan (WAJA), POTA sites, and US Counties.
- Re-configured SVG mapping engine to support multi-class highlighting, tooltip hovering, and interactive mouse-wheel zooming.
- Expanded heuristics algorithm for regex extracting references for POTA identifiers.
- Polished US county logic resolving names with missing identifier tails (Parish, Borough, etc).
- Restructured `MenuBar` into classic interface layout (`Configuration`, `Tracking`, `Misc`, `About`).
- Config options (IP Address, Ports, Usernames, etc.) are now seamlessly modified inline within the menu tree avoiding tertiary pop-ups.
- Expanded interaction on Leaderboard Dashboard allowing users to click tracking totals to immediately instantiate respective maps.
- Architected unified, list-driven Preferences page incorporating all configurations seamlessly into a master control panel (KDevelop style).
- Engineered exact hit-testing for SVG interactive map tools substituting coarse bounding-box lookups with live pixel rendering masks for pinpoint accuracy on jagged US county boundaries.
