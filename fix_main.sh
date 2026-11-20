sed -i '/if (f.open/,/}/d' cpp_source/main.cpp
sed -i 's/app.installEventFilter/app.installEventFilter(new GlobalShortcutFilter(\&app));/g' cpp_source/main.cpp
sed -i 's/QPixmap getSplashLogo()/QPixmap getAppLogo()/g' cpp_source/main.cpp
sed -i '/QIcon getWindowIcon()/,/}/d' cpp_source/main.cpp
