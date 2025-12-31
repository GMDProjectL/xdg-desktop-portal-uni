#include "screencast.h"
#include "screencastrequest.h"
#include "screencastsession.h"
#include <QDBusConnection>
#include <QTimer>
#include <QUuid>
#include <QPoint>
#include <QSize>
#include <QDBusArgument>
#include <QtDBus>

ScreenCast::ScreenCast(QObject *parent)
    : QDBusAbstractAdaptor{parent}
    , m_mutterScreencast(new MutterScreenCast(this))
{
    qDBusRegisterMetaType<ScreenCastStream>();
    qDBusRegisterMetaType<QList<ScreenCastStream>>();

    if (!m_mutterScreencast->isAvailable()) {
        qWarning() << "Niri screencast is not available!";
    }

    connect(m_mutterScreencast, &MutterScreenCast::pipeWireStreamAdded, this, &ScreenCast::onPipeWireStreamAdded);
}

uint ScreenCast::CreateSession(
    const QDBusObjectPath &handle,
    const QDBusObjectPath &session_handle,
    const QString &app_id,
    const QVariantMap &options,
    QVariantMap &results)
{
    Q_UNUSED(app_id)
    Q_UNUSED(options)

    QDBusConnection bus = QDBusConnection::sessionBus();

    // Create and export Request
    QObject *requestObj = new QObject(this);
    ScreenCastRequest *request = new ScreenCastRequest(requestObj);
    bus.registerObject(handle.path(), requestObj, QDBusConnection::ExportAdaptors);

    // Create and export Session
    QObject *sessionObj = new QObject(this);
    ScreenCastSession *session = new ScreenCastSession(sessionObj);
    bus.registerObject(session_handle.path(), sessionObj, QDBusConnection::ExportAdaptors);

    // Store session
    m_sessions[session_handle.path()] = session;

    // Return session ID
    QString sessionId = QUuid::createUuid().toString();
    results["session_id"] = sessionId;

    // Cleanup Request after success
    connect(request, &ScreenCastRequest::closed, requestObj, [=]() {
        QDBusConnection::sessionBus().unregisterObject(handle.path());
        m_sessions.remove(session_handle.path());
        requestObj->deleteLater();
    });

    // Cleanup Session when closed
    connect(session, &ScreenCastSession::Closed, sessionObj, [=]() {
        QDBusConnection::sessionBus().unregisterObject(session_handle.path());
        m_sessions.remove(session_handle.path());
        sessionObj->deleteLater();
    });

    QTimer::singleShot(0, request, &ScreenCastRequest::closed);

    return 0; // Success
}

uint ScreenCast::SelectSources(
    const QDBusObjectPath &handle,
    const QDBusObjectPath &session_handle,
    const QString &app_id,
    const QVariantMap &options,
    QVariantMap &results)
{
    Q_UNUSED(session_handle)
    Q_UNUSED(app_id)
    Q_UNUSED(options)
    Q_UNUSED(results)

    QDBusConnection bus = QDBusConnection::sessionBus();

    // Create and export Request
    QObject *requestObj = new QObject(this);
    ScreenCastRequest *request = new ScreenCastRequest(requestObj);
    bus.registerObject(handle.path(), requestObj, QDBusConnection::ExportAdaptors);

    connect(request, &ScreenCastRequest::closed, requestObj, [=]() {
        QDBusConnection::sessionBus().unregisterObject(handle.path());
        m_sessions.remove(session_handle.path());
        requestObj->deleteLater();
    });

    // Here you would show UI for source selection
    // For minimal implementation, just auto-succeed
    QTimer::singleShot(0, request, &ScreenCastRequest::closed);

    return 0;
}

