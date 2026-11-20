#include "cabrilloconverterdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QSettings>
#include <QSplitter>
#include <QMap>
#include <QTextBrowser>
#include <QSet>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QPushButton>
#include <algorithm>

CabrilloConverterDialog::CabrilloConverterDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("ADIF to Cabrillo Converter"));
    resize(1000, 700);
    
    setupUI();
    loadSettings();
}

void CabrilloConverterDialog::setupUI() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    // Left Column: Header Fields Sidebar (within a scroll area or nice layouts)
    QWidget *leftSidebar = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftSidebar);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);

    QGroupBox *headerGroup = new QGroupBox(tr("Cabrillo Header Fields"), leftSidebar);
    QFormLayout *formLayout = new QFormLayout(headerGroup);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->setLabelAlignment(Qt::AlignLeft);
    formLayout->setContentsMargins(10, 12, 10, 10);
    formLayout->setSpacing(8);

    m_editContest = new QLineEdit(headerGroup);
    m_editLocation = new QLineEdit(headerGroup);
    m_editClaimedScore = new QLineEdit(headerGroup);
    m_btnShowScore = new QPushButton(tr("Show Score"), headerGroup);
    m_btnShowScore->setAutoDefault(false);
    m_btnShowScore->setStyleSheet("padding: 2px 8px; font-weight: bold;");
    connect(m_btnShowScore, &QPushButton::clicked, this, &CabrilloConverterDialog::onShowScoreClicked);

    m_editOperators = new QLineEdit(headerGroup);
    m_editName = new QLineEdit(headerGroup);
    m_editAddress = new QLineEdit(headerGroup);
    m_editAddressCity = new QLineEdit(headerGroup);
    m_editAddressStateProvince = new QLineEdit(headerGroup);
    m_editAddressPostalCode = new QLineEdit(headerGroup);
    m_editAddressCountry = new QLineEdit(headerGroup);

    formLayout->addRow(tr("Contest:"), m_editContest);
    formLayout->addRow(tr("Location / Section:"), m_editLocation);

    QHBoxLayout *scoreLayout = new QHBoxLayout();
    scoreLayout->addWidget(m_editClaimedScore);
    scoreLayout->addWidget(m_btnShowScore);
    formLayout->addRow(tr("Claimed Score:"), scoreLayout);

    formLayout->addRow(tr("Operators (Callsign):"), m_editOperators);
    formLayout->addRow(tr("Name:"), m_editName);
    formLayout->addRow(tr("Address:"), m_editAddress);
    formLayout->addRow(tr("City:"), m_editAddressCity);
    formLayout->addRow(tr("State/Province:"), m_editAddressStateProvince);
    formLayout->addRow(tr("Postal Code:"), m_editAddressPostalCode);
    formLayout->addRow(tr("Country:"), m_editAddressCountry);

    QGroupBox *categoryGroup = new QGroupBox(tr("Category Configuration"), leftSidebar);
    QFormLayout *catForm = new QFormLayout(categoryGroup);
    catForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    catForm->setContentsMargins(10, 12, 10, 10);
    catForm->setSpacing(8);

    m_comboCategoryOperator = new QComboBox(categoryGroup);
    m_comboCategoryOperator->addItems({"SINGLE-OP", "MULTI-OP"});

    m_comboCategoryAssisted = new QComboBox(categoryGroup);
    m_comboCategoryAssisted->addItems({"NON-ASSISTED", "ASSISTED"});

    m_comboCategoryBand = new QComboBox(categoryGroup);
    m_comboCategoryBand->addItems({"ALL", "160M", "80M", "40M", "20M", "15M", "10M"});

    m_comboCategoryMode = new QComboBox(categoryGroup);
    m_comboCategoryMode->addItems({"CW", "SSB", "MIXED", "DATA", "RTTY"});

    m_comboCategoryPower = new QComboBox(categoryGroup);
    m_comboCategoryPower->addItems({"LOW", "HIGH", "QRP"});

    m_comboCategoryStation = new QComboBox(categoryGroup);
    m_comboCategoryStation->addItems({"FIXED", "MOBILE", "PORTABLE"});

    m_comboCategoryTransmitter = new QComboBox(categoryGroup);
    m_comboCategoryTransmitter->addItems({"ONE", "TWO", "MANY"});

    catForm->addRow(tr("Operator:"), m_comboCategoryOperator);
    catForm->addRow(tr("Assisted:"), m_comboCategoryAssisted);
    catForm->addRow(tr("Band:"), m_comboCategoryBand);
    catForm->addRow(tr("Mode:"), m_comboCategoryMode);
    catForm->addRow(tr("Power:"), m_comboCategoryPower);
    catForm->addRow(tr("Station:"), m_comboCategoryStation);
    catForm->addRow(tr("Transmitter:"), m_comboCategoryTransmitter);

    leftLayout->addWidget(headerGroup);
    leftLayout->addWidget(categoryGroup);

    // Dialogue actions (OK, Close) on bottom-left
    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_btnConvert = new QPushButton(tr("OK (Generate)"), leftSidebar);
    m_btnConvert->setDefault(true);
    m_btnConvert->setStyleSheet("background-color: #3b82f6; color: white; font-weight: bold; padding: 6px 12px; border-radius: 4px;");
    connect(m_btnConvert, &QPushButton::clicked, this, &CabrilloConverterDialog::onConvertClicked);

    QPushButton *btnCancel = new QPushButton(tr("Cancel"), leftSidebar);
    btnCancel->setStyleSheet("padding: 6px 12px; border-radius: 4px;");
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    actionLayout->addWidget(m_btnConvert);
    actionLayout->addWidget(btnCancel);
    leftLayout->addLayout(actionLayout);
    leftLayout->addStretch();

    // Right Column: Splitter with input ADIF, output Cabrillo, and layout template controls
    QSplitter *rightSplitter = new QSplitter(Qt::Vertical, this);

    // 1. Input ADIF Pane
    QWidget *inputContainer = new QWidget(this);
    QVBoxLayout *inputLayout = new QVBoxLayout(inputContainer);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    
    QHBoxLayout *inputHeader = new QHBoxLayout();
    QLabel *inputLabel = new QLabel(tr("<b>Paste ADIF Log Data Here Or Import ADIF File</b>"), inputContainer);
    QPushButton *btnBrowse = new QPushButton(tr("Browse..."), inputContainer);
    btnBrowse->setAutoDefault(false);
    inputHeader->addWidget(inputLabel);
    inputHeader->addStretch();
    inputHeader->addWidget(btnBrowse);
    inputLayout->addLayout(inputHeader);

    m_textAdifInput = new QTextEdit(inputContainer);
    m_textAdifInput->setPlaceholderText(tr("<record:1> ... <eor>"));
    inputLayout->addWidget(m_textAdifInput);

    connect(btnBrowse, &QPushButton::clicked, this, [this]() {
        QSettings settings(QCoreApplication::organizationName(), "Qwain");
        settings.beginGroup("Files");
        QString defUserDir = settings.value("userDirectory", QDir::homePath()).toString();
        settings.endGroup();

        QString file = QFileDialog::getOpenFileName(this, tr("Import ADIF File"), defUserDir, tr("ADIF Files (*.adi *.adif);;Log Files (*.txt *.log);;All Files (*)"));
        if (!file.isEmpty()) {
            QFile f(file);
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&f);
                m_textAdifInput->setPlainText(in.readAll());
                f.close();
            } else {
                QMessageBox::warning(this, tr("Error"), tr("Could not open file for reading: %1").arg(file));
            }
        }
    });

    rightSplitter->addWidget(inputContainer);

    // 2. Custom Template Layout Pane
    QWidget *templateContainer = new QWidget(this);
    QVBoxLayout *templateLayout = new QVBoxLayout(templateContainer);
    templateLayout->setContentsMargins(0, 0, 0, 0);
    templateLayout->setSpacing(4);

    QHBoxLayout *templateHeader = new QHBoxLayout();
    m_chkUseCustomTemplate = new QCheckBox(tr("Use Custom Contact Template Layout"), templateContainer);
    m_chkUseCustomTemplate->setChecked(false);
    connect(m_chkUseCustomTemplate, &QCheckBox::toggled, this, &CabrilloConverterDialog::onUseCustomTemplateToggled);
    templateHeader->addWidget(m_chkUseCustomTemplate);
    templateHeader->addStretch();

    QLabel *helpLabel = new QLabel(tr("<span style='color: #6b7280; font-size: 11px;'>Tags: {freq} {mode} {date} {time} {my_call} {my_rst} {my_exch} {dx_call} {dx_rst} {dx_exch}</span>"), templateContainer);
    templateHeader->addWidget(helpLabel);
    templateLayout->addLayout(templateHeader);

    m_textTemplateLayout = new QTextEdit(templateContainer);
    m_textTemplateLayout->setPlainText("QSO: {freq} {mode} {date} {time} {my_call} {my_rst} {my_exch} {dx_call} {dx_rst} {dx_exch}");
    m_textTemplateLayout->setEnabled(false);
    m_textTemplateLayout->setMaximumHeight(65);
    templateLayout->addWidget(m_textTemplateLayout);
    rightSplitter->addWidget(templateContainer);

    // 3. Converted Cabrillo Output Pane
    QWidget *outputContainer = new QWidget(this);
    QVBoxLayout *outputLayout = new QVBoxLayout(outputContainer);
    outputLayout->setContentsMargins(0, 0, 0, 0);
    
    QHBoxLayout *outputHeader = new QHBoxLayout();
    QLabel *outputLabel = new QLabel(tr("<b>Converted Cabrillo Log:</b>"), outputContainer);
    
    m_btnCopyToClipboard = new QPushButton(tr("Copy to Clipboard"), outputContainer);
    m_btnCopyToClipboard->setEnabled(false);
    connect(m_btnCopyToClipboard, &QPushButton::clicked, this, &CabrilloConverterDialog::onCopyToClipboardClicked);

    outputHeader->addWidget(outputLabel);
    outputHeader->addStretch();
    outputHeader->addWidget(m_btnCopyToClipboard);
    outputLayout->addLayout(outputHeader);

    m_textCabrilloOutput = new QTextEdit(outputContainer);
    m_textCabrilloOutput->setReadOnly(true);
    m_textCabrilloOutput->setFontFamily("Courier New");
    outputLayout->addWidget(m_textCabrilloOutput);
    rightSplitter->addWidget(outputContainer);

    // Dynamic Claimed Score updater based on ADIF Input Text Changed
    connect(m_textAdifInput, &QTextEdit::textChanged, this, &CabrilloConverterDialog::onAdifInputChanged);

    // Set initial split ratios
    rightSplitter->setSizes({220, 90, 270});

    mainLayout->addWidget(leftSidebar, 1);
    mainLayout->addWidget(rightSplitter, 2);
}

