#ifndef ADDRESSRENDERINSPECTORWINDOW_H
#define ADDRESSRENDERINSPECTORWINDOW_H

#include <QDialog>
#include <QTextBrowser>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>

class AddressRenderInspectorWindow : public QDialog {
    Q_OBJECT
public:
    explicit AddressRenderInspectorWindow(const QString &callsign, const QString &templateName,
                                          const QString &addrName, const QString &addrStreet,
                                          const QString &addrCity, const QString &addrCountry,
                                          QGraphicsScene *previewScene, QWidget *parent = nullptr);

private:
    void buildUi();
    void runDiagnosticTrace();

    QString m_callsign;
    QString m_templateName;
    QString m_addrName;
    QString m_addrStreet;
    QString m_addrCity;
    QString m_addrCountry;
    QGraphicsScene *m_sourceScene;

    QTextBrowser *m_rawBrowser;
    QTextBrowser *m_formattedBrowser;
    QTextBrowser *m_templateBreakdownBrowser;
    QGraphicsView *m_liveView;
    QGraphicsScene *m_liveScene;
    QTextBrowser *m_feedbackBrowser;
};

#endif // ADDRESSRENDERINSPECTORWINDOW_H
