cmake_minimum_required(VERSION 3.19)
project(xdg-desktop-portal-uni LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core DBus Widgets Quick QuickControls2)
find_package(UniQmlTk REQUIRED)

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
    /usr/include/unisettings
)

target_link_libraries(xdg-desktop-portal-uni
    PRIVATE
    Qt::Core
    Qt::DBus
    Qt::Widgets
    Qt::Quick
    Qt::QuickControls2
    UniQmlTk unisettings
)

# Install the executable to libexec
install(TARGETS xdg-desktop-portal-uni
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Configure and install .portal file
set(PORTAL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/xdg-desktop-portal/portals")
install(FILES data/uni.portal
    DESTINATION ${PORTAL_DIR}
)

set(LIBEXECDIR /${CMAKE_INSTALL_LIBDIR})

# Configure and install D-Bus service file
set(DBUS_SERVICE_DIR "${CMAKE_INSTALL_DATAROOTDIR}/dbus-1/services")
configure_file(data/org.freedesktop.impl.portal.desktop.uni.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.impl.portal.desktop.uni.service
    @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.impl.portal.desktop.uni.service
    DESTINATION ${DBUS_SERVICE_DIR}
)

# Configure and install systemd user service file
set(SYSTEMD_USER_UNIT_DIR "${CMAKE_INSTALL_LIBDIR}/systemd/user")
configure_file(data/xdg-desktop-portal-uni.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/xdg-desktop-portal-uni.service
    @ONLY
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/xdg-desktop-portal-uni.service
    DESTINATION ${SYSTEMD_USER_UNIT_DIR}
)
