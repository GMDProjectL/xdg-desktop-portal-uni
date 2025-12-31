#ifndef SCREENCAST_H
#define SCREENCAST_H

#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusVariant>
#include <QDBusObjectPath>
#include "screencastsession.h"
#include "mutterscreencast.h"
#include "screencastrequest.h"

class ScreenCast : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.ScreenCast")
    Q_PROPERTY(uint AvailableSourceTypes READ availableSourceTypes)
    Q_PROPERTY(uint AvailableCursorModes READ availableCursorModes)
    Q_PROPERTY(uint version READ version)

public:
    explicit ScreenCast(QObject *parent = nullptr);

    uint availableSourceTypes() const { return 1 | 2; } // mon|win
    uint availableCursorModes() const { return 1 | 2 | 4; } // everything
    uint version() const { return 4; }

public slots:
    uint CreateSession(
        const QDBusObjectPath& handle,
        const QDBusObjectPath& session_handle,
        const QString &app_id,
        const QVariantMap& options,
        QVariantMap& results
    );

    uint SelectSources(
        const QDBusObjectPath& handle,
        const QDBusObjectPath& session_handle,
        const QString& app_id,
        const QVariantMap& options,
        QVariantMap& results
    );

    uint Start(
        const QDBusObjectPath& handle,
        const QDBusObjectPath& session_handle,
        const QString& app_id,
        const QString& parent_window,
        const QVariantMap& options,
        QVariantMap& results
    );

    void onPipeWireStreamAdded(const QString &streamPath, uint nodeId);

private:
    struct PendingStart {
        QObject *requestObj;
        ScreenCastRequest *request;
        QString streamPath;
        QVariantMap *results;
    };

    QMap<QString, ScreenCastSession*> m_sessions;
    MutterScreenCast* m_mutterScreencast;

    QMap<QString, QString> m_portalToNiriSession;
    QMap<QString, uint> m_streamNodeIds;
    QMap<QString, PendingStart> m_pendingStarts;
};

struct ScreenCastStream {
    uint nodeId;
    QVariantMap properties;
};
Q_DECLARE_METATYPE(ScreenCastStream)

QDBusArgument &operator<<(QDBusArgument &arg, const ScreenCastStream &stream);
const QDBusArgument &operator>>(const QDBusArgument &arg, ScreenCastStream &stream);

#endif // SCREENCAST_H
