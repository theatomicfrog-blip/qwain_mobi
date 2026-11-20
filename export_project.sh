#!/bin/bash
PROJECT="AdifMonitor"
DATE_TIME=$(date +"%H:%M.%d-%m-%Y")
ZIP_NAME="${PROJECT}.${DATE_TIME}.zip"
echo "Creating archive $ZIP_NAME..."

if command -v zip &> /dev/null; then
    zip -r "$ZIP_NAME" cpp_source/ CMakeLists.txt What_I_Did.md \*.md \*.xml \*.json \*.js \*.ts \*.qrc \*.txt \*.png \*.svg \*.cpp \*.h -x "node_modules/*" ".next/*" ".git/*" "dist/*" "clean_files/*" "*.zip" "*AdifMonitor*" "*adifmonitor*" "*terminal-adif-monitor*"
else
    echo "zip command not found. Falling back to Python 3 zipping..."
    python3 -c "
import os, zipfile

zip_name = '$ZIP_NAME'
exclude_dirs = {'node_modules', '.next', '.git', 'dist', 'clean_files'}
include_exts = {'.xml', '.json', '.js', '.ts', '.qrc', '.txt', '.png', '.svg', '.cpp', '.h', '.md'}

with zipfile.ZipFile(zip_name, 'w', zipfile.ZIP_DEFLATED) as zipf:
    for root, dirs, files in os.walk('.'):
        # Prune excluded directories in-place to avoid walking down them
        dirs[:] = [d for d in dirs if d not in exclude_dirs]
        for file in files:
            if file.lower().endswith('.zip') or 'adifmonitor' in file.lower() or 'terminal-adif-monitor' in file.lower():
                continue
            file_path = os.path.relpath(os.path.join(root, file), '.')
            ext = os.path.splitext(file)[1].lower()
            if (file_path == 'CMakeLists.txt' or 
                file_path == 'What_I_Did.md' or 
                file_path == 'What I did.txt' or 
                file_path == 'Advanced Manual for Qwain.md' or
                file_path == 'Qwain_User_Manual.md' or 
                file_path == 'AGENTS.md' or
                root.startswith('./cpp_source') or 
                root.startswith('cpp_source') or 
                ext in include_exts):
                zipf.write(file_path)
"
fi

echo "Done. Exported as $ZIP_NAME"

