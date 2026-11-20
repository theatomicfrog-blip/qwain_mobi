# Exact Prompt Specification: ConfigDialog > "User Interface" Page

This document provides a complete, self-contained prompt describing the **"User Interface" configuration page** from `cpp_source/configdialog.cpp`. You can copy and paste the prompt in the box below directly into any AI code generator, UI design engine, or frontend framework compiler (e.g., React, Vue, Svelte, Tailwind, Flutter, Qt/QML, GTK, SwiftUI) to faithfully recreate the page, its components, visual hierarchy, styling, and interactive behaviors.

---

```markdown
### SYSTEM ROLE & INSTRUCTION
You are an expert UI/UX engineer and frontend developer. Build a pixel-accurate, fully functional settings page replicating the "User Interface" tab of the application preferences dialog ("Configure User Interface") as defined in the desktop C++/Qt application `configdialog.cpp`.

### DIALOG CONTAINER CONTEXT
* **Window Title:** "Configure User Interface"
* **Window Size:** Default minimum dimensions: 600px width × 450px height.
* **Layout Structure:**
  - Left Sidebar: Category navigation list (`QListWidget`, width 180–200px) showing icons/labels for categories (Network Configuration, Credentials, Feeds, Logging, Backup/Restore, **User Interface**, Printers, QSL Card Options, Station Information).
  - Main Right Content Area: Multi-page stacked widget displaying the selected category page.
  - Dialog Footer: Action button box aligned to the bottom right with three standard buttons:
    * `[ OK ]` (commits settings and closes dialog)
    * `[ Apply ]` (commits settings immediately without closing dialog)
    * `[ Cancel ]` (discards uncommitted modifications and closes dialog)

---

### "USER INTERFACE" PAGE SPECIFICATION

The "User Interface" configuration page uses a two-column form layout (`QFormLayout` / label-value grid) with left-aligned descriptive labels and right-aligned interactive control groups.

#### 1. Row 1: "Skin Background Color:"
* **Controls Group (Horizontal Row):**
  1. **Color Value / Swatch Badge (`m_currentColorLabel`):**
     - Displays the current hex color string (e.g., `#221e1f`).
     - Rendered with a visible 1px solid border and a sample background swatch reflecting the current skin color.
     - Default initial value: `#221e1f`.
  2. **"Choose Color..." Button (`QPushButton`):**
     - Clicking opens a color picker / palette modal dialog (`StyleDialog`).
     - Selecting a color updates the hex label, the skin color variable, and immediately refreshes the preview.
  3. **Horizontal Spacing:** 15px gap.
  4. **Revert Color Scheme Sub-Group (Vertical Layout):**
     - **Button:** `[ Revert Color Scheme ]`
     - **Sub-label beneath button:** "Use this if you mess up your Custom Color Scheme" (small 10px font, muted gray `#888888`).
     - **Behavior on click:** Pops up a modal dialog titled *"Revert Color Scheme"*:
       * Header text: "Revert To: Dark, Light, or None"
       * Dropdown options:
         - **Dark:** Sets skin background color to `#181b22`, clock text to white (`#ffffff`), call info text to white (`#ffffff`), QSO history text to white (`#ffffff`), system scheme to "Dark".
         - **Light:** Sets skin background color to `#f8fafc`, clock text to black (`#000000`), call info text to black (`#000000`), QSO history text to black (`#000000`), system scheme to "Light".
         - **None:** Sets skin background color to `#221e1f`, clock text to white (`#ffffff`), call info text to white (`#ffffff`), QSO history text to white (`#ffffff`), system scheme to "None".
       * `[ OK ]` / `[ Cancel ]` buttons. When OK is clicked, resets all corresponding colors and scheme combo box.

#### 2. Row 2: "System Scheme:"
* **Controls Group (Horizontal Row):**
  1. **Scheme Dropdown (`QComboBox`):**
     - Options:
       * `Light`
       * `Dark`
       * `None` (Default)
       * `Custom`
  2. **Live Preview Button (`QPushButton`):**
     - Button text: "Example Button".
     - Dynamically changes its CSS / stylesheet in real-time when the scheme dropdown selection changes:
       * When **None**: Native OS default button style.
       * When **Light**: Flat light background (`#f5f5f5`), dark text (`#333333`), borderless, subtle hover highlight (`#e0e0e0`).
       * When **Dark**: Flat dark background (`#2b2b2b`), crisp white text (`#ffffff`), borderless, hover highlight (`#3b3b3b`).
       * When **Custom**: Styled with subtle border and accents matching the custom skin background color.
  3. **Horizontal Spacer:** Pushes controls to the left.

