cmake_minimum_required(VERSION 3.16)

project(mpris-media-controller VERSION 2.0.0 LANGUAGES CXX)

# Usiamo C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Qt Widgets provides the standalone controller UI and Qt DBus talks to MPRIS.
find_package(Qt6 REQUIRED COMPONENTS Core DBus Gui Widgets)

add_executable(mpris_media_controller
    media-main.cpp
    mediacontroller.cpp
    mediacontroller.h
)

target_link_libraries(mpris_media_controller
    PRIVATE
        Qt6::Core
        Qt6::DBus
        Qt6::Gui
        Qt6::Widgets
)

install(TARGETS mpris_media_controller
    RUNTIME DESTINATION bin
)
