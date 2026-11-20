import re

with open('cpp_source/configdialog.cpp', 'r') as f:
    content = f.read()

target = 'item->setData(Qt::UserRole, kp);'
if target in content:
    content = content.replace(target, target + '\\n            item->setToolTip(kp);')
    with open('cpp_source/configdialog.cpp', 'w') as f:
        f.write(content)
        print("Patched tooltip")
else:
    print("Could not find setData in configdialog.cpp")
