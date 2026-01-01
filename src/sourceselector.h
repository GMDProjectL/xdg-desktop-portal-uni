#ifndef SOURCESELECTOR_H
#define SOURCESELECTOR_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QQmlEngine>
#include <QQuickView>
#include <QQmlApplicationEngine>
#include <qtmetamacros.h>

class SourceSelector : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum SourceType {
        Monitor = 0,
        Window = 1
    };
    Q_ENUM(SourceType)

    struct Source {
        SourceType type;
        QString id;
        QString displayName;
    };

    explicit SourceSelector(QObject* parent = nullptr, QString requestAppId = "");
    ~SourceSelector();

    void show();
    int exec();
    Source getSelectedSource() const { return m_selectedSource; }

    Q_INVOKABLE QString getAppDisplayName(QString appId);

signals:
    void accepted();
    void rejected();

private slots:
    void onSourceSelected(int index);
    void onCancelled();

private:
    void setupUI();
    void populateSources();

    QQuickView *m_view;
    QQmlApplicationEngine *m_engine;
    QVector<Source> m_sources;
    Source m_selectedSource;
    QObjectList m_sourceObjects;
    QString m_requestAppId;
};

// QML-compatible model item
class SourceItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int type READ type CONSTANT)
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)

public:
    SourceItem(int type, const QString& id, const QString& displayName, QObject* parent = nullptr)
        : QObject(parent), m_type(type), m_id(id), m_displayName(displayName) {}

    int type() const { return m_type; }
    QString id() const { return m_id; }
    QString displayName() const { return m_displayName; }

private:
    int m_type;
    QString m_id;
    QString m_displayName;
};

#endif // SOURCESELECTOR_H
