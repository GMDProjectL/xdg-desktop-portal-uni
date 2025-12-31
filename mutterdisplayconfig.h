#ifndef MUTTERDISPLAYCONFIG_H
#define MUTTERDISPLAYCONFIG_H

#include <QObject>
#include <QDBusAbstractInterface>
#include <QDBusConnection>
#include <QDBusReply>
#include <QVariantMap>
#include <QVector>
#include <QString>

// DisplayConfig interface to get monitor information
class MutterDisplayConfigInterface : public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.gnome.Mutter.DisplayConfig"; }

    MutterDisplayConfigInterface(QObject *parent = nullptr)
        : QDBusAbstractInterface(
              "org.gnome.Mutter.DisplayConfig",
              "/org/gnome/Mutter/DisplayConfig",
              staticInterfaceName(),
              QDBusConnection::sessionBus(),
              parent)
    {}

public slots:
    QDBusPendingReply<> GetCurrentState()
    {
        return asyncCall("GetCurrentState");
    }
};


struct MonitorInfo {
    QString connector;
    QString vendor;
    QString product;
    QString serial;
    QString displayName;
    int currentWidth;
    int currentHeight;
    double currentRefreshRate;
    bool isBuiltin;
};

// Wrapper class to manage display config queries
class MutterDisplayConfig : public QObject
{
    Q_OBJECT
public:
    explicit MutterDisplayConfig(QObject *parent = nullptr);
    ~MutterDisplayConfig();

    bool isAvailable() const;
    QVector<MonitorInfo> getMonitors();

private:
    MutterDisplayConfigInterface *m_displayConfig;
};

#endif // MUTTERDISPLAYCONFIG_H
