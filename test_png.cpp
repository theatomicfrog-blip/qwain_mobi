#include <QApplication>
#include <QPixmap>
#include <QImageReader>
#include <iostream>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    std::cout << "Supported formats: ";
    for (const QByteArray &fmt : QImageReader::supportedImageFormats()) {
        std::cout << fmt.constData() << " ";
    }
    std::cout << std::endl;
    QPixmap p("cpp_source/logo_QWAIN.png");
    std::cout << "isNull: " << p.isNull() << " " << p.width() << "x" << p.height() << std::endl;
    return 0;
}
