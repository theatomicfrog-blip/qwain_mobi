2026-10-15 08:14:00 (PDT)

# Qwain Codebase Architecture Reference Guide
*A Human-Readable Architecture Mapping: Legacy Monolithic Structure vs. New Streamlined Modular Architecture*

---

## 1. Executive Summary & Purpose
To optimize AI Studio token usage, accelerate compilation speeds, and ensure isolated, safe feature development, the Qwain C++/Qt codebase is being refactored from large monolithic files into modular, single-responsibility C++ source modules.

This document serves as a reference for aligning prompt requests with exact module files, allowing future features and edits to target small, focused source files (200–500 lines) instead of parsing multi-thousand-line monolithic files.

---

## 2. Module Mapping: Legacy vs. New Architecture

| Legacy Monolithic File | New Refactored Module Files | Primary Responsibilities & Subsystem Role |
|---|---|---|
| **`qsldesigner.cpp`**<br>*(6,570 lines / ~293 KB)* | **`qsldesigner.cpp` / `.h`** | Interactive WYSIWYG canvas editor, drag-and-drop panning, crosshairs, reference points, and element alignment. |
| | **`qsldesigner_tpl.cpp` / `.h`** | **Template I/O Subsystem**: Modal export options dialog, Qwain `.tpl` signature encoding (`"QWAIN-TPL-V1"`), base64 image embedding/extraction, and template import validation (`QslTemplateIO`). |
| **`mainwindow.cpp`**<br>*(5,180 lines / ~230 KB)* | **`mainwindow.cpp` / `.h`** | Main application window, ADIF logfile import/export, SQLite database integration, and UI menu triggers. |
| | **`mw_lookups.cpp` / `.h`** | **Callsign & XML Lookup Subsystem**: QRZ.com XML session authentication, HamQTH callsign queries, XML tag extraction, and lookup parsing helpers (`MwLookupEngine`). |
| **`qslpreviewdialog.cpp`**<br>*(7,088 lines / ~309 KB)* | **`qslpreviewdialog.cpp` / `.h`** | Main QSL card/envelope preview dialog, contact table management, manual overrides, and preview canvas updates. |

---

## 3. Subsystem Communication & Token Savings Guidelines

### Token Savings Summary
- **Old Request Cost**: Modifying a template export/import or QRZ lookup feature required parsing `qsldesigner.cpp` (6,570 lines) or `mainwindow.cpp` (5,180 lines) in its entirety (**~80,000+ tokens per turn**).
- **New Modular Cost**: Future template export/import prompts only inspect `qsldesigner_tpl.cpp` (~280 lines) or `mw_lookups.cpp` (~40 lines) (**~2,500–4,000 tokens per turn**).
- **Net Token Consumption Reduction**: **85% – 95% savings per prompt turn!**

---

## 4. Build Configuration (`CMakeLists.txt`)
All refactored modules are integrated into the root build configuration:

```cmake
add_executable(Qwain
    cpp_source/main.cpp
    cpp_source/mainwindow.cpp
    cpp_source/mainwindow.h
    cpp_source/mw_lookups.cpp       # New Callsign & XML Lookup Module
    cpp_source/mw_lookups.h
    cpp_source/qsldesigner.cpp
    cpp_source/qsldesigner.h
    cpp_source/qsldesigner_tpl.cpp   # New All-Inclusive Template I/O Module
    cpp_source/qsldesigner_tpl.h
    cpp_source/qslpreviewdialog.cpp
    cpp_source/qslpreviewdialog.h
    # ... remaining UI dialogs and resources
)
```
