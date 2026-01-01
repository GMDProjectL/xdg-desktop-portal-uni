#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include "sourceselector.h"
#include "mutterdisplayconfig.h"
#include "muttershellintrospect.h"
#include <QQmlContext>
#include <QQuickItem>
#include <QQmlListProperty>
#include <qlogging.h>

SourceSelector::SourceSelector(QObject *parent, QString requestAppId)
    : QObject(parent)
    , m_view(nullptr)
    , m_engine(nullptr)
    , m_requestAppId(requestAppId)
{
    populateSources();
}

SourceSelector::~SourceSelector()
{
    if (m_view) {
        delete m_view;
    }
}



std::string parseNameFromFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::string line;
    bool inDesktopEntry = false;
    
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line == "[Desktop Entry]") {
            inDesktopEntry = true;
        } else if (line[0] == '[') {
            inDesktopEntry = false;
        } else if (inDesktopEntry && line.find("Name=") == 0) {
            return line.substr(5);
        }
    }
    return "";
}

std::string parseWMClassFromFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    std::string line;
    bool inDesktopEntry = false;
    
    while (std::getline(file, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line == "[Desktop Entry]") {
            inDesktopEntry = true;
        } else if (line[0] == '[') {
            inDesktopEntry = false;
        } else if (inDesktopEntry && line.find("StartupWMClass=") == 0) {
            return line.substr(15);
        }
    }
    return "";
}

std::string findDisplayName(const std::string& identifier) {
    // Build search paths
    std::vector<std::string> searchPaths;
    
    if (const char* home = std::getenv("HOME")) {
        searchPaths.push_back(std::string(home) + "/.local/share/applications/");
    }
    searchPaths.push_back("/usr/local/share/applications/");
    searchPaths.push_back("/usr/share/applications/");
    
    // Search through all desktop files
    for (const auto& searchPath : searchPaths) {
        if (!std::filesystem::exists(searchPath)) continue;
        
        for (const auto& entry : std::filesystem::directory_iterator(searchPath)) {
            if (entry.path().extension() != ".desktop") continue;
            
            std::string filename = entry.path().filename().string();
            std::string nameFromFile = parseNameFromFile(entry.path());
            nameFromFile = nameFromFile != "" ? nameFromFile : identifier;
            
            // Check if filename matches (desktop ID)
            if (filename == identifier || filename == identifier + ".desktop") {
                return nameFromFile;
            }
            
            // Check if StartupWMClass matches
            std::string wmClass = parseWMClassFromFile(entry.path());

            if (wmClass == "") return identifier;

            if (wmClass == identifier) {
                return nameFromFile;
            }
        }
    }
    
    return "";
}

QString SourceSelector::getAppDisplayName(QString appId) {
    auto displayName = findDisplayName(appId.toStdString());
    return QString::fromStdString(displayName);
}

void SourceSelector::setupUI()
{
    m_engine = new QQmlApplicationEngine(this);

    connect(m_engine, &QQmlApplicationEngine::warnings, this,
            [](const QList<QQmlError> &warnings) {
                for (const auto &warning : warnings) {
                    qWarning() << "QML Warning:" << warning.toString();
                }
            });

    connect(m_engine, &QQmlApplicationEngine::objectCreated, this,
            [](QObject *obj, const QUrl &url) {
                if (!obj) {
                    qCritical() << "Failed to create QML object from:" << url;
                } else {
                    qInfo() << "Successfully created QML object from:" << url;
                }
            });

    // Create QML model and store it as member
    m_sourceObjects.clear();
    for (const auto &source : m_sources) {
        m_sourceObjects.append(new SourceItem(
            static_cast<int>(source.type),
            source.id,
            source.displayName,
            this  // Changed parent to 'this' instead of m_engine
            ));
    }

    // Set context property BEFORE loading QML
    m_engine->rootContext()->setContextProperty("requestAppId", QVariant::fromValue(m_requestAppId));
    m_engine->rootContext()->setContextProperty("sourceModel", QVariant::fromValue(m_sourceObjects));
    m_engine->rootContext()->setContextProperty("selectorApi", this);

    // Load the QML file
    m_engine->load(QUrl(QStringLiteral("qrc:/SourceSelectorModule/qml/SourceSelector.qml")));

    qInfo() << "Loaded SourceSelector.qml";

    // Check if loading succeeded
    if (m_engine->rootObjects().isEmpty()) {
        qWarning() << "Failed to load QML file";
        return;
    }

    QObject *root = m_engine->rootObjects().first();

    qInfo() << "root is " << &root;

    // Connect signals
    QObject::connect(root, SIGNAL(sourceSelected(int)),
                     this, SLOT(onSourceSelected(int)));
    QObject::connect(root, SIGNAL(cancelled()),
                     this, SLOT(onCancelled()));

    qInfo() << "Connected sourceSelected and onCancelled";
}