uint ScreenCast::Start(
    const QDBusObjectPath &handle,
    const QDBusObjectPath &session_handle,
    const QString &app_id,
    const QString &parent_window,
    const QVariantMap &options,
    QVariantMap &results)
{
    Q_UNUSED(app_id)
    Q_UNUSED(parent_window)

    QDBusConnection bus = QDBusConnection::sessionBus();
    QObject *requestObj = new QObject(this);
    ScreenCastRequest *request = new ScreenCastRequest(requestObj);
    bus.registerObject(handle.path(), requestObj, QDBusConnection::ExportAdaptors);

    // Create Niri session
    QString niriSessionPath = m_mutterScreencast->createSession();
    m_portalToNiriSession[session_handle.path()] = niriSessionPath;

    QString streamPath = m_mutterScreencast->recordMonitor(niriSessionPath, "DP-1", 1);

    // Start session BEFORE building results
    m_mutterScreencast->startSession(niriSessionPath);

    // WAIT briefly for PipeWire signal (synchronous-ish approach)
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(1000); // 1 second max wait

    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(m_mutterScreencast, &MutterScreenCast::pipeWireStreamAdded,
            &loop, &QEventLoop::quit);

    timeout.start();
    loop.exec(); // Wait for signal or timeout

    // NOW build results with whatever we have
    uint nodeId = m_streamNodeIds.value(streamPath, 0);

    if (nodeId == 0) {
        qWarning() << "No PipeWire node ID received!";
        request->Close();
        return 1; // Return error
    }

    QVariantMap streamProperties;

    // Create position struct
    QDBusArgument posArg;
    posArg.beginStructure();
    posArg << 0 << 0;
    posArg.endStructure();
    streamProperties["position"] = QVariant::fromValue(posArg);

    // Create size struct
    QDBusArgument sizeArg;
    sizeArg.beginStructure();
    sizeArg << 1920 << 1080;
    sizeArg.endStructure();
    streamProperties["size"] = QVariant::fromValue(sizeArg);

    streamProperties["source_type"] = QVariant::fromValue<uint>(1);

    // Create stream list
    QList<ScreenCastStream> streams;
    ScreenCastStream stream;
    stream.nodeId = nodeId;
    stream.properties = streamProperties;
    streams.append(stream);

    results["streams"] = QVariant::fromValue(streams);


    connect(request, &ScreenCastRequest::closed, requestObj, [=]() {
        QDBusConnection::sessionBus().unregisterObject(handle.path());
        m_sessions.remove(session_handle.path());
        QString niriPath = m_portalToNiriSession.value(session_handle.path());
        if (!niriPath.isEmpty()) {
            m_mutterScreencast->stopSession(niriPath);
        }
        requestObj->deleteLater();
    });

    // QTimer::singleShot(0, request, &ScreenCastRequest::closed);
    return 0;
}


void ScreenCast::onPipeWireStreamAdded(const QString &streamPath, uint nodeId)
{
    qInfo() << "PipeWire node ID" << nodeId << "for stream" << streamPath;

    m_streamNodeIds[streamPath] = nodeId;

    // Check if this is for a pending Start request
    if (!m_pendingStarts.contains(streamPath)) {
        return;
    }

    PendingStart pending = m_pendingStarts.take(streamPath);

    // NOW build the results with the REAL node ID
    QVariantList streams;
    QVariantMap streamProperties;

    QVariantMap streamParams = m_mutterScreencast->getStreamParameters(streamPath);
    // ... extract position/size like before ...

    streamProperties["position"] = QVariant::fromValue(QPoint(0, 0));
    streamProperties["size"] = QVariant::fromValue(QSize(1920, 1080));
    streamProperties["source_type"] = 1;

    QVariantList stream;
    stream << nodeId;  // REAL node ID from Niri!
    stream << streamProperties;

    streams << QVariant::fromValue(stream);
    (*pending.results)["streams"] = streams;

    // NOW emit the closed signal to complete the request
    QTimer::singleShot(0, pending.request, &ScreenCastRequest::closed);
}

QDBusArgument &operator<<(QDBusArgument &arg, const ScreenCastStream &stream) {
    arg.beginStructure();
    arg << stream.nodeId << stream.properties;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, ScreenCastStream &stream) {
    arg.beginStructure();
    arg >> stream.nodeId >> stream.properties;
    arg.endStructure();
    return arg;
}
