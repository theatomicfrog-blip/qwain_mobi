sed -i '/if (!sharedMemory.create(1)) {/a \
        sharedMemory.attach();\
        sharedMemory.detach();\
        if (!sharedMemory.create(1)) { \
            QMessageBox::warning(nullptr, "Qwain", "An instance of Qwain is already running.");\
            return 0;\
        }' cpp_source/main.cpp
