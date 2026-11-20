/*
 * ABANDONWARE PUBLIC LICENSE
 * This software is released into the public domain as abandonware.
 * It is provided AS-IS without warranty of any kind. Anyone is free
 * to copy, modify, publish, use, compile, sell, or distribute this
 * software for any purpose, commercial or non-commercial.
 *
 * DISCLAIMER: W4DJF wants to be as far removed from this abomination
 * as mathematically, legally, and spiritually possible. Use at your own risk.
 */

#include "txtweakerwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QCoreApplication>
#include <QFileInfo>

TxTweakerWidget::TxTweakerWidget(QWidget *parent) : QWidget(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Disclaimer and Banner Paragraph
    QLabel *bannerLabel = new QLabel(this);
    bannerLabel->setWordWrap(true);
    bannerLabel->setText(
        "<center><b>I DO NOT CONDONE THE USE OF THIS SOFTWARE.</b><br>"
        "Several people have asked for something like this.. so here it is."
        "</center><br>"
        "Not all digital mode loggers log the same, nor do lazy users.<br><br>"
        "Many loggers fail to record Transmit Power when recording a QSO.<br>"
        "This utility reads your ADIF logfile, scans for digital modes<br>"
        "without a &lt;tx_pwr&gt; adif tag. If it finds a 0, or &lt;nul&gt;, it simply inserts a value in the QSO<br>"
        "record for you. Helps some online and home QSL card printing<br>"
        "a more complete QSL card.<br><br>"
        "<b>USE:</b><br>"
        "&nbsp;&nbsp;&nbsp;&nbsp;Download your logfile from where ever you get yours from.<br>"
        "&nbsp;&nbsp;&nbsp;&nbsp;Click Browse and select the ADIF logfile you downloaded.<br>"
        "&nbsp;&nbsp;&nbsp;&nbsp;Click Process ADIF.<br>"
        "&nbsp;&nbsp;&nbsp;&nbsp;Click Save Tweaked File As..<br>"
        "&nbsp;&nbsp;&nbsp;&nbsp;Upload your new ADIF back to where you wish"
    );
    mainLayout->addWidget(bannerLabel);

    // Settings Group
    QGroupBox *tweaksGroup = new QGroupBox("Power Tweaks Configuration", this);
    QVBoxLayout *tweaksLayout = new QVBoxLayout(tweaksGroup);

    QGridLayout *grid = new QGridLayout();
    grid->addWidget(new QLabel("<b>Mode</b>"), 0, 0);
    grid->addWidget(new QLabel("<b>Min (Watts)</b>"), 0, 1, Qt::AlignCenter);
    grid->addWidget(new QLabel("<b>Max (Watts)</b>"), 0, 2, Qt::AlignCenter);

    // FT2
    m_chkFT2 = new QCheckBox("FT-2", this);
    m_ft2MinSpin = new QSpinBox(this); m_ft2MinSpin->setRange(0, 9999); m_ft2MinSpin->setSingleStep(5);
    m_ft2MaxSpin = new QSpinBox(this); m_ft2MaxSpin->setRange(0, 9999); m_ft2MaxSpin->setSingleStep(5);
    grid->addWidget(m_chkFT2, 1, 0); grid->addWidget(m_ft2MinSpin, 1, 1); grid->addWidget(m_ft2MaxSpin, 1, 2);

    // FT4
    m_chkFT4 = new QCheckBox("FT-4", this);
    m_ft4MinSpin = new QSpinBox(this); m_ft4MinSpin->setRange(0, 9999); m_ft4MinSpin->setSingleStep(5);
    m_ft4MaxSpin = new QSpinBox(this); m_ft4MaxSpin->setRange(0, 9999); m_ft4MaxSpin->setSingleStep(5);
    grid->addWidget(m_chkFT4, 2, 0); grid->addWidget(m_ft4MinSpin, 2, 1); grid->addWidget(m_ft4MaxSpin, 2, 2);

    // FT8
    m_chkFT8 = new QCheckBox("FT-8", this);
    m_ft8MinSpin = new QSpinBox(this); m_ft8MinSpin->setRange(0, 9999); m_ft8MinSpin->setSingleStep(5);
    m_ft8MaxSpin = new QSpinBox(this); m_ft8MaxSpin->setRange(0, 9999); m_ft8MaxSpin->setSingleStep(5);
    grid->addWidget(m_chkFT8, 3, 0); grid->addWidget(m_ft8MinSpin, 3, 1); grid->addWidget(m_ft8MaxSpin, 3, 2);

    tweaksLayout->addLayout(grid);

    m_saveSettingsBtn = new QPushButton("Save Settings", this);
    tweaksLayout->addWidget(m_saveSettingsBtn, 0, Qt::AlignRight);
    mainLayout->addWidget(tweaksGroup);

    // ADIF Batch Processing Group
    QGroupBox *batchGroup = new QGroupBox("Batch Process ADIF Log File", this);
    QVBoxLayout *batchLayout = new QVBoxLayout(batchGroup);

    QHBoxLayout *fileLayout = new QHBoxLayout();
    m_inputPathEdit = new QTextEdit(this);
    m_inputPathEdit->setMaximumHeight(35);
    m_inputPathEdit->setPlaceholderText("Select input ADIF file...");
    fileLayout->addWidget(m_inputPathEdit);

    QPushButton *browseBtn = new QPushButton("Browse", this);
    fileLayout->addWidget(browseBtn);
    batchLayout->addLayout(fileLayout);

    m_processBtn = new QPushButton("Process ADIF", this);
    m_processBtn->setStyleSheet("QPushButton { font-weight: bold; background-color: #2c87c0; color: white; padding: 6px; }");
    batchLayout->addWidget(m_processBtn);

    m_logArea = new QTextEdit(this);
    m_logArea->setReadOnly(true);
    m_logArea->setPlaceholderText("Processing output log...");
    batchLayout->addWidget(m_logArea);

    m_saveAsBtn = new QPushButton("Save Tweaked File As..", this);
    m_saveAsBtn->setStyleSheet("QPushButton { font-weight: bold; padding: 6px; }");
    batchLayout->addWidget(m_saveAsBtn);

    mainLayout->addWidget(batchGroup);

    // Connect signals
    connect(m_saveSettingsBtn, &QPushButton::clicked, this, &TxTweakerWidget::saveSettings);
    connect(browseBtn, &QPushButton::clicked, this, &TxTweakerWidget::onBrowseInputAdif);
    connect(m_processBtn, &QPushButton::clicked, this, &TxTweakerWidget::onProcessAdif);
    connect(m_saveAsBtn, &QPushButton::clicked, this, &TxTweakerWidget::onSaveTweakedFileAs);

    loadSettings();
}

