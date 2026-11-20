sed -i '/QSharedMemory sharedMemory/,/app.installEventFilter/c \
    QSharedMemory sharedMemory("QwainSingleInstanceMemoryKey");\
    if (!sharedMemory.create(1)) {\
        sharedMemory.attach();\
        sharedMemory.detach();\
        if (!sharedMemory.create(1)) {\
            QMessageBox::warning(nullptr, "Qwain", "An instance of Qwain is already running.");\
            return 0;\
        }\
    }\
\
    app.installEventFilter' cpp_source/main.cpp