void CabrilloConverterDialog::onStateChanged(const QString &text) {
    m_editLocation->setText(text.trimmed().toUpper());
}

void CabrilloConverterDialog::onUseCustomTemplateToggled(bool checked) {
    m_textTemplateLayout->setEnabled(checked);
}

void CabrilloConverterDialog::onCopyToClipboardClicked() {
    QApplication::clipboard()->setText(m_textCabrilloOutput->toPlainText());
    QMessageBox::information(this, tr("Copied"), tr("Cabrillo log copied to clipboard!"));
}

struct AdifRecord {
    QMap<QString, QString> fields;
};

// Simple, robust ADIF Record scanner/parser
static QList<AdifRecord> parseAdifLog(const QString &adifText) {
    QList<AdifRecord> records;
    int pos = 0;
    AdifRecord currentRecord;
    
    while (pos < adifText.length()) {
        int tagStart = adifText.indexOf('<', pos);
        if (tagStart == -1) break;
        int tagEnd = adifText.indexOf('>', tagStart);
        if (tagEnd == -1) break;
        
        QString tagContent = adifText.mid(tagStart + 1, tagEnd - tagStart - 1).trimmed();
        pos = tagEnd + 1;
        
        QStringList parts = tagContent.split(':');
        if (parts.isEmpty()) continue;
        
        QString tagName = parts[0].toUpper();
        if (tagName == "EOR") {
            if (!currentRecord.fields.isEmpty()) {
                records.append(currentRecord);
                currentRecord = AdifRecord();
            }
            continue;
        }
        
        if (parts.size() >= 2) {
            bool ok;
            int len = parts[1].toInt(&ok);
            if (ok && len > 0) {
                if (pos + len <= adifText.length()) {
                    QString val = adifText.mid(pos, len);
                    currentRecord.fields[tagName] = val;
                    pos += len;
                }
            }
        }
    }
    // Handle files that don't terminate cleanly with an EOR tag
    if (!currentRecord.fields.isEmpty()) {
        records.append(currentRecord);
    }
    return records;
}

static QString mapAdifModeToCabrillo(QString mode) {
    mode = mode.toUpper().trimmed();
    if (mode == "CW") return "CW";
    if (mode == "SSB" || mode == "AM" || mode == "FM" || mode == "LSB" || mode == "USB") return "PH";
    if (mode == "RTTY") return "RY";
    if (mode == "FT8" || mode == "FT4" || mode == "PSK" || mode == "PSK31" || mode == "DATA" || mode == "MFSK") return "DG";
    return "PH"; // fallback standard phone mode
}

static QString formatFreqToCabrillo(const QString &freqStr, const QString &bandStr) {
    QString f = freqStr.trimmed();
    if (f.isEmpty()) {
        QString b = bandStr.toUpper().trimmed();
        if (b == "160M") return "1800";
        if (b == "80M") return "3500";
        if (b == "40M") return "7000";
        if (b == "20M") return "14000";
        if (b == "15M") return "21000";
        if (b == "10M") return "28000";
        return "21000"; // fallback default
    }
    
    // Check if it's already in kHz or contains a period
    if (f.contains('.')) {
        bool ok;
        double mhz = f.toDouble(&ok);
        if (ok) {
            return QString::number(qRound(mhz * 1000.0));
        }
    }
    return f;
}

