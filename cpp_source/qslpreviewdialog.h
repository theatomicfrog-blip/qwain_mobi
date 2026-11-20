// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

#ifndef QSLPREVIEWDIALOG_H
#define QSLPREVIEWDIALOG_H

#include <QDialog>
#include <QGraphicsView>
#include <functional>
#include <QGraphicsScene>
#include <QTextBrowser>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QLabel>
#include <QListWidget>
#include <QSettings>
#include <QVariantMap>
#include <QDoubleSpinBox>
#include <QHash>
#include <QGroupBox>
#include <QFormLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QSlider>
#include <QComboBox>
#include <QToolButton>
#include <QPointer>
#include "mainwindow.h" // for QsoDetail


class QslPreviewDialog : public QDialog {
    Q_OBJECT
    friend class FinalTweaksDialog;
    friend class PrintExceptionDialog;
public:
    explicit QslPreviewDialog(const QString &callsign, const QString &infoHtml, const QList<QsoDetail> &originalQsos, QWidget *parent = nullptr, const QString &printMode = "prompt");
    ~QslPreviewDialog();
    void setPrintQueue(const QStringList &queue, class MainWindow *mainWnd);
    QStringList getRemainingQueue() const;
    void setPreviewState(const QVariantMap &state);
    QVariantMap getPreviewState() const;
    
    QString getSelectedCardTemplate() const;
    QString getSelectedEnvTemplate() const;
    bool getPrintCard() const;
    bool getPrintEnvelope() const;
    void setSelectedOptions(const QString &cardTemp, const QString &envTemp, bool pCard, bool pEnv);
    bool performPrint(bool isEnvelope); // Expose if we want to call it directly? Or use a new method
    void doBatchPrint();
    void showPreviewWindowOnly();
    void setDataOnly(bool dataOnly);
    void setCurrentTemplate(const QString &name) { m_currentTemplate = name; }
    
    
protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    
private slots:

    void onTemplateChosen(const QString& name);
    void updatePreview();
    void openAddressTestDialog();
    void changeAddressFont();
    void changeCommentFont();
    void changeCallFont();
    void undo();
    void resetToDefault();
    void onSendToPrinterClicked();
    void processNextBatchItem();
    void doContinuousBatchPrint();
    void printQsl();
    void refreshContactsTable(const QString &mode = "unprinted");
    void onLogfileClicked(const QString &link);

private:
    class QTextBrowser *m_diagLogBrowser = nullptr;
    void logDiag(const QString &msg);
    void refreshAddressData(const QString& callsign);
    void updateThumbnail(const QString& name);
    QGraphicsScene *m_thumbnailScene = nullptr;
    QGraphicsView *m_thumbnailView = nullptr;
    void loadTemplate(const QString &name);
    void renderTemplate();
    void saveStateForUndo();
    void updateQrzLogForSelected(const QList<QsoDetail> &selectedList = QList<QsoDetail>());
    void onQrzLogUpdateReply();
    void saveUniqueTweaks();
    int calculateMaxContactsForCardTemplate(const QString &cardTemplateName);
    // Explicitly pass target template to guarantee template integrity and prevent template mixup/swapping
    void spoolToPrinter(bool isEnvelope, QString selectedPrinter, const QPixmap &previewPixmap, const QString &jobTemplate = QString());

    QString m_callsign;
    QString m_infoHtml;
    QList<QsoDetail> m_qsos;
    QStringList m_printQueue;
    int m_printQueueIndex = 0;
    class MainWindow *m_mainWnd = nullptr;
    QGraphicsScene *m_scene = nullptr;
    QGraphicsView *m_view = nullptr;
    QTableWidget *m_contactsTable = nullptr;
    QLabel *m_lblLogfile = nullptr;
    QListWidget *m_templateList = nullptr;
    QListWidget *m_envTemplateList = nullptr;
    bool m_previewingEnvelope = false;
    QWidget *m_previewWindow = nullptr;
    class QNetworkAccessManager *m_netManager = nullptr;
    
    QDoubleSpinBox *m_topMarginSpin = nullptr;
    QDoubleSpinBox *m_leftMarginSpin = nullptr;
    QCheckBox *m_chkUsePrintTweaks = nullptr;
    
    QString m_currentTemplate;
    QList<QVariantMap> m_templateItems;
    QString m_bgImagePath;
    double m_paperW;
    double m_paperH;
    QString m_printerName;
    int m_printCorrection;
    double m_printOffsetX;
    double m_printOffsetY;
    
    QString m_addrName;
    QString m_addrStreet;
    QString m_addrCity;
    QString m_addrCountry;
    bool m_dataOnly = false;
    
