with open('cpp_source/qslpreviewdialog.cpp', 'r') as f:
    text = f.read()

import re
m = re.search(r'connect\(btnBox, &QDialogButtonBox::rejected, &confirmDlg, &QDialog::reject\);.*?m_batchSelectedPrinter = printerCombo->currentData\(\)\.toString\(\);\n    }', text, flags=re.DOTALL)
if m:
    print("Found using regex.")
    with open('target.txt', 'w') as f2:
        f2.write(m.group(0))
else:
    print("Not found with regex.")