int SourceSelector::exec()
{
    if (!m_engine) {
        setupUI();
    }

    if (m_engine->rootObjects().isEmpty()) {
        return 0;
    }

    QObject *root = m_engine->rootObjects().first();

    // Make it modal - set the modality property
    root->setProperty("modality", Qt::ApplicationModal);

    // Show and activate the window
    root->setProperty("visible", true);
    root->setProperty("modality", Qt::ApplicationModal);
    QMetaObject::invokeMethod(root, "raise");
    QMetaObject::invokeMethod(root, "requestActivate");

    // Create event loop for modal behavior
    QEventLoop loop;
    connect(this, &SourceSelector::accepted, &loop, [&loop]() { loop.exit(1); });
    connect(this, &SourceSelector::rejected, &loop, [&loop]() { loop.exit(0); });

    return loop.exec();
}

void SourceSelector::populateSources()
{
    m_sources.clear();

    MutterDisplayConfig displayConfig;

    if (!displayConfig.isAvailable()) {
        qWarning() << "Hell no, display config is not available...";
        return;
    }

    QVector<MonitorInfo> monitors = displayConfig.getMonitors();
    qInfo() << "Found " << monitors.size() << " monitors.";

    for (const auto& monitor : monitors) {
        Source source;
        source.type = Monitor;
        source.id = monitor.connector;

        if (!monitor.displayName.isEmpty()) {
            source.displayName = QString("%1 (%2x%3 @ %4 Hz)")
            .arg(monitor.displayName)
                .arg(monitor.currentWidth)
                .arg(monitor.currentHeight)
                .arg(monitor.currentRefreshRate, 0, 'f', 2); // didn't know Qt has string formatting built-in, I was gonna use fmt or smth like that lol
        }
        else {
            source.displayName = QString("%1 (%2x%3)")
            .arg(monitor.connector)
                .arg(monitor.currentWidth)
                .arg(monitor.currentHeight);
        }

        m_sources.append(source);
        qInfo() << "Added monitor: " << source.displayName;
    }

    MutterShellIntrospect shellIntrospect;
    if (!shellIntrospect.isAvailable()) {
        qWarning() << "Sorry, no windows for you, I guess.";
        return;
    }

    QVector<WindowInfo> windows = shellIntrospect.getWindows();
    qInfo() << "Found" << windows.size() << "windows";

    for (const auto &window : windows) {
        Source source;
        source.type = Window;
        source.id = QString::number(window.windowId);

        if (!window.title.isEmpty()) {
            source.displayName = window.title;
        } else if (!window.appId.isEmpty()) {
            source.displayName = QString::fromStdString(findDisplayName(window.appId.toStdString()));
        } else {
            source.displayName = QString("Window %1").arg(window.windowId);
        }

        m_sources.append(source);
        qInfo() << "Added window:" << source.displayName;
    }
}

void SourceSelector::show()
{
    qInfo() << "SourceSelector::show() ENTERED";

    if (!m_engine) {
        qInfo() << "Creating engine";
        setupUI();
    }

    qInfo() << "Engine exists:" << (m_engine != nullptr);
    qInfo() << "Root objects count:" << (m_engine ? m_engine->rootObjects().size() : -1);

    if (!m_engine->rootObjects().isEmpty()) {
        QObject *root = m_engine->rootObjects().first();

        qInfo() << "Root object before show:" << root;
        qInfo() << "Visible before:" << root->property("visible").toBool();

        QMetaObject::invokeMethod(root, "show");
        QMetaObject::invokeMethod(root, "raise");
        QMetaObject::invokeMethod(root, "requestActivate");

        qInfo() << "Visible after show():" << root->property("visible").toBool();
        qInfo() << "Window flags:" << root->property("flags");
    }
    else {
        qInfo() << "NO ROOT OBJECTS - QML FAILED TO LOAD!";
    }
}

void SourceSelector::onSourceSelected(int index)
{
    if (index >= 0 && index < m_sources.size()) {
        m_selectedSource = m_sources[index];
        emit accepted();
    }
}

void SourceSelector::onCancelled()
{
    emit rejected();
}