void TxTweakerWidget::loadSettings() {
    QSettings settings("TxTweaker", "TxTweaker");
    settings.beginGroup("LogTweaker");

    m_chkFT2->setChecked(settings.value("useTweaks_FT2", true).toBool());
    m_ft2MinSpin->setValue(settings.value("FT2_min", 10).toInt());
    m_ft2MaxSpin->setValue(settings.value("FT2_max", 100).toInt());

    m_chkFT4->setChecked(settings.value("useTweaks_FT4", true).toBool());
    m_ft4MinSpin->setValue(settings.value("FT4_min", 10).toInt());
    m_ft4MaxSpin->setValue(settings.value("FT4_max", 100).toInt());

    m_chkFT8->setChecked(settings.value("useTweaks_FT8", true).toBool());
    m_ft8MinSpin->setValue(settings.value("FT8_min", 10).toInt());
    m_ft8MaxSpin->setValue(settings.value("FT8_max", 100).toInt());

    settings.endGroup();
}

void TxTweakerWidget::saveSettings() {
    QSettings settings("TxTweaker", "TxTweaker");
    settings.beginGroup("LogTweaker");

    settings.setValue("useTweaks_FT2", m_chkFT2->isChecked());
    settings.setValue("FT2_min", m_ft2MinSpin->value());
    settings.setValue("FT2_max", m_ft2MaxSpin->value());

    settings.setValue("useTweaks_FT4", m_chkFT4->isChecked());
    settings.setValue("FT4_min", m_ft4MinSpin->value());
    settings.setValue("FT4_max", m_ft4MaxSpin->value());

    settings.setValue("useTweaks_FT8", m_chkFT8->isChecked());
    settings.setValue("FT8_min", m_ft8MinSpin->value());
    settings.setValue("FT8_max", m_ft8MaxSpin->value());

    settings.endGroup();
    settings.sync();

    QMessageBox::information(this, "Settings Saved", "Power Tweaks settings saved successfully.");
}

void TxTweakerWidget::onBrowseInputAdif() {
    QString path = QFileDialog::getOpenFileName(this, "Select ADIF Log File", QDir::homePath(), "ADIF Files (*.adi *.adif);;All Files (*)");
    if (!path.isEmpty()) {
        m_inputPathEdit->setText(path);
        m_lastInputFileName = QFileInfo(path).fileName();
    }
}

