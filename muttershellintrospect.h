#ifndef MUTTERSHELLINTROSPECT_H
#define MUTTERSHELLINTROSPECT_H

#include <QObject>
#include <QDBusAbstractInterface>
#include <QDBusConnection>
#include <QDBusReply>
#include <QVariantMap>
#include <QVector>
#include <QString>

// ShellIntrospect interface to get window information
class MutterShellIntrospectInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.gnome.Shell.Introspect"; }

    MutterShellIntrospectInterface(QObject *parent = nullptr)
        : QDBusAbstractInterface(
              "org.gnome.Shell.Introspect",
              "/org/gnome/Shell/Introspect",
              staticInterfaceName(),
              QDBusConnection::sessionBus(),
              parent)
    {}

public slots:
    QDBusReply<QVariantMap> GetWindows()
    {
        return call(QDBus::Block, "GetWindows");
    }
};

// Window information structure
struct WindowInfo {
    uint64_t windowId;
    QString title;
    QString appId;
};

// Wrapper class to manage shell introspection queries
class MutterShellIntrospect : public QObject
{
    Q_OBJECT
public:
    explicit MutterShellIntrospect(QObject *parent = nullptr);
    ~MutterShellIntrospect();

    bool isAvailable() const;
    QVector<WindowInfo> getWindows();

private:
    MutterShellIntrospectInterface *m_shellIntrospect;
};

#endif // MUTTERSHELLINTROSPECT_H
