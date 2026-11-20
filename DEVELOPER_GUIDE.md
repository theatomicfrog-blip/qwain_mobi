# Qwain & TxTweaker — Developer Architecture & Code Flow Guide

## Executive Overview
**Qwain** is an open-source, full-featured Amateur Radio workspace application built with C++17 and Qt (compatible with Qt 5.15+ and Qt 6.x). It provides real-time QSO logging, live network ingestion (WSJT-X / N1MM UDP/TCP streaming), online callsign/QSL lookups (QRZ.com XML API), worked-entity statistics (DXCC, WAS, VUCC, POTA, Grids), a custom vector QSL card and envelope designer, batch QSL printing engines, and automated configuration backup rolling archives.

The repository also includes **`tx_tweaker`**, a standalone utility binary designed to batch-inject `<TX_PWR>` transmit power tags into digital mode ADIF logs (FT-2, FT-4, FT-8).

---

## Architecture Diagram & Data Flow

```
                     +----------------------------------+
                     |           main.cpp               |
                     +----------------------------------+
                                      |
                                      v
                     +----------------------------------+
                     |          MainWindow              |
                     +----------------------------------+
                        /     |              |     \
                       /      |              |      \
                      v       v              v       v
             +------------+ +------------+ +-----------+ +---------------+
             | SQLite DB  | | QSettings  | | QRZ Cache | | Network Server|
             |qwain.sqlite| | Qwain.conf | |qrz_cache.db| |  TCP/UDP Recv |
             +------------+ +------------+ +-----------+ +---------------+
                   |              |              |               |
                   +--------------+--------------+---------------+
                                  |
            +---------------------+---------------------+
            |                                           |
            v                                           v
  +--------------------+                     +--------------------+
  |    QslDesigner     |                     |  QslManagerDialog  |
  | Vector Canvas Tool |                     | Batch Print Engine |
  +--------------------+                     +--------------------+
```

---

## 1. Application Lifecycle & Bootstrapping

1. **Entry Point (`cpp_source/main.cpp`)**:
   - Sets application metadata (`OrganizationName: Qwain`, `ApplicationName: Qwain`).
   - Initializes `QApplication` and command-line parser.
   - Instantiates `LoadingSplash` splash screen.
   - Instantiates `MainWindow w`.
   - Calls `w.show()`, closes splash screen, and enters `app.exec()` event loop.

2. **MainWindow Initialization (`cpp_source/mainwindow.cpp`)**:
   - `initDatabase()`: Connects to `qwain.sqlite` (or creates the database if missing) and executes DDL to ensure the `qsos` table schema exists.
   - `readSettings()`: Loads UI preferences, fonts, window geometries, and QRZ credentials from `Qwain.conf`.
   - **Network Listeners**: Binds UDP/TCP sockets for real-time logging streams (WSJT-X / N1MM).
   - **Timers**:
     - `clockTimer` (1s): Triggers `updateClock()` to update the status bar UTC time.
     - `qrzProbeTimer` (42 mins): Probes QRZ.com API credentials silently to keep session key seed active.
     - `m_qrzQueueTimer`: Polite, rate-limited background processing queue for QRZ callsign lookups.

3. **Shutdown Teardown (`MainWindow::closeEvent`)**:
   - Invokes `performRollingConfigBackup()` to rotate `.1.bak`..`.5.bak` and gzip archive to `./config_bak/Config.bak_YYYY-MM-DD_HH-mm-ss.gz` when 5 backups accumulate.
   - Calls `writeSettings()` to save `Uptime/TotalSeconds`, window position, and state to `Qwain.conf`.
   - Accepts exit event and invokes `QApplication::quit()`.

---

## 2. Core Data Structures & Database Persistence

### In-Memory QSO Structure (`cpp_source/qsodata.h`)
```cpp
struct QsoDetail {
    QString call;       // Callsign (e.g. W4DJF)
    QString date;       // QSO Date (YYYYMMDD)
    QString time;       // Time ON (HHMMSS)
    QString band;       // Band (e.g. 20M)
    QString mode;       // Mode (e.g. FT8, CW, SSB)
    QString freq;       // Frequency in MHz
    QString rst_sent;   // Signal Report Sent
    QString rst_rcvd;   // Signal Report Received
    QString tx_pwr;     // Transmit Power (Watts)
    QString country;    // DXCC Country Name
    QString state;      // Primary Administrative Subdivision / State
    QString dxcc;       // DXCC Entity Code
    QString pota;       // POTA Park Reference (e.g. US-1234)
    QString grid;       // Maidenhead Grid Square (4 or 6 chars)
    QString lat, lon;   // Coordinates
    QString county;     // Secondary Administrative Subdivision / County
    QString notes;      // QSO Notes / Equipment
    QString qsl_sent;   // QSL Sent Flag (Y/N/R)
    QString qslsdate;   // QSL Sent Date
};
```

### SQLite Database Schemas (`getAppDataDir()`)

1. **`qwain.sqlite`** (Primary Logbook):
   - Table: `qsos`
   - Columns: `id (PRIMARY KEY)`, `call`, `qso_date`, `time_on`, `freq`, `band`, `mode`, `country`, `state`, `dxcc`, `pota`, `grid`, `lat`, `lon`, `county`, `notes`, `qsl_sent`, `qslsdate`.

2. **`qrz_cache.db`** (Callsign Details Cache):
   - Caches parsed XML responses for callsigns to minimize QRZ.com API overhead.

