sed -i 's/QPixmap getAppLogo()/QPixmap getSplashLogo()/g' cpp_source/main.cpp
sed -i '/QPixmap getSplashLogo/i \
QIcon getWindowIcon() {\
    QIcon icon(":/logo.png");\
    if (icon.isNull()) icon = QIcon("logo.png");\
    if (icon.isNull()) icon = QIcon("arrl_small.png");\
    return icon;\
}\
' cpp_source/main.cpp
