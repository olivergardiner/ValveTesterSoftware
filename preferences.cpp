#include "preferences.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

void Preferences::load(const QString &filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        // File absent — persist defaults
        save(filePath);
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        enableLogging = obj.value("enableLogging").toBool(false);
        testMode      = obj.value("testMode").toInt(0);
    }
}

void Preferences::save(const QString &filePath) const
{
    QJsonObject obj;
    obj["enableLogging"] = enableLogging;
    obj["testMode"]      = testMode;
    QFile f(filePath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(obj).toJson());
}
