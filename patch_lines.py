import re
with open('configdialog.cpp', 'r') as f:
    content = f.read()

content = content.replace('m_printersList);\\n            item->setData(Qt::UserRole, kp);\\n            item->setToolTip(kp);',
                          'm_printersList);\\n            item->setData(Qt::UserRole, kp);\\n            item->setToolTip(kp);')

# Wait, let's just use regex to replace literal \n with newline for QListWidgetItem
content = re.sub(r'QListWidgetItem\(formatPrinterDisplay\(kp\), m_printersList\);\\n\s*item->setData\(Qt::UserRole, kp\);(\\n\s*item->setToolTip\(kp\);)?',
                 r'QListWidgetItem(formatPrinterDisplay(kp), m_printersList);\n            item->setData(Qt::UserRole, kp);\n            item->setToolTip(kp);',
                 content)

with open('configdialog.cpp', 'w') as f:
    f.write(content)
