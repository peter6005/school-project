cmake_minimum_required(VERSION 3.16)

project(app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort Charts)

qt_standard_project_setup()

qt_add_executable(app
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(app
    PRIVATE
        Qt6::Widgets
        Qt6::SerialPort
        Qt6::Charts
)

set_target_properties(app PROPERTIES
    WIN32_EXECUTABLE TRUE
)
