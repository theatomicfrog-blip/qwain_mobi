# QWAIN: The Definitive User's Manual
*Compiled on: 2026-10-04 18:45:00 (PDT)*

Welcome to **Qwain** (the Amateur Radio ADIF Log Viewer, WYSIWYG QSL Designer, and Batch Printing Manager). Whether you are a beginner looking to design your very first QSL card or a seasoned contesting pro managing multi-thousand contact batch runs with custom power tweaks and standard ITU emissions, this manual contains everything you need to know.

---

## SECTION 1: Welcome & Overview

Qwain is a highly specialized, desktop-focused tool built using Qt and C++ to provide:
1. **Interactive ADIF Log Parsing**: Load and view local `.adi` or `.adif` radio logs.
2. **QSL & Envelope Designer (WYSIWYG)**: A pixel-perfect designer with support for custom layouts, text fields, checkmarks, ellipses (circles), and background images.
3. **QSL Manager**: Manage queues, check online and local catch-ups, and batch-add unprinted cards.
4. **QSL Previewer**: Preview templates dynamically populated with contact log details, apply mechanical printing offsets, and batch dispatch print jobs safely to CUPS or PDF.

---

## SECTION 2: Minimalist Getting Started Guide (Novice Track)

If you have just launched Qwain for the first time, follow these **four required steps** to get up and running:

### Step 1: Set Up Your Station Credentials
1. Click **Configuration** -> **Settings** on the main menu bar.
2. On the **Station Details** page, enter:
   - **Your Callsign**: The legal station callsign (e.g., `W4DJF`).
   - **Your Name**: Your name as you'd like it rendered.
   - **Mailing Address**: Street address, City, State, Zip Code, and Country.
3. Click **Apply** or **OK**. These values are stored persistently and are used as fallbacks if your ADIF log does not contain operator address tags.

### Step 2: Set Up Your Print Output (CUPS / PDF)
Qwain integrates directly with your system's print daemon (such as CUPS on Linux).
1. Navigate to **Settings** -> **Direct Printers**.
2. Select your default label or envelope printer from the **Printers List** or choose **Print to PDF** for file-only testing.
3. If using direct printing, you can perform a **Print Test Page** to confirm communication between Qwain and your system's print queue.

### Step 3: Load Your ADIF Log File
1. Click **File** -> **Open Logfile** (or double-click the main central log table).
2. Select your log file (usually ends in `.adi` or `.adif`).
3. Qwain will immediately parse the log, populate the main contacts grid, and compute unprinted statistics.

### Step 4: Print Your First Card
1. Click the **QSL Manager** button in the main window.
2. Double-click any contact from the search list to add it to the **Print Queue** on the right sidebar.
3. Click **Print Selected QSLs** at the bottom.
4. A preview window will appear. Choose a template on the left, check your offsets, and click **Send to Printer**!

---

## SECTION 3: The MainWindow Dashboard (`MainWindow`)

The **MainWindow** is the main control console you see upon opening Qwain. It organizes log exploration, online research, real-time widgets, and global settings.

### 1. Menus & Navigation Actions
- **File Menu**:
  - **Open Logfile**: Loads a standard `.adi` or `.adif` radio log file.
  - **Prepare New ADI**: Empties the current workspace to start a new logging session.
  - **Backup Data / Restore Data**: Backs up or restores your local Qwain settings, logs, and cache profiles to an external directory.
- **Tools Menu**:
  - **QSL Manager**: Opens the batch print queue and QSL management window.
  - **QSL Template Designer**: Launches the pixel-per-pixel WYSIWYG card design workshop.
  - **Cache Viewer**: Explores and manually edits locally cached QRZ lookup and flag image files.
  - **Icon Tools**: Rebuilds or trims downloaded country flag icon libraries.
- **Configuration Menu**:
  - **Settings**: Launches the central preferences stack (Station details, direct printers, QSL options, etc.).
- **Help Menu**:
  - **About Qwain**: Shows system versioning, licensing details, and contributor credits.

### 2. Central Table Viewer
The centerpiece of the main dashboard is the **Interactive Log Grid**:
- Lists all loaded QSO contacts with columns for Callsign, UTC Date, Time, Band, Frequency, Mode, RST Sent, RST Received, QSL Sent, and QSL Received.
- Double-clicking any row loads that contact's profile immediately into the right-hand **Info Browser Viewport**.

