#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("QSL Template Creator");
    app.setOrganizationName("W4DJF_Software");
    
    MainWindow w;
    w.show();
    
    return app.exec();
}
