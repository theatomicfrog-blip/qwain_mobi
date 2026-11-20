import os
import time
import datetime
import zipfile

os.environ['TZ'] = 'America/Los_Angeles'
time.tzset()

dt_40 = datetime.datetime(2024, 1, 1, 12, 0, 0)
ts_40 = time.mktime(dt_40.timetuple())

exclude_dirs = {
    'node_modules', '.next', '.git', 'dist', 'clean_files', 'build', 'agent_build', 'home', 'hooks', 'template_creator'
}
include_exts = {
    '.xml', '.json', '.js', '.ts', '.qrc', '.txt', '.png', '.svg', '.cpp', '.h', 
    '.md', '.desktop', '.pro', '.stash', '.gz', '.csv', '.sh'
}

for root, dirs, files in os.walk('.'):
    dirs[:] = [d for d in dirs if d not in exclude_dirs and not d.startswith('.')]
    for file in files:
        file_path = os.path.relpath(os.path.join(root, file), '.')
        if file_path.startswith('/'): continue
        os.utime(file_path, (ts_40, ts_40))

zip_name = '/export.zip'
with zipfile.ZipFile(zip_name, 'w', zipfile.ZIP_DEFLATED) as zipf:
    for root, dirs, files in os.walk('.'):
        dirs[:] = [d for d in dirs if d not in exclude_dirs and not d.startswith('.')]
        for file in files:
            file_path = os.path.relpath(os.path.join(root, file), '.')
            if file_path.startswith('/'): continue
            if file.lower().endswith('.zip') or 'adifmonitor' in file.lower() or 'terminal-adif-monitor' in file.lower():
                continue
            if file == 'CMakeCache.txt' or file == 'Makefile' or file.endswith('.cmake'):
                continue
            ext = os.path.splitext(file)[1].lower()
            is_special_file = file_path in [
                'CMakeLists.txt', 'CMakeLists_test.txt', 'What_I_Did.md', 
                '.eslintrc.json', '.env.example', '.gitignore'
            ]
            if (is_special_file or 
                file_path.startswith('cpp_source') or 
                ext in include_exts or
                file.startswith('.')):
                zipf.write(file_path)

print("Export completed!")
