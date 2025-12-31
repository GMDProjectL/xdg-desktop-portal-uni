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
    qInfo() << "SelectSources called!";

    Q_UNUSED(app_id)
    Q_UNUSED(options)

    QDBusConnection bus = QDBusConnection::sessionBus();
    QObject *requestObj = new QObject(this);
    ScreenCastRequest *request = new ScreenCastRequest(requestObj);
    bus.registerObject(handle.path(), requestObj, QDBusConnection::ExportAdaptors);

    // Create source selector dialog
    SourceSelector *dialog = new SourceSelector(this);

    // Handle accepted (user selected a source)
    connect(dialog, &SourceSelector::accepted, this, [=]() {
        SourceSelector::Source selected = dialog->getSelectedSource();

        // Store selection for Start method
        SelectedSource source;
        source.sourceId = selected.id;
        source.isWindow = (selected.type == SourceSelector::Window);
        source.sessionHandle = session_handle.path();
        m_selectedSources[session_handle.path()] = source;

        qInfo() << "User selected:" << selected.displayName;

        // Complete the request
        QMetaObject::invokeMethod(request, &ScreenCastRequest::closed, Qt::QueuedConnection);

        // Safe cleanup
        dialog->deleteLater();
    });

    // Handle rejected (user cancelled)
    connect(dialog, &SourceSelector::rejected, this, [=]() {
        qInfo() << "User cancelled source selection";

        // Still need to complete the request
        QMetaObject::invokeMethod(request, &ScreenCastRequest::closed, Qt::QueuedConnection);

        // Safe cleanup
        dialog->deleteLater();
    });

    // Setup request cleanup
    connect(request, &ScreenCastRequest::closed, requestObj, [=]() {
        QDBusConnection::sessionBus().unregisterObject(handle.path());
        requestObj->deleteLater();
    });

    int result = dialog->exec(); // lol just shut up

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

    SelectedSource source = m_selectedSources.value(session_handle.path());
    QString streamPath;

    if (source.isWindow) {
        streamPath = m_mutterScreencast->recordWindow(
            niriSessionPath, source.sourceId.toULongLong(), 1);
    } else {
        streamPath = m_mutterScreencast->recordMonitor(
            niriSessionPath, source.sourceId, 1);
    }

    // Start session BEFORE building results
    m_mutterScreencast->startSession(niriSessionPath);

    // wait till pipewire stream lol
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(1000); // MAGIC NUMBERS! :D

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

    QVariantList streams;
    QVariantMap streamProperties;

    QVariantMap streamParams = m_mutterScreencast->getStreamParameters(streamPath);

    streamProperties["position"] = QVariant::fromValue(QPoint(0, 0));
    streamProperties["size"] = QVariant::fromValue(QSize(1920, 1080));
    streamProperties["source_type"] = 1;

    QVariantList stream;
    stream << nodeId;
    stream << streamProperties;

    streams << QVariant::fromValue(stream);
    (*pending.results)["streams"] = streams;

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