static QString mapTnCounty(const QString &countyName) {
    QString c = countyName.trimmed().toUpper();
    // Strip "TN," prefix if present (e.g., "TN,LOUDON" or "TN, UNION")
    if (c.startsWith("TN,")) {
        c = c.mid(3).trimmed();
    }
    
    static QMap<QString, QString> tnMap;
    if (tnMap.isEmpty()) {
        tnMap["ANDERSON"] = "ANDE";
        tnMap["BEDFORD"] = "BEDF";
        tnMap["BENTON"] = "BENT";
        tnMap["BLEDSOE"] = "BLED";
        tnMap["BLOUNT"] = "BLOU";
        tnMap["BRADLEY"] = "BRAD";
        tnMap["CAMPBELL"] = "CAMP";
        tnMap["CANNON"] = "CANN";
        tnMap["CARROLL"] = "CARR";
        tnMap["CARTER"] = "CART";
        tnMap["CHEATHAM"] = "CHEA";
        tnMap["CHESTER"] = "CHES";
        tnMap["CLAIBORNE"] = "CLAI";
        tnMap["CLAY"] = "CLAY";
        tnMap["COCKE"] = "COCK";
        tnMap["COFFEE"] = "COFF";
        tnMap["CROCKETT"] = "CROC";
        tnMap["CUMBERLAND"] = "CUMB";
        tnMap["DAVIDSON"] = "DAVI";
        tnMap["DECATUR"] = "DECA";
        tnMap["DEKALB"] = "DEKA";
        tnMap["DICKSON"] = "DICK";
        tnMap["DYER"] = "DYER";
        tnMap["FAYETTE"] = "FAYE";
        tnMap["FENTRESS"] = "FENT";
        tnMap["FRANKLIN"] = "FRAN";
        tnMap["GIBSON"] = "GIBS";
        tnMap["GILES"] = "GILE";
        tnMap["GRAINGER"] = "GRAI";
        tnMap["GREENE"] = "GREE";
        tnMap["GRUNDY"] = "GRUN";
        tnMap["HAMBLEN"] = "HAMB";
        tnMap["HAMILTON"] = "HAMI";
        tnMap["HANCOCK"] = "HANC";
        tnMap["HARDEMAN"] = "HARD";
        tnMap["HARDIN"] = "HARN";
        tnMap["HAWKINS"] = "HAWK";
        tnMap["HAYWOOD"] = "HAYW";
        tnMap["HENDERSON"] = "HEND";
        tnMap["HENRY"] = "HENR";
        tnMap["HICKMAN"] = "HICK";
        tnMap["HOUSTON"] = "HOUS";
        tnMap["HUMPHREYS"] = "HUMP";
        tnMap["JACKSON"] = "JACK";
        tnMap["JEFFERSON"] = "JEFF";
        tnMap["JOHNSON"] = "JOHN";
        tnMap["KNOX"] = "KNOX";
        tnMap["LAKE"] = "LAKE";
        tnMap["LAUDERDALE"] = "LAUD";
        tnMap["LAWRENCE"] = "LAWR";
        tnMap["LEWIS"] = "LEWI";
        tnMap["LINCOLN"] = "LINC";
        tnMap["LOUDON"] = "LOUD";
        tnMap["MACON"] = "MACO";
        tnMap["MADISON"] = "MADI";
        tnMap["MARION"] = "MARI";
        tnMap["MARSHALL"] = "MARS";
        tnMap["MAURY"] = "MAUR";
        tnMap["MCMINN"] = "MCMI";
        tnMap["MCNAIRY"] = "MCNA";
        tnMap["MEIGS"] = "MEIG";
        tnMap["MONROE"] = "MONR";
        tnMap["MONTGOMERY"] = "MONT";
        tnMap["MOORE"] = "MOOR";
        tnMap["MORGAN"] = "MORG";
        tnMap["OBION"] = "OBIO";
        tnMap["OVERTON"] = "OVER";
        tnMap["PERRY"] = "PERR";
        tnMap["PICKETT"] = "PICK";
        tnMap["POLK"] = "POLK";
        tnMap["PUTNAM"] = "PUTN";
        tnMap["RHEA"] = "RHEA";
        tnMap["ROANE"] = "ROAN";
        tnMap["ROBERTSON"] = "ROBE";
        tnMap["RUTHERFORD"] = "RUTH";
        tnMap["SCOTT"] = "SCOT";
        tnMap["SEQUATCHIE"] = "SEQU";
        tnMap["SEVIER"] = "SEVI";
        tnMap["SHELBY"] = "SHEL";
        tnMap["SMITH"] = "SMIT";
        tnMap["STEWART"] = "STEW";
        tnMap["SULLIVAN"] = "SULL";
        tnMap["SUMNER"] = "SUMN";
        tnMap["TIPTON"] = "TIPT";
        tnMap["TROUSDALE"] = "TROU";
        tnMap["UNICOI"] = "UNIC";
        tnMap["UNION"] = "UNIO";
        tnMap["VAN BUREN"] = "VANB";
        tnMap["WARREN"] = "WARR";
        tnMap["WASHINGTON"] = "WASH";
        tnMap["WAYNE"] = "WAYN";
        tnMap["WEAKLEY"] = "WEAK";
        tnMap["WHITE"] = "WHIT";
        tnMap["WILLIAMSON"] = "WILL";
        tnMap["WILSON"] = "WILS";
    }
    
    if (tnMap.contains(c)) {
        return tnMap.value(c);
    }
    return c.isEmpty() ? countyName : c; // fallback to stripped version if not matched, or original
}

struct ScoreResult {
    int finalScore;
    int validCount;
    int duplicateCount;
    int repeaterCount;
    int totalMultipliers;
    int totalBonusPoints;
    int tnCounties;
    int states;
    int provinces;
    int dxcc;
    int tcgBonus;
    int roverBonus;
};

static ScoreResult computeContestScore(const QList<AdifRecord> &records,
                                       const QString &entrantLoc,
                                       const QString &stationCat,
                                       const QString &myOperators);

static QString getCanadianFourLetterProv(const QString &provStr);