### 3. Online Lookup & Research Console (`m_lookupEdit`)
- At the top right, enter any callsign into the **Callsign Lookup Box** and press Enter.
- Qwain will connect online (via **QRZ API** or local caches) to perform a deep query, displaying the station's:
  - Full Name, QTH (Location), State, Zip, and Grid locator.
  - Profile image or logo.
  - Operating badges like **LoTW User** or **eQSL Participant**.

### 4. Interactive Real-Time Widgets
- **3D Interactive Globe (`GlobeWidget`)**:
  - A real-time, rotating 3D vector model of the Earth plotted on your sidebar.
  - Every contact from your loaded log file is plotted on the globe at its exact GPS coordinates.
  - Click and drag the globe to rotate it; use the mouse wheel to zoom in on specific prefixes.
- **Grayline Map Tracker (`GraylineWidget`)**:
  - Displays a projection of the world map with a dynamic, shaded day/night grayline boundary.
  - Allows DXers to predict propagation enhancements along the terminator path in real-time.
- **DX Cluster Sidebar (`DxClusterWidget`)**:
  - Connects to active DX Cluster nodes to display live spotted frequencies and spotted stations.
  - Double-clicking any spot immediately fills your lookup console.

### 5. Statistics Bar & Live Award Indicators
- **QSO Count & Total Logs**: Renders at the bottom status bar, detailing total loaded, unprinted, and confirmed contacts.
- **Strict UTC Clock (`m_timeLabel`)**: Displays a high-accuracy, live-updating clock synced in strict ham radio UTC time.
- **Animated Award Alerts**:
  - When importing a log, if Qwain detects a QSO in a US County or Grid square you have never worked before, it fires an **animated slide-in notification** (`New County Worked!`, `New Grid Sector!`) with custom slide animations to celebrate your award progress!

---

## SECTION 4: The QSL Designer (`QslDesigner`)

The **QSL Designer** is a pixel-perfect, WYSIWYG editor. Access it by clicking **Tools** -> **QSL Template Designer**.

### Canvas Dimensions & Margins
- Default paper sizes are defined for **Standard QSL Cards** (e.g., 6.5 in x 3.625 in or No. 6 3/4 envelopes) at a standard resolution of 96 DPI.
- You can add high-quality background images (PNG/JPG) using the **Set Background Image** button.

### Creating and Editing Template Elements
Right-click anywhere on the designer canvas to add a new block, or double-click an existing block to edit its properties.

#### Available Defined QSLDesigner Tags
When adding a text or cell block, you can enter plain text or use the following dynamic tags. During preview or printing, Qwain will automatically substitute these tags with matching contact data from your log:

| Designer Tag | Extracted Log Field / Behavior |
| :--- | :--- |
| `<call>` | The callsign of the contacted station (e.g., `G4XYZ`). |
| `<day>` | The 2-digit day of the QSO (`01`-`31`). |
| `<month>` | The 2-digit month of the QSO (`01`-`12`). |
| `<month_abbr>` | The 3-letter abbreviated month name (e.g., `Jan`, `Feb`). |
| `<month_spelled>` | The full English name of the month (e.g., `January`). |
| `<year>` | The 4-digit year of the QSO (e.g., `2026`). |
| `<time_on>` / `<time_utc>` | The 4-digit time in UTC (e.g., `1430`). |
| `<band>` | The radio band (e.g., `20M`, `40M`). |
| `<freq>` | The exact frequency in MHz (e.g., `14.0740` - if logged, else falls back to `<band>`). |
| `<mode>` | The emission mode (custom mapping applies if "Use Standard Emission Modes" is checked). |
| `<rst_sent>` | The signal report sent to the contact (e.g., `599` or `-08`). |
| `<rst_rcvd>` | The signal report received from the contact (e.g., `579` or `-11`). |
| `<tone>` / `<tone_sent>` / `<tone_rcvd>` | The 3rd digit (Tone) extracted from a CW RST report (e.g., `9` in `599`). Digital dB logs or phone reports render as blank. |
| `<tx_pwr>` | The transmit power in Watts. If empty, can generate values dynamically if power tweaks are enabled. |
| `<qrp>` | Renders **"QRP"** (or your Custom Brag text) if the QSO power is $\le$ 6 Watts. |
| `<gridsquare>` | The grid square locator of the contacted station (e.g., `FN20`). |
| `<pota_ref>` | The Parks On The Air reference number (e.g., `K-1234`). |
| `<comment>` / `<notes>` / `<message>` | Your persistent manual comment or custom message entered in the preview pane. |

