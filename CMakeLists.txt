cmake_minimum_required(VERSION 3.16)

project(Mani VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

qt_add_executable(Mani
    src/main.cpp
    src/MainWindow.cpp
    src/database/Database.cpp
    src/util/Money.cpp
    src/util/AppPaths.cpp
    src/util/MesActivo.cpp
    src/util/Totales.cpp
    src/ui/CuentaDialog.cpp
    src/ui/CuentaCardWidget.cpp
    src/ui/TotalesPanelWidget.cpp
    src/ui/HistorialDialog.cpp
)

target_include_directories(Mani PRIVATE src)

target_link_libraries(Mani PRIVATE Qt6::Widgets Qt6::Sql)

if(APPLE)
    set_target_properties(Mani PROPERTIES MACOSX_BUNDLE TRUE)
endif()
