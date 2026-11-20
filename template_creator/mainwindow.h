#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QDockWidget>
#include <QAction>
#include <QMenu>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QGraphicsLineItem>

class TemplateView;
class PropertiesPanel;
class Document;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNew();
    void onOpen();
    void onSave();
    void onExportPreview();
    void onSelectionChanged();

private:
    void setupUi();
    void createActions();
    
    TemplateView *m_view;
    QGraphicsScene *m_scene;
    PropertiesPanel *m_propertiesPanel;
    Document *m_doc;

    // Crosshairs
    QGraphicsLineItem *m_crosshairH;
    QGraphicsLineItem *m_crosshairV;
    QLabel *m_cursorLabel;
    QComboBox *m_unitCombo;
    QString m_addMode;
    QString m_addArg;
    
    QAction *m_newAct;
    QAction *m_openAct;
    QAction *m_saveAct;
    QAction *m_exportAct;
};

#endif // MAINWINDOW_H
