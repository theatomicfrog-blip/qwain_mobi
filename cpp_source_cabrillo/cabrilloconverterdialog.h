#ifndef CABRILLOCONVERTERDIALOG_H
#define CABRILLOCONVERTERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QMap>

class CabrilloConverterDialog : public QDialog {
    Q_OBJECT

public:
    explicit CabrilloConverterDialog(QWidget *parent = nullptr);
    ~CabrilloConverterDialog() = default;

private slots:
    void onStateChanged(const QString &text);
    void onConvertClicked();
    void onUseCustomTemplateToggled(bool checked);
    void onCopyToClipboardClicked();
    void onAdifInputChanged();
    void onShowScoreClicked();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    QString performConversion();

    // Line Edits for left side (Header fields)
    QLineEdit *m_editContest;
    QLineEdit *m_editLocation;
    QLineEdit *m_editClaimedScore;
    QLineEdit *m_editOperators;
    QLineEdit *m_editName;
    QLineEdit *m_editAddress;
    QLineEdit *m_editAddressCity;
    QLineEdit *m_editAddressStateProvince;
    QLineEdit *m_editAddressPostalCode;
    QLineEdit *m_editAddressCountry;

    // Combos for fixed choices
    QComboBox *m_comboCategoryOperator;
    QComboBox *m_comboCategoryAssisted;
    QComboBox *m_comboCategoryBand;
    QComboBox *m_comboCategoryMode;
    QComboBox *m_comboCategoryPower;
    QComboBox *m_comboCategoryStation;
    QComboBox *m_comboCategoryTransmitter;

    // Panes on the right side
    QTextEdit *m_textAdifInput;
    QTextEdit *m_textCabrilloOutput;

    // Custom Template option and field
    QCheckBox *m_chkUseCustomTemplate;
    QTextEdit *m_textTemplateLayout;

    QPushButton *m_btnConvert;
    QPushButton *m_btnCopyToClipboard;
    QPushButton *m_btnShowScore;
};

#endif // CABRILLOCONVERTERDIALOG_H
