const fs = require("fs");
let cppPath = "cpp_source/qslpreviewdialog.cpp";
let cppCode = fs.readFileSync(cppPath, "utf8");

// Change list widget addition
let searchList = `m_templateList = new QListWidget(this);
    QSettings tSettings(QCoreApplication::organizationName(), "AdifMonitor");
    tSettings.beginGroup("QslTemplates");
    m_templateList->addItems(tSettings.childGroups());
    tSettings.endGroup();
    connect(m_templateList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        onTemplateChosen(item->text());
    });
    connect(m_templateList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current, QListWidgetItem *previous) {
        if (current) updateThumbnail(current->text());
    });
    
    rightLayout->addWidget(m_templateList, 1);
    m_thumbnailScene = new QGraphicsScene(this);
    m_thumbnailView = new QGraphicsView(m_thumbnailScene, this);
    m_thumbnailView->setFixedHeight(150);
    m_thumbnailView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_thumbnailView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rightLayout->addWidget(new QLabel("<b>Preview:</b>"));
    rightLayout->addWidget(m_thumbnailView, 0);`;

let replaceList = `m_templateList = new QListWidget(this);
    rightLayout->addWidget(m_templateList, 1);

    rightLayout->addWidget(new QLabel("<b>Envelopes:</b>"));
    m_envTemplateList = new QListWidget(this);
    rightLayout->addWidget(m_envTemplateList, 1);

    QSettings tSettings(QCoreApplication::organizationName(), "AdifMonitor");
    tSettings.beginGroup("QslTemplates");
    for (const QString &name : tSettings.childGroups()) {
        tSettings.beginGroup(name);
        bool isEnv = tSettings.value("isEnvelope", false).toBool();
        tSettings.endGroup();
        if (isEnv) m_envTemplateList->addItem(name);
        else m_templateList->addItem(name);
    }
    tSettings.endGroup();

    connect(m_templateList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        m_previewingEnvelope = false;
        onTemplateChosen(item->text());
    });
    connect(m_templateList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current, QListWidgetItem *previous) {
        if (current && !m_previewingEnvelope) {
            updateThumbnail(current->text());
        }
    });

    connect(m_envTemplateList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        m_previewingEnvelope = true;
        onTemplateChosen(item->text());
    });
    connect(m_envTemplateList, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *current, QListWidgetItem *previous) {
        if (current && m_previewingEnvelope) {
            updateThumbnail(current->text());
        }
    });

    // Dedicated preview window
    m_previewDialog = new QDialog(this);
    m_previewDialog->setWindowTitle("Preview");
    m_previewDialog->resize(300, 200);
    QVBoxLayout *previewLayout = new QVBoxLayout(m_previewDialog);
    m_thumbnailScene = new QGraphicsScene(this);
    m_thumbnailView = new QGraphicsView(m_thumbnailScene, m_previewDialog);
    m_thumbnailView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_thumbnailView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    previewLayout->addWidget(m_thumbnailView);
    // show as a non-modal tool window
    m_previewDialog->setWindowFlags(Qt::Tool);
    m_previewDialog->show();`;

cppCode = cppCode.replace(searchList, replaceList);

// Add button
let searchBtn = `contactsBtnLayout->addWidget(m_btnReset);
    contactsBtnLayout->addStretch(1);`;

let replaceBtn = `contactsBtnLayout->addWidget(m_btnReset);

    m_btnTogglePreview = new QPushButton("QSL Card/Envelope", this);
    m_btnTogglePreview->setSizePolicy(sp);
    contactsBtnLayout->addWidget(m_btnTogglePreview);

    connect(m_btnTogglePreview, &QPushButton::clicked, this, [this]() {
        m_previewingEnvelope = !m_previewingEnvelope;
        if (m_previewingEnvelope && m_envTemplateList->currentItem()) {
            onTemplateChosen(m_envTemplateList->currentItem()->text());
        } else if (!m_previewingEnvelope && m_templateList->currentItem()) {
            onTemplateChosen(m_templateList->currentItem()->text());
        }
    });

    contactsBtnLayout->addStretch(1);`;

cppCode = cppCode.replace(searchBtn, replaceBtn);

fs.writeFileSync(cppPath, cppCode);