### Designing a Table Block
Instead of placing individual elements for each parameter, you can design a tabular grid:
1. Set the table to **1 row and multiple columns** (e.g., 5 columns).
2. Right-click each column block and enter a dynamic tag (e.g., Col 0: `<day>`, Col 1: `<month_abbr>`, Col 2: `<year>`, Col 3: `<mode>`, Col 4: `<rst_sent>`).
3. During printing, Qwain will align and render the log details horizontally within the grid.

### Layout Adjustment & Aesthetics
- **Position and Bounds**: Edit fields via double-clicking to set exact X, Y, Width, and Height parameters.
- **Font & Styling**: Click **Change Font/Color** in the properties dialog to modify typography, sizes, weights, and text colors.
- **Rotations**: Specify exact rotation angles (e.g., `90` or `180` degrees) for vertical spine labels or reverse printing.
- **Saving Templates**: Click **File** -> **Save Template As...** and enter a custom filename. All template layouts are saved as persistent configurations in Qwain's local settings database.

---

## SECTION 4: The QSL Manager Dialog (`QslManagerDialog`)

Open the manager by clicking **QSL Manager** from the main view toolbar. This is your command center for sorting and queue management.

### Left Panel: Search & Query Filters
- **Search Box**: Type any callsign, country, or county to filter instantly.
- **All Contacts**: Shows all QSOs loaded in the current logfile.
- **Only Unprinted**: Displays only contacts that do not have `QSL_SENT` marked as `Y` in the ADIF log.
- **Result Grid Columns**:
  - `Callsign`: The target station.
  - `Date / Time`: The exact timestamp.
  - `Band / Mode`: Core contact parameters.
  - `QSL Status`: Current unprinted or printed representation.

### Right Panel: Print Queue (`m_printQueueList`)
- **Adding to Queue**: 
  - Double-click any row in the left-hand search table to add that contact immediately to the print queue.
  - Click **Add All Unprinted to Queue** to load all unprinted cards instantly.
- **Removing from Queue**:
  - **Double-click any item** directly inside the Print Queue list. It will instantly delete and remove that callsign from your queue, updating the print button enablement in real time.
- **Clear Queue**: Click the small **"Clear Queue"** link at the bottom of the sidebar to empty the print queue completely.

---

## SECTION 5: The QSL Preview & Printing Dialog (`QslPreviewDialog`)

When you click **Print Selected QSLs** in the QSL Manager, the QSL Preview dialog opens. This dialog acts as a buffer between your layout templates and the physical printer.

### Panel Workflows & Mechanical Alignment

#### 1. Left Sidebar (Template Choices & Layout Selectors)
- **Card Templates / Envelope Templates**: Choose from your saved templates. Selecting any item instantly redraws the card inside the central vector viewport.
- **Print Adjustments (Margins / Margins shift)**: 
  - Printers have slight mechanical tolerances. Qwain provides pixel-accurate adjustments for **Top Margin** and **Left Margin** offsets.
  - Enter numeric shifts (e.g., `+0.12` or `-0.05` inches) to shift the entire printed image in-place, aligning the text blocks perfectly onto pre-printed QSL card blanks.

#### 2. Central Viewport (Dynamic Live Preview)
- Displays the currently selected card or envelope layout.
- The vector rendering engine parses the log parameters of the first contact in your queue, applying any active Standard Emission conversions or QRP substitution values, and paints the final card surface in real-time.

#### 3. Right Sidebar (Status Updates & Mechanical Checkboxes)
- **Mark QRZ Sent** / **Mark eQSL Sent**: If checked, Qwain will connect online during batch printing and mark the QSOs as printed on your QRZ/eQSL logbooks.
- **Print Card** / **Print Envelope**: Choose whether to print only the QSL Card, only the Envelope, or both sequentially.
- **Reverse Printing**: Useful for duplex card feeds.
- **Manual Comment / Notes Box**: Enter persistent message strings that substitute directly into any `<comment>` tag.

#### 4. The Unified Batch Printing Workflow
- Click **Send to Printer** to start batch printing.
- **Multi-QSO Template Transition**: 
  - If you are batch printing multiple QSOs for the same callsign and a custom Multi-QSO template exists, Qwain will complete the standard run, detect any overflow records, and present a custom transition popup.
  - The dialog shows a 60% sized vector image of the overflow template card. Load your multi-card stock, and click **OK** to process overflow printing.
- **Emergency Stop (Panic Button)**:
  - If the printer jams, feeds incorrectly, or starts printing garbage, click the red **PANIC / EMERGENCY STOP** button.
  - Qwain will instantly freeze the print queue and dispatch process commands to **purge and cancel all CUPS queues** on your local machine.