    QPushButton *m_btnUndo = nullptr;
    QPushButton *m_btnReset = nullptr;
    QList<QMap<QString, QVariant>> m_undoStack;

    int m_currentCardIndex = 0;
    QGroupBox *m_chkOptionsGroup = nullptr;
    QCheckBox *m_chkComment = nullptr;
    QPushButton *m_btnCommentEditor = nullptr;
    QCheckBox *m_chkQRZ = nullptr;
    QCheckBox *m_chkEqsl = nullptr;
    QPushButton *m_btnAddressFont = nullptr;
    QPushButton *m_btnCommentFont = nullptr;
    QPushButton *m_btnCallFont = nullptr;
    QPushButton *m_btnFreqBandSwitch = nullptr;
    bool m_showFreq = false;
    bool templateHasFreqOrBand() const;
    QLabel *m_lblCardsUsed = nullptr;
    QSlider *m_chkSizeSlider = nullptr;
    QCheckBox *m_chkPrintCard = nullptr;
    QCheckBox *m_chkPrintEnvelope = nullptr;
    QCheckBox *m_chkPrintReverse = nullptr;
    QPushButton *m_btnPrint = nullptr;
    
    int m_totalCardsSentToPrinter = 0;
    bool m_anyCardJobUsedMultipleCards = false;
    
    enum class BatchSequence { CardEnvelope, CardsThenEnvelopes };
    enum class BatchPacing { Continuous, OneAtATime };
    BatchSequence m_batchSequence = BatchSequence::CardEnvelope;
    BatchPacing m_batchPacing = BatchPacing::OneAtATime;
    bool m_batchPrintingEnvelopesPhase = false;
    bool m_batchContinuousPrinting = false;
    bool m_currentItemPrinted = false;
    QString m_batchSelectedPrinter;
    bool m_batchConfirmedCard = false;
    bool m_batchConfirmedEnv = false;
    QComboBox *m_chkTypeCombo = nullptr;
    
    void updateSendToPrinterButtonText();
    
    
    QMap<QString, bool> m_customChkBoxStates;
    QMap<QString, QCheckBox*> m_customChkBoxes;
    
    QWidget *m_blankFieldsWidget = nullptr;
    QHBoxLayout *m_blankFieldsLayout = nullptr;
    QPushButton *m_btnEnterMissingData = nullptr;
    QMap<QString, QString> m_manualFieldData;
    bool m_addressManuallyAccepted = false;
    
    void updateCustomCheckmarks();
    void promptMissingAddressData();
    void onFeedbackClicked();
    bool eventFilter(QObject *watched, QEvent *event) override;
    QHash<QString, QList<QsoDetail>> m_cachedQueueQsos;
    QCheckBox *m_chkCardThenEnv = nullptr;
    QCheckBox *m_chkAllCardsThenEnvs = nullptr;
    QCheckBox *m_chkPacingOneAtATime = nullptr;
    QCheckBox *m_chkPacingContinuous = nullptr;
    QComboBox *m_comboCardPrinter = nullptr;
    QComboBox *m_comboEnvPrinter = nullptr;
    QGroupBox *m_printingPrefsGroup = nullptr;
    QToolButton *m_btnHelp = nullptr;
    
    void savePrintingPreferences();
    void loadPrintingPreferences();
    void onPanicTriggered();
    
    QList<QsoDetail> loadQsosFromDefinedLogfiles(const QString &callsign);
    
    QSpinBox *m_spinPacingDelay = nullptr;
    QPointer<class UnifiedPrintDialog> m_unifiedPrintDlg;
    
    bool m_batchPrintingOverflowPhase = false;
    bool m_isProgrammaticTemplateChange = false;
    QStringList m_overflowQueue;
    QStringList m_savedOriginalQueue;
    int m_savedOriginalQueueIndex = 0;
    QString m_savedOriginalTemplate;
    bool checkAndTransitionToOverflow();
    void finishOverflowPhase();
    QString getAssociatedOverflowTemplate() const;
    bool isOverflowSupportActive() const;

    // OVERFLOW TEMPLATE PROMPT & SELECTION
    // Purpose: When contacts to be printed exceed regular template capacity,
    //          tracks user decision (use overflow template vs continue with regular template)
    //          and provides interactive prompt.
    enum class OverflowChoice {
        Unset,
        UseOverflowTemplate,
        ContinueRegularTemplate
    };
    OverflowChoice m_batchOverflowChoice = OverflowChoice::Unset;
    bool m_userDeclinedOverflow = false;
    bool promptUserForOverflowTemplate(int contactCount, int maxCapacity, const QString &cardTemplate);

