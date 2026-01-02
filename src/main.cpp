#include <QApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QtDBus>
#include "screencast.h"
#include <cstdlib>

int main(int argc, char *argv[])
{
    setenv("QSG_RENDER_LOOP", "threaded", 1);
    setenv("vblank_mode", "0", 1);

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QDBusConnection bus = QDBusConnection::sessionBus();

    // Register service
    if (!bus.registerService("org.freedesktop.impl.portal.desktop.uni")) {
        qWarning() << "Failed to register DBus service";
        return 1;
    }

    // Create main object
    QObject *service = new QObject(&app);
    ScreenCast *screencast = new ScreenCast(service);


    // Register object
    if (!bus.registerObject("/org/freedesktop/portal/desktop",
                            service,
                            QDBusConnection::ExportAdaptors)) {
        qWarning() << "Failed to register DBus object";
        return 1;
    }

    qInfo() << "ScreenCast portal backend started";

    return app.exec();
}
