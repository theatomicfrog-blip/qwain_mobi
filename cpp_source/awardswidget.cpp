#include "awardswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include "paths.h"
#include <QMouseEvent>
#include <QMessageBox>
#include <QApplication>

// A clickable label class
class ClickableLabel : public QLabel {
    Q_OBJECT
public:
    explicit ClickableLabel(const QString &id, QWidget *parent = nullptr) : QLabel(parent), m_id(id) {
        setCursor(Qt::PointingHandCursor);
    }
signals:
    void clicked(const QString &id);
    void doubleClicked(const QString &id);
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            emit clicked(m_id);
        }
    }
    void mouseDoubleClickEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            emit doubleClicked(m_id);
        }
    }
private:
    QString m_id;
};


AwardsWidget::AwardsWidget(const QString &adifPath, QWidget *parent) 
    : QWidget(parent), m_adifPath(adifPath), m_net(new QNetworkAccessManager(this))
{
    loadAwards();
    setupUI();
    parseAdif();
}

AwardsWidget::~AwardsWidget() {}

void AwardsWidget::loadAwards() {
    AwardGoal euAward;
    euAward.id = "AW-EU";
    euAward.name = "43-Country European UN Award";
    euAward.description = "Work all 43 European UN member states + special stations.";
    euAward.imageUrl = "https://4u1a.club/wp-content/uploads/2023/09/AW-EU-650x519-1-300x240.png";
    euAward.reqLabels = QStringList() << "Albania (ZA)" << "Andorra (C3)" << "Austria (OE)" << "Belarus (EW)" 
        << "Belgium (ON)" << "Bosnia & Herz. (E7)" << "Bulgaria (LZ)" << "Croatia (9A)" << "Czech Rep. (OK)" 
        << "Denmark (OZ)" << "Estonia (ES)" << "Finland (OH)" << "France (F)" << "Germany (DL)" << "Greece (SV)" 
        << "Hungary (HA)" << "Iceland (TF)" << "Ireland (EI)" << "Italy (I)" << "Latvia (YL)" << "Liechtenstein (HB0)" 
        << "Lithuania (LY)" << "Luxembourg (LX)" << "Malta (9H)" << "Monaco (3A)" << "Montenegro (4O)" 
        << "Netherlands (PA)" << "Norway (LA)" << "Poland (SP)" << "Portugal (CT)" << "Moldova (ER)" 
        << "Romania (YO)" << "Russia (UA)" << "San Marino (T7)" << "Serbia (YU)" << "Slovakia (OM)" 
        << "Slovenia (S5)" << "Spain (EA)" << "Sweden (SM)" << "Switzerland (HB)" << "N. Macedonia (Z3)" 
        << "Ukraine (UR)" << "United Kingdom (G)";
    m_awards["AW-EU"] = euAward;
    
    // Add dummy ones for the rest to make 10 total images (including UN logo maybe)
    QStringList otherIds = {"AW-AS", "AW-NA", "AW-SA", "AW-AF", "AW-OK", "AW-ANT", "AW-UN", "AW-EXTRA1", "AW-EXTRA2"};
    QStringList urls = {
        "https://4u1a.club/wp-content/uploads/2023/09/AW-AS-650x519-1-300x240.png",
        "https://4u1a.club/wp-content/uploads/2023/09/AW-NA-650x519-1-300x240.png",
        "https://4u1a.club/wp-content/uploads/2023/09/AW-SA-650x519-1-300x240.png",
        "https://4u1a.club/wp-content/uploads/2023/09/AW-AF-650x519-1-300x240.png",
        "https://4u1a.club/wp-content/uploads/2023/09/AW-OK-650x519-1-300x240.png",
        "https://4u1a.club/wp-content/uploads/2023/09/AW-ANT-650x519-1-240x300.png",
        "https://4u1a.club/wp-content/uploads/2023/09/UN-Logo.png", // generic placeholder
        "", ""
    };
    
    for (int i=0; i<otherIds.size(); ++i) {
        AwardGoal a;
        a.id = otherIds[i];
        a.name = otherIds[i] + " Award";
        a.description = "Description for " + otherIds[i];
        a.imageUrl = urls[i];
        for(int j=1; j<=20; ++j) a.reqLabels << QString("Requirement %1").arg(j);
        m_awards[a.id] = a;
    }
}