QString TxTweakerWidget::extractAdifField(const QString &record, const QString &fieldName) {
    QRegularExpression re(QString("<%1:(\\d+)(?::[^>]*)?>([^<]*)").arg(fieldName), QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = re.match(record);
    if (match.hasMatch()) {
        int len = match.captured(1).toInt();
        QString val = match.captured(2);
        if (val.length() > len) val = val.left(len);
        return val.trimmed();
    }
    return QString();
}

QString TxTweakerWidget::computeTweakPower(const QString &mode, const QString &seedStr) {
    int minPwr = 10;
    int maxPwr = 100;
    QString m = mode.toUpper();

    if (m == "FT2" && m_chkFT2->isChecked()) {
        minPwr = m_ft2MinSpin->value(); maxPwr = m_ft2MaxSpin->value();
    } else if (m == "FT4" && m_chkFT4->isChecked()) {
        minPwr = m_ft4MinSpin->value(); maxPwr = m_ft4MaxSpin->value();
    } else if (m == "FT8" && m_chkFT8->isChecked()) {
        minPwr = m_ft8MinSpin->value(); maxPwr = m_ft8MaxSpin->value();
    } else {
        return QString();
    }

    if (minPwr > maxPwr) std::swap(minPwr, maxPwr);
    uint seed = qHash(seedStr);
    QRandomGenerator rand(seed);
    int steps = (maxPwr - minPwr) / 5;
    if (steps < 0) steps = 0;
    int val = minPwr + (rand.bounded(steps + 1) * 5);
    return QString::number(val);
}

void TxTweakerWidget::onProcessAdif() {
    QString inputPath = m_inputPathEdit->toPlainText().trimmed();
    if (inputPath.isEmpty() || !QFile::exists(inputPath)) {
        QMessageBox::warning(this, "Error", "Please select a valid input ADIF file.");
        return;
    }

    saveSettings();

    QFile inFile(inputPath);
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Could not open input file for reading.");
        return;
    }

    QString content = QString::fromUtf8(inFile.readAll());
    inFile.close();

    m_logArea->clear();
    m_logArea->append("Processing " + inputPath + "...");

    QRegularExpression eorRe("<eor>", QRegularExpression::CaseInsensitiveOption);
    QStringList records = content.split(eorRe);

    int tweakedCount = 0;
    int totalCount = 0;
    QStringList outputRecords;

    for (int i = 0; i < records.size(); ++i) {
        QString rec = records[i];
        if (rec.trimmed().isEmpty()) continue;

        if (i == 0 && rec.contains("<EOH>", Qt::CaseInsensitive)) {
            outputRecords.append(rec);
            continue;
        }

        totalCount++;
        QString call = extractAdifField(rec, "CALL");
        QString mode = extractAdifField(rec, "MODE");
        QString existingPwr = extractAdifField(rec, "TX_PWR");

        if (existingPwr.isEmpty() || existingPwr.toLower() == "n/a" || existingPwr == "0" || existingPwr.toLower() == "<nul>") {
            QString date = extractAdifField(rec, "QSO_DATE");
            QString time = extractAdifField(rec, "TIME_ON");
            QString band = extractAdifField(rec, "BAND");
            QString seed = call + date + time + band + mode;

            QString computedPwr = computeTweakPower(mode, seed);
            if (!computedPwr.isEmpty()) {
                QString newPwrTag = QString("<TX_PWR:%1>%2").arg(computedPwr.length()).arg(computedPwr);
                
                QRegularExpression txPwrRe("<TX_PWR:\\d+>[^\\s<]*", QRegularExpression::CaseInsensitiveOption);
                if (rec.contains(txPwrRe)) {
                    rec.replace(txPwrRe, newPwrTag);
                } else {
                    rec.append(" " + newPwrTag);
                }
                tweakedCount++;
                m_logArea->append(QString("QSO #%1 [%2 %3]: Set TX_PWR = %4W").arg(totalCount).arg(call, mode, computedPwr));
            }
        }
        outputRecords.append(rec);
    }

    m_tweakedAdifContent = outputRecords.join("<eor>\n");

    m_logArea->append("\n----------------------------------------");
    m_logArea->append(QString("Complete! Processed %1 QSOs, injected TX_PWR into %2 records.").arg(totalCount).arg(tweakedCount));
    m_logArea->append("Click 'Save Tweaked File As..' below to export your new ADIF log.");

    QMessageBox::information(this, "Processing Complete", QString("Batch processing complete!\n\nProcessed %1 QSOs.\nInjected TX_PWR into %2 records.\n\nClick 'Save Tweaked File As..' to choose output destination.").arg(totalCount).arg(tweakedCount));
}

void TxTweakerWidget::onSaveTweakedFileAs() {
    if (m_tweakedAdifContent.isEmpty()) {
        QMessageBox::warning(this, "No Data", "No processed ADIF data available. Please click 'Process ADIF' first.");
        return;
    }

    QString defaultName = "tweaked_log.adi";
    if (!m_lastInputFileName.isEmpty()) {
        defaultName = m_lastInputFileName;
        if (defaultName.endsWith(".adi", Qt::CaseInsensitive)) {
            defaultName.chop(4);
            defaultName += "_tweaked.adi";
        } else if (defaultName.endsWith(".adif", Qt::CaseInsensitive)) {
            defaultName.chop(5);
            defaultName += "_tweaked.adif";
        } else {
            defaultName += "_tweaked.adi";
        }
    }

    QString defaultPath = QDir::homePath() + "/" + defaultName;
    QString savePath = QFileDialog::getSaveFileName(this, "Save Tweaked File As..", defaultPath, "ADIF Files (*.adi *.adif);;All Files (*)");
    if (savePath.isEmpty()) return;

    QFile outFile(savePath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Failed to write output file to: " + savePath);
        return;
    }

    QTextStream out(&outFile);
    out << m_tweakedAdifContent;
    outFile.close();

    QMessageBox::information(this, "Success", "Tweaked ADIF file saved successfully to:\n" + savePath);
}
