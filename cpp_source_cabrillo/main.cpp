#include <QApplication>
#include "cabrilloconverterdialog.h"

int main(int argc, char *argv[]) {
    // Enable High DPI scaling if supported in older Qt versions, or handled automatically in Qt 6
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    #endif

    QApplication app(argc, argv);
    
    // Set organization and application details so settings persist correctly via QSettings
    QCoreApplication::setOrganizationName("Qwain");
    QCoreApplication::setApplicationName("CabrilloConverter");
    
    CabrilloConverterDialog dialog;
    // Show as a standalone window
    dialog.show();
    
    return app.exec();
}
