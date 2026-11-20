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

# Main Qwain GUI executable build target
add_executable(Qwain
    cpp_source/main.cpp
    cpp_source/qrzcache.cpp
    cpp_source/qrzcache.h
    cpp_source/mainwindow.cpp
    cpp_source/mainwindow.h
    cpp_source/configdialog.cpp
    cpp_source/configdialog.h
    cpp_source/loadingsplash.h
    cpp_source/mailcheckdialog.cpp
    cpp_source/mailcheckdialog.h
    cpp_source/countyprepdialog.cpp
    cpp_source/countyprepdialog.h
    cpp_source/cacheviewerdialog.cpp
    cpp_source/cacheviewerdialog.h
    cpp_source/icontoolsdialog.cpp
    cpp_source/icontoolsdialog.h
    cpp_source/prefetchflagsdialog.cpp
    cpp_source/prefetchflagsdialog.h
    cpp_source/qsldesigner.cpp
    cpp_source/qsldesigner.h
    cpp_source/qslmanagerdialog.cpp
    cpp_source/qslmanagerdialog.h
    cpp_source/schemedesigner.h
    cpp_source/receiveqsldialog.cpp
    cpp_source/receiveqsldialog.h
    cpp_source/qslpreviewdialog.cpp
    cpp_source/qslpreviewdialog.h
    cpp_source/qsldesigner_tpl.cpp
    cpp_source/qsldesigner_tpl.h
    cpp_source/mw_lookups.cpp
    cpp_source/mw_lookups.h
    cpp_source/qslpreview_spooler.cpp
    cpp_source/qslpreview_spooler.h
    cpp_source/qsldesigner_canvas.cpp
    cpp_source/qsldesigner_canvas.h
    cpp_source/qsltemplateinspector.cpp
    cpp_source/addressrenderinspectorwindow.cpp
        cpp_source/qsltemplateinspector.h
    cpp_source/addressrenderinspectorwindow.h
        cpp_source/styledialog.cpp
    cpp_source/styledialog.h
    cpp_source/gtkfontchooserwidget.cpp
    cpp_source/gtkfontchooserwidget.h
    cpp_source/imageviewer.cpp
    cpp_source/imageviewer.h
    cpp_source/imageeditordialog.cpp
    cpp_source/imageeditordialog.h
    # Real-time livestream viewer dialog
    cpp_source/livestreamviewer.cpp
    cpp_source/livestreamviewer.h
    # QSO statistics analytics dialog
    cpp_source/statsdialog.cpp
    cpp_source/statsdialog.h
    cpp_source/globewidget.cpp
    cpp_source/globewidget.h
    cpp_source/graylinewidget.cpp
    cpp_source/graylinewidget.h
    cpp_source/qrzsyncdialog.cpp
    cpp_source/qrzsyncdialog.h
    cpp_source/dxclusterwidget.cpp
    cpp_source/dxclusterwidget.h
    cpp_source/waja_mapping.cpp
    cpp_source/waja_mapping.h
    cpp_source/cabrilloconverterdialog.cpp
    cpp_source/cabrilloconverterdialog.h
    # Zero-dependency QR Code generator (Nayuki)
    cpp_source/qrcodegen.cpp
    cpp_source/qrcodegen.hpp
    cpp_source/resources.qrc
)

# Ensure header files in cpp_source directory are found regardless of build working directory
target_include_directories(Qwain PRIVATE cpp_source ${CMAKE_CURRENT_SOURCE_DIR}/cpp_source ${CMAKE_CURRENT_SOURCE_DIR})

find_package(OpenGL QUIET)
if (OpenGL_FOUND)
    target_link_libraries(Qwain ${QT_LIBRARIES} OpenGL::GL)
else()
    target_link_libraries(Qwain ${QT_LIBRARIES})
endif()

# Standalone QRZ diagnostic utility
add_executable(qrz_diag
    cpp_source/qrz_diag.cpp
)
target_include_directories(qrz_diag PRIVATE cpp_source ${CMAKE_CURRENT_SOURCE_DIR}/cpp_source ${CMAKE_CURRENT_SOURCE_DIR})
if (QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(qrz_diag Qt6::Core Qt6::Network)
else()
    target_link_libraries(qrz_diag Qt5::Core Qt5::Network)
endif()

# Standalone TX Power Tweaker utility
add_executable(tx_tweaker
    cpp_source/tx_tweaker_main.cpp
    cpp_source/txtweakerwidget.cpp
)
target_include_directories(tx_tweaker PRIVATE cpp_source ${CMAKE_CURRENT_SOURCE_DIR}/cpp_source ${CMAKE_CURRENT_SOURCE_DIR})
if (QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(tx_tweaker Qt6::Core Qt6::Widgets)
else()
    target_link_libraries(tx_tweaker Qt5::Core Qt5::Widgets)
endif()
