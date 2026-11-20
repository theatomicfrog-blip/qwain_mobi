sed -i '/if (splash) {/,/w.activateWindow();/c \
    if (splash) {\
        QPropertyAnimation *fadeIn = new QPropertyAnimation(splash, "windowOpacity");\
        fadeIn->setDuration(1000);\
        fadeIn->setStartValue(0.0);\
        fadeIn->setEndValue(1.0);\
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);\
        QTimer::singleShot(2500, splash, [splash, \&w]() { splash->finish(\&w); });\
        w.raise();\
        w.activateWindow();\
    }' cpp_source/main.cpp
