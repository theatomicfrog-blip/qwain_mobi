#include "mainwindow.h"
#include "template_view.h"
#include "properties_panel.h"
#include "document.h"
#include "elements.h"
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QFileInfo>
#include <QInputDialog>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_scene = new QGraphicsScene(this);
    m_view = new TemplateView(this);
    m_view->setScene(m_scene);
    setCentralWidget(m_view);

    m_propertiesPanel = new PropertiesPanel(this);
    QDockWidget *dock = new QDockWidget("Properties", this);
    dock->setWidget(m_propertiesPanel);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    m_doc = new Document(m_scene, m_view, this);

    createActions();
    setupUi();

    connect(m_scene, &QGraphicsScene::selectionChanged, this, &MainWindow::onSelectionChanged);
    connect(m_propertiesPanel, &PropertiesPanel::elementChanged, this, [this]() {
        m_scene->update();
    });
    
    // Give scene an initial dimension
    m_view->setPaperSizeInInches(QSizeF(5.5, 3.5));
    resize(1000, 700);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    // Crosshairs
    m_crosshairH = m_scene->addLine(-1000, 0, 2000, 0, QPen(Qt::red, 1, Qt::DotLine));
    m_crosshairV = m_scene->addLine(0, -1000, 0, 2000, QPen(Qt::red, 1, Qt::DotLine));
    m_crosshairH->setZValue(100);
    m_crosshairV->setZValue(100);
    m_crosshairH->setAcceptedMouseButtons(Qt::NoButton);
    m_crosshairV->setAcceptedMouseButtons(Qt::NoButton);
    
    QToolBar *tools = addToolBar("Tools");
    
    m_unitCombo = new QComboBox(this);
    m_unitCombo->addItem("in");
    m_unitCombo->addItem("cm");
    tools->addWidget(m_unitCombo);
    
    m_cursorLabel = new QLabel("<b>X: 0.00 in &nbsp; Y: 0.00 in</b>", this);
    m_cursorLabel->setStyleSheet("QLabel { background-color: white; border: 1px solid black; padding: 5px; font-size: 16px; color: blue; }");
    tools->addWidget(m_cursorLabel);
    
    tools->addSeparator();

    connect(m_view, &TemplateView::mouseMoved, this, [this](QPointF scenePos) {
        m_crosshairH->setLine(-1000, scenePos.y(), 2000, scenePos.y());
        m_crosshairV->setLine(scenePos.x(), -1000, scenePos.x(), 2000);
        double valX = qAbs(scenePos.x()) / 96.0;
        double valY = qAbs(scenePos.y()) / 96.0;
        QString unit = m_unitCombo->currentText();
        if (unit == "cm") {
            valX *= 2.54;
            valY *= 2.54;
        }
        m_cursorLabel->setText(QString("<b>X: %1 %3 &nbsp;&nbsp; Y: %2 %3</b>").arg(valX, 0, 'f', 2).arg(valY, 0, 'f', 2).arg(unit));
    });

    connect(m_view, &TemplateView::mouseClicked, this, [this](QPointF scenePos) {
        if (!m_addMode.isEmpty()) {
            if (m_addMode == "TEXT") {
                TextElement *el = new TextElement();
                el->setPos(scenePos);
                m_scene->addItem(el);
            } else if (m_addMode == "TABLE") {
                TableElement *el = new TableElement();
                el->setPos(scenePos);
                m_scene->addItem(el);
            } else if (m_addMode == "REF_POINT") {
                RefElement *el = new RefElement();
                el->setElementId(m_addArg);
                el->setPos(scenePos.x() - el->rect().width() / 2.0, scenePos.y() - el->rect().height() / 2.0);
                m_scene->addItem(el);
            } else if (m_addMode == "ADDRESS_BOX") {
                TextElement *el = new TextElement();
                el->setText("Full Contact Name\nAddress Line 2\nCity, State, Zip\n<div align='right'>Country</div>");
                el->setElementId("ADDRESS_BOX");
                el->setRect(QRectF(0, 0, 250, 120));
                el->setPos(scenePos.x() - el->rect().width() / 2.0, scenePos.y() - el->rect().height() / 2.0);
                m_scene->addItem(el);
            } else if (m_addMode == "CHK_CUSTOM") {
                TextElement *el = new TextElement();
                el->setText("\u2714"); // Checkmark character
                el->setElementId("CHK_CUSTOM");
                el->setPos(scenePos.x() - el->rect().width() / 2.0, scenePos.y() - el->rect().height() / 2.0);
                m_scene->addItem(el);
            }
            
            QApplication::restoreOverrideCursor();
            m_addMode.clear();
        }
    });

    QAction *addText = tools->addAction("Add Text Element");
    connect(addText, &QAction::triggered, this, [this](){
        m_addMode = "TEXT";
        QApplication::setOverrideCursor(Qt::CrossCursor);
    });
    
    QAction *addTable = tools->addAction("Add QSO Table");
    connect(addTable, &QAction::triggered, this, [this](){
        m_addMode = "TABLE";
        QApplication::setOverrideCursor(Qt::CrossCursor);
    });
    
    QAction *addRef = tools->addAction("Add Orientation Mark");
    connect(addRef, &QAction::triggered, this, [this](){
        int refCount = 0;
        for (QGraphicsItem *item : m_scene->items()) {
            if (dynamic_cast<RefElement*>(item)) refCount++;
        }
        if (refCount >= 2) {
            QMessageBox::warning(this, "Max Ref Points", "You can only place 2 reference points.");
            return;
        }
        m_addMode = "REF_POINT";
        m_addArg = "REF_POINT_" + QString::number(refCount + 1);
        QApplication::setOverrideCursor(Qt::CrossCursor);
    });
    
    QAction *addAddress = tools->addAction("Add Address Box");
    connect(addAddress, &QAction::triggered, this, [this](){
        m_addMode = "ADDRESS_BOX";
        QApplication::setOverrideCursor(Qt::CrossCursor);
    });
    
    QAction *addChk = tools->addAction("Add Custom Checkmark");
    connect(addChk, &QAction::triggered, this, [this](){
        m_addMode = "CHK_CUSTOM";
        QApplication::setOverrideCursor(Qt::CrossCursor);
    });

    QAction *delItem = tools->addAction("Delete Selected");
    connect(delItem, &QAction::triggered, this, [this](){
        QList<QGraphicsItem*> sel = m_scene->selectedItems();
        for (auto item : sel) {
            m_scene->removeItem(item);
            delete item;
        }
    });

    tools->addSeparator();

    QAction *setBg = tools->addAction("Set Background...");
    connect(setBg, &QAction::triggered, this, [this](){
        QSettings settings;
        QString lastDir = settings.value("LastImageDir", "").toString();
        QString file = QFileDialog::getOpenFileName(this, "Select Image", lastDir, "Images (*.png *.jpg *.jpeg)");
        if (!file.isEmpty()) {
            settings.setValue("LastImageDir", QFileInfo(file).absolutePath());
            m_view->setBackgroundImage(file);
        }
    });

    QAction *toggleBg = tools->addAction("Toggle Background");
    connect(toggleBg, &QAction::triggered, this, [this](){
        m_view->setBgOpacity(m_view->bgOpacity() > 0.0 ? 0.0 : 1.0);
    });

    QAction *setPaperSize = tools->addAction("Set Paper Size");
    connect(setPaperSize, &QAction::triggered, this, [this](){
        bool okX, okY;
        double w = QInputDialog::getDouble(this, "Paper Width", "Width (inches):", m_view->paperSizeInInches().width(), 1.0, 20.0, 2, &okX);
        if (okX) {
            double h = QInputDialog::getDouble(this, "Paper Height", "Height (inches):", m_view->paperSizeInInches().height(), 1.0, 20.0, 2, &okY);
            if (okY) {
                m_view->setPaperSizeInInches(QSizeF(w, h));
            }
        }
    });

    QLabel *opacityLbl = new QLabel(" Bg Opacity: ", this);
    tools->addWidget(opacityLbl);

    QSlider *opacitySlider = new QSlider(Qt::Horizontal, this);
    opacitySlider->setRange(0, 100);
    opacitySlider->setValue(100);
    opacitySlider->setMaximumWidth(100);
    tools->addWidget(opacitySlider);
    
    connect(opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_view->setBgOpacity(value / 100.0);
    });

    // We can also have it sync from m_view when loading a document,
    // but the slider is just a simple tool for now. To make it totally sync:
    connect(m_view, &TemplateView::opacityChanged, opacitySlider, [opacitySlider](double op) {
        opacitySlider->setValue(op * 100);
    });
}

