import os
import time
import datetime
import zipfile

# Set local timezone to America/Los_Angeles (or any timezone, since we use past times it will be safe)
os.environ['TZ'] = 'America/Los_Angeles'
time.tzset()

# Define target timestamps far in the past (Year 2024)
dt_40 = datetime.datetime(2024, 1, 1, 12, 0, 0) # 12:00:00
dt_45 = datetime.datetime(2024, 1, 1, 12, 5, 0) # 12:05:00

ts_40 = time.mktime(dt_40.timetuple())
ts_45 = time.mktime(dt_45.timetuple())

exclude_dirs = {
    'node_modules', '.next', '.git', 'dist', 'clean_files', 'build', 'agent_build', 'home', 'hooks', 'template_creator'
}
include_exts = {
    '.xml', '.json', '.js', '.ts', '.qrc', '.txt', '.png', '.svg', '.cpp', '.h', 
    '.md', '.desktop', '.pro', '.stash', '.gz', '.csv', '.sh'
}

print("1. Touching all workspace files to stable past timestamps (Year 2024)...")

count_40 = 0
count_45 = 0

for root, dirs, files in os.walk('.'):
    # Prune excluded directories in-place to prevent walking into them
    dirs[:] = [d for d in dirs if d not in exclude_dirs and not d.startswith('.')]
    
    for file in files:
        file_path = os.path.relpath(os.path.join(root, file), '.')
        if file_path.startswith('/'):
            continue
            
        # Determine appropriate timestamp
        if file == 'Makefile':
            os.utime(file_path, (ts_45, ts_45))
            count_45 += 1
            print(f"  Touched Makefile to 12:05:00: {file_path}")
        else:
            os.utime(file_path, (ts_40, ts_40))
            count_40 += 1

print(f"Total touched: {count_40} files to 12:00:00, and {count_45} files to 12:05:00.\n")

print("2. Packaging export.zip with year 2024 past timestamps...")
zip_name = '/export.zip'

with zipfile.ZipFile(zip_name, 'w', zipfile.ZIP_DEFLATED) as zipf:
    for root, dirs, files in os.walk('.'):
        dirs[:] = [d for d in dirs if d not in exclude_dirs and not d.startswith('.')]
        for file in files:
            file_path = os.path.relpath(os.path.join(root, file), '.')
            if file_path.startswith('/'):
                continue
            if file.lower().endswith('.zip') or 'adifmonitor' in file.lower() or 'terminal-adif-monitor' in file.lower():
                continue
            ext = os.path.splitext(file)[1].lower()
            
            # Explicitly include specific files or file types/extensions
            is_special_file = file_path in [
                'CMakeLists.txt', 'CMakeLists_test.txt', 'Makefile', 'What_I_Did.md', 
                '.qmake.stash', '.eslintrc.json', '.env.example', '.gitignore'
            ]
            
            if (is_special_file or 
                file_path.startswith('cpp_source') or 
                ext in include_exts or
                file.startswith('.')):  # Ensure hidden files like .qmake.stash are included
                
                zipf.write(file_path)

print(f"Successfully created zip file: {zip_name}\n")

print("3. Verifying the zip file contents and timestamps:")
with zipfile.ZipFile(zip_name, 'r') as z:
    infos = z.infolist()
    print(f"Total files in ZIP: {len(infos)}")
    
    # Check if Makefile is newer than others and .qmake.stash has correct time
    makefile_info = None
    qmake_stash_info = None
    future_files_count = 0
    
    for info in infos:
        if info.filename == 'Makefile':
            makefile_info = info
        elif info.filename == '.qmake.stash':
            qmake_stash_info = info
            
        # Any timestamp in year 2026 is considered future-shifted relative to 2024
        if info.date_time[0] >= 2026:
            future_files_count += 1
            
    if makefile_info:
        print(f"  Makefile Timestamp in ZIP: {makefile_info.date_time}")
    else:
        print("  WARNING: Makefile NOT found in ZIP!")
        
    if qmake_stash_info:
        print(f"  .qmake.stash Timestamp in ZIP: {qmake_stash_info.date_time}")
    else:
        print("  WARNING: .qmake.stash NOT found in ZIP!")
        
    print(f"  Files in the future (Year 2026 or later): {future_files_count}")
