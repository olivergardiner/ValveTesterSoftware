#pragma once
#include "devicetemplate.h"
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>

class TemplateStore
{
public:
    TemplateStore();

    // Load user templates from file (creates file with empty user section if absent)
    void load(const QString &filePath);
    // Persist user templates to the same file passed to load()
    void save() const;

    QStringList    systemNames() const;
    QStringList    userNames()   const;

    bool           hasSystem(const QString &name) const;
    bool           hasUser  (const QString &name) const;
    bool           hasAny   (const QString &name) const;

    DeviceTemplate get(const QString &name) const; // searches user first, then system
    void           saveUser(const QString &name, const DeviceTemplate &tmpl);

private:
    QMap<QString, DeviceTemplate> m_system;
    QMap<QString, DeviceTemplate> m_user;
    QString m_filePath;

    void populateDefaults();

    static DeviceTemplate fromJson(const QJsonObject &obj);
    static QJsonObject    toJson  (const DeviceTemplate &tmpl);
};