3. **`qrz_graphics.db`** (Graphics & Avatar Cache):
   - Caches flag icons, QRZ profile images, and user avatars as raw byte arrays.

---

## 3. Real-Time Network & QRZ.com Integration Engine

### Network Streams
- `MainWindow` listens on configured TCP and UDP ports for ADIF-formatted datagrams emitted by digital mode loggers like WSJT-X, JTDX, and N1MM.
- On receipt, `importLogfileToDb()` parses the record, commits it to `qwain.sqlite`, updates statistics counters, and triggers real-time UI animation widgets (`m_newQsoLabel`, `m_newGridLabel`, `m_newCountyLabel`).

### QRZ.com Lookup Queue
- When a callsign lookup is requested, `doQrzLookup()` validates session key seeds.
- If unauthenticated or expired, it authenticates against `https://xmldata.qrz.com/xml/current/` to receive a `qrzSessionKey`.
- Lookups are queued in `m_qrzQueue` and processed incrementally via `m_qrzQueueTimer` with adaptive delay pacing to adhere to QRZ rate limits.

---

## 4. QSL Designer Architecture (`QslDesigner` & `PrintTableArea`)

`QslDesigner` (`cpp_source/qsldesigner.h` / `.cpp`) is a vector-based layout designer for QSL cards and mailing envelopes:

* **Canvas (`QGraphicsScene`)**:
  - Hosts card/envelope template elements (`PrintTableArea`, text boxes, images, checkboxes, circles).
  - Handles mouse dragging, resizing, grid snapping, and double-click property dialogs.
* **QSO Block / Table Item (`PrintTableArea`)**:
  - Implements dynamic QSO table grid rendering.
  - Double-clicking header cells opens the **QSO Block Header Editor**, allowing custom column headers, font family selection, font size, bold, and italic styles.
  - Double-clicking data cells opens the **Field Value Editor**, allowing alignment, font, and color styling.
* **Auto-Increment Naming Engine**:
  - Adding new elements automatically increments identifiers without prompting:
    - Text boxes: `"Text Box 1"`, `"Text Box 2"`...
    - Checkmarks: `"Check 1"`, `"Check 2"`...
    - Circles: `"Circle 1"`, `"Circle 2"`...
* **Template JSON Format**:
  - Layouts are serialized and deserialized to JSON (`*.json`), storing canvas dimensions, element positions, rotation angles, colors, fonts, and table column definitions.

---

## 5. Subsystem Modules Overview

| Class File | Role & Capabilities |
| :--- | :--- |
| **`MainWindow`** | Central workspace coordinator, network server, SQLite logger, stats aggregator, and clock manager. |
| **`QslDesigner`** | Drag-and-drop vector template layout designer for QSL cards & envelopes. |
| **`QslManagerDialog`** | Batch QSL generator: selects QSOs, binds template JSON, formats cards, and spools to PDF/printer. |
| **`ReceiveQslDialog`** | Ingests physical/electronic QSL receipts and matches them against `qwain.sqlite` records. |
| **`ConfigDialog`** | Preferences window for callsign/grid settings, themes, fonts, QRZ credentials, and direct `[Backup Conf]`. |
| **`StatsDialog`** | Aggregate statistics viewer: total QSOs, band/mode breakdowns, DXCC entity maps, and uptime tracking. |
| **`TxTweakerWidget`** | Standalone utility widget & binary target (`tx_tweaker`) for digital mode `<TX_PWR>` log batch processing. |

---

## 6. Rolling Configuration Backup Engine

Implemented in `MainWindow::performRollingConfigBackup()` (`cpp_source/mainwindow.cpp`):

```
       [ Qwain Exit ]
              |
              v
    Qwain.conf.5.bak Exists?
       /             \
    (Yes)            (No)
     /                 \
Archive .1..5.bak       Shift:
into ./config_bak/     .4.bak -> .5.bak
Config.bak_TIMESTAMP.gz .3.bak -> .4.bak
& Clear .1..5.bak       .2.bak -> .3.bak
     \                  .1.bak -> .2.bak
      \                /
       v              v
   Copy Qwain.conf -> Qwain.conf.1.bak
              |
              v
    Write New Qwain.conf
```

---

## 7. Standalone Utility: `tx_tweaker`

* **Source**: `cpp_source/tx_tweaker_main.cpp`, `cpp_source/txtweakerwidget.h`, `cpp_source/txtweakerwidget.cpp`.
* **Binary Target**: `tx_tweaker` (declared in root `CMakeLists.txt` and `cpp_source/CMakeLists.txt`).
* **License**: Released under the **Abandonware Public License**.
* **Functionality**:
  - Scans digital mode ADIF records (**FT-2**, **FT-4**, **FT-8**).
  - Detects missing, `0`, `n/a`, or `<nul>` `<TX_PWR>` tags.
  - Injects randomized power values in 5W increments (using hash seeds of callsign, date, time, band, and mode) within user-configured min/max wattage ranges.
  - Features a **Save Tweaked File As..** export dialog.

---

## 8. Build System & Compilation

### Requirements
- C++17 compatible compiler (`g++` / MinGW / MSVC)
- Qt 5.15+ or Qt 6.x (`Core`, `Widgets`, `Network`, `Sql`, `PrintSupport`, `Svg`)
- CMake 3.10+

### Linux Build
```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

### Windows Build (MinGW)
```cmd
mkdir build & cd build
cmake -G "MinGW Makefiles" ..
mingw32-make -j4
```