---

## SECTION 6: Advanced Configuration (Advanced Track)

If you are a power user looking to optimize your print runs, navigate to **Configuration** -> **Settings** -> **QSL Card Options** to access these custom setups.

### 1. Power Tweaks Group
To make printed cards look more organic or to align with your operating patterns without modifying your legal log files, you can use randomized power tweaks:
- Check **Use TX power tweaks**.
- Enable/disable tweaks for individual modes (**FT2**, **FT4**, **FT8**, and **SSB**).
- Define precise range spinners (Min/Max Watts, e.g., `10W` to `100W`).
- During preview rendering, Qwain seeds a random number generator using the QSO's timestamp and mode to generate a random power value within your defined steps (increments of 5 Watts) to substitute into the `<tx_pwr>` field.
- **IMPORTANT**: Power tweaks are strictly visual/print-time. Your local `.adi` file and online logs are preserved with your actual logged power.

### 2. Standard Emission Modes Group
Amateur radio logs record specific operating modes, but QSL card conventions often require ITU emission designators:
- Check **Use Standard Emission Modes** to dynamically map logged modes during card printing:
  - `SSB` -> `J3E`
  - `CW` -> `A1A`
  - `AM` -> `A3E`
  - `FM` -> `F3E`
  - `RTTY` -> `F1B`
  - `PSK31` -> `J2B`
  - `DSTAR` -> `F7W`
- Uncheck standard designators and check **Use Generic Emission Modes** to print exact logged strings (e.g., `"FT8"`, `"RTTY"`).

### 3. QRP Bragging Settings (Dynamic Auto-Scaling)
- Check **Use QRP Bragging**.
- **QRP Trigger Threshold**: If the transmit power is **$\le$ 6 Watts**, Qwain triggers QRP bragging.
- **Substituted Value**:
  - By default, Qwain substitutes the string `"QRP"`.
  - Check **Custom QRP brag** and enter a custom 4-character string (e.g., `"5W"`, `"QRPp"`, `"MINI"`).
  - The sub-menu dynamically updates the status label (`Currently Substitutes : [Value]`) in real-time. Unchecking the box falls back to `"QRP"`. This custom text is stored persistently.
- **Dynamic Two-Way Auto-Scaling (Grow & Shrink)**:
  - When rendering substituted `<qrp>` or fallback `<tone>` fields, the text-on-path layout engine automatically scans font sizes from **`3.0pt`** up to **`48.0pt`** (or 2.5x the base font size).
  - It dynamically selects the absolute largest possible font size that fits inside both the width and height of the cell.
  - If the cell is too small, it shrinks the font to fit; if the cell is large, it automatically grows the font to fill the space cleanly.

### 4. Qwain Logfile Synchronizations
When a card is printed, Qwain automatically logs the event to help you maintain strict compliance and auditing:
- **System Logs**: Emits system log entries showing `[Timestamp] [Callsign] Card Printed` accompanied by Mode, RST Sent, RST Received, and the exact transmit power printed.
- **Local ADIF Updating**: When marking QSOs as sent, Qwain automatically updates or inserts the `<MODE>`, `<RST_SENT>`, `<RST_RCVD>`, and `<TX_PWR>` fields directly into the local ADIF logfile (`.adi`) along with `<QSL_SENT:1>Y` and `<QSLSDATE>`, matching the printed card parameters exactly.

---

## SECTION 7: Quick Troubleshooting FAQ

#### Q1: My text is clipped or rendering outside the cell margins on the card.
**A**: Go to **Configuration** -> **Settings** -> **QSL Card Options**. Ensure **Use QRP Bragging** is enabled for any QRP fields so the two-way auto-scaling engine can resize the text dynamically to fit the cell bounds. For other fields, open the **QSL Template Designer** and double-click the element to adjust its font size or bounding box size.

#### Q2: I clicked "Send to Printer" but the printer is spitting out incorrect card counts.
**A**: Immediately click the red **PANIC / EMERGENCY STOP** button in the Preview or Batch dialog. This instantly signals the operating system to clear and purge all active CUPS queues. If the printer continues feeding, turn off its power switch to clear its internal memory buffer.

#### Q3: How do I remove a single callsign from my active print queue?
**A**: Simply double-click the callsign entry inside the right-hand **Print Queue** panel in the QSL Manager. It will immediately delete and remove that item from the print run.

---
*Happy operating and 73 from the Qwain Development Team!*