#### 3. Row 3: "Status Scrollback Lines:"
* **Control:** Integer SpinBox (`QSpinBox`).
* **Value Range:** Minimum `10`, Maximum `200`.
* **Default Value:** `50`.
* **Purpose:** Sets the maximum number of text history lines retained in the application's live status/console pane.

#### 4. Row 4: "Clock (UTC):"
* **Controls Group (Horizontal Row):**
  1. **Sample Text Badge (`QLabel`):**
     - Displays text: `"Sample Text"`.
     - Styled with a dark background (`#333333`), 2px padding, and renders with the active Clock font family, size (pt), weight/boldness, and text color.
     - Default: Arial 10pt, Color: White (`#ffffff`).
  2. **"Change Style..." Button (`QPushButton`):**
     - Clicking opens a font and color chooser dialog (`GtkFontChooserWidget`).
     - Allows choosing font family, size, boldness, and text color.
     - When accepted, updates the Clock Sample Text badge immediately.

#### 5. Row 5: "Callsign Info:"
* **Controls Group (Horizontal Row):**
  1. **Sample Text Badge (`QLabel`):**
     - Displays text: `"Sample Text"`.
     - Dark background (`#333333`), 2px padding, rendered with active Callsign Info font, size, weight, and color.
     - Default: Arial 10pt, Color: White (`#ffffff`).
  2. **"Change Style..." Button (`QPushButton`):**
     - Clicking opens font and color chooser dialog.
     - When accepted, updates the Callsign Info Sample Text badge immediately.

#### 6. Row 6: "QSO History:"
* **Controls Group (Horizontal Row):**
  1. **Sample Text Badge (`QLabel`):**
     - Displays text: `"Sample Text"`.
     - Dark background (`#333333`), 2px padding, rendered with active QSO History font, size, weight, and color.
     - Default: Arial 10pt, Color: White (`#ffffff`).
  2. **"Change Style..." Button (`QPushButton`):**
     - Clicking opens font and color chooser dialog.
     - When accepted, updates the QSO History Sample Text badge immediately.

---

### DATA PERSISTENCE & SETTINGS MAPPING
When the user clicks `[ Apply ]` or `[ OK ]`, the following settings keys are stored:
* `SkinColor` (string, hex color like `#221e1f`)
* `UI/systemScheme` (string: "Light", "Dark", "None", or "Custom")
* `UI/statusScrollLimit` (integer, 10–200, default 50)
* `UI/clockFont` & `UI/clockColor` (font family, pt size, color hex)
* `UI/callInfoFont` & `UI/callInfoColor` (font family, pt size, color hex)
* `UI/qsoListFont` & `UI/qsoListColor` (font family, pt size, color hex)
* `MapStyle` (integer, default 1: "Simplified (Black & White)")
* `UI/alwaysEastern` (boolean, default false)

### STYLING & ACCESSIBILITY REQUIREMENTS
* Clean, uncluttered layout matching professional technical software (e.g. amateur radio workstation / developer utility).
* Label column aligned cleanly on the left with consistent baseline alignment to controls.
* High contrast preview badges for font/color samples against their `#333333` swatch backgrounds.
* Responsive to dialog resizing, with form rows expanding horizontally.
```

---

### Summary of Component Architecture

| UI Row Label | Control Type | Default Value | Child / Secondary Elements |
|:---|:---|:---|:---|
| **Skin Background Color:** | Color Badge + Button + Revert Group | `#221e1f` | • Hex text & color swatch<br>• `[ Choose Color... ]`<br>• `[ Revert Color Scheme ]`<br>• Sub-caption: *"Use this if you mess up your Custom Color Scheme"* |
| **System Scheme:** | Dropdown (`QComboBox`) + Button Preview | `None` | • Options: `Light`, `Dark`, `None`, `Custom`<br>• `[ Example Button ]` with live dynamic CSS styling |
| **Status Scrollback Lines:** | SpinBox (`QSpinBox`) | `50` | • Range: `10` to `200` lines |
| **Clock (UTC):** | Sample Badge + Style Button | Arial 10pt, `#FFFFFF` | • `Sample Text` on `#333` background<br>• `[ Change Style... ]` (Font & Color modal) |
| **Callsign Info:** | Sample Badge + Style Button | Arial 10pt, `#FFFFFF` | • `Sample Text` on `#333` background<br>• `[ Change Style... ]` (Font & Color modal) |
| **QSO History:** | Sample Badge + Style Button | Arial 10pt, `#FFFFFF` | • `Sample Text` on `#333` background<br>• `[ Change Style... ]` (Font & Color modal) |