QString CabrilloConverterDialog::performConversion() {
    // Process QSO Records first to compute score
    QString adif = m_textAdifInput->toPlainText();
    QList<AdifRecord> records = parseAdifLog(adif);

    ScoreResult scoreRes = computeContestScore(records,
                                               m_editLocation->text(),
                                               m_comboCategoryStation->currentText(),
                                               m_editOperators->text());

    QString out;
    out += "START-OF-LOG: 3.0\n";
    out += "CONTEST: " + m_editContest->text().trimmed().toUpper() + "\n";
    out += "LOCATION: " + m_editLocation->text().trimmed().toUpper() + "\n";
    out += "CALLSIGN: " + m_editOperators->text().trimmed().toUpper() + "\n";
    out += "NAME: " + m_editName->text().trimmed() + "\n";
    out += "ADDRESS: " + m_editAddress->text().trimmed() + "\n";
    out += "ADDRESS-CITY: " + m_editAddressCity->text().trimmed() + "\n";
    if (!m_editAddressStateProvince->text().trimmed().isEmpty()) {
        out += "ADDRESS-STATE-PROVINCE: " + m_editAddressStateProvince->text().trimmed().toUpper() + "\n";
    }
    if (!m_editAddressPostalCode->text().trimmed().isEmpty()) {
        out += "ADDRESS-POSTALCODE: " + m_editAddressPostalCode->text().trimmed() + "\n";
    }
    out += "ADDRESS-COUNTRY: " + m_editAddressCountry->text().trimmed().toUpper() + "\n";
    
    out += "CATEGORY-OPERATOR: " + m_comboCategoryOperator->currentText() + "\n";
    out += "CATEGORY-ASSISTED: " + m_comboCategoryAssisted->currentText() + "\n";
    out += "CATEGORY-BAND: " + m_comboCategoryBand->currentText() + "\n";
    out += "CATEGORY-MODE: " + m_comboCategoryMode->currentText() + "\n";
    out += "CATEGORY-POWER: " + m_comboCategoryPower->currentText() + "\n";
    out += "CATEGORY-STATION: " + m_comboCategoryStation->currentText() + "\n";
    out += "CATEGORY-TRANSMITTER: " + m_comboCategoryTransmitter->currentText() + "\n";
    
    QString scoreStr = m_editClaimedScore->text().trimmed();
    if (!records.isEmpty()) {
        scoreStr = QString::number(scoreRes.finalScore);
        m_editClaimedScore->setText(scoreStr);
    }
    out += "CLAIMED-SCORE: " + scoreStr + "\n";
    out += "CREATED-BY: Qwain ADIF-to-Cabrillo Converter\n";
    
    bool useCustom = m_chkUseCustomTemplate->isChecked();
    QString templateStr = m_textTemplateLayout->toPlainText();
    
    struct ColumnDef {
        QString key;
        int width;
        bool leftAlign;
    };
    QList<ColumnDef> cols;
    QStringList staticLines;
    QString keywordLine;
    QString maskLine;
    bool hasBraces = false;

    if (useCustom) {
        hasBraces = templateStr.contains("{") && templateStr.contains("}");
        if (!hasBraces) {
            // Find all parts starting with "QSO:" (handles single-line pasted templates beautifully)
            QStringList qsoParts;
            int searchPos = 0;
            while (true) {
                int idx = templateStr.indexOf("QSO:", searchPos, Qt::CaseInsensitive);
                if (idx == -1) break;
                // Find the next "QSO:" or end of string
                int nextIdx = templateStr.indexOf("QSO:", idx + 4, Qt::CaseInsensitive);
                QString part;
                if (nextIdx == -1) {
                    part = templateStr.mid(idx).trimmed();
                    searchPos = templateStr.length();
                } else {
                    part = templateStr.mid(idx, nextIdx - idx).trimmed();
                    searchPos = nextIdx;
                }
                qsoParts.append(part);
            }

            // Assign keyword and mask lines from qsoParts
            for (const QString &part : qsoParts) {
                if (part.contains("yyyy", Qt::CaseInsensitive) || part.contains("****") || part.contains("nnnn") || part.contains("aaaa")) {
                    maskLine = part;
                } else {
                    keywordLine = part;
                }
            }

            // Fallback: If we only got one QSO line and no explicit mask line, treat it as the keyword line
            if (keywordLine.isEmpty() && qsoParts.size() == 1) {
                keywordLine = qsoParts[0];
            }

            // Remove static non-QSO lines from templateStr to print them once
            QStringList tLines = templateStr.split('\n');
            for (const QString &line : tLines) {
                QString trimmed = line.trimmed();
                if (!trimmed.isEmpty() && !trimmed.startsWith("QSO:", Qt::CaseInsensitive)) {
                    staticLines.append(trimmed);
                }
            }

            // Parse columns from keywordLine and maskLine
            QString kwBody = keywordLine;
            if (kwBody.startsWith("QSO:", Qt::CaseInsensitive)) {
                kwBody = kwBody.mid(4).trimmed();
            }
            QString maskBody = maskLine;
            if (maskBody.startsWith("QSO:", Qt::CaseInsensitive)) {
                maskBody = maskBody.mid(4).trimmed();
            }

            QStringList kwTokens = kwBody.split(' ', Qt::SkipEmptyParts);
            QStringList maskTokens = maskBody.split(' ', Qt::SkipEmptyParts);

            int callCount = 0;
            int rstCount = 0;
            int exchCount = 0;

            for (int i = 0; i < kwTokens.size(); ++i) {
                QString kw = kwTokens[i].toLower();
                int width = 6;
                if (i < maskTokens.size()) {
                    width = maskTokens[i].length();
                } else {
                    if (kw.contains("freq")) width = 5;
                    else if (kw == "mo" || kw == "mode") width = 2;
                    else if (kw == "date") width = 10;
                    else if (kw == "time") width = 4;
                    else if (kw == "call") width = 13;
                    else if (kw == "rst") width = 3;
                    else if (kw == "qth" || kw == "exch" || kw == "loc") width = 6;
                }

                ColumnDef col;
                col.width = width;
                col.leftAlign = true;

                if (kw.contains("freq")) {
                    col.key = "freq";
                    col.leftAlign = false;
                } else if (kw == "mo" || kw == "mode") {
                    col.key = "mode";
                } else if (kw == "date") {
                    col.key = "date";
                } else if (kw == "time") {
                    col.key = "time";
                } else if (kw == "call") {
                    callCount++;
                    if (callCount == 1) col.key = "my_call";
                    else col.key = "dx_call";
                } else if (kw == "rst") {
                    rstCount++;
                    col.leftAlign = false;
                    if (rstCount == 1) col.key = "my_rst";
                    else col.key = "dx_rst";
                } else if (kw == "qth" || kw == "exch" || kw == "loc" || kw == "state" || kw == "sec" || kw == "cnty" || kw == "prov") {
                    exchCount++;
                    if (exchCount == 1) col.key = "my_exch";
                    else col.key = "dx_exch";
                } else {
                    col.key = "unknown";
                }
                cols.append(col);
            }
        }
    }

    QString myCall = m_editOperators->text().trimmed().toUpper();
    if (myCall.isEmpty()) myCall = "N0CALL";
    
    QString myExch = m_editLocation->text().trimmed().toUpper();
    if (myExch.isEmpty()) myExch = "DX";

    for (const AdifRecord &rec : records) {
        QString call = rec.fields.value("CALL").trimmed().toUpper();
        if (call.isEmpty()) continue; // skip entries with no callsign
        
        QString freq = formatFreqToCabrillo(rec.fields.value("FREQ"), rec.fields.value("BAND"));
        QString mode = mapAdifModeToCabrillo(rec.fields.value("MODE"));
        
        // Date parsing
        QString rawDate = rec.fields.value("QSO_DATE").trimmed();
        QString formattedDate;
        if (rawDate.length() == 8) {
            formattedDate = rawDate.mid(0, 4) + "-" + rawDate.mid(4, 2) + "-" + rawDate.mid(6, 2);
        } else {
            formattedDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        }
        
        // Time parsing
        QString rawTime = rec.fields.value("TIME_ON").trimmed();
        QString formattedTime;
        if (rawTime.length() >= 4) {
            formattedTime = rawTime.mid(0, 4);
        } else {
            formattedTime = "0000";
        }
        
        // Default exchange extraction logic
        QString dxState = rec.fields.value("STATE").trimmed().toUpper();
        QString dxCnty = rec.fields.value("CNTY").trimmed().toUpper();
        QString dxExch = rec.fields.value("SRX_STRING").trimmed().toUpper();
        if (dxExch.isEmpty()) dxExch = rec.fields.value("SRX").trimmed().toUpper();
        if (dxExch.isEmpty()) dxExch = dxState;
        if (dxExch.isEmpty()) dxExch = rec.fields.value("PROV").trimmed().toUpper();
        
        // Determine if it is a Canadian contact and map appropriate Canadian province
        bool isCanada = (rec.fields.value("DXCC").trimmed() == "197" || 
                         rec.fields.value("COUNTRY").trimmed().toUpper() == "CANADA");
        
        QString canadaProv4;
        if (!dxState.isEmpty()) canadaProv4 = getCanadianFourLetterProv(dxState);
        if (canadaProv4.isEmpty() && !rec.fields.value("PROV").isEmpty()) {
            canadaProv4 = getCanadianFourLetterProv(rec.fields.value("PROV"));
        }
        if (canadaProv4.isEmpty() && !dxExch.isEmpty()) {
            canadaProv4 = getCanadianFourLetterProv(dxExch);
        }
        
        if (isCanada || !canadaProv4.isEmpty()) {
            if (!canadaProv4.isEmpty()) {
                dxExch = canadaProv4;
            } else {
                // Infer province based on callsign prefix
                QString c = call.toUpper();
                if (c.startsWith("VE3") || c.startsWith("VA3")) canadaProv4 = "ONTA";
                else if (c.startsWith("VE2") || c.startsWith("VA2")) canadaProv4 = "QUEB";
                else if (c.startsWith("VE6") || c.startsWith("VA6")) canadaProv4 = "ALBE";
                else if (c.startsWith("VE7") || c.startsWith("VA7")) canadaProv4 = "BRIT";
                else if (c.startsWith("VE4") || c.startsWith("VA4")) canadaProv4 = "MANI";
                else if (c.startsWith("VE5") || c.startsWith("VA5")) canadaProv4 = "SASK";
                else if (c.startsWith("VE1") || c.startsWith("VA1")) canadaProv4 = "NOVA";
                else if (c.startsWith("VE9")) canadaProv4 = "NEWB";
                else if (c.startsWith("VO1") || c.startsWith("VO2")) canadaProv4 = "NEWF";
                else if (c.startsWith("VY2")) canadaProv4 = "PRIN";
                else if (c.startsWith("VY1")) canadaProv4 = "YUKO";
                else if (c.startsWith("VY0")) canadaProv4 = "NUNA";
                else if (c.startsWith("VE8")) canadaProv4 = "NORT";
                else canadaProv4 = "ONTA"; // standard fallback
                
                dxExch = canadaProv4;
            }
        } else if (dxState == "TN" || dxExch == "TN" || dxExch.startsWith("TN,")) {
            // If the contact state or exchange is TN, substitute with mapped county if available
            QString countyToMap = dxCnty;
            if (countyToMap.isEmpty()) {
                countyToMap = dxExch;
            }
            if (!countyToMap.isEmpty()) {
                QString mappedCounty = mapTnCounty(countyToMap);
                if (!mappedCounty.isEmpty()) {
                    dxExch = mappedCounty;
                }
            }
        }
        if (dxExch.isEmpty()) dxExch = "599"; // default standard fallback

        // Similarly, handle operator exchange mapping if operator location/exch is TN
        QString currentMyExch = myExch;
        if (currentMyExch == "TN" || currentMyExch.startsWith("TN,")) {
            QString myCnty = rec.fields.value("MY_CNTY").trimmed().toUpper();
            if (myCnty.isEmpty()) {
                myCnty = currentMyExch;
            }
            if (!myCnty.isEmpty()) {
                QString mappedMyCnty = mapTnCounty(myCnty);
                if (!mappedMyCnty.isEmpty()) {
                    currentMyExch = mappedMyCnty;
                }
            }
        }

        QString rstSent = rec.fields.value("RST_SENT").trimmed();
        if (rstSent.isEmpty()) rstSent = (mode == "PH" ? "59" : "599");
        
        QString rstRcvd = rec.fields.value("RST_RCVD").trimmed();
        if (rstRcvd.isEmpty()) rstRcvd = (mode == "PH" ? "59" : "599");
        
        if (!useCustom) {
            // Apply perfect standard Cabrillo column widths with padding for aesthetics
            QString formattedLine = QString("QSO: %1 %2 %3 %4 %5 %6 %7 %8 %9 %10")
                .arg(freq.rightJustified(5, ' '))
                .arg(mode.leftJustified(2, ' '))
                .arg(formattedDate)
                .arg(formattedTime)
                .arg(myCall.leftJustified(13, ' '))
                .arg(rstSent.rightJustified(3, ' '))
                .arg(currentMyExch.leftJustified(6, ' '))
                .arg(call.leftJustified(13, ' '))
                .arg(rstRcvd.rightJustified(3, ' '))
                .arg(dxExch.leftJustified(6, ' '));
            out += formattedLine + "\n";
        } else {
            if (hasBraces) {
                QString qsoLine = templateStr;
                qsoLine.replace("{freq}", freq);
                qsoLine.replace("{mode}", mode);
                qsoLine.replace("{date}", formattedDate);
                qsoLine.replace("{time}", formattedTime);
                qsoLine.replace("{my_call}", myCall);
                qsoLine.replace("{my_rst}", rstSent);
                qsoLine.replace("{my_exch}", currentMyExch);
                qsoLine.replace("{dx_call}", call);
                qsoLine.replace("{dx_rst}", rstRcvd);
                qsoLine.replace("{dx_exch}", dxExch);
                out += qsoLine + "\n";
            } else {
                if (cols.isEmpty()) {
                    // Fallback to standard perfect Cabrillo columns if no columns were parsed
                    QString formattedLine = QString("QSO: %1 %2 %3 %4 %5 %6 %7 %8 %9 %10")
                        .arg(freq.rightJustified(5, ' '))
                        .arg(mode.leftJustified(2, ' '))
                        .arg(formattedDate)
                        .arg(formattedTime)
                        .arg(myCall.leftJustified(13, ' '))
                        .arg(rstSent.rightJustified(3, ' '))
                        .arg(currentMyExch.leftJustified(6, ' '))
                        .arg(call.leftJustified(13, ' '))
                        .arg(rstRcvd.rightJustified(3, ' '))
                        .arg(dxExch.leftJustified(6, ' '));
                    out += formattedLine + "\n";
                } else {
                    // Formatting based on keyword & mask columns
                    QString formattedLine = "QSO: ";
                    for (int i = 0; i < cols.size(); ++i) {
                        const ColumnDef &col = cols[i];
                        QString val;
                        if (col.key == "freq") val = freq;
                        else if (col.key == "mode") val = mode;
                        else if (col.key == "date") val = formattedDate;
                        else if (col.key == "time") val = formattedTime;
                        else if (col.key == "my_call") val = myCall;
                        else if (col.key == "my_rst") val = rstSent;
                        else if (col.key == "my_exch") val = currentMyExch;
                        else if (col.key == "dx_call") val = call;
                        else if (col.key == "dx_rst") val = rstRcvd;
                        else if (col.key == "dx_exch") val = dxExch;
                        else val = "";

                        if (col.leftAlign) {
                            formattedLine += val.leftJustified(col.width, ' ');
                        } else {
                            formattedLine += val.rightJustified(col.width, ' ');
                        }
                        if (i < cols.size() - 1) {
                            formattedLine += " ";
                        }
                    }
                    out += formattedLine + "\n";
                }
            }
        }
    }
    
    out += "END-OF-LOG:\n";
    return out;
}

