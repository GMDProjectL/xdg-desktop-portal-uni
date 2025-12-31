#ifndef MUTTERSCREENCAST_H
#define MUTTERSCREENCAST_H

#include <QDBusAbstractInterface>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QDBusReply>
#include <QObject>
#include <QVariantMap>

// Main ScreenCast interface
class MutterScreenCastInterface : public QDBusAbstractInterface
{
    Q_OBJECT
    Q_PROPERTY(int version READ version)

public:
    static inline const char *staticInterfaceName()
    { return "org.gnome.Mutter.ScreenCast"; }

    MutterScreenCastInterface(QObject *parent = nullptr)
        : QDBusAbstractInterface(
              "org.gnome.Mutter.ScreenCast",
              "/org/gnome/Mutter/ScreenCast",
              staticInterfaceName(),
              QDBusConnection::sessionBus(),
              parent)
    {}

    int version() const
    {
        return property("version").toInt();
    }

public slots:
    QDBusReply<QDBusObjectPath> CreateSession(const QVariantMap &properties)
    {
        QList<QVariant> args;
        args << QVariant::fromValue(properties);
        return callWithArgumentList(QDBus::Block, "CreateSession", args);
    }
};

// Session interface
class MutterScreenCastSessionInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:
    static inline const char *staticInterfaceName()
    { return "org.gnome.Mutter.ScreenCast.Session"; }

    MutterScreenCastSessionInterface(const QString &path, QObject *parent = nullptr)
        : QDBusAbstractInterface(
              "org.gnome.Mutter.ScreenCast",
              path,
              staticInterfaceName(),
              QDBusConnection::sessionBus(),
              parent)
    {
        // Connect to Closed signal
        QDBusConnection::sessionBus().connect(
            "org.gnome.Mutter.ScreenCast",
            path,
            staticInterfaceName(),
            "Closed",
            this,
            SIGNAL(Closed())
            );
    }

public slots:
    QDBusReply<void> Start()
    {
        return call(QDBus::Block, "Start");
    }

    QDBusReply<void> Stop()
    {
        return call(QDBus::Block, "Stop");
    }

    QDBusReply<QDBusObjectPath> RecordMonitor(const QString &connector, const QVariantMap &properties)
    {
        QList<QVariant> args;
        args << connector << QVariant::fromValue(properties);
        return callWithArgumentList(QDBus::Block, "RecordMonitor", args);
    }

    QDBusReply<QDBusObjectPath> RecordWindow(const QVariantMap &properties)
    {
        QList<QVariant> args;
        args << QVariant::fromValue(properties);
        return callWithArgumentList(QDBus::Block, "RecordWindow", args);
    }

signals:
    void Closed();
};

// Stream interface
class MutterScreenCastStreamInterface : public QDBusAbstractInterface
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap parameters READ parameters)

public:
    static inline const char *staticInterfaceName()
    { return "org.gnome.Mutter.ScreenCast.Stream"; }

    MutterScreenCastStreamInterface(const QString &path, QObject *parent = nullptr)
        : QDBusAbstractInterface(
              "org.gnome.Mutter.ScreenCast",
              path,
              staticInterfaceName(),
              QDBusConnection::sessionBus(),
              parent)
    {
        // Connect to PipeWireStreamAdded signal
        QDBusConnection::sessionBus().connect(
            "org.gnome.Mutter.ScreenCast",
            path,
            staticInterfaceName(),
            "PipeWireStreamAdded",
            this,
            SIGNAL(PipeWireStreamAdded(uint))
            );
    }

    QVariantMap parameters() const
    {
        return qvariant_cast<QVariantMap>(property("Parameters"));
    }

signals:
    void PipeWireStreamAdded(uint nodeId);
};

// Wrapper class to manage the lifecycle
class MutterScreenCast : public QObject
{
    Q_OBJECT

public:
    explicit MutterScreenCast(QObject *parent = nullptr);
    ~MutterScreenCast();

    bool isAvailable() const;

    // Create a new screencast session
    QString createSession();

    // Record a monitor output
    QString recordMonitor(const QString &sessionPath, const QString &connector,
                          uint cursorMode = 0);

    // Record a window
    QString recordWindow(const QString &sessionPath, uint64_t windowId,
                         uint cursorMode = 0);

    // Start the session
    bool startSession(const QString &sessionPath);

    // Stop the session
    bool stopSession(const QString &sessionPath);

    // Get stream parameters (position, size)
    QVariantMap getStreamParameters(const QString &streamPath);

signals:
    void sessionClosed(const QString &sessionPath);
    void pipeWireStreamAdded(const QString &streamPath, uint nodeId);

private:
    MutterScreenCastInterface *m_screencast;
    QMap<QString, MutterScreenCastSessionInterface*> m_sessions;
    QMap<QString, MutterScreenCastStreamInterface*> m_streams;
};

#endif // MUTTERSCREENCAST_H
