cmake_minimum_required(VERSION 3.19)
project(xdg-desktop-portal-uni LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core DBus Widgets Quick QuickControls2)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GIO REQUIRED gio-unix-2.0)


qt_standard_project_setup()

file(GLOB_RECURSE PROJ_SRC src/*.cpp)

qt_add_executable(xdg-desktop-portal-uni ${PROJ_SRC})

qt_add_qml_module(xdg-desktop-portal-uni
    URI SourceSelectorModule
    VERSION 1.0
    QML_FILES qml/SourceSelector.qml
    RESOURCES
)

target_include_directories(xdg-desktop-portal-uni PUBLIC
    src
    src/niri
    ${GIO_INCLUDE_DIRS}
)

target_compile_options(xdg-desktop-portal-uni PRIVATE ${GIO_CFLAGS_OTHER})

target_link_libraries(xdg-desktop-portal-uni
    PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Widgets
    Qt::Quick
    Qt::QuickControls2
    ${GIO_LIBRARIES}
)


include(GNUInstallDirs)

install(TARGETS xdg-desktop-portal-uni
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
