#include "qslstatusdashboard.h"
#include "qrzcache.h"
#include <QHeaderView>
#include <QSettings>
#include "paths.h"

QslStatusDashboard::QslStatusDashboard(MainWindow *mainWnd, QWidget *parent) 
    : QDialog(parent), m_mainWnd(mainWnd) {
    setWindowTitle("QSL Status Dashboard");
    resize(800, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *topLayout = new QHBoxLayout();
    m_chkMailOnly = new QCheckBox("Show only Mail-Accepted (MQSL)", this);
    m_chkMailOnly->setChecked(true); // Default to filtering by mail-accepted
    m_chkDirectOnly = new QCheckBox("Show only VIA Direct", this);
    m_statusLbl = new QLabel(this);
    
    topLayout->addWidget(m_chkMailOnly);
    topLayout->addWidget(m_chkDirectOnly);
    topLayout->addStretch();
    topLayout->addWidget(m_statusLbl);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({"Callsign", "Date", "Band", "Mode", "QSL Msg / Via", "Mail Status", "QRZ QslMgr", "QRZ xcheck Via", "Local ADIF QSL Rcvd"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->verticalHeader()->setVisible(false);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_table);

    connect(m_chkMailOnly, &QCheckBox::stateChanged, this, &QslStatusDashboard::populateTable);
    connect(m_chkDirectOnly, &QCheckBox::stateChanged, this, &QslStatusDashboard::populateTable);

    populateTable();
}

void QslStatusDashboard::populateTable() {
    m_table->setRowCount(0);
    int shown = 0;

    QrzCache qrzCache(getAppDataDir() + "/qrz_cache.ini", QSettings::IniFormat);

    const auto &allQsos = m_mainWnd->allQsos();
    for (auto it = allQsos.begin(); it != allQsos.end(); ++it) {
        QString call = it.key();

        qrzCache.beginGroup(call);
        bool isMqsl = qrzCache.value("mqsl").toBool();
        QString qslmgr = qrzCache.value("qslmgr").toString();
        bool isXcheckMailable = qrzCache.value("xcheck_mailable").toBool();
        QString xcheckVia = qrzCache.value("xcheck_via").toString();
        qrzCache.endGroup();

        for (const QsoDetail &d : it.value()) {
            bool adifMqsl = d.mqslRcvd;
            QString adifVia = d.qslVia;
            QString adifMsg = d.qslMsg;
            
            bool isDirect = adifVia.contains("DIRECT", Qt::CaseInsensitive) || adifMsg.contains("DIRECT", Qt::CaseInsensitive) || qslmgr.contains("DIRECT", Qt::CaseInsensitive) || xcheckVia.contains("DIRECT", Qt::CaseInsensitive);

            bool isMailable = isMqsl || isXcheckMailable || adifMqsl;

            if (m_chkMailOnly->isChecked() && !isMailable) continue;
            if (m_chkDirectOnly->isChecked() && !isDirect) continue;

            QString mailStatus;
            if (adifMqsl) mailStatus += "ADIF ";
            if (isMqsl) mailStatus += "QRZ ";
            if (isXcheckMailable) mailStatus += "XCheck";
            if (mailStatus.isEmpty()) mailStatus = "No";

            m_table->insertRow(shown);
            m_table->setItem(shown, 0, new QTableWidgetItem(call));
            m_table->setItem(shown, 1, new QTableWidgetItem(d.date));
            m_table->setItem(shown, 2, new QTableWidgetItem(d.band));
            m_table->setItem(shown, 3, new QTableWidgetItem(d.mode));
            m_table->setItem(shown, 4, new QTableWidgetItem(adifMsg.isEmpty() ? adifVia : (adifVia + " " + adifMsg)));
            m_table->setItem(shown, 5, new QTableWidgetItem(mailStatus.trimmed()));
            m_table->setItem(shown, 6, new QTableWidgetItem(qslmgr));
            m_table->setItem(shown, 7, new QTableWidgetItem(xcheckVia));
            m_table->setItem(shown, 8, new QTableWidgetItem(adifMqsl ? "Yes" : "No"));
            shown++;
        }
    }
    
    m_table->resizeColumnsToContents();
    m_statusLbl->setText(QString::number(shown) + " QSOs match criteria");
}

