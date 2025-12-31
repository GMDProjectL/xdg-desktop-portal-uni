cmake_minimum_required(VERSION 3.19)
project(xdg-desktop-portal-uni LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core DBus Widgets Quick)

qt_standard_project_setup()

qt_add_executable(xdg-desktop-portal-uni
    main.cpp
    screencast.h screencast.cpp
    screencastrequest.h screencastrequest.cpp
    screencastsession.h screencastsession.cpp
    mutterscreencast.h mutterscreencast.cpp
)

target_link_libraries(xdg-desktop-portal-uni
    PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Widgets
    Qt::Quick
)

include(GNUInstallDirs)

install(TARGETS xdg-desktop-portal-uni
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
