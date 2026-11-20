cmake_minimum_required(VERSION 3.10)
project(Qwain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Step 1: Search for mandatory Core and Widgets modules for Qt5 or Qt6
find_package(Qt5 COMPONENTS Core Widgets QUIET)
if (Qt5_FOUND)
    set(QT_VERSION_MAJOR 5)
else()
    find_package(Qt6 COMPONENTS Core Widgets QUIET)
    if (Qt6_FOUND)
        set(QT_VERSION_MAJOR 6)
    endif()
endif()

# Step 2: Fallback MXE cross-compilation prefix path detection
if (NOT Qt5_FOUND AND NOT Qt6_FOUND)
    file(GLOB MXE_PREFIX_PATHS 
        "/usr/lib/mxe/usr/*"
        "/opt/mxe/usr/*"
        "$ENV{HOME}/mxe/usr/*"
        "$ENV{HOME}/mxe/usr/*/qt5"
        "$ENV{HOME}/mxe/usr/*/qt6"
    )
    foreach(mxe_dir ${MXE_PREFIX_PATHS})
        if (IS_DIRECTORY "${mxe_dir}")
            list(APPEND CMAKE_PREFIX_PATH "${mxe_dir}")
        endif()
    endforeach()

    find_package(Qt5 COMPONENTS Core Widgets QUIET)
    if (Qt5_FOUND)
        set(QT_VERSION_MAJOR 5)
    else()
        find_package(Qt6 COMPONENTS Core Widgets QUIET)
        if (Qt6_FOUND)
            set(QT_VERSION_MAJOR 6)
        endif()
    endif()
endif()

if (NOT Qt5_FOUND AND NOT Qt6_FOUND)
    message(FATAL_ERROR "\nCould not find Qt5 or Qt6 development libraries for the target architecture.\n\nWhen cross-compiling for Windows with MXE, verify your MXE Qt installation path or pass CMAKE_PREFIX_PATH:\n  cmake -B build -DCMAKE_PREFIX_PATH=/home/w4djf/mxe/usr/x86_64-w64-mingw32.shared/qt5\n")
endif()

# Step 3: Discover optional modules individually via Qt5<Module> / Qt6<Module> so missing packages do not break build configuration
set(QT_LIBRARIES "")

if (QT_VERSION_MAJOR EQUAL 5)
    list(APPEND QT_LIBRARIES Qt5::Core Qt5::Widgets)
    
    foreach(comp Network Svg PrintSupport Sql OpenGL XmlPatterns TextToSpeech)
        find_package(Qt5${comp} QUIET)
        if (Qt5${comp}_FOUND)
            list(APPEND QT_LIBRARIES Qt5::${comp})
            if ("${comp}" STREQUAL "TextToSpeech")
                add_compile_definitions(HAVE_TEXT_TO_SPEECH)
            endif()
        else()
            if ("${comp}" STREQUAL "XmlPatterns")
                add_compile_definitions(QT_NO_XMLPATTERNS)
            endif()
        endif()
    endforeach()
else()
    list(APPEND QT_LIBRARIES Qt6::Core Qt6::Widgets)
    add_compile_definitions(QT_NO_XMLPATTERNS)
    
    foreach(comp Network Svg SvgWidgets PrintSupport Sql OpenGL OpenGLWidgets TextToSpeech)
        find_package(Qt6${comp} QUIET)
        if (Qt6${comp}_FOUND)
            list(APPEND QT_LIBRARIES Qt6::${comp})
            if ("${comp}" STREQUAL "TextToSpeech")
                add_compile_definitions(HAVE_TEXT_TO_SPEECH)
            endif()
        endif()
    endforeach()
endif()

add_executable(Qwain
    main.cpp
    qrzcache.cpp
    qrzcache.h
    mainwindow.cpp
    mainwindow.h
    configdialog.cpp
    configdialog.h
    loadingsplash.h
    mailcheckdialog.cpp
    mailcheckdialog.h
    countyprepdialog.cpp
    countyprepdialog.h
    cacheviewerdialog.cpp
    cacheviewerdialog.h
    icontoolsdialog.cpp
    icontoolsdialog.h
    prefetchflagsdialog.cpp
    prefetchflagsdialog.h
    qsldesigner.cpp
    qsldesigner.h
    qslmanagerdialog.cpp
    qslmanagerdialog.h
    schemedesigner.h
    receiveqsldialog.cpp
    receiveqsldialog.h
    livestreamviewer.cpp
    livestreamviewer.h
    qslpreviewdialog.cpp
    qslpreviewdialog.h
    qsldesigner_tpl.cpp
    qsldesigner_tpl.h
    mw_lookups.cpp
    mw_lookups.h
    qslpreview_spooler.cpp
    qslpreview_spooler.h
    qsldesigner_canvas.cpp
    qsldesigner_canvas.h
    qsltemplateinspector.cpp
    qsltemplateinspector.h
    styledialog.cpp
    styledialog.h
    gtkfontchooserwidget.cpp
    gtkfontchooserwidget.h
    imageviewer.cpp
    imageviewer.h
    imageeditordialog.cpp
    imageeditordialog.h
    globewidget.cpp
    globewidget.h
    graylinewidget.cpp
    graylinewidget.h
    qrzsyncdialog.cpp
    qrzsyncdialog.h
    dxclusterwidget.cpp
    dxclusterwidget.h
    waja_mapping.cpp
    waja_mapping.h
    cabrilloconverterdialog.cpp
    cabrilloconverterdialog.h
    statsdialog.cpp
    statsdialog.h
    resources.qrc
)

target_include_directories(Qwain PRIVATE . ${CMAKE_CURRENT_SOURCE_DIR})

find_package(OpenGL QUIET)
if (OpenGL_FOUND)
    target_link_libraries(Qwain ${QT_LIBRARIES} OpenGL::GL)
else()
    target_link_libraries(Qwain ${QT_LIBRARIES})
endif()

add_executable(qrz_diag qrz_diag.cpp)
target_include_directories(qrz_diag PRIVATE . ${CMAKE_CURRENT_SOURCE_DIR})
if (QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(qrz_diag Qt6::Core Qt6::Network)
else()
    target_link_libraries(qrz_diag Qt5::Core Qt5::Network)
endif()

add_executable(tx_tweaker tx_tweaker_main.cpp txtweakerwidget.cpp)
target_include_directories(tx_tweaker PRIVATE . ${CMAKE_CURRENT_SOURCE_DIR})
if (QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(tx_tweaker Qt6::Core Qt6::Widgets)
else()
    target_link_libraries(tx_tweaker Qt5::Core Qt5::Widgets)
endif()
