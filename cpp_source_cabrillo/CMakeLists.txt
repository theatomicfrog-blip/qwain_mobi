cmake_minimum_required(VERSION 3.10)
project(CabrilloConverter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic Qt Meta-Object Compiler (MOC) and User Interface Compiler (UIC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Search for Qt6 first, fallback to Qt5
find_package(Qt6 COMPONENTS Core Widgets REQUIRED QUIET)
if (Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
endif()

# Standalone Cabrillo Converter Executable
add_executable(CabrilloConverter
    main.cpp
    cabrilloconverterdialog.cpp
    cabrilloconverterdialog.h
)

# Link the appropriate Qt libraries based on discovered version
if (QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(CabrilloConverter PRIVATE Qt6::Core Qt6::Widgets)
else()
    target_link_libraries(CabrilloConverter PRIVATE Qt5::Core Qt5::Widgets)
endif()
