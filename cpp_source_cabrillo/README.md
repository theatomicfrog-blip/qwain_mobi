# Standalone ADIF to Cabrillo Converter

A clean, high-performance, and lightweight standalone desktop utility to convert **ADIF (Amateur Data Interchange Format)** amateur radio logs into **Cabrillo** format logs for contest submissions.

This utility was spun off directly from the main **Qwain** logging suite. It has **zero** third-party library dependencies other than standard Qt (Qt 5 or Qt 6) and the C++ Standard Library.

---

## 🛠️ Prerequisites & Installation

Before compiling, you need to install a C++ compiler, **CMake**, and **Qt Development libraries** (either Qt 5 or Qt 6).

### 🐧 Debian / Ubuntu Linux
Open your terminal and run:
```bash
sudo apt update
sudo apt install -y build-essential cmake qtbase5-dev qtchooser qt5-qmake
```
*(If you prefer Qt 6, run `sudo apt install -y build-essential cmake qt6-base-dev` instead).*

### 🍏 macOS (via Homebrew)
Open your terminal and run:
```bash
brew install cmake qt
```

### 🪟 Windows
1. Download and install **CMake** from [cmake.org](https://cmake.org/download/).
2. Download and install **Qt** using the [Qt Online Installer](https://www.qt.io/download-open-source).
3. Ensure you have **Visual Studio** (with C++ Desktop development workload) or **MinGW** installed.

---

## 🚀 Beginner Compilation Guide

To build the executable, follow these step-by-step instructions.

### Step 1: Open Terminal / Command Prompt
Navigate to the `cpp_source_cabrillo` directory:
```bash
cd cpp_source_cabrillo
```

### Step 2: Create a Build Directory
It is best practice to perform an "out-of-source" build to keep the source files clean:
```bash
mkdir build
cd build
```

### Step 3: Configure the Build with CMake
Run CMake to generate the build files.

* On **Linux / macOS**:
  ```bash
  cmake ..
  ```
  *(If your Qt installation is in a custom path, you can specify it using: `cmake -DCMAKE_PREFIX_PATH=/path/to/Qt ..`)*

* On **Windows** (using Visual Studio):
  ```bash
  cmake ..
  ```

### Step 4: Compile the Executable
Now compile the binary:
```bash
cmake --build .
```

### Step 5: Run the Standalone Application
Once compilation finishes successfully, you can run the executable directly!

* On **Linux / macOS**:
  ```bash
  ./CabrilloConverter
  ```
* On **Windows**:
  ```cmd
  Debug\CabrilloConverter.exe
  ```

---

## 🌟 Key Features
- **Header Field Auto-Population**: Fully configures Cabrillo fields like Location, Operators, Claimed Score, etc.
- **Fixed Choices Configuration**: Operators, Assisted, Band, Mode, Power, Station, and Transmitter dropdown menus.
- **Flexible Data Input**: Direct drag-and-drop or copy-paste support for ADIF text files.
- **Layout Customization**: Support for Custom templates for highly flexible formatting.
- **Copy-to-Clipboard & Fast Export**: Zero-lag conversions.