void CabrilloConverterDialog::onConvertClicked() {
    QString output = performConversion();
    m_textCabrilloOutput->setPlainText(output);
    m_btnCopyToClipboard->setEnabled(!output.isEmpty());
    saveSettings();
}


void CabrilloConverterDialog::loadSettings() {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("CabrilloConverter");
    
    m_editContest->setText(settings.value("Contest", settings.value("StateProvince", "TN").toString()).toString());
    m_editAddressStateProvince->setText(settings.value("AddressStateProvince", "TN").toString());
    m_editLocation->setText(settings.value("Location", "TN").toString());
    m_editClaimedScore->setText(settings.value("ClaimedScore", "").toString());
    m_editOperators->setText(settings.value("Operators", "N0CALL").toString());
    m_editName->setText(settings.value("Name", "Joe Operator").toString());
    m_editAddress->setText(settings.value("Address", "123 Radio Lane").toString());
    m_editAddressCity->setText(settings.value("City", "Maynardville").toString());
    m_editAddressPostalCode->setText(settings.value("PostalCode", "37807").toString());
    m_editAddressCountry->setText(settings.value("Country", "USA").toString());
    
    m_comboCategoryOperator->setCurrentText(settings.value("CatOperator", "SINGLE-OP").toString());
    m_comboCategoryAssisted->setCurrentText(settings.value("CatAssisted", "NON-ASSISTED").toString());
    m_comboCategoryBand->setCurrentText(settings.value("CatBand", "ALL").toString());
    m_comboCategoryMode->setCurrentText(settings.value("CatMode", "CW").toString());
    m_comboCategoryPower->setCurrentText(settings.value("CatPower", "LOW").toString());
    m_comboCategoryStation->setCurrentText(settings.value("CatStation", "FIXED").toString());
    m_comboCategoryTransmitter->setCurrentText(settings.value("CatTransmitter", "ONE").toString());

    m_chkUseCustomTemplate->setChecked(settings.value("UseCustomTemplate", false).toBool());
    m_textTemplateLayout->setPlainText(settings.value("CustomTemplate", "QSO: {freq} {mode} {date} {time} {my_call} {my_rst} {my_exch} {dx_call} {dx_rst} {dx_exch}").toString());
    
    settings.endGroup();
}

void CabrilloConverterDialog::saveSettings() {
    QSettings settings(QCoreApplication::organizationName(), "Qwain");
    settings.beginGroup("CabrilloConverter");
    
    settings.setValue("Contest", m_editContest->text());
    settings.setValue("StateProvince", m_editContest->text()); // Legacy key
    settings.setValue("AddressStateProvince", m_editAddressStateProvince->text());
    settings.setValue("Location", m_editLocation->text());
    settings.setValue("ClaimedScore", m_editClaimedScore->text());
    settings.setValue("Operators", m_editOperators->text());
    settings.setValue("Name", m_editName->text());
    settings.setValue("Address", m_editAddress->text());
    settings.setValue("City", m_editAddressCity->text());
    settings.setValue("PostalCode", m_editAddressPostalCode->text());
    settings.setValue("Country", m_editAddressCountry->text());
    
    settings.setValue("CatOperator", m_comboCategoryOperator->currentText());
    settings.setValue("CatAssisted", m_comboCategoryAssisted->currentText());
    settings.setValue("CatBand", m_comboCategoryBand->currentText());
    settings.setValue("CatMode", m_comboCategoryMode->currentText());
    settings.setValue("CatPower", m_comboCategoryPower->currentText());
    settings.setValue("CatStation", m_comboCategoryStation->currentText());
    settings.setValue("CatTransmitter", m_comboCategoryTransmitter->currentText());

    settings.setValue("UseCustomTemplate", m_chkUseCustomTemplate->isChecked());
    settings.setValue("CustomTemplate", m_textTemplateLayout->toPlainText());
    
    settings.endGroup();
}

