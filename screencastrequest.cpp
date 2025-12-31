#include "screencastrequest.h"

ScreenCastRequest::ScreenCastRequest(QObject *parent)
    : QDBusAbstractAdaptor{parent}
{}

ScreenCastRequest::~ScreenCastRequest()
{
}

void ScreenCastRequest::Close()
{
    emit closed();
}
