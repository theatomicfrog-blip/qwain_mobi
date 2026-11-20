#!/bin/bash
# Exit on error
set -e

echo "=== Step 1: Setting up paths and desktop configuration ==="
# Create a deployment-safe desktop file with correct paths
cat <<EOF > qwain_deploy.desktop
[Desktop Entry]
Version=1.0
Type=Application
Name=Qwain
Comment=Amateur Radio Logbook
Exec=Qwain
Icon=logo_QWAIN
Terminal=false
Categories=Utility;HamRadio;
EOF

# Ensure the icon is named exactly what's inside the desktop file and in the root
if [ -f "logo_QWAIN.png" ]; then
    echo "Found logo_QWAIN.png"
elif [ -f "logo.png" ]; then
    cp logo.png logo_QWAIN.png
    echo "Copied logo.png to logo_QWAIN.png"
fi

echo "=== Step 2: Running linuxdeployqt inside Docker ==="
sudo docker run --rm \
  -v "$(pwd)":/app \
  -v /home/w4djf/appimage/squashfs-root:/tools \
  -w /app \
  qwain-builder /tools/AppRun qwain_deploy.desktop -appimage -no-translations

# Clean up temporary desktop file
rm -f qwain_deploy.desktop

# Find the built AppImage (it might be in the root, or inside some output subdir depending on linuxdeployqt)
APPIMAGE_FILE=$(find . -maxdepth 1 -name "*.AppImage" | head -n 1)

if [ -z "$APPIMAGE_FILE" ]; then
    # Sometimes it generates inside a temporary directory or with a specific pattern. Let's search recursive:
    APPIMAGE_FILE=$(find . -name "*.AppImage" | head -n 1)
fi

if [ ! -z "$APPIMAGE_FILE" ]; then
    REAL_PATH=$(realpath "$APPIMAGE_FILE")
    DIR_PATH=$(dirname "$REAL_PATH")
    BASE_NAME=$(basename "$APPIMAGE_FILE")
    
    echo "=================================================="
    echo "SUCCESS: Portable AppImage built successfully!"
    echo "File location: $REAL_PATH"
    echo "=================================================="
    echo "To run your app, execute:"
    echo "cd $DIR_PATH && ./$BASE_NAME"
    echo "=================================================="
else
    echo "ERROR: AppImage was not found. Please check linuxdeployqt logs."
fi
