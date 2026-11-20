sed -i '/QApplication app(argc, argv);/a \    printf("main started!\\n"); fflush(stdout);' cpp_source/main.cpp
cd build && make -j$(nproc) && xvfb-run ./Qwain --diag > /tmp/qwain.log 2>&1 &
sleep 5
cat /tmp/qwain.log
