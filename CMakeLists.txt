cmake_minimum_required(VERSION 3.19)
project(xdg-desktop-portal-uni LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core DBus Widgets Quick QuickControls2)

qt_standard_project_setup()

qt_add_executable(xdg-desktop-portal-uni
    main.cpp
    screencast.h screencast.cpp
    screencastrequest.h screencastrequest.cpp
    screencastsession.h screencastsession.cpp
    mutterscreencast.h mutterscreencast.cpp
    sourceselector.h sourceselector.cpp
)

qt_add_qml_module(xdg-desktop-portal-uni
    URI SourceSelectorModule
    VERSION 1.0
    QML_FILES SourceSelector.qml
    RESOURCES
)

target_link_libraries(xdg-desktop-portal-uni
    PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Widgets
    Qt::Quick
    Qt::QuickControls2
)


include(GNUInstallDirs)

install(TARGETS xdg-desktop-portal-uni
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