    bool m_hasTextOverride = false;
    bool m_hasFontOverride = false;
    bool m_forceDisableAutoShrink = false;
    QString m_overrideFieldId;
    QString m_textOverride;
    QFont m_fontOverride;
    bool m_hasPosOverride = false;
    qreal m_overrideX = 0;
    qreal m_overrideY = 0;
    // Country element overrides for PrintExceptionDialog
    bool m_hasCountryPosOverride = false;
    qreal m_countryOverrideX = 0;
    qreal m_countryOverrideY = 0;
    bool m_hasCountryTextOverride = false;
    QString m_countryTextOverride;
    bool m_hasCountryFontOverride = false;
    QFont m_countryFontOverride;
    QString m_countryFieldId;

    QList<QPair<QString, double>> m_detectedClippingFields;
    QMap<QString, QString> m_clippingFieldTexts;
    QMap<QString, QFont> m_clippingFieldFonts;
    QPixmap grabOverridePixmap(const QString &fieldId, const QString &text, const QFont &font, bool hasText, bool hasFont, bool disableAutoShrink);
};

#include <QRadioButton>
#include <QLabel>
#include <QTextEdit>
#include <QFontComboBox>
#include <QSpinBox>
#include <QButtonGroup>
#include <QPushButton>

class PrintExceptionDialog : public QDialog {
    Q_OBJECT
public:
    PrintExceptionDialog(QslPreviewDialog *parentDialog, const QString &fieldId, const QString &initialText, const QFont &initialFont);
    
    int selectedOption() const { return m_selectedOption; }
    QString getOverrideText() const { return m_overrideText; }
    QFont getOverrideFont() const { return m_overrideFont; }
    bool hasPosOverride() const { return m_hasPosOverride; }
    qreal overrideX() const { return m_overrideX; }
    qreal overrideY() const { return m_overrideY; }

    // Country element override getters
    bool hasCountryPosOverride() const { return m_hasCountryPosOverride; }
    qreal countryOverrideX() const { return m_countryOverrideX; }
    qreal countryOverrideY() const { return m_countryOverrideY; }
    bool hasCountryTextOverride() const { return m_hasCountryTextOverride; }
    QString getCountryOverrideText() const { return m_countryText; }
    bool hasCountryFontOverride() const { return m_hasCountryFontOverride; }
    QFont getCountryOverrideFont() const { return m_countryFont; }
    QString countryFieldId() const { return m_countryFieldId; }
    
private slots:
    void onCustomChanged();
    void onProceedClicked();
    void onPostponeClicked();
    void onCancelClicked();
    void onTargetElementToggled(int id);
    
private:
    void generatePreviews();
    
protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    
    QslPreviewDialog *m_parent;
    QString m_fieldId;
    QFont m_currentFont;
    int m_selectedOption = 1;
    QString m_overrideText;
    QFont m_overrideFont;
    bool m_hasPosOverride = false;
    qreal m_overrideX = 0;
    qreal m_overrideY = 0;
    bool m_isDragging = false;
    QPoint m_dragStartMousePos;
    QPointF m_dragStartOverridePos;

    // Element selection state: 1 = Address Box, 2 = Country Box
    int m_targetElement = 1;
    bool m_hasCountryElement = false;
    QString m_countryFieldId;
    QString m_addressText;
    QFont m_addressFont;
    QString m_countryText;
    QFont m_countryFont;
    bool m_hasCountryPosOverride = false;
    qreal m_countryOverrideX = 0;
    qreal m_countryOverrideY = 0;
    bool m_hasCountryTextOverride = false;
    bool m_hasCountryFontOverride = false;
    
    QRadioButton *m_radio1 = nullptr;
    QRadioButton *m_radio2 = nullptr;
    QRadioButton *m_radio3 = nullptr;
    QLabel *m_lbl1 = nullptr;
    QLabel *m_lbl2 = nullptr;
    QLabel *m_lbl3 = nullptr;
    QLabel *m_preview1 = nullptr;
    QLabel *m_preview2 = nullptr;
    QLabel *m_preview3 = nullptr;
    QTextEdit *m_textEdit = nullptr;
    QFontComboBox *m_fontCombo = nullptr;
    QSpinBox *m_spinFontSize = nullptr;
    QButtonGroup *m_btnGroup = nullptr;

    // Target element radio buttons in Option 3
    QWidget *m_elemSelectWidget = nullptr;
    QRadioButton *m_radioElemAddress = nullptr;
    QRadioButton *m_radioElemCountry = nullptr;
    QButtonGroup *m_elemGroup = nullptr;

    QPushButton *m_btnProceed = nullptr;
    QPushButton *m_btnPostpone = nullptr;
    QPushButton *m_btnCancel = nullptr;
};

#endif
