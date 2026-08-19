cmake_minimum_required(VERSION 3.16)

project(Mani VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network)

set(MANI_PLATFORM_SOURCES src/platform/TouchIdAuth_stub.cpp)
if(APPLE)
    set(MANI_PLATFORM_SOURCES src/platform/TouchIdAuth_mac.mm)
endif()

qt_add_executable(Mani
    src/main.cpp
    src/MainWindow.cpp
    src/database/Database.cpp
    src/util/Money.cpp
    src/util/DolarHoy.cpp
    src/util/AppPaths.cpp
    src/util/AppLock.cpp
    src/util/CsvIO.cpp
    src/util/MesActivo.cpp
    src/util/Totales.cpp
    src/ui/CuentaDialog.cpp
    src/ui/CuentaCardWidget.cpp
    src/ui/TotalesPanelWidget.cpp
    src/ui/HistorialDialog.cpp
    src/ui/MovimientoDialog.cpp
    src/ui/MovimientosCuentaDialog.cpp
    src/ui/BarChartWidget.cpp
    src/ui/ReportesDialog.cpp
    src/ui/LockDialog.cpp
    src/ui/PasswordConfigDialog.cpp
    ${MANI_PLATFORM_SOURCES}
)

target_include_directories(Mani PRIVATE src)

target_link_libraries(Mani PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network)

target_compile_definitions(Mani PRIVATE MANI_APP_VERSION="${PROJECT_VERSION}")

if(APPLE)
    set_target_properties(Mani PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Mani"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.mani.app"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "app.icns"
    )
    set_source_files_properties(resources/app.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(Mani PRIVATE resources/app.icns)
    target_link_libraries(Mani PRIVATE "-framework LocalAuthentication")
endif()
