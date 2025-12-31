#include "mutterdisplayconfig.h"
#include <QDebug>
#include <QDBusReply>

MutterDisplayConfig::MutterDisplayConfig(QObject *parent)
    : QObject(parent)
    , m_displayConfig(new MutterDisplayConfigInterface(this))
{
    if (!m_displayConfig->isValid()) {
        qWarning() << "Failed to connect to Mutter DisplayConfig interface";
    }
}

MutterDisplayConfig::~MutterDisplayConfig()
{
}

bool MutterDisplayConfig::isAvailable() const
{
    return m_displayConfig->isValid();
}

QVector<MonitorInfo> MutterDisplayConfig::getMonitors()
{
    QVector<MonitorInfo> monitors;

    QDBusPendingReply<> reply = m_displayConfig->GetCurrentState();
    reply.waitForFinished();

    if (!reply.isValid()) {
        qWarning() << "GetCurrentState failed:" << reply.error().message();
        return monitors;
    }

    QDBusMessage msg = reply.reply();
    QList<QVariant> args = msg.arguments();

    if (args.size() < 2) {
        qWarning() << "Invalid reply structure - expected at least 2 arguments";
        return monitors;
    }

    // Argument 1 is the monitors array: a((ssss)a(siiddada{sv})a{sv})
    const QDBusArgument &monitorsArg = args.at(1).value<QDBusArgument>();

    monitorsArg.beginArray();
    while (!monitorsArg.atEnd()) {
        MonitorInfo monitor;

        try {
            monitorsArg.beginStructure();

            // Parse connector/vendor/product/serial (ssss)
            monitorsArg.beginStructure();
            monitorsArg >> monitor.connector >> monitor.vendor >> monitor.product >> monitor.serial;
            monitorsArg.endStructure();

            // Parse modes array a(siiddada{sv})
            monitorsArg.beginArray();
            while (!monitorsArg.atEnd()) {
                monitorsArg.beginStructure();

                QString modeId;
                int width, height;
                double refreshRate, preferredScale;

                monitorsArg >> modeId >> width >> height >> refreshRate >> preferredScale;

                // Skip supported scales array (ad)
                monitorsArg.beginArray();
                while (!monitorsArg.atEnd()) {
                    double scale;
                    monitorsArg >> scale;
                }
                monitorsArg.endArray();

                // Read mode properties (a{sv})
                QVariantMap modeProps;
                monitorsArg.beginMap();
                while (!monitorsArg.atEnd()) {
                    monitorsArg.beginMapEntry();
                    QString key;
                    QVariant value;
                    monitorsArg >> key >> value;
                    modeProps[key] = value;
                    monitorsArg.endMapEntry();
                }
                monitorsArg.endMap();

                // Check if this is the current mode
                if (modeProps.value("is-current").toBool()) {
                    monitor.currentWidth = width;
                    monitor.currentHeight = height;
                    monitor.currentRefreshRate = refreshRate;
                }

                monitorsArg.endStructure();
            }
            monitorsArg.endArray();

            // Parse monitor properties (a{sv})
            QVariantMap properties;
            monitorsArg.beginMap();
            while (!monitorsArg.atEnd()) {
                monitorsArg.beginMapEntry();
                QString key;
                QVariant value;
                monitorsArg >> key >> value;
                properties[key] = value;
                monitorsArg.endMapEntry();
            }
            monitorsArg.endMap();

            monitor.displayName = properties.value("display-name", monitor.connector).toString();
            monitor.isBuiltin = properties.value("is-builtin", false).toBool();

            monitorsArg.endStructure();
            monitors.append(monitor);

        } catch (...) {
            qWarning() << "Failed to parse monitor structure, skipping";
            // Try to recover by ending the current structure
            try { monitorsArg.endStructure(); } catch (...) {}
        }
    }
    monitorsArg.endArray();

    qInfo() << "Successfully parsed" << monitors.size() << "monitors";
    return monitors;
}