void AwardsWidget::setupUI() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    
    // Left side: images grid (UN logo + others)
    QWidget *leftPanel = new QWidget;
    QGridLayout *gridLayout = new QGridLayout(leftPanel);
    
    QStringList orderedIds = {"AW-UN", "AW-EU", "AW-AS", "AW-NA", "AW-SA", "AW-AF", "AW-OK", "AW-ANT", "AW-EXTRA1", "AW-EXTRA2"};
    int row = 0, col = 0;
    
    for (int i=0; i<orderedIds.size(); ++i) {
        QString id = orderedIds[i];
        ClickableLabel *lbl = new ClickableLabel(id);
        lbl->setFixedSize(120, 100);
        lbl->setScaledContents(true);
        lbl->setFrameStyle(QFrame::Box | QFrame::Plain);
        lbl->setText(id);
        
        connect(lbl, &ClickableLabel::clicked, this, &AwardsWidget::onImageClicked);
        connect(lbl, &ClickableLabel::doubleClicked, this, &AwardsWidget::onImageDoubleClicked);
        
        m_imgLabels[id] = lbl;
        
        if (id == "AW-UN") {
            // make it bigger?
            lbl->setFixedSize(250, 210);
            gridLayout->addWidget(lbl, 0, 0, 2, 2);
            col = 2;
        } else {
            gridLayout->addWidget(lbl, row, col);
            col++;
            if (col > 4) {
                col = 2; // UN logo takes col 0 and 1
                row++;
            }
        }
        
        if (!m_awards[id].imageUrl.isEmpty()) {
            downloadImage(m_awards[id].imageUrl, lbl);
        }
    }
    
    // Right side: details
    QWidget *rightPanel = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    
    m_titleLabel = new QLabel("Select an award");
    QFont f = m_titleLabel->font();
    f.setBold(true);
    f.setPointSize(14);
    m_titleLabel->setFont(f);
    
    m_descLabel = new QLabel("");
    m_descLabel->setWordWrap(true);
    
    m_reqTable = new QTableWidget(0, 4);
    m_reqTable->horizontalHeader()->hide();
    m_reqTable->verticalHeader()->hide();
    m_reqTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_reqTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_reqTable->setSelectionMode(QAbstractItemView::NoSelection);
    
    rightLayout->addWidget(m_titleLabel);
    rightLayout->addWidget(m_descLabel);
    rightLayout->addWidget(new QLabel("<b>Requirements</b>"));
    rightLayout->addWidget(m_reqTable, 1);
    
    mainLayout->addWidget(leftPanel, 1);
    mainLayout->addWidget(rightPanel, 2);
}

void AwardsWidget::downloadImage(const QString &urlStr, QLabel *label) {
    // Unified storage: cache award map assets strictly inside ~/.config/Qwain
    QString cacheDir = getAppDataDir() + "/QwainAwards";
    QDir().mkpath(cacheDir);
    QString fileName = urlStr.section('/', -1);
    QString localPath = cacheDir + "/" + fileName;
    
    if (QFile::exists(localPath)) {
        label->setPixmap(QPixmap(localPath));
        return;
    }
    
    QNetworkRequest req(urlStr);
    QNetworkReply *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, label, localPath]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QPixmap pix;
            if (pix.loadFromData(data)) {
                label->setPixmap(pix);
                QFile f(localPath);
                if (f.open(QIODevice::WriteOnly)) {
                    f.write(data);
                    f.close();
                }
            }
        }
        reply->deleteLater();
    });
}

