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

#include <QApplication>
#include <QCoreApplication>
#include "txtweakerwidget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("TxTweaker");
    QCoreApplication::setApplicationName("TxTweaker");

    TxTweakerWidget mainWidget;
    mainWidget.setWindowTitle("TX Power Tweaker v1.0");
    mainWidget.resize(640, 720);
    mainWidget.show();

    return app.exec();
}