static QString getModeGroup(const QString &cabrilloMode) {
    QString m = cabrilloMode.toUpper().trimmed();
    if (m == "CW") return "CW";
    if (m == "PH") return "PH";
    return "DG"; // RY, DG, and other digital modes are treated as the same Digital group
}

static QString normalizeBand(const QString &b, const QString &freqStr) {
    QString res = b.trimmed().toUpper();
    if (res.endsWith("M")) return res;
    // Fallback: map frequency in MHz or kHz to band
    bool ok;
    double freq = freqStr.toDouble(&ok);
    if (ok && freq > 0.0) {
        if (freq < 30.0) {
            freq *= 1000.0; // convert MHz to kHz
        }
        if (freq >= 1800 && freq <= 2000) return "160M";
        if (freq >= 3500 && freq <= 4000) return "80M";
        if (freq >= 7000 && freq <= 7300) return "40M";
        if (freq >= 10100 && freq <= 10150) return "30M";
        if (freq >= 14000 && freq <= 14350) return "20M";
        if (freq >= 18068 && freq <= 18168) return "17M";
        if (freq >= 21000 && freq <= 21450) return "15M";
        if (freq >= 24890 && freq <= 24990) return "12M";
        if (freq >= 28000 && freq <= 29700) return "10M";
        if (freq >= 50000 && freq <= 54000) return "6M";
        if (freq >= 144000 && freq <= 148000) return "2M";
    }
    return res.isEmpty() ? "UNKNOWN" : res;
}

static bool isValidUsState(const QString &st) {
    static QSet<QString> states = {
        "AL", "AK", "AZ", "AR", "CA", "CO", "CT", "DE", "FL", "GA", 
        "HI", "ID", "IL", "IN", "IA", "KS", "KY", "LA", "ME", "MD", 
        "MA", "MI", "MN", "MS", "MO", "MT", "NE", "NV", "NH", "NJ", 
        "NM", "NY", "NC", "ND", "OH", "OK", "OR", "PA", "RI", "SC", 
        "SD", "TN", "TX", "UT", "VT", "VA", "WA", "WV", "WI", "WY"
    };
    return states.contains(st);
}

static QString getCanadianProvAbbr(const QString &prov) {
    QString p = prov.trimmed().toUpper();
    static QMap<QString, QString> provs = {
        {"BC", "BC"}, {"BRITISH COLUMBIA", "BC"},
        {"AB", "AB"}, {"ALBERTA", "AB"},
        {"SK", "SK"}, {"SASKATCHEWAN", "SK"},
        {"MB", "MB"}, {"MANITOBA", "MB"},
        {"ON", "ON"}, {"ONTARIO", "ON"},
        {"QC", "QC"}, {"QUEBEC", "QC"}, {"PQ", "QC"},
        {"NB", "NB"}, {"NEW BRUNSWICK", "NB"},
        {"NS", "NS"}, {"NOVA SCOTIA", "NS"},
        {"PE", "PE"}, {"PEI", "PE"}, {"PRINCE EDWARD ISLAND", "PE"},
        {"NL", "NL"}, {"NF", "NL"}, {"LB", "NL"}, {"NEWFOUNDLAND", "NL"}, {"LABRADOR", "NL"},
        {"YT", "YT"}, {"YUKON", "YT"},
        {"NT", "NT"}, {"NWT", "NT"}, {"NORTHWEST TERRITORIES", "NT"},
        {"NU", "NU"}, {"NUNAVUT", "NU"}
    };
    return provs.value(p, "");
}

static QString getCanadianFourLetterProv(const QString &provStr) {
    QString p = provStr.trimmed().toUpper();
    if (p.isEmpty()) return "";
    
    if (p == "BC" || p == "BRITISH COLUMBIA") return "BRIT";
    if (p == "AB" || p == "ALBERTA") return "ALBE";
    if (p == "SK" || p == "SASKATCHEWAN") return "SASK";
    if (p == "MB" || p == "MANITOBA") return "MANI";
    if (p == "ON" || p == "ONTARIO") return "ONTA";
    if (p == "QC" || p == "PQ" || p == "QUEBEC") return "QUEB";
    if (p == "NB" || p == "NEW BRUNSWICK") return "NEWB";
    if (p == "NS" || p == "NOVA SCOTIA") return "NOVA";
    if (p == "PE" || p == "PEI" || p == "PRINCE EDWARD ISLAND") return "PRIN";
    if (p == "NL" || p == "NF" || p == "LB" || p == "NEWFOUNDLAND" || p == "LABRADOR") return "NEWF";
    if (p == "YT" || p == "YUKON") return "YUKO";
    if (p == "NT" || p == "NWT" || p == "NORTHWEST TERRITORIES") return "NORT";
    if (p == "NU" || p == "NUNAVUT") return "NUNA";
    
    if (p.startsWith("BRIT")) return "BRIT";
    if (p.startsWith("ALBE")) return "ALBE";
    if (p.startsWith("SASK")) return "SASK";
    if (p.startsWith("MANI")) return "MANI";
    if (p.startsWith("ONTA")) return "ONTA";
    if (p.startsWith("QUEB")) return "QUEB";
    if (p.startsWith("NEWB")) return "NEWB";
    if (p.startsWith("NOVA")) return "NOVA";
    if (p.startsWith("PRIN")) return "PRIN";
    if (p.startsWith("NEWF")) return "NEWF";
    if (p.startsWith("YUKO")) return "YUKO";
    if (p.startsWith("NORT")) return "NORT";
    if (p.startsWith("NUNA")) return "NUNA";
    
    return "";
}

static QString getWorkedTnCounty(const AdifRecord &rec) {
    static QSet<QString> tnCounties = {
        "ANDE", "BEDF", "BENT", "BLED", "BLOU", "BRAD", "CAMP", "CANN", "CARR", "CART",
        "CHEA", "CHES", "CLAI", "CLAY", "COCK", "COFF", "CROC", "CUMB", "DAVI", "DECA",
        "DEKA", "DICK", "DYER", "FAYE", "FENT", "FRAN", "GIBS", "GILE", "GRAI", "GREE",
        "GRUN", "HAMB", "HAMI", "HANC", "HARD", "HARN", "HAWK", "HAYW", "HEND", "HENR",
        "HICK", "HOUS", "HUMP", "JACK", "JEFF", "JOHN", "KNOX", "LAKE", "LAUD", "LAWR",
        "LEWI", "LINC", "LOUD", "MACO", "MADI", "MARI", "MARS", "MAUR", "MCMI", "MCNA",
        "MEIG", "MONR", "MONT", "MOOR", "MORG", "OBIO", "OVER", "PERR", "PICK", "POLK",
        "PUTN", "RHEA", "ROAN", "ROBE", "RUTH", "SCOT", "SEQU", "SEVI", "SHEL", "SMIT",
        "STEW", "SULL", "SUMN", "TIPT", "TROU", "UNIC", "UNIO", "VANB", "WARR", "WASH",
        "WAYN", "WEAK", "WHIT", "WILL", "WILS"
    };

    QString state = rec.fields.value("STATE").trimmed().toUpper();
    QString srx = rec.fields.value("SRX_STRING").trimmed().toUpper();
    if (srx.isEmpty()) srx = rec.fields.value("SRX").trimmed().toUpper();
    
    QString cnty = rec.fields.value("CNTY").trimmed().toUpper();
    QString qth = rec.fields.value("QTH").trimmed().toUpper();

    if (state == "TN" || srx.startsWith("TN,") || cnty.startsWith("TN,")) {
        if (!cnty.isEmpty()) {
            QString mapped = mapTnCounty(cnty);
            if (tnCounties.contains(mapped)) return mapped;
        }
        if (!srx.isEmpty()) {
            QString mapped = mapTnCounty(srx);
            if (tnCounties.contains(mapped)) return mapped;
        }
        if (!qth.isEmpty()) {
            QString mapped = mapTnCounty(qth);
            if (tnCounties.contains(mapped)) return mapped;
        }
    }
    
    if (!cnty.isEmpty()) {
        QString mapped = mapTnCounty(cnty);
        if (tnCounties.contains(mapped)) return mapped;
    }
    if (!srx.isEmpty()) {
        QString mapped = mapTnCounty(srx);
        if (tnCounties.contains(mapped)) return mapped;
    }
    
    return "";
}

