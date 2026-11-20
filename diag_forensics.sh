#!/bin/bash
echo "=== Qwain Forensics Diagnostic Report ==="
echo ""
echo "[1] Image Integrity Check:"
file cpp_source/logo.png cpp_source/logo_QWAIN.png
echo ""
echo "[2] Splash Screen Logic Check (main.cpp):"
grep -A 10 "QSplashScreen \*splash" cpp_source/main.cpp
echo ""
echo "[3] MainWindow Logo Check (mainwindow.cpp):"
grep -A 8 "QLabel \*logoLabel" cpp_source/mainwindow.cpp
echo ""
echo "[4] Shared Memory Lock Check:"
ipcs -m
echo "========================================="
