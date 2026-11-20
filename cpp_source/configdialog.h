// All rights reserved. Commercial use, redistribution, modification, or incorporation into any product or service requires prior written permission.
// 
// This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
// ============================================================================
// COMPONENT: Configuration & Settings Dialog (configdialog.h)
// ROLE: Provides workspace configuration preferences tab views.
//       Manages station callsign/grid settings, QRZ API credentials, UI fonts,
//       and the direct [Backup Conf] button for defined configuration files.
// INVOKED BY: MainWindow Settings menu.
// PRODUCES: Updated QSettings values saved to Qwain.conf.
// ============================================================================

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>
#include <QString>

class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QCheckBox;
class QComboBox;
class QLabel;
class QTextEdit;
class QTextBrowser;
class QGraphicsPixmapItem;
class QGraphicsScene;
class QGraphicsView;
class QButtonGroup;
class QFrame;
#include <QFont>
#include <QColor>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <functional>
#include <vector>

class ConfigDialog : public QDialog
{
    Q_OBJECT

public:
    ConfigDialog(QWidget *parent = nullptr);

public slots:
    void applySettings();
    void accept() override;
signals:
    void settingsApplied();

private slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);
    void chooseSkinColor();
    void testQrzLogin();
    void showSeed();
    void prepNewLogfile();
    void forceSyncLogfile();
    void startMailCheck();
    void filterLotwDatabase();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void createStationPage();
    
    QNetworkAccessManager *m_netManager;
    QListWidget *m_contentsWidget;
    QStackedWidget *m_pagesWidget;
    
    QLineEdit *m_stationCallsign;
    QListWidget *m_stationRadios;
    QListWidget *m_stationAntennas;
    QListWidget *m_stationBands;
    QListWidget *m_stationChecks;
    QListWidget *m_stationAdifTags;

    QComboBox *m_protocolCombo;
    QLineEdit *m_ipEdit;
    QSpinBox *m_portSpin;
    QLineEdit *m_logfileEdit;
    QLineEdit *m_userDirectoryEdit;
    QLineEdit *m_systemLogfileEdit;
    QLineEdit *m_debugLogfileEdit;
    QLineEdit *m_lotwCsvUrlEdit;
    QLineEdit *m_ctyDatUrlEdit;
    QLabel *m_lotwInfoLabel;
    QSpinBox *m_lotwDaysKeepSpin;
    
    QLineEdit *m_qrzUserEdit;
    QLineEdit *m_qrzPassEdit;
    QLineEdit *m_qrzApiKeyEdit;
    QCheckBox *m_qrzXmlSubCheckbox;
    QCheckBox *m_arrlMemberCheckbox;
    QLineEdit *m_lotwUserEdit;
    QLineEdit *m_lotwPassEdit;
    QLineEdit *m_clublogEmailEdit;
    QLineEdit *m_clublogCallEdit;
    QLineEdit *m_clublogPassEdit;
    QLineEdit *m_clublogApiKeyEdit;
    QLineEdit *m_eqslUserEdit;
    QLineEdit *m_eqslPassEdit;
    QLineEdit *m_gridEdit;
    QLineEdit *m_configDirEdit;
    
    QCheckBox *m_feed1Check;
    QLineEdit *m_feed1UrlEdit;
    QComboBox *m_feed1RefreshCombo;
    
    QCheckBox *m_feed2Check;
    QLineEdit *m_feed2UrlEdit;
    QComboBox *m_feed2RefreshCombo;
    
    QCheckBox *m_feed3Check;
    QLineEdit *m_feed3UrlEdit;
    QComboBox *m_feed3RefreshCombo;

    QLineEdit *m_dxClusterEdit;
    QComboBox *m_dxClusterCombo;
    QListWidget *m_dxClusterListWidget;
    QPushButton *m_btnRemoveCluster;
    QLineEdit *m_clusterIpEdit;
    QSpinBox *m_clusterPortSpin;
    QLineEdit *m_clusterStationIdEdit;
    QLineEdit *m_clusterCommentEdit;
    QPushButton *m_btnAddCluster;
    
    QComboBox *m_scrollDirCombo1;
    class QSlider *m_scrollSpeedSlider1;
    QComboBox *m_scrollDirCombo2;
    class QSlider *m_scrollSpeedSlider2;
    QComboBox *m_scrollDirCombo3;
    class QSlider *m_scrollSpeedSlider3;
    
    QLabel *m_currentColorLabel;
    QString m_selectedColor;
    QString m_initialSelectedColor;
    QComboBox *m_mapStyleCombo;
    QSpinBox *m_statusScrollLimit;
    QCheckBox *m_alwaysEasternCheck;
    QComboBox *m_schemeCombo;
    QPushButton *m_schemeButtonPreview;
    QTextBrowser *m_uiPreview = nullptr;
    int m_currentPreviewPane = 0;
    
    QFont m_clockFont, m_initialClockFont;
    QColor m_clockColor, m_initialClockColor;
    QFont m_feed1Font, m_initialFeed1Font;
    QColor m_feed1Color, m_initialFeed1Color;
    QFont m_feed2Font, m_initialFeed2Font;
    QColor m_feed2Color, m_initialFeed2Color;
    QFont m_feed3Font, m_initialFeed3Font;
    QColor m_feed3Color, m_initialFeed3Color;
    QFont m_callInfoFont, m_initialCallInfoFont;
    QColor m_callInfoColor, m_initialCallInfoColor;
    QFont m_qsoListFont, m_initialQsoListFont;
    QColor m_qsoListColor, m_initialQsoListColor;
    
    void createIcons();
    void createNetworkPage();
    void createCredentialsPage();
    void createFeedsPage();
    void createLoggingPage();
    void createUIPage();
    void createBackupPage();
    void createPrintersPage();
    void createQslCardOptionsPage();
    void updateUIPreview();
    void updatePreviewButtonStyle();
    void updateMiniConsolePreview();
    void updatePrintersList();
    void updateBackupInfoLabels();

    // Modern Radio Workstation Appearance widgets
    QButtonGroup *m_schemeButtonGroup = nullptr;
    QPushButton *m_tileLight = nullptr;
    QPushButton *m_tileDark = nullptr;
    QPushButton *m_tileNative = nullptr;
    QPushButton *m_tileCustom = nullptr;
    QFrame *m_swatchFrame = nullptr;
    QWidget *m_miniPreviewWorkspace = nullptr;
    QLabel *m_miniClockLabel = nullptr;
    QLabel *m_miniCallLabel = nullptr;
    QLabel *m_miniQsoLabel = nullptr;
    
    QLabel *m_lblDate = nullptr;
    QLabel *m_lblContacts = nullptr;
    QLabel *m_lblImages = nullptr;
    QLabel *m_lblFiles = nullptr;
    QLabel *m_lblCompressedSize = nullptr;
    QLabel *m_lblUncompressedSize = nullptr;
    QLabel *m_lblRatio = nullptr;
    QLabel *m_lblMd5 = nullptr;
    
    QListWidget *m_printersList;
    
    // QSL Card Options UI
    QCheckBox *m_chkUseTweaks = nullptr;
    QCheckBox *m_chkFT2Tweaks = nullptr;
    QCheckBox *m_chkFT4Tweaks = nullptr;
    QCheckBox *m_chkFT8Tweaks = nullptr;
    QSpinBox *m_ft2MinSpin = nullptr;
    QSpinBox *m_ft2MaxSpin = nullptr;
    QSpinBox *m_ft4MinSpin = nullptr;
    QSpinBox *m_ft4MaxSpin = nullptr;
    QSpinBox *m_ft8MinSpin = nullptr;
    QSpinBox *m_ft8MaxSpin = nullptr;
    QCheckBox *m_chkUseStandardEmissions = nullptr;
    QCheckBox *m_chkUseGenericEmissions = nullptr;
    QCheckBox *m_chkUseQrpSubstitution = nullptr;
    QCheckBox *m_chkCustomQrpBrag = nullptr;
    QLineEdit *m_txtCustomQrpBragText = nullptr;
    QLabel *m_lblCurrentlySubstitutes = nullptr;

    std::vector<std::function<void()>> m_styleLabelRefreshers;
};

#endif // CONFIGDIALOG_H
