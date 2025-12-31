#ifndef SCREENCASTSESSION_H
#define SCREENCASTSESSION_H

#include <QObject>
#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>

class ScreenCastSession : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Session")
    Q_PROPERTY(uint version READ version)

public:
    explicit ScreenCastSession(QObject *parent = nullptr);
    ~ScreenCastSession() override;

    uint version() const { return 4; }

public slots:
    void Close();

signals:
    void Closed();

private:
    QString m_sessionId;
    QList<uint> m_streamNodeIds;
};

#endif // SCREENCASTSESSION_H