void AwardsWidget::parseAdif() {
    QFile file(m_adifPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    
    QString content = QString::fromUtf8(file.readAll());
    int pos = 0;
    while ((pos = content.indexOf("<CALL:", pos, Qt::CaseInsensitive)) != -1) {
        int endTag = content.indexOf(">", pos);
        if (endTag == -1) break;
        int colon = content.indexOf(":", pos + 1);
        if (colon == -1 || colon > endTag) { pos = endTag; continue; }
        int len = content.mid(colon + 1, endTag - colon - 1).toInt();
        QString call = content.mid(endTag + 1, len).toUpper();
        
        QString country = "";
        QString dxcc = "";
        
        // crude search for DXCC and COUNTRY in the same record
        // usually records end with <EOR>
        int eor = content.indexOf("<EOR>", pos, Qt::CaseInsensitive);
        if (eor == -1) eor = content.length();
        
        int cPos = content.indexOf("<COUNTRY:", pos, Qt::CaseInsensitive);
        if (cPos != -1 && cPos < eor) {
            int cEnd = content.indexOf(">", cPos);
            int cColon = content.indexOf(":", cPos + 1);
            if (cColon != -1 && cColon < cEnd) {
                int cLen = content.mid(cColon + 1, cEnd - cColon - 1).toInt();
                country = content.mid(cEnd + 1, cLen).toUpper();
            }
        }
        
        int dPos = content.indexOf("<DXCC:", pos, Qt::CaseInsensitive);
        if (dPos != -1 && dPos < eor) {
            int dEnd = content.indexOf(">", dPos);
            int dColon = content.indexOf(":", dPos + 1);
            if (dColon != -1 && dColon < dEnd) {
                int dLen = content.mid(dColon + 1, dEnd - dColon - 1).toInt();
                dxcc = content.mid(dEnd + 1, dLen).toUpper();
            }
        }
        
        evaluateUN43(call, country, dxcc);
        
        pos = eor;
    }
}

void AwardsWidget::evaluateUN43(const QString &call, const QString &country, const QString &dxcc) {
    if (call == "4U1A" || call == "4U2U" || call == "4U0R" || call == "4UNR" || call == "4Y1A" || call == "C7A") {
        m_specialWorked = true;
    }
    if (call.startsWith("4U1A")) {
        m_4u1aMemberWorked = true;
    }
    
    // Mapping 43 countries by common prefixes/DXCC/names
    // ZA
    if (call.startsWith("ZA") || dxcc == "5" || country.contains("ALBANIA")) m_eu43Worked["Albania (ZA)"] = true;
    if (call.startsWith("C3") || dxcc == "20" || country.contains("ANDORRA")) m_eu43Worked["Andorra (C3)"] = true;
    if (call.startsWith("OE") || dxcc == "206" || country.contains("AUSTRIA")) m_eu43Worked["Austria (OE)"] = true;
    if (call.startsWith("EW") || call.startsWith("EU") || dxcc == "27" || country.contains("BELARUS")) m_eu43Worked["Belarus (EW)"] = true;
    if (call.startsWith("ON") || dxcc == "209" || country.contains("BELGIUM")) m_eu43Worked["Belgium (ON)"] = true;
    if (call.startsWith("E7") || call.startsWith("T9") || dxcc == "401" || country.contains("BOSNIA")) m_eu43Worked["Bosnia & Herz. (E7)"] = true;
    if (call.startsWith("LZ") || dxcc == "212" || country.contains("BULGARIA")) m_eu43Worked["Bulgaria (LZ)"] = true;
    if (call.startsWith("9A") || dxcc == "497" || country.contains("CROATIA")) m_eu43Worked["Croatia (9A)"] = true;
    if (call.startsWith("OK") || dxcc == "503" || country.contains("CZECH")) m_eu43Worked["Czech Rep. (OK)"] = true;
    if (call.startsWith("OZ") || dxcc == "221" || country.contains("DENMARK")) m_eu43Worked["Denmark (OZ)"] = true;
    if (call.startsWith("ES") || dxcc == "52" || country.contains("ESTONIA")) m_eu43Worked["Estonia (ES)"] = true;
    if (call.startsWith("OH") || dxcc == "224" || country.contains("FINLAND")) m_eu43Worked["Finland (OH)"] = true;
    if (call.startsWith("F") || dxcc == "227" || country.contains("FRANCE")) m_eu43Worked["France (F)"] = true;
    if (call.startsWith("DL") || dxcc == "230" || country.contains("GERMANY") || country.contains("FED. REP. OF GERMANY")) m_eu43Worked["Germany (DL)"] = true;
    if (call.startsWith("SV") || dxcc == "236" || country.contains("GREECE")) m_eu43Worked["Greece (SV)"] = true;
    if (call.startsWith("HA") || dxcc == "239" || country.contains("HUNGARY")) m_eu43Worked["Hungary (HA)"] = true;
    if (call.startsWith("TF") || dxcc == "242" || country.contains("ICELAND")) m_eu43Worked["Iceland (TF)"] = true;
    if (call.startsWith("EI") || dxcc == "245" || country.contains("IRELAND")) m_eu43Worked["Ireland (EI)"] = true;
    if (call.startsWith("I") || dxcc == "248" || country.contains("ITALY")) m_eu43Worked["Italy (I)"] = true;
    if (call.startsWith("YL") || dxcc == "145" || country.contains("LATVIA")) m_eu43Worked["Latvia (YL)"] = true;
    if (call.startsWith("HB0") || dxcc == "251" || country.contains("LIECHTENSTEIN")) m_eu43Worked["Liechtenstein (HB0)"] = true;
    if (call.startsWith("LY") || dxcc == "146" || country.contains("LITHUANIA")) m_eu43Worked["Lithuania (LY)"] = true;
    if (call.startsWith("LX") || dxcc == "254" || country.contains("LUXEMBOURG")) m_eu43Worked["Luxembourg (LX)"] = true;
    if (call.startsWith("9H") || dxcc == "257" || country.contains("MALTA")) m_eu43Worked["Malta (9H)"] = true;
    if (call.startsWith("3A") || dxcc == "260" || country.contains("MONACO")) m_eu43Worked["Monaco (3A)"] = true;
    if (call.startsWith("4O") || dxcc == "514" || country.contains("MONTENEGRO")) m_eu43Worked["Montenegro (4O)"] = true;
    if (call.startsWith("PA") || dxcc == "263" || country.contains("NETHERLANDS")) m_eu43Worked["Netherlands (PA)"] = true;
    if (call.startsWith("LA") || dxcc == "265" || country.contains("NORWAY")) m_eu43Worked["Norway (LA)"] = true;
    if (call.startsWith("SP") || dxcc == "269" || country.contains("POLAND")) m_eu43Worked["Poland (SP)"] = true;
    if (call.startsWith("CT") || dxcc == "272" || country.contains("PORTUGAL")) m_eu43Worked["Portugal (CT)"] = true;
    if (call.startsWith("ER") || dxcc == "179" || country.contains("MOLDOVA")) m_eu43Worked["Moldova (ER)"] = true;
    if (call.startsWith("YO") || dxcc == "275" || country.contains("ROMANIA")) m_eu43Worked["Romania (YO)"] = true;
    if (call.startsWith("UA") || dxcc == "54" || country.contains("RUSSIA") || country.contains("EUROPEAN RUSSIA")) m_eu43Worked["Russia (UA)"] = true;
    if (call.startsWith("T7") || dxcc == "278" || country.contains("SAN MARINO")) m_eu43Worked["San Marino (T7)"] = true;
    if (call.startsWith("YU") || dxcc == "296" || country.contains("SERBIA")) m_eu43Worked["Serbia (YU)"] = true;
    if (call.startsWith("OM") || dxcc == "504" || country.contains("SLOVAKIA")) m_eu43Worked["Slovakia (OM)"] = true;
    if (call.startsWith("S5") || dxcc == "499" || country.contains("SLOVENIA")) m_eu43Worked["Slovenia (S5)"] = true;
    if (call.startsWith("EA") || dxcc == "281" || country.contains("SPAIN")) m_eu43Worked["Spain (EA)"] = true;
    if (call.startsWith("SM") || dxcc == "284" || country.contains("SWEDEN")) m_eu43Worked["Sweden (SM)"] = true;
    if (call.startsWith("HB") || dxcc == "287" || country.contains("SWITZERLAND")) m_eu43Worked["Switzerland (HB)"] = true;
    if (call.startsWith("Z3") || dxcc == "502" || country.contains("MACEDONIA")) m_eu43Worked["N. Macedonia (Z3)"] = true;
    if (call.startsWith("UR") || dxcc == "288" || country.contains("UKRAINE")) m_eu43Worked["Ukraine (UR)"] = true;
    if (call.startsWith("G") || call.startsWith("M") || call.startsWith("2") || dxcc == "223" || country.contains("UNITED KINGDOM") || country.contains("ENGLAND")) m_eu43Worked["United Kingdom (G)"] = true;
}

void AwardsWidget::onImageClicked(const QString &awardId) {
    m_currentAwardId = awardId;
    updateRightPanel(awardId);
}

void AwardsWidget::onImageDoubleClicked(const QString &awardId) {
    updateRightPanel(awardId);
    
    QDialog dlg(this);
    dlg.setWindowTitle("Requirements / Status");
    dlg.resize(600, 400);
    QVBoxLayout *l = new QVBoxLayout(&dlg);
    
    QLabel *t = new QLabel(QString("<h2>%1</h2>").arg(m_awards[awardId].name));
    l->addWidget(t);
    
    QTableWidget *tw = new QTableWidget;
    tw->setColumnCount(4);
    tw->horizontalHeader()->hide();
    tw->verticalHeader()->hide();
    tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tw->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tw->setSelectionMode(QAbstractItemView::NoSelection);
    
    QStringList reqs = m_awards[awardId].reqLabels;
    if (awardId == "AW-EU") {
        reqs.append("Special Station (4U1A/etc)");
        reqs.append("4U1A Member QSO");
    }
    
    int rows = (reqs.size() + 3) / 4;
    tw->setRowCount(rows);
    
    for (int i=0; i<reqs.size(); ++i) {
        int r = i / 4;
        int c = i % 4;
        QTableWidgetItem *item = new QTableWidgetItem(reqs[i]);
        
        bool worked = false;
        if (awardId == "AW-EU") {
            if (reqs[i] == "Special Station (4U1A/etc)") worked = m_specialWorked;
            else if (reqs[i] == "4U1A Member QSO") worked = m_4u1aMemberWorked;
            else worked = m_eu43Worked.value(reqs[i], false);
        } else {
            // Placeholder logic for other awards
            worked = (i % 3 == 0); // just to show some red/green
        }
        
        if (worked) {
            item->setBackground(QColor(180, 255, 180));
            item->setForeground(Qt::black);
        } else {
            item->setBackground(QColor(255, 180, 180));
            item->setForeground(Qt::black);
        }
        
        item->setTextAlignment(Qt::AlignCenter);
        tw->setItem(r, c, item);
    }
    
    l->addWidget(tw);
    dlg.exec();
}

void AwardsWidget::updateRightPanel(const QString &awardId) {
    if (!m_awards.contains(awardId)) return;
    AwardGoal ag = m_awards[awardId];
    
    m_titleLabel->setText(ag.name);
    m_descLabel->setText(ag.description);
    
    QStringList reqs = ag.reqLabels;
    if (awardId == "AW-EU") {
        reqs.append("Special Station (4U1A/etc)");
        reqs.append("4U1A Member QSO");
    }
    
    int rows = (reqs.size() + 3) / 4;
    m_reqTable->setRowCount(rows);
    m_reqTable->clearContents();
    
    for (int i=0; i<reqs.size(); ++i) {
        int r = i / 4;
        int c = i % 4;
        QTableWidgetItem *item = new QTableWidgetItem(reqs[i]);
        
        bool worked = false;
        if (awardId == "AW-EU") {
            if (reqs[i] == "Special Station (4U1A/etc)") worked = m_specialWorked;
            else if (reqs[i] == "4U1A Member QSO") worked = m_4u1aMemberWorked;
            else worked = m_eu43Worked.value(reqs[i], false);
        } else {
            worked = (i % 3 == 0); // placeholder
        }
        
        if (worked) {
            item->setBackground(QColor(180, 255, 180));
            item->setForeground(Qt::black);
        } else {
            item->setBackground(QColor(255, 180, 180));
            item->setForeground(Qt::black);
        }
        
        item->setTextAlignment(Qt::AlignCenter);
        m_reqTable->setItem(r, c, item);
    }
}
#include "awardswidget.moc"
