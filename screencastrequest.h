#ifndef SCREENCASTREQUEST_H
#define SCREENCASTREQUEST_H

#include <QObject>
#include <QDBusObjectPath>
#include <QDBusAbstractAdaptor>

class ScreenCastRequest : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.impl.portal.Request")

public:
    explicit ScreenCastRequest(QObject *parent = nullptr);
    ~ScreenCastRequest() override;

public slots:
    void Close();

signals:
    void closed();

private:
    QString m_objectPath;
};

#endif // SCREENCASTREQUEST_H
