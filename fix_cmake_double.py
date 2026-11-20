import re

with open('cpp_source/CMakeLists.txt', 'r') as f:
    lines = f.readlines()

new_lines = []
found_europe = False
for line in lines:
    if 'europeawarddialog.cpp' in line:
        if not found_europe:
            new_lines.append(line)
            found_europe = True
    else:
        new_lines.append(line)

with open('cpp_source/CMakeLists.txt', 'w') as f:
    for line in new_lines:
        f.write(line)
