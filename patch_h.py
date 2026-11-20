import re

with open('configdialog.h', 'r') as f:
    content = f.read()

# Add QNetworkAccessManager and QNetworkReply to includes (they are there)
# Add m_netManager to class ConfigDialog private section
content = content.replace('QListWidget *m_contentsWidget;', 'QNetworkAccessManager *m_netManager;\\n    QListWidget *m_contentsWidget;')

# Add createAwardsPage() to private section
content = content.replace('void createBackupPage();', 'void createBackupPage();\\n    void createAwardsPage();')

# Add processPrefetchQueue() to private section
content = content.replace('void updatePrintersList();', 'void updatePrintersList();\\n    void processPrefetchQueue();')

# Add m_stationAwards, etc.
content = content.replace('QListWidget *m_stationAdifTags;', 'QListWidget *m_stationAdifTags;\\n    QListWidget *m_stationAwards;\\n    QListWidget *m_stationCategories;\\n    QListWidget *m_stationCategoryAwards;')

with open('configdialog.h', 'w') as f:
    f.write(content)