void MainWindow::createActions() {
    QMenu *fileMenu = menuBar()->addMenu("&File");
    
    m_newAct = fileMenu->addAction("&New", this, &MainWindow::onNew, QKeySequence::New);
    m_openAct = fileMenu->addAction("&Open...", this, &MainWindow::onOpen, QKeySequence::Open);
    m_saveAct = fileMenu->addAction("&Save...", this, &MainWindow::onSave, QKeySequence::Save);
    fileMenu->addSeparator();
    m_exportAct = fileMenu->addAction("Export Preview Image", this, &MainWindow::onExportPreview);
}

void MainWindow::onNew() {
    for (QGraphicsItem *item : m_scene->items()) {
        if (dynamic_cast<BaseElement*>(item)) {
            m_scene->removeItem(item);
            delete item;
        }
    }
    m_view->setBackgroundImage("");
}

void MainWindow::onOpen() {
    QSettings settings;
    QString lastDir = settings.value("LastOpenedDir", "").toString();
    QString file = QFileDialog::getOpenFileName(this, "Open Template", lastDir, "JSON Files (*.json)");
    if (!file.isEmpty()) {
        settings.setValue("LastOpenedDir", QFileInfo(file).absolutePath());
        if (!m_doc->load(file)) {
            QMessageBox::warning(this, "Error", "Could not load template file.");
        }
    }
}

