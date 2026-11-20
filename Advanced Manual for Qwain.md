# Advanced Manual for Qwain

**Qwain: The Ultimate QSL Card Designing, Printing, and Live QSO Companion**
*Author: Google AI Studio Build (Senior Software Engineer)*
*Date: October 5, 2026*

---

## Welcome to Qwain!

In the historic spirit of **TWAIN**—the scanner and camera hardware standard jokingly backronymed as *"Technology Without an Interesting Name"*—**Qwain** embraces a similar, humble philosophy: **A boring name with an exceptionally useful, high-fidelity function.**

Whether you are a novice ham operator setting up your very first station log, or an advanced DXer coordinating thousands of printed cards, envelopes, and multi-database updates, Qwain is engineered to sit quietly in your shack as a master companion. It bridges the once-difficult gap between logged QSO data, online lookups, real-time award maps, and physical high-quality print media.

This manual is divided into progressive sections, providing both a **Minimalist Quick Start Guide** for immediate operation and an **Exhaustive Interface Directory** explaining every clickable pixel, button, configuration parameter, sub-dialog, and backend system.

---

## Table of Contents
1. [Chapter 1: Quick-Start Guide (Novice Track)](#chapter-1-quick-start-guide-novice-track)
2. [Chapter 2: The Core Workspace (MainWindow Tour)](#chapter-2-the-core-workspace-mainwindow-tour)
3. [Chapter 3: QSL Template Designer (QslDesigner)](#chapter-3-qsl-template-designer-qsldesigner)
4. [Chapter 4: QSL Card Manager (QslManagerDialog)](#chapter-4-qsl-card-manager-qslmanagerdialog)
5. [Chapter 5: Pre-Print Layout and Batching (QslPreviewDialog)](#chapter-5-pre-print-layout-and-batching-qslpreviewdialog)
6. [Chapter 6: Recieving Inbound Cards (ReceiveQslsDialog)](#chapter-6-recieving-inbound-cards-receiveqsldialog)
7. [Chapter 7: Backdrop Assets and Solid Color Gallery (ImageViewer)](#chapter-7-backdrop-assets-and-solid-color-gallery-imageviewer)
8. [Chapter 8: Off-line QRZ Cache Tools (CacheViewerDialog)](#chapter-8-off-line-qrz-cache-tools-cacheviewerdialog)
9. [Chapter 9: Advanced Configuration Reference Guide](#chapter-9-advanced-configuration-reference-guide)

---

## Chapter 1: Quick-Start Guide (Novice Track)

If you have just launched the Qwain binary for the very first time, this chapter gets your station configured, your logfile linked, and your first QSL card printed in **under 5 minutes**.

### Step 1: Set Up Station Info and Logfile Link
1. Click **Configuration** (top toolbar menu) -> Select **Settings...** (or press the corresponding hotkey).
2. On the left list, select **Station Info**.
   * Fill in your **My Call** (e.g., `W4DJF`), your **Operator**, and your **Grid Square** (e.g., `EM73`). This ensures Qwain knows how to calculate DX distances, bearings, and award criteria.
3. Select **Logging / ADIF** from the left list.
   * Click **Choose Logfile** and select your active amateur radio logfile (in standard `.adi` or `.adif` format). 
   * *If you don't have one, create an empty text file named `my_station.adif` on your drive, and select it.* Qwain will automatically monitor this file in real time and load any new contacts you add via other logging programs (like WSJT-X, N1MM, or DXKeeper).

### Step 2: Set Up Lookups and Online Credentials
To automatically populate station names, postal addresses, and card details, hook Qwain up to QRZ:
1. Click **Credentials / API Keys** in the Settings window.
2. Enter your **QRZ Username** and **Password**.
3. *Optional Checkbox:* If you have an XML Subscription, check **QRZ XML or Higher Subscriber?** to enable premium high-speed API lookups.
4. Click **Network & Integration** to verify the TCP/UDP server status. By default, Qwain binds to Port **2237** (the standard UDP port for WSJT-X / JTDX logging notifications). Any live contact you save in WSJT-X will immediately pop up inside Qwain with full biography, map tracking, and photos!

### Step 3: Create a Minimalist QSL Card Template
Before you can print, you must design a card template:
1. Click **Design QSLs** on the main window toolbar.
2. Click **New Template** (top toolbar in the Designer).
3. Type a template name (e.g., `Standard Card`) and check **QSL Card** (instead of Envelope).
4. Click **Set Background Image**. You can choose one of the pre-loaded SVG/PNG backgrounds or upload your own shack photo.
5. Click **Add Field** -> Click anywhere on the card canvas. A list of tags will appear. Select **`<call>`** (the callsign of the recipient).
6. Drag the `<call>` element to the top center. In the **Properties Panel** (right side), set its font size to `24`, make it **Bold**, and choose a vibrant color.
7. Click **Define Blocks** (or "Define Fields") -> Create a QSO Table block. Drag this grid box to the bottom half of the card. This grid is where your contacts' Band, Mode, Date, Time, and Signal Reports (RST) will automatically populate!
8. Click **Save Template**.

### Step 4: Add to Print Queue and Send to Printer
1. Click **Print QSLs** (main window toolbar) to open the **QSL Manager**.
2. Double-click any QSO in the results grid to add it directly to the print queue on the right.
3. Click the green **Print QSL** button below the queue.
4. The **QSL Preview Dialog** will open. Select your freshly created `Standard Card` template from the left list. Your QSO details (Callsign, Band, Mode, Date) will be rendered on the card in high-fidelity!
5. Click **Send to Printer** at the bottom-right. Your system print dialog opens—confirm, and your card begins printing!

---

## Chapter 2: The Core Workspace (MainWindow Tour)

The Qwain main window acts as your control center. It handles real-time logging, inbound XML/UDP data streams, statistics, and viewport management.

```
+-----------------------------------------------------------------------------+
| QWAIN - Log: [W4DJF.adif]  UDP Active: [Port 2237]  Local UTC Time: 08:54:12|
+-----------------------------------------------------------------------------+
| [Configuration v]  [Print QSLs]  [Receive QSLs]  [Design QSLs]  [Viewports v]|
+-----------------------------------------------------------------------------+
| LOOKUP: [ K1JT           ]  [ Search Match List ]  +----------------------+ |
| Name: Joseph H. Taylor      [W1AW               ]  |                      | |
| Grid: FN20qi                [K1JT               ]  |   Interactive        | |
| Country: United States      [K1N                ]  |   Rotating Globe /   | |
| Address: Princeton Univ.                           |   Grayline Map       | |
|                                                    |                      | |
| QSO Count: 14,832           Leaderboard Rank: #12  +----------------------+ |
+-----------------------------------------------------------------------------+
| Logs/System Terminal:                                                       | |
| [08:54:10] Loaded 14,832 QSOs from ADIF.                                    | |
| [08:54:12] UDP listener started successfully on port 2237.                  | |
+-----------------------------------------------------------------------------+
```

### 1. Interactive Inputs & Search Panel
* **Callsign Search Input (`m_lookupEdit`):** Type any callsign here to instantly search your local cache database. As you type, the **Search Match List (`m_searchMatchesList`)** dynamically filters similar entries below.
* **Search Match List:** Single-click any callsign in this list to populate its biography and card statistics. Double-clicking immediately adds the station to your QSL print queue!

### 2. Live Info Browser & Photo Display (`m_infoBrowser`)
This rich-HTML browser rendering displays:
* The operator's full name, address, US county, CQ/ITU zones, grid square, and DX country.
* Checkmarks indicating their membership in virtual databases: **LoTW (Logbook of The World)**, **eQSL**, and **Mail QSL**.
* A large, high-fidelity photo or QRZ avatar of the operator (if available).
* Live distance and bearing calculations from your coordinates to theirs.

### 3. Integrated Dynamic Viewports
Access these highly visual auxiliary tracking tools via the **Viewports** menu:
* **The Interactive Globe (`m_globeWidget`):** A real-time, interactive 3D globe showing Great Circle path lines, worked entities, and station locations.
* **Grayline Overlay Map (`GraylineWidget`):** Shows greyline day/night boundary curves globally, letting you easily track solar-enhanced propagation conditions.
* **DX Cluster Spots Grid (`dxClusterWindow`):** Connects to a standard telnet DX cluster and parses spot logs in real time. Single-clicking a cluster spot automatically fills the lookup box with that station's profile!
* **Awards Widget (`awardswidget`):** Visualizes your status toward DXCC, WAS (Worked All States), POTA (Parks On the Air), WAJA (Worked All Japan Prefectures), and Grids. It highlights entities Worked (Green), Confirmed (Blue), and Needed (Red).

### 4. Menu & Dropdown Command Bar

#### Configuration Menu
* **Username/IP/Port Labels:** Displays real-time status of your active QRZ session, local IP address, active binding port, and current ADIF logfile.
* **Settings...:** Opens the exhaustive master configuration options window.
* **Misc Submenu:**
  * **Cache Tools:** Opens the offline QRZ database management dashboard.
  * **Icon Tools:** Provides advanced tools to clear, resize, and re-cache QRZ avatars and station logos.
  * **QSL Template ImageViewer:** Browse, upload, or prune card background images.
  * **QRZ Instant QSO Uploader:** Instantly uploads individual log records to your online QRZ cloud logbook.
  * **Suite Options:**
    * *Fix:* Checks your active ADIF logfile for logical inconsistencies (e.g., mismatched state/counties) and repairs them.
    * *Fix RSTs to QRZ:* Corrects signal reports (RST) for digital modes on your QRZ profile.
    * *County Prep:* Cross-references all US callsigns with FIPS county data and appends missing county information.
  * **I'm Bored -> Qwain Invaders:** A fully featured retro retro arcade-style Space Invaders game built into the application to keep you entertained during slow band conditions!

---

## Chapter 3: QSL Template Designer (QslDesigner)

The **QslDesigner** is a state-of-the-art WYSIWYG (What You See Is What You Get) desktop publishing canvas. It lets you create pixel-perfect templates for standard QSL cards (typically 3.5" x 5.5" / 140mm x 90mm) or paper envelopes.

```
+-----------------------------------------------------------------------------+
| QSL Designer - Editing: [Standard Card]                                     |
+-----------------------------------------------------------------------------+
| [New Template] [Save] [Duplicate] [Rename] [Delete]  Units: [ Millimeters v]|
+-----------------------------------------------------------------------------+
| LEFT PANEL (Templates) | CENTER CANVAS (WYSIWYG Visual Designer)             |
|                        | +------------------------------------------------+ |
| Cards:                 | | [W4DJF] (Static Title Text)                    | |
| - Standard Card        | |                                                | |
| - Retro Color Card     | |  <call> (Dynamic Tag Field)                    | |
|                        | |  +------------------------------------------+  | |
| Envelopes:             | |  | QSO Record Table (Blocks Window)         |  | |
| - Letter Envelope      | |  +------------------------------------------+  | |
| - DL Envelope          | |  [ ] LoTW Checked  [x] QRZ Checked             | |
|                        | +------------------------------------------------+ |
|                        | Status: X: 110.2mm  Y: 45.1mm [Cursor: m_unit]     |
+-----------------------------------------------------------------------------+
| PROPERTIES PANEL (Selected Item):                                           |
| ID: [ <call> ]   Font: [ Arial v]  Size: [ 18 ]  Color: [ #ff0000 ]  Rot:[ 0 ]|
+-----------------------------------------------------------------------------+
```

### 1. Canvas Navigation and Panning
* **Panning (Move Canvas):** Press and hold the **Middle Mouse Button** (or right-click empty space) and drag to pan around large layouts.
* **Zooming:** Scroll the **Mouse Wheel** up to zoom in for high-precision alignment, and scroll down to zoom out.
* **Crosshairs & Telemetry (`m_unitCombo`):** A green coordinate crosshair follows your cursor. The status bar displays your cursor position in your chosen unit: **Inches (in)**, **Millimeters (mm)**, or **Pixels (px)**.

### 2. Element Operations Bar
* **Copy (`btnCopy`):** Copies selected canvas layers to the clipboard.
* **Paste (`btnPaste`):** Pastes copied layers onto the canvas.
* **Remove (`btnRm`):** Deletes the selected text, box, checkmark, or table grid element from the template.
* **Undo (`btnUndo`):** Instantly rolls back your last design change (supports unlimited state recovery).
* **Toggle Test View Mode (`btnTestView`):** Temporarily replaces dynamic tags with mock data (e.g., rendering `<call>` as `W1AW` and populating a 3-row mock table) so you can audit the final layout prior to saving.

### 3. Template Creation and File Commands
* **New Template:** Installs a blank white canvas. Prompts you to declare if this is a **QSL Card** or **Envelope**, and select the target paper dimensions.
* **Save Current Template:** Commits your layout coordinates directly to the system template configuration database.
* **Duplicate Template:** Clones the active template under a new file name.
* **Rename Template / Delete Template:** Reorganizes or purges the selected template layout.
* **Change Paper Size:** Fine-tune layout bounds. Offers presets like standard index cards (3"x5", 4"x6"), European standard QSL size, and DL/No.10 envelopes.

### 4. Interactive Properties Panel (Right Column)
Select any layer on the canvas to edit its properties instantly:
* **ID / Tag name (`m_idEdit`):** Double-click an element or edit this field to change its tag type. (e.g., renaming a text block to `<call>` hooks it to the QSO recipient).
* **Font Dropdown (`m_fontCombo`):** Choose from standard vector typefaces (Arial, Times New Roman, Courier, Georgia, Helvetica, etc.).
* **Size Spin Box (`m_sizeSpin`):** Fine-tune font scale. *Allows going to extra-large font sizes to highlight your station callsign, or ultra-small to pack data in tightly.*
* **Color Selection Button (`m_colorBtn`):** Launches a professional RGB/HSV color picker to set text color.
* **Rotation Angle Input (`m_rotSpin`):** Rotates any element by an arbitrary angle (0° to 360°), useful for vertical text along card borders.
* **Width & Height Spin Boxes (`m_widthSpin` / `m_heightSpin`):** Adjust dimensions of checkboxes, reference margins, and table block matrices.

### 5. Canvas Special Layers Creator
* **Add Custom Checkmark (`addCustomCheckmark`):** Inserts checkmark targets (e.g., empty square boxes with text labels like "LoTW" or "eQSL"). During print rendering, Qwain will **automatically draw a crisp black checkmark (`X` or `/`)** inside these boxes if the QSO is verified in those databases!
* **Add Reference Point (`addReferencePoint`):** Places calibration markers at the top-left or outer boundaries of the template. This helps you align card cardstocks in your physical printer feed tray.
* **Add Field (`addNewField`):** Places dynamic data blocks. When printed, Qwain replaces these with the corresponding log entries:
  * **`<call>`**: Callsign of recipient.
  * **`<name>`**: Recipient's full name.
  * **`<addr1>` & `<addr2>`**: Street address lines.
  * **`<city>`, `<state>`, `<zip>`, `<country>`**: Location fields.
  * **`<qrp>`**: Renders the text `QRP` (or your user-defined bragging string) if the QSO power is $\le$ 6 Watts.
  * **`<tone>`**: Renders the sent/received CW signal tone character, or acts as a fallback for the QRP bragging string if the template lacks a dedicated `<qrp>` tag.
* **Define Blocks (`defineFields`):** Configures the **QSO Table block**. This inserts a table matrix on your template where you can map columns (`DATE`, `TIME`, `BAND`, `MODE`, `RST_SENT`, `RST_RCVD`, etc.). Click **Configure Block Columns** in the property panel to add, remove, and resize columns to match your visual taste.

---

## Chapter 4: QSL Card Manager (QslManagerDialog)

The **QslManagerDialog** acts as your central print queue controller. This window is where you search your entire ADIF logfile, filter QSOs by date or location, inspect bios, and queue cards for physical printing.

```
+-----------------------------------------------------------------------------+
| QSL Manager & Search Queue                                                  |
+-----------------------------------------------------------------------------+
| CALLSIGN SEARCH: [ K1JT           ]  [x] Date Filter  [ Past Week  ] [Today] |
| Country DXCC:    [ United States  ]  [   Lotw Only ]  [ Mail Only  ] [All  ] |
+-----------------------------------------------------------------------------+
| RESULTS MATCH MATRIX:                      | OUTGOING PRINT QUEUE LIST      |
| Callsign | Band | Mode | Date     | RST    | Double-click item to REMOVE:   |
| W1AW     | 20m  | CW   | 2026-10-1| 599    | K1JT | 20m FT8 | 2026-10-04    |
| K1JT     | 20m  | FT8  | 2026-10-4| -12    | W1AW | 15m CW  | 2026-10-03    |
| K1N      | 40m  | SSB  | 2026-09-8| 59     |                                |
|          |      |      |          |        |                                |
| Select All Results (Context Menu)         | [btnPrintQsl: PRINT QUEUE (2)] |
+-----------------------------------------------------------------------------+
```

### 1. The Multi-Criteria Filter Console
* **Callsign Field (`m_callsignLookup`):** Type all or part of a callsign to instantly populate the matching QSOs in the results grid.
* **Date Range Checkbox & Date Selector Button (`m_btnDateRange`):** Enable this to restrict results to a specific calendar period. Clicking the selector launches an interactive dual-calendar dialog to choose start and end dates.
* **DXCC Country ToolButton (`m_btnCountryDxcc`):** Launches a searchable list of global countries. Selecting one filters your log to display only QSOs from that specific DX CC country.
* **Time Presets Shortcuts:**
  * **Today Button:** Restricts matching QSOs to the active UTC calendar day.
  * **Past Week Button:** Populates QSOs worked over the last 7 calendar days.
  * **Last X Days:** Prompts you to enter an arbitrary day count (e.g., 90 days) to filter recent logs.
* **Database Verification Checkboxes:**
  * **Only LoTW (`m_chkLotw`):** Filters results to show only contacts that are registered on Logbook of The World.
  * **Only Mail (`m_chkMail`):** Restricts matches to operators who specifically request physical paper mail-QSL exchange.

### 2. Search Results Grid & Context Commands
The **Results Table (`m_resultsTable`)** organizes matching QSOs in tabular rows. 
* **Row Selection:** Single-click any row to instantly fetch their QRZ biography and address into the information panel.
* **Double-Clicking Rows:** Instantly queues the selected QSO to the outbound printing list.

#### Context Menu (Right-Click Results Grid)
* **Add to Print Queue:** Queues the highlighted QSOs.
* **View Station QRZ Profile:** Opens their online biography.
* **Select All:** Selects every QSO in the current search table.
* **Clear Selection:** Deselects all highlighted rows.
* **Delete Log Entry:** Safely removes a misplaced QSO record directly from your local logfile and database.

### 3. Outbound Print Queue Control
* **The Outgoing Queue List (`m_printQueueList`):** Shows your active batch queue. 
  * *Important Queue Removal Action:* If you accidentally queue a station, simply **double-click the item** in this print queue list. Qwain will **instantly delete the item** from the queue list, recalculate queue counts, and save the updated queue state without popping up annoying, intrusive prompt screens.
* **Queue Persistence:** Your print queue is fully persistent. If you close Qwain or experience a power failure, your queue is automatically saved and reloaded upon next launch.
* **Print QSL Button (`m_btnPrintQsl`):** Opens the **QSL Preview Dialog** to configure print options and initiate the print job.

---

## Chapter 5: Pre-Print Layout and Batching (QslPreviewDialog)

The **QslPreviewDialog** is the final gateway before ink meets paper. It renders a real-time, high-precision layout preview of what your QSL cards and envelopes will look like. It also manages advanced batch configurations, print pacing, printer routing, and overflow cards.

```
+-----------------------------------------------------------------------------+
| QSL Preview & Printing Console - QSO: [K1JT]                                |
+-----------------------------------------------------------------------------+
| CARD TEMPLATES:        | REAL-TIME high-fidelity PREVIEW RENDERER           |
| - Standard Card [Active] | +----------------------------------------------+ |
| - DX Photo Card        | | W4DJF                                          | |
|                        | |                      To: K1JT                  | |
| ENVELOPE TEMPLATES:    | |  QSO Table:                                    | |
| - Letter Envelope      | |  2026-10-04 | 08:54 | 20m | FT8 | QRP          | |
| - DL Envelope          | |                                                | |
|                        | |  [ ] LoTW Checked  [x] QRZ Checked             | |
|                        | +----------------------------------------------+ |
+-----------------------------------------------------------------------------+
| OPTIONS PANEL:                                                              |
| [x] Print Card  [ ] Print Envelope   [x] Print Reverse (180deg Rotate)      |
| Checkmark Scale: [---o---]  Top Margin Correction: [ 0.0mm ] Left: [ 0.0mm ]|
|                                                                             |
| [QRP OPTIONS (Use QRP Bragging: ENABLED)]:                                  |
| Currently Substitutes : QRP       [x] Custom QRP Brag: [ QRP  ]             |
|                                                                             |
| BATCH PREFERENCES:                                                          |
| Sequence: (x) Card-Envelope  ( ) All Cards then Envs   Pacing: (x) Continuous|
| Pacing: ( ) One-at-a-time (Prompt User)    Pacing Inter-Card Delay: [ 3s ]  |
|                                                                             |
| [Undo]  [Reset]                                [btnPrint: SEND TO PRINTER ] |
+-----------------------------------------------------------------------------+
```

### 1. Interactive Control Elements Directory

#### Template Chooser Lists (`m_templateList` / `m_envTemplateList`)
* Dynamic selector columns on the left let you flip between card layouts and envelope templates instantly. Changing templates updates the center preview render immediately.

#### Dynamic Font Choice Buttons
* **Address Font (`m_btnAddressFont`):** Customize the font family, style (e.g. bold/italic), and size for the recipient's mailing address layer.
* **Comment Font (`m_btnCommentFont`):** Customize the font style for your hand-written card comments.

#### Command Buttons
* **Comment Editor (`m_btnCommentEditor`):** Launches a rich-text entry window. This lets you type a personalized, handwritten-style comment (e.g., *"Thanks for the lovely 20m FT8 QSO, Joe! 73"*). This message is instantly placed into your card layout's `<comment>` field!
* **Undo (`m_btnUndo`):** Instantly rolls back your last design change (supports unlimited state recovery).
* **Reset (`m_btnReset`):** Reverts all layout modifications, card scaling, and margin corrections back to the default template settings.
* **Freq/Band Switch (`m_btnFreqBandSwitch`):** Flips column displays in the QSO grid between absolute frequencies (e.g. `14.074 MHz`) and standard amateur band descriptors (e.g. `20m`).
* **Help (`m_btnHelp`):** Displays detailed inline help overlays about QSL card specifications, margins, and print rules.

#### Layout Tweaks and Scaling Panel
* **Checkmark Scale Slider (`m_chkSizeSlider`):** Modifies the scale (from micro-small to extra-large) of the auto-rendered database checkmarks (`X` or `/`) inside the template checkboxes. This ensures checkmarks perfectly match your custom graphics!
* **Margin Spinboxes (`m_topMarginSpin` / `m_leftMarginSpin`):** Adjusts physical printer offset calibration by fractions of a millimeter. Use this to correct hardware drift when printing on precut index cards.
* **Print Reverse Checkbox (`m_chkPrintReverse`):** Rotates the printed output by 180 degrees. This is invaluable if your printer feeds cardstocks upside down or from the reverse tray.

---

### 2. QRP Options and Bragging Configurations

Under **Configuration** -> **Settings** -> **QSL Card Options**, you can access the newly designed **QRP Options Submenu**. This is where you configure how Qwain handles low-power QSO tags.

#### A. Controls and Interactive Actions
* **"Use QRP Bragging" Checkbox (`m_chkUseQrpSubstitution`):** Turns on smart low-power QRP text replacement when a QSO's transmit power is **6 Watts or less ($\le$ 6W)**.
* **Currently Substitutes Label (`m_lblCurrentlySubstitutes`):** Displays a live preview of the text that will be written onto the card (defaults to `QRP`).
* **Custom QRP Brag Checkbox (`m_chkCustomQrpBrag`):** Enable this to override the standard "QRP" tag text.
* **Custom QRP Brag Text Box (`m_txtCustomQrpBragText`):** A persistent 4-character maximum input box. Enter any custom alphanumeric string (such as `5W`, `QRPp`, `BRAG`, etc.). 
  * *Live updates:* Toggling the checkbox or typing text immediately updates the *Currently Substitutes* label. Unchecking the box instantly reverts the substitution string back to `QRP`.

#### B. The Rendering Fallback Hierarchy
When your QSO power is $\le$ 6W and *Use QRP Bragging* is enabled, the print engine applies this strict three-tier layout fallback rule:
1. **Target Dedicated `<qrp>` Field First:** If your template contains a dedicated `<qrp>` tag or a table cell mapped to QRP, the custom bragging string (e.g., `QRP`) is written into that cell. Your normal signal report remains intact in the `<tone>` block.
2. **Fallback to Available `<tone>` Field:** If the template does **not** contain a dedicated `<qrp>` box, but does have an unused `<tone>` field (`<tone>`, `<tone_sent>`, or `<tone_rcvd>`), Qwain will automatically substitute your QRP bragging string inside that `<tone>` field! This ensures your card looks correct and signals your low-power achievement even on legacy templates.
3. **Silent layout preservation:** If **neither** field exists on your template, no substitution is performed, and your card renders cleanly without raising any warning panels or printer error logs.

---

### 3. Batch Printing Preferences (`m_printingPrefsGroup`)
When printing a queued sequence of dozens of callsigns, use these settings to automate the layout flow:

#### Print Sequencing
* **Card-Envelope Sequence (`m_chkCardThenEnv`):** Sends the QSL card, and then its corresponding envelope to the printer before moving to the next callsign in the queue. Perfect for hand-stuffing envelopes row-by-row.
* **All Cards then Envelopes (`m_chkAllCardsThenEnvs`):** Sends every single QSL card in your print queue to the card printer first. Once completed, it sends all mailing envelopes in a single batch to your envelope printer.

#### Pacing Options
* **One-at-a-time Pacing (`m_chkPacingOneAtATime`):** Qwain will pause after each card/envelope is printed and prompt you with a dialog. This allows you to verify alignment, load a new cardstock in your printer, and press "Proceed" when ready.
* **Continuous Pacing (`m_chkPacingContinuous`):** Fully automated batch printing. Sends jobs sequentially with an adjustable cooldown delay (via the **Pacing Delay SpinBox `m_spinPacingDelay`**). Useful for large, heavy-duty office printers.

#### Hardware Routing
* **Card Printer Dropdown (`m_comboCardPrinter`) & Envelope Printer (`m_comboEnvPrinter`):** Lets you route cardstock designs to one physical printer (e.g. a heavy-tray photo printer) and envelopes to another printer (e.g. an envelope-fed laser printer) simultaneously!

---

### 4. Overflow Card Support (Advanced QSO Wrapping)
What happens if you worked a contest station 10 times, but your chosen card template only has room for 3 table rows? Qwain handles this automatically!
* **Detection & Overflow Split:** If the number of contacts in your queue for a single station exceeds your template's table limit, Qwain splits the QSO list.
* **Multi-Card Generation:** It prints the first 3 rows on Card 1 (rendered as *"Card 1 of 2"*), and automatically generates Card 2 (*"Card 2 of 2"*) for the remaining rows!
* **Auto-Routing:** No manual layout manipulation required; Qwain tracks, formats, and numbers overflow cards seamlessly.

---

## Chapter 6: Recieving Inbound Cards (ReceiveQslsDialog)

Handling inbound cards is just as critical as printing outbound ones. The **ReceiveQslsDialog** acts as an incoming scanner and log matchmaker, letting you log confirmed paper cards, verify databases, and queue return cards with a single click.

```
+-----------------------------------------------------------------------------+
| Receive QSL Cards & Logger                                                  |
+-----------------------------------------------------------------------------+
| INBOUND CARD SEARCH: [ JA3YAA         ] (Type Callsign or Scan Barcode)      |
+-----------------------------------------------------------------------------+
| CHOSEN STATION BIO:                        | MATCHING QSO LOG ENTRIES       |
| Name: Osaka University Club                | Select QSO confirmed by card:  |
| Country: Japan   State: Osaka              | [x] 15m FT8 | 2026-10-01 | 599  |
| QSL Via: Bureau                            | [ ] 10m SSB | 2026-08-14 | 59   |
| Address: Toyonaka, Osaka 560-0043          |                                |
|                                            | [Select All Results Button]    |
+-----------------------------------------------------------------------------+
| CONFIRMATION OPTIONS:                                                       |
| Date Card Received: [ 2026-10-05 ] [Calendar v]                            |
|                                                                             |
| [x] Queue Return Card (Auto-adds JA3YAA to Outbound QSL Print Queue)        |
+-----------------------------------------------------------------------------+
| Real-time Logs:                                                             |
| [08:54:30] Matched 2 QSOs for JA3YAA.                                       |
| [08:54:35] [btnSubmit: COMMIT INBOUND CONFIRMATION & SYNC LOGS ]            |
+-----------------------------------------------------------------------------+
```

### 1. Interactive Input Controls
* **Search Field (`m_searchEdit`):** Input the incoming callsign here. *Advanced feature:* If your inbound QSL cards are printed with barcodes containing QSO or Callsign metadata, click this field, scan the barcode, and Qwain will instantly load their profile!
* **Date Received Entry (`m_dateReceivedEdit`):** Specify the date you received their card.
* **Calendar Picker ToolButton (`m_calendarBtn`):** Launches an interactive visual calendar popup. Pick a date, and it automatically formats the received date correctly.

### 2. Inbound Match Matrix (`m_contactsTable`)
When you search a callsign, all unconfirmed QSOs with that station are populated in this table.
* Check the boxes next to the specific contacts that match the physical card you are holding.
* **Select All (`m_selectAllBtn`):** Instantly selects every matching QSO in the list.

### 3. Quick Outbound Return Matcher
* **Queue Return QSL Card (`m_sendReturnCardBtn`):** Toggle this button **ON**. When you submit the confirmation, Qwain will **automatically calculate, format, and add an outbound return QSL card for this station directly into your QSL printing queue!** You can confirm their card and prepare their return card in a single action!

### 4. Commit Actions and Logs
* **Real-time Status Terminal (`m_statusLog`):** Displays immediate database response notifications (e.g., *"Successfully matched QSO"*, *"QRZ sync complete"*, *"ADIF logfile updated"*).
* **Submit Button (`m_submitBtn`):** Commits the received confirmation. This modifies your local ADIF file, marks the status as received, updates your local SQLite database records, and uploads the verified QSL receipt status directly to QRZ, eQSL, or LoTW.

---

## Chapter 7: Backdrop Assets and Solid Color Gallery (ImageViewer)

The **ImageViewer** is your asset manager. It handles card backdrops, operator photos, club logos, and flat color canvases, keeping your templates looking gorgeous.

```
+-----------------------------------------------------------------------------+
| Image Viewer & Solid Color Backdrop Gallery                                 |
+-----------------------------------------------------------------------------+
| GALLERY ITEMS:                           | LARGE HIGH-FIDELITY PREVIEW      |
| [ Thumbnail: Sunset ] [ Thumbnail: Shack]  |                                  |
| [ Thumbnail: Blue   ] [ Thumbnail: Solid]  |                                  |
|                                          |   Sunset Background Image        |
|                                          |   140mm x 90mm                   |
|                                          |                                  |
+-----------------------------------------------------------------------------+
| ASSET OPERATIONS:                                                           |
| [btnUploadLocal: UPLOAD PHOTO ]  [btnUploadUrl: DOWNLOAD FROM LINK ]        |
| [btnRemoveImage: DELETE PHOTO ]  [x] Solid Color Mode                       |
| Favorites History: [ #0000ff ] [ #ff0000 ] [ #00ff00 ] [Color Wheel Dialog] |
+-----------------------------------------------------------------------------+
```

### 1. Gallery Grid & High-Fidelity Previews
* **Gallery list (`m_galleryList`):** Displays previews of your visual assets. 
* **Preview Panel (`m_previewLabel`):** Shows a large, high-fidelity render of the selected graphic, including its file size, dimensions, and color space details.

### 2. Asset Import Options
* **Upload Local Image:** Opens your system file manager. Import standard image files (`.jpg`, `.png`, `.bmp`) or scalable vectors (`.svg`) directly into your card design project directory.
* **Upload from URL:** Launches an input prompt. Paste any web image URL, and Qwain downloads, processes, and formats the image for your cards.
* **Remove Image:** Instantly purges the selected backdrop from your assets directory.

### 3. Solid Color Mode & Color History Console
* **Solid Color Checkbox (`m_useSolidColorChk`):** Turn this **ON** to swap image backdrops for a flat solid color. This is excellent for creating clean, minimalist layout designs.
* **Live Color Wheel (`m_colorWheel`):** Opens a professional color wheel dialog. Click and drag to pick standard RGB/HSV color values with real-time canvas updates.
* **Favorites History Row (`m_historyLayout`):** Automatically saves your custom color choices. Click any color box in this row to instantly apply previously used shades.

---

## Chapter 8: Off-line QRZ Cache Tools (CacheViewerDialog)

To maintain lightning-fast response times and allow offline shack operations, Qwain stores downloaded QRZ station biographies, photos, and addresses in a local database cache. The **CacheViewerDialog** is your diagnostic command board for this cache database.

```
+-----------------------------------------------------------------------------+
| Offline QRZ Cache Manager                                                  |
+-----------------------------------------------------------------------------+
| FILTER SEARCH: [ W1AW             ]                                         |
+-----------------------------------------------------------------------------+
| CACHED STATION DIRECTORY:         | OFFLINE DATA VIEWER (Rich HTML)         |
| Callsign | Last Updated           |                                         |
| W1AW     | 2026-10-04 18:20       |  W1AW - ARRL Headquarters               |
| K1JT     | 2026-10-03 14:15       |  Address: 225 Main St, Newington CT     |
| K1N      | 2026-09-08 09:12       |  Grid: FN31pr                           |
|          |                        |  Cached Photo: [ arrl_hq.png ]          |
+-----------------------------------------------------------------------------+
| DATABASE SYSTEM CONTROLS:                                                   |
| [Refresh List] [btnEdit: EDIT CACHE ] [btnDelete: PURGE ENTRY]             |
| [btnRebuild: DEFRAGMENT INDEX ]       [btnSync: FORCE ONLINE QRZ UPDATE ]   |
+-----------------------------------------------------------------------------+
```

### 1. Offline Database Indexer
* **Filter Search (`m_searchEdit`):** Instantly filters cached stations as you type.
* **Cache Listing (`m_listWidget`):** Shows all cached callsigns and their last update timestamp.

### 2. Live Controls & Sync Commands
* **Refresh List:** Refreshes the active listing from the SQLite database.
* **Edit Cache (`onEditClicked`):** Launches an editor window. If an operator has moved or changed their mailing details but hasn't updated QRZ, you can manually correct their cached address, name, or comments here!
* **Purge Entry (`onDeleteClicked`):** Deletes the selected station cache record.
* **Defragment Index (`onRebuildClicked`):** Rebuilds indices and purges corrupt keys, optimizing search performance.
* **Force Online QRZ Update (`onRefreshDataClicked`):** Bypasses the local cache, connects to the internet, and downloads the absolute latest station bio, address, and profile pictures from QRZ, updating your local cache.

---

## Chapter 9: Advanced Configuration Reference Guide

Access this exhaustive settings module via **Configuration Menu** -> **Settings...**. It is organized into 11 specialized panels.

| Settings Category | Sub-Category Option | Direct Functional Explanation |
| :--- | :--- | :--- |
| **Network & Integration** | UDP Binding Port | Sets the socket port (default `2237`) to listen for live XML spot notifications from external loggers. |
| | TCP Server Port | Sets the listening port to receive remote ADIF data feeds. |
| **Credentials & APIs** | QRZ Username/Password | Your login details to fetch bios, addresses, and update logbooks. |
| | QRZ XML Subscriber | Enable this to bypass scrapers and use high-speed premium XML database connections. |
| | LoTW Certificate Path | Browse and link your active `.p12` TQSL credentials file to synchronize confirmations. |
| **DX Cluster & Feeds** | Active Nodes | Map and store cluster server addresses (telnet) to fetch real-time spots. |
| | Refresh Rate | Adjust intervals (in seconds) to pull new spots. |
| **Logging / ADIF** | Station Logfile Path | Selects the target `.adi` file to monitor and write printed card updates. |
| | Duplicate Check Rules | Set criteria (e.g. Band + Mode + Date) to identify duplicate QSO entries. |
| **Backup & Cloud Sync** | Auto Backup Path | Sets the directory to write automatic backup archives on closing Qwain. |
| | Local Archive Retention | Set max backups to keep (e.g. 50 files) to save disk space. |
| **User Interface** | Selected Theme | Switch between Dark, Light, System, and Custom visual interfaces. |
| | Status Scroll Limit | Set maximum console log rows before oldest entries are pruned. |
| | Always Eastern Time | If checked, automatically translates logged times into US Eastern Time. |
| **Printers & Media** | Layout Settings | Choose paper dimensions (presets for US Letter, DL Envelope, EU QSL standard). |
| | Margins & Offsets | Calibrate printer hardware paper trays to correct image drift. |
| **QSL Options (QRP)** | Use QRP Bragging | Substitutes standard signal reports for QRP text if QSO power is $\le$ 6W. |
| | Custom QRP Brag | Overrides standard "QRP" text with custom persistent text (max 4 characters). |
| | Emission Modes | Toggle Standard (ITU-compliant codes) or Generic (logged text) emission names. |
| **Station Info** | My Call / Operator | Sets your primary station details used in distance calculations. |
| | CQ/ITU Zones | Manually select zones to verify award maps. |
| **Debug & Diagnostic** | Log Levels | Change reporting logging detail (Off, Error, Info, Deep Debug). |
| | System Diagnostics | Runs built-in self-tests to analyze database integrity. |

---

## Conclusion

Qwain is designed to make your logging experience smooth and precise. By following the guides and workflows in this manual, you can take full control of your station's printing, lookup, and database management. 

Clear propagation, beautiful cards, and happy hunting! **73!**
