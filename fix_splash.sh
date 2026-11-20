sed -i '199,212c\
    if (splash) {\
        splash->finish(&w);\
        w.raise();\
        w.activateWindow();\
    }' cpp_source/main.cpp