void MainWindow::onSave() {
    QSettings settings;
    QString lastDir = settings.value("LastOpenedDir", "").toString();
    QString file = QFileDialog::getSaveFileName(this, "Save Template", lastDir, "JSON Files (*.json)");
    if (!file.isEmpty()) {
        settings.setValue("LastOpenedDir", QFileInfo(file).absolutePath());
        if (!m_doc->save(file)) {
            QMessageBox::warning(this, "Error", "Could not save template file.");
        }
    }
}

void MainWindow::onExportPreview() {
    m_scene->clearSelection();
    
    QRectF envelopeRect = m_scene->sceneRect();
    RefElement *ref1 = nullptr, *ref2 = nullptr;
    for (QGraphicsItem *item : m_scene->items()) {
        if (RefElement *a = dynamic_cast<RefElement*>(item)) {
            if (!ref1) ref1 = a;
            else if (!ref2) ref2 = a;
        }
    }
    
    if (ref1 && ref2) {
        QPointF p1 = ref1->mapToScene(ref1->rect().center());
        QPointF p2 = ref2->mapToScene(ref2->rect().center());
        double minX = qMin(p1.x(), p2.x());
        double minY = qMin(p1.y(), p2.y());
        double maxX = qMax(p1.x(), p2.x());
        double maxY = qMax(p1.y(), p2.y());
        double pad = 0.1 * 96.0;
        envelopeRect = QRectF(minX - pad, minY, (maxX - minX) + 2 * pad, maxY - minY);
    }
    
    QImage img(envelopeRect.size().toSize(), QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter painter(&img);
    // Draw background manually to the image
    QRectF paperRect = m_scene->sceneRect();
    if (!m_view->backgroundImage().isEmpty()) {
        QPixmap pm(m_view->backgroundImage());
        if (!pm.isNull()) {
            painter.setOpacity(m_view->bgOpacity());
            QRectF targetBgRect(paperRect.x() - envelopeRect.x(), paperRect.y() - envelopeRect.y(), paperRect.width(), paperRect.height());
            painter.drawPixmap(targetBgRect, pm, QRectF(pm.rect()));
            painter.setOpacity(1.0);
        }
    } else {
        painter.fillRect(img.rect(), Qt::white);
    }
    
    // Hide RefElements before rendering
    if (ref1) ref1->hide();
    if (ref2) ref2->hide();
    
    m_scene->render(&painter, QRectF(), envelopeRect);
    
    if (ref1) ref1->show();
    if (ref2) ref2->show();
    
    QString file = QFileDialog::getSaveFileName(this, "Export Preview", "", "Images (*.png)");
    if (!file.isEmpty()) {
        img.save(file);
    }
}

void MainWindow::onSelectionChanged() {
    QList<QGraphicsItem*> sel = m_scene->selectedItems();
    if (sel.isEmpty()) {
        m_propertiesPanel->setElement(nullptr);
    } else {
        BaseElement *el = dynamic_cast<BaseElement*>(sel.first());
        if (el) m_propertiesPanel->setElement(el);
    }
}