static ScoreResult computeContestScore(const QList<AdifRecord> &records,
                                       const QString &entrantLocStr,
                                       const QString &stationCatStr,
                                       const QString &myOperatorsStr)
{
    ScoreResult res;
    res.finalScore = 0;
    res.validCount = 0;
    res.duplicateCount = 0;
    res.repeaterCount = 0;
    res.totalMultipliers = 0;
    res.totalBonusPoints = 0;
    res.tnCounties = 0;
    res.states = 0;
    res.provinces = 0;
    res.dxcc = 0;
    res.tcgBonus = 0;
    res.roverBonus = 0;

    if (records.isEmpty()) return res;

    bool isTnStation = false;
    static QSet<QString> tnCounties = {
        "ANDE", "BEDF", "BENT", "BLED", "BLOU", "BRAD", "CAMP", "CANN", "CARR", "CART",
        "CHEA", "CHES", "CLAI", "CLAY", "COCK", "COFF", "CROC", "CUMB", "DAVI", "DECA",
        "DEKA", "DICK", "DYER", "FAYE", "FENT", "FRAN", "GIBS", "GILE", "GRAI", "GREE",
        "GRUN", "HAMB", "HAMI", "HANC", "HARD", "HARN", "HAWK", "HAYW", "HEND", "HENR",
        "HICK", "HOUS", "HUMP", "JACK", "JEFF", "JOHN", "KNOX", "LAKE", "LAUD", "LAWR",
        "LEWI", "LINC", "LOUD", "MACO", "MADI", "MARI", "MARS", "MAUR", "MCMI", "MCNA",
        "MEIG", "MONR", "MONT", "MOOR", "MORG", "OBIO", "OVER", "PERR", "PICK", "POLK",
        "PUTN", "RHEA", "ROAN", "ROBE", "RUTH", "SCOT", "SEQU", "SEVI", "SHEL", "SMIT",
        "STEW", "SULL", "SUMN", "TIPT", "TROU", "UNIC", "UNIO", "VANB", "WARR", "WASH",
        "WAYN", "WEAK", "WHIT", "WILL", "WILS"
    };

    QString entrantLoc = entrantLocStr.trimmed().toUpper();
    if (entrantLoc == "TN" || tnCounties.contains(mapTnCounty(entrantLoc))) {
        isTnStation = true;
    }

    bool isMobileOrRover = false;
    QString stationCat = stationCatStr.toUpper();
    if (stationCat == "MOBILE" || stationCat == "PORTABLE" || entrantLoc.contains("ROVER") || entrantLoc.contains("MOBILE")) {
        isMobileOrRover = true;
    }

    QSet<QString> seenQsos; // Key: CALL + ":" + BAND + ":" + MODE_GROUP
    QMap<QString, int> qsoCountByMyCounty;
    
    QMap<QString, QSet<QString>> tnCountiesByBand;
    QMap<QString, QSet<QString>> statesByBand;
    QMap<QString, QSet<QString>> provincesByBand;
    QMap<QString, QSet<QString>> dxccByBand;

    QSet<QString> seenK4TcgBonus;

    for (const AdifRecord &rec : records) {
        QString call = rec.fields.value("CALL").trimmed().toUpper();
        if (call.isEmpty()) continue;

        QString freqStr = rec.fields.value("FREQ");
        QString band = normalizeBand(rec.fields.value("BAND"), freqStr);
        QString mode = mapAdifModeToCabrillo(rec.fields.value("MODE"));
        QString modeGroup = getModeGroup(mode);

        QString dxState = rec.fields.value("STATE").trimmed().toUpper();
        QString dxCnty = rec.fields.value("CNTY").trimmed().toUpper();
        QString dxExch = rec.fields.value("SRX_STRING").trimmed().toUpper();
        if (dxExch.isEmpty()) dxExch = rec.fields.value("SRX").trimmed().toUpper();
        if (dxExch.isEmpty()) dxExch = dxState;
        if (dxExch.isEmpty()) dxExch = rec.fields.value("PROV").trimmed().toUpper();

        if (dxState == "TN" || dxExch == "TN" || dxExch.startsWith("TN,")) {
            QString countyToMap = dxCnty;
            if (countyToMap.isEmpty()) {
                countyToMap = dxExch;
            }
            if (!countyToMap.isEmpty()) {
                QString mappedCounty = mapTnCounty(countyToMap);
                if (!mappedCounty.isEmpty()) {
                    dxExch = mappedCounty;
                }
            }
        }

        QString myExchVal = entrantLoc;
        if (myExchVal.isEmpty()) myExchVal = "DX";
        QString currentMyExch = myExchVal;
        if (currentMyExch == "TN" || currentMyExch.startsWith("TN,")) {
            QString myCnty = rec.fields.value("MY_CNTY").trimmed().toUpper();
            if (myCnty.isEmpty()) {
                myCnty = currentMyExch;
            }
            if (!myCnty.isEmpty()) {
                QString mappedMyCnty = mapTnCounty(myCnty);
                if (!mappedMyCnty.isEmpty()) {
                    currentMyExch = mappedMyCnty;
                }
            }
        }

        bool isRepeater = false;
        if (rec.fields.value("PROP_MODE").trimmed().toUpper() == "REP") {
            isRepeater = true;
        }
        QString comment = rec.fields.value("COMMENT").trimmed().toUpper();
        QString notes = rec.fields.value("NOTES").trimmed().toUpper();
        if (comment.contains("REPEATER") || comment.contains("DIGIPEATER") ||
            notes.contains("REPEATER") || notes.contains("DIGIPEATER")) {
            isRepeater = true;
        }

        if (isRepeater) {
            res.repeaterCount++;
            continue;
        }

        QString qsoKey = call + ":" + band + ":" + modeGroup;
        if (seenQsos.contains(qsoKey)) {
            res.duplicateCount++;
            continue;
        }

        seenQsos.insert(qsoKey);
        res.validCount++;

        if (isTnStation) {
            qsoCountByMyCounty[currentMyExch]++;
        }

        QString workedCnty = getWorkedTnCounty(rec);
        if (workedCnty.isEmpty() && tnCounties.contains(dxExch)) {
            workedCnty = dxExch;
        }

        if (!workedCnty.isEmpty()) {
            tnCountiesByBand[band].insert(workedCnty);
        } else {
            if (isTnStation) {
                QString dxcc = rec.fields.value("DXCC").trimmed();
                QString country = rec.fields.value("COUNTRY").trimmed().toUpper();
                QString provAbbr = getCanadianProvAbbr(dxState);
                if (provAbbr.isEmpty()) provAbbr = getCanadianProvAbbr(rec.fields.value("PROV"));

                if (dxcc == "197" || country == "CANADA" || !provAbbr.isEmpty()) {
                    if (provAbbr.isEmpty()) provAbbr = "VE";
                    provincesByBand[band].insert(provAbbr);
                } else {
                    QString stateToUse = dxState;
                    if (stateToUse == "DC") stateToUse = "MD";
                    
                    if (isValidUsState(stateToUse) && stateToUse != "TN") {
                        statesByBand[band].insert(stateToUse);
                    } else {
                        bool isExcluded = (country == "UNITED STATES" || country == "USA" || country == "US" ||
                                           country == "CANADA" || country == "ALASKA" || country == "HAWAII" ||
                                           dxcc == "291" || dxcc == "197" || dxcc == "6" || dxcc == "110");
                        if (!isExcluded && (!country.isEmpty() || !dxcc.isEmpty())) {
                            QString countryName = country.isEmpty() ? dxcc : country;
                            dxccByBand[band].insert(countryName);
                        }
                    }
                }
            }
        }

        // TCG HQ Station (K4TCG) Check
        if (call == "K4TCG") {
            QString bonusKey = band + ":" + modeGroup;
            if (!seenK4TcgBonus.contains(bonusKey)) {
                seenK4TcgBonus.insert(bonusKey);
                res.tcgBonus += 100;
                res.totalBonusPoints += 100;
            }
        }
    }

    if (isTnStation && isMobileOrRover) {
        for (auto it = qsoCountByMyCounty.begin(); it != qsoCountByMyCounty.end(); ++it) {
            QString myCnty = it.key();
            int count = it.value();
            if (count >= 10 && tnCounties.contains(myCnty)) {
                res.roverBonus += 500;
                res.totalBonusPoints += 500;
                for (const QString &b : tnCountiesByBand.keys()) {
                    if (!tnCountiesByBand[b].contains(myCnty)) {
                        tnCountiesByBand[b].insert(myCnty);
                    }
                }
            }
        }
    }

    int totalTnCounties = 0;
    int totalStates = 0;
    int totalProvinces = 0;
    int totalDxcc = 0;

    for (const QString &b : tnCountiesByBand.keys()) totalTnCounties += tnCountiesByBand[b].size();
    for (const QString &b : statesByBand.keys()) totalStates += statesByBand[b].size();
    for (const QString &b : provincesByBand.keys()) totalProvinces += provincesByBand[b].size();
    for (const QString &b : dxccByBand.keys()) totalDxcc += dxccByBand[b].size();

    res.tnCounties = totalTnCounties;
    res.states = totalStates;
    res.provinces = totalProvinces;
    res.dxcc = totalDxcc;

    res.totalMultipliers = totalTnCounties + totalStates + totalProvinces + totalDxcc;
    res.finalScore = (res.validCount * 3 * res.totalMultipliers) + res.totalBonusPoints;

    return res;
}

