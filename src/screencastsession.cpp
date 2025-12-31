#include "screencastsession.h"
#include <QUuid>

ScreenCastSession::ScreenCastSession(QObject *parent)
    : QDBusAbstractAdaptor(parent)
    , m_sessionId(QUuid::createUuid().toString())
{
}

ScreenCastSession::~ScreenCastSession()
{
}

void ScreenCastSession::Close()
{
    emit Closed();
}
