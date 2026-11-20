sed -i '/if (splash) {/,$d' cpp_source/main.cpp
cat << 'INNER_EOF' >> cpp_source/main.cpp
    if (splash) {
        QPropertyAnimation *fadeIn = new QPropertyAnimation(splash, "windowOpacity");
        fadeIn->setDuration(1000);
        fadeIn->setStartValue(0.0);
        fadeIn->setEndValue(1.0);
        fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
        QTimer::singleShot(2500, splash, [splash, &w]() {
            w.show();
            w.raise();
            w.activateWindow();
            splash->finish(&w);
        });
    } else {
        w.show();
        w.raise();
        w.activateWindow();
    }
    return app.exec();
}
INNER_EOF
