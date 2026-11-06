cmake_minimum_required(VERSION 3.16)

project(krunner-mpris VERSION 1.0.0 LANGUAGES CXX)

# Usiamo C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ECM e le impostazioni standard KDE.
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)

# Mettiamo la libreria compilata nella cartella build/lib.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Qt:
# - Core: classi fondamentali Qt
# - DBus: comunicazione con i player MPRIS
# - Gui: QIcon/QPixmap per mostrare la copertina
find_package(Qt6 REQUIRED COMPONENTS Core DBus Gui Widgets)

# KRunner.
find_package(KF6 REQUIRED COMPONENTS Runner)

# Creiamo il plugin KRunner.
kcoreaddons_add_plugin(krunner_mpris
    SOURCES
        mprisrunner.cpp
        mprisrunner.json
    INSTALL_NAMESPACE "kf6/krunner"
)

# Librerie necessarie al plugin.
target_link_libraries(krunner_mpris
    PRIVATE
        Qt6::Core
        Qt6::DBus
        Qt6::Gui
        KF6::Runner
)

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
    RUNTIME DESTINATION ${KDE_INSTALL_BINDIR}
)
