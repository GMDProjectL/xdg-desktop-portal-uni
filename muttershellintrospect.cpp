#include "muttershellintrospect.h"
#include <QDebug>
#include <QDBusReply>

MutterShellIntrospect::MutterShellIntrospect(QObject *parent)
    : QObject(parent)
    , m_shellIntrospect(new MutterShellIntrospectInterface(this))
{
    if (!m_shellIntrospect->isValid()) {
        qWarning() << "Failed to connect to Mutter Shell Introspect interface";
    }
}

MutterShellIntrospect::~MutterShellIntrospect()
{
}

bool MutterShellIntrospect::isAvailable() const
{
    return m_shellIntrospect->isValid();
}

QVector<WindowInfo> MutterShellIntrospect::getWindows()
{
    QVector<WindowInfo> windows;

    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.gnome.Shell.Introspect",
        "/org/gnome/Shell/Introspect",
        "org.gnome.Shell.Introspect",
        "GetWindows"
        );

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg);

    if (reply.type() == QDBusMessage::ErrorMessage) {
        qWarning() << "GetWindows failed:" << reply.errorMessage();
        return windows;
    }

    if (reply.arguments().isEmpty()) {
        qWarning() << "GetWindows returned no arguments";
        return windows;
    }

    // The reply is a{ta{sv}} - map of uint64 to variant map
    const QDBusArgument &arg = reply.arguments().at(0).value<QDBusArgument>();

    arg.beginMap();
    while (!arg.atEnd()) {
        WindowInfo window;

        try {
            arg.beginMapEntry();

            // Read window ID (uint64)
            qulonglong windowId;
            arg >> windowId;
            window.windowId = windowId;

            // Read properties dict
            QVariantMap properties;
            arg >> properties;

            window.title = properties.value("title").toString();
            window.appId = properties.value("app-id").toString();

            arg.endMapEntry();

            windows.append(window);

        } catch (...) {
            qWarning() << "Failed to parse window entry, skipping";
            try { arg.endMapEntry(); } catch (...) {}
        }
    }
    arg.endMap();

    qInfo() << "Successfully parsed" << windows.size() << "windows";
    return windows;
}
