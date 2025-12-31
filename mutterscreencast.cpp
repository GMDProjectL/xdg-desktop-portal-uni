#include "mutterscreencast.h"
#include <QDebug>

MutterScreenCast::MutterScreenCast(QObject *parent)
    : QObject(parent)
    , m_screencast(new MutterScreenCastInterface(this))
{
    if (!m_screencast->isValid()) {
        qWarning() << "Failed to connect to Mutter ScreenCast interface";
    }
}

MutterScreenCast::~MutterScreenCast()
{
    // Clean up all sessions
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        it.value()->Stop();
    }
}

bool MutterScreenCast::isAvailable() const
{
    return m_screencast->isValid();
}

QString MutterScreenCast::createSession()
{
    QVariantMap properties;
    // Empty properties for now, could add "remote-desktop-session-id" if needed

    QDBusReply<QDBusObjectPath> reply = m_screencast->CreateSession(properties);
    if (!reply.isValid()) {
        qWarning() << "CreateSession failed:" << reply.error().message();
        return QString();
    }

    QString sessionPath = reply.value().path();

    // Create session interface
    auto *session = new MutterScreenCastSessionInterface(sessionPath, this);
    m_sessions[sessionPath] = session;

    // Connect closed signal
    connect(session, &MutterScreenCastSessionInterface::Closed, this, [this, sessionPath]() {
        qInfo() << "Session closed:" << sessionPath;
        emit sessionClosed(sessionPath);

        // Clean up
        m_sessions.remove(sessionPath);
    });

    qInfo() << "Created session:" << sessionPath;
    return sessionPath;
}

QString MutterScreenCast::recordMonitor(const QString &sessionPath,
                                        const QString &connector,
                                        uint cursorMode)
{
    auto *session = m_sessions.value(sessionPath);
    if (!session) {
        qWarning() << "No session found for path:" << sessionPath;
        return QString();
    }

    QVariantMap properties;
    properties["cursor-mode"] = cursorMode; // 0=Hidden, 1=Embedded, 2=Metadata

    QDBusReply<QDBusObjectPath> reply = session->RecordMonitor(connector, properties);
    if (!reply.isValid()) {
        qWarning() << "RecordMonitor failed:" << reply.error().message();
        return QString();
    }

    QString streamPath = reply.value().path();

    // Create stream interface
    auto *stream = new MutterScreenCastStreamInterface(streamPath, this);
    m_streams[streamPath] = stream;

    // Connect PipeWire stream signal
    connect(stream, &MutterScreenCastStreamInterface::PipeWireStreamAdded,
            this, [this, streamPath](uint nodeId) {
                qInfo() << "PipeWire stream added:" << streamPath << "node:" << nodeId;
                emit pipeWireStreamAdded(streamPath, nodeId);
            });

    qInfo() << "Created stream:" << streamPath << "for monitor:" << connector;
    return streamPath;
}

QString MutterScreenCast::recordWindow(const QString &sessionPath,
                                       uint64_t windowId,
                                       uint cursorMode)
{
    auto *session = m_sessions.value(sessionPath);
    if (!session) {
        qWarning() << "No session found for path:" << sessionPath;
        return QString();
    }

    QVariantMap properties;
    properties["window-id"] = QVariant::fromValue(windowId);
    properties["cursor-mode"] = cursorMode;

    QDBusReply<QDBusObjectPath> reply = session->RecordWindow(properties);
    if (!reply.isValid()) {
        qWarning() << "RecordWindow failed:" << reply.error().message();
        return QString();
    }

    QString streamPath = reply.value().path();

    // Create stream interface
    auto *stream = new MutterScreenCastStreamInterface(streamPath, this);
    m_streams[streamPath] = stream;

    // Connect PipeWire stream signal
    connect(stream, &MutterScreenCastStreamInterface::PipeWireStreamAdded,
            this, [this, streamPath](uint nodeId) {
                qInfo() << "PipeWire stream added:" << streamPath << "node:" << nodeId;
                emit pipeWireStreamAdded(streamPath, nodeId);
            });

    qInfo() << "Created stream:" << streamPath << "for window:" << windowId;
    return streamPath;
}

bool MutterScreenCast::startSession(const QString &sessionPath)
{
    auto *session = m_sessions.value(sessionPath);
    if (!session) {
        qWarning() << "No session found for path:" << sessionPath;
        return false;
    }

    QDBusReply<void> reply = session->Start();
    if (!reply.isValid()) {
        qWarning() << "Start failed:" << reply.error().message();
        return false;
    }

    qInfo() << "Started session:" << sessionPath;
    return true;
}

bool MutterScreenCast::stopSession(const QString &sessionPath)
{
    auto *session = m_sessions.value(sessionPath);
    if (!session) {
        qWarning() << "No session found for path:" << sessionPath;
        return false;
    }

    QDBusReply<void> reply = session->Stop();
    if (!reply.isValid()) {
        qWarning() << "Stop failed:" << reply.error().message();
        return false;
    }

    qInfo() << "Stopped session:" << sessionPath;
    return true;
}

QVariantMap MutterScreenCast::getStreamParameters(const QString &streamPath)
{
    auto *stream = m_streams.value(streamPath);
    if (!stream) {
        qWarning() << "No stream found for path:" << streamPath;
        return QVariantMap();
    }

    return stream->parameters();
}
