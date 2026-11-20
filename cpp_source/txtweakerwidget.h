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

#ifndef TXTWEAKERWIDGET_H
#define TXTWEAKERWIDGET_H

#include <QWidget>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QSettings>

class TxTweakerWidget : public QWidget {
    Q_OBJECT
public:
    explicit TxTweakerWidget(QWidget *parent = nullptr);
    ~TxTweakerWidget() override = default;

private slots:
    void loadSettings();
    void saveSettings();
    void onBrowseInputAdif();
    void onProcessAdif();
    void onSaveTweakedFileAs();

private:
    QCheckBox *m_chkFT2;
    QSpinBox *m_ft2MinSpin;
    QSpinBox *m_ft2MaxSpin;

    QCheckBox *m_chkFT4;
    QSpinBox *m_ft4MinSpin;
    QSpinBox *m_ft4MaxSpin;

    QCheckBox *m_chkFT8;
    QSpinBox *m_ft8MinSpin;
    QSpinBox *m_ft8MaxSpin;

    QTextEdit *m_inputPathEdit;
    QTextEdit *m_logArea;
    QPushButton *m_saveSettingsBtn;
    QPushButton *m_processBtn;
    QPushButton *m_saveAsBtn;

    QString m_tweakedAdifContent;
    QString m_lastInputFileName;

    QString computeTweakPower(const QString &mode, const QString &seedStr);
    QString extractAdifField(const QString &record, const QString &fieldName);
};

#endif // TXTWEAKERWIDGET_H