void CabrilloConverterDialog::onAdifInputChanged() {
    QString adif = m_textAdifInput->toPlainText();
    QList<AdifRecord> records = parseAdifLog(adif);
    if (!records.isEmpty()) {
        ScoreResult scoreRes = computeContestScore(records,
                                                   m_editLocation->text(),
                                                   m_comboCategoryStation->currentText(),
                                                   m_editOperators->text());
        m_editClaimedScore->setText(QString::number(scoreRes.finalScore));
    } else {
        m_editClaimedScore->clear();
    }
}

void CabrilloConverterDialog::onShowScoreClicked() {
    QString adif = m_textAdifInput->toPlainText();
    QList<AdifRecord> records = parseAdifLog(adif);
    
    ScoreResult scoreRes = computeContestScore(records,
                                               m_editLocation->text(),
                                               m_comboCategoryStation->currentText(),
                                               m_editOperators->text());

    QDialog dlg(this);
    dlg.setWindowTitle(tr("Score Breakdown Details"));
    dlg.resize(460, 520);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    bool isDark = (dlg.palette().color(QPalette::Window).lightness() < 128);
    QString titleColor = isDark ? "#f8fafc" : "#0f172a";
    QString baseColor = isDark ? "#e2e8f0" : "#334155";
    QString secondaryColor = isDark ? "#94a3b8" : "#64748b";
    QString borderColor = isDark ? "#475569" : "#cbd5e1";
    QString highlightColor = isDark ? "#38bdf8" : "#0284c7";

    QLabel *titleLabel = new QLabel(QString("<span style='font-size: 14px; font-weight: bold; color: %1;'>Cabrillo Contest Score Breakdown</span>").arg(titleColor), &dlg);
    layout->addWidget(titleLabel);

    QFrame *line = new QFrame(&dlg);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet(QString("color: %1;").arg(borderColor));
    layout->addWidget(line);

    QTextBrowser *browser = new QTextBrowser(&dlg);
    browser->setFrameShape(QFrame::NoFrame);
    browser->setStyleSheet(QString("background-color: transparent; color: %1;").arg(baseColor));
    
    QPalette pal = browser->palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    browser->setPalette(pal);

    QString html = tr(
        "<table cellpadding='4' cellspacing='0' style='width: 100%; font-family: sans-serif; font-size: 11px; color: %1;'>"
        "  <tr><td colspan='2' style='font-size: 12px; font-weight: bold; padding-top: 4px; padding-bottom: 4px; color: %2;'>QSO Statistics</td></tr>"
        "  <tr><td>Valid QSOs:</td><td align='right'><b>%3</b> (3 points each)</td></tr>"
        "  <tr><td style='color: %4;'>Duplicate QSOs:</td><td align='right' style='color: %4;'>%5 (0 points)</td></tr>"
        "  <tr><td style='color: %4;'>Repeater QSOs:</td><td align='right' style='color: %4;'>%6 (0 points)</td></tr>"
        "  <tr><td colspan='2' style='font-size: 12px; font-weight: bold; padding-top: 12px; padding-bottom: 4px; color: %2;'>Multipliers (by Band)</td></tr>"
        "  <tr><td>Tennessee Counties:</td><td align='right'>%7</td></tr>"
        "  <tr><td>US States & Sections:</td><td align='right'>%8</td></tr>"
        "  <tr><td>Canadian Provinces:</td><td align='right'>%9</td></tr>"
        "  <tr><td>DXCC Countries:</td><td align='right'>%10</td></tr>"
        "  <tr style='font-weight: bold; color: %2;'><td>Total Multipliers:</td><td align='right'>%11</td></tr>"
        "  <tr><td colspan='2' style='font-size: 12px; font-weight: bold; padding-top: 12px; padding-bottom: 4px; color: %2;'>Bonus Points</td></tr>"
        "  <tr><td>K4TCG HQ Station (100 pts/band-mode):</td><td align='right'>+%12 pts</td></tr>"
        "  <tr><td>TN Mobile/Rover (500 pts/county &ge; 10 QSOs):</td><td align='right'>+%13 pts</td></tr>"
        "  <tr style='font-weight: bold; color: %2;'><td>Total Bonus Points:</td><td align='right'>+%14 pts</td></tr>"
        "</table>"
        "<br>"
        "<div style='font-size: 12px; font-weight: bold; color: %2; margin-bottom: 4px;'>Scoring Calculation:</div>"
        "<div style='font-size: 11px; font-family: monospace; padding: 8px; border: 1px solid %15; border-radius: 4px; color: %1; line-height: 1.4;'>"
        "  Score = (Valid QSOs [%3] &times; 3 &times; Multipliers [%11]) + Bonus [%14]<br>"
        "  Score = (%16) + %14 = <b style='font-size: 12px; color: %17;'>%18</b>"
        "</div>"
    )
    .arg(baseColor)
    .arg(titleColor)
    .arg(scoreRes.validCount)
    .arg(secondaryColor)
    .arg(scoreRes.duplicateCount)
    .arg(scoreRes.repeaterCount)
    .arg(scoreRes.tnCounties)
    .arg(scoreRes.states)
    .arg(scoreRes.provinces)
    .arg(scoreRes.dxcc)
    .arg(scoreRes.totalMultipliers)
    .arg(scoreRes.tcgBonus)
    .arg(scoreRes.roverBonus)
    .arg(scoreRes.totalBonusPoints)
    .arg(borderColor)
    .arg(scoreRes.validCount * 3 * scoreRes.totalMultipliers)
    .arg(highlightColor)
    .arg(scoreRes.finalScore);

    browser->setHtml(html);
    layout->addWidget(browser);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *btnClose = new QPushButton(tr("Close"), &dlg);
    btnClose->setAutoDefault(true);
    btnClose->setStyleSheet("padding: 5px 15px; font-weight: bold;");
    connect(btnClose, &QPushButton::clicked, &dlg, &QDialog::accept);
    btnLayout->addWidget(btnClose);
    layout->addLayout(btnLayout);

    dlg.exec();
}
