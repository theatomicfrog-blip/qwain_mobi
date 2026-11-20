cmake_minimum_required(VERSION 3.5)
project(TemplateCreator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

add_executable(TemplateCreator
    main.cpp
    mainwindow.h mainwindow.cpp
    template_view.h template_view.cpp
    elements.h elements.cpp
    properties_panel.h properties_panel.cpp
    document.h document.cpp
)

target_link_libraries(TemplateCreator PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)
