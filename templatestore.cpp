#include "templatestore.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

// ─── Construction ─────────────────────────────────────────────────────────────

TemplateStore::TemplateStore()
{
    populateDefaults();
}

// ─── Default system templates ─────────────────────────────────────────────────

void TemplateStore::populateDefaults()
{
    // ECC83 / 12AX7 – Double Triode
    {
        DeviceTemplate t;
        t.deviceType = "Double Triode"; t.testType = "Anode Characteristic";
        t.anodeStart = "0"; t.anodeStop = "300"; t.anodeStep = "5";
        t.gridStart  = "0"; t.gridStop  = "4";   t.gridStep  = "0.5";
        t.iaMax = "6"; t.pMax = "1";
        m_system["ECC83"] = m_system["12AX7"] = t;
    }
    // ECC82 / 12AU7 – Double Triode
    {
        DeviceTemplate t;
        t.deviceType = "Double Triode"; t.testType = "Anode Characteristic";
        t.anodeStart = "0"; t.anodeStop = "400"; t.anodeStep = "5";
        t.gridStart  = "0"; t.gridStop  = "20";  t.gridStep  = "2";
        t.iaMax = "20"; t.pMax = "2.8";
        m_system["ECC82"] = m_system["12AU7"] = t;
    }
    // ECC81 / 12AT7 – Double Triode
    {
        DeviceTemplate t;
        t.deviceType = "Double Triode"; t.testType = "Anode Characteristic";
        t.anodeStart = "0"; t.anodeStop = "350"; t.anodeStep = "5";
        t.gridStart  = "0"; t.gridStop  = "10";  t.gridStep  = "1";
        t.iaMax = "20"; t.pMax = "2.5";
        m_system["ECC81"] = m_system["12AT7"] = t;
    }
    // 300B – Triode
    {
        DeviceTemplate t;
        t.deviceType = "Triode"; t.testType = "Anode Characteristic";
        t.anodeStart = "0"; t.anodeStop = "500"; t.anodeStep = "5";
        t.gridStart  = "0"; t.gridStop  = "80";  t.gridStep  = "10";
        t.iaMax = "120"; t.pMax = "40";
        m_system["300B"] = t;
    }
    // 2A3 – Triode
    {
        DeviceTemplate t;
        t.deviceType = "Triode"; t.testType = "Anode Characteristic";
        t.anodeStart = "0"; t.anodeStop = "400"; t.anodeStep = "5";
        t.gridStart  = "0"; t.gridStop  = "50";  t.gridStep  = "5";
        t.iaMax = "100"; t.pMax = "15";
        m_system["2A3"] = t;
    }
    // EL34 – Pentode
    {
        DeviceTemplate t;
        t.deviceType = "Pentode"; t.testType = "Anode Characteristic";
        t.anodeStart = "0"; t.anodeStop = "500"; t.anodeStep = "5";
        t.gridStart  = "0"; t.gridStop  = "30";  t.gridStep  = "2.5";
        t.screenStart = "250"; t.screenStop = "250"; t.screenStep = "0";
        t.iaMax = "200"; t.pMax = "25";
        m_system["EL34"] = t;
    }
    // EL84 – Pentode
    {
        DeviceTemplate t;
        t.deviceType = "Pentode"; t.testType = "Anode Characteristic";
        t.anodeStart = "0"; t.anodeStop = "300"; t.anodeStep = "5";
        t.gridStart  = "0"; t.gridStop  = "15";  t.gridStep  = "1.5";
        t.screenStart = "250"; t.screenStop = "250"; t.screenStep = "0";
        t.iaMax = "175"; t.pMax = "12";
        m_system["EL84"] = t;
    }
    // 6L6 – Pentode
    {
        DeviceTemplate t;
        t.deviceType = "Pentode"; t.testType = "Anode Characteristic";
        t.anodeStart = "0"; t.anodeStop = "450"; t.anodeStep = "5";
        t.gridStart  = "0"; t.gridStop  = "30";  t.gridStep  = "2.5";
        t.screenStart = "250"; t.screenStop = "250"; t.screenStep = "0";
        t.iaMax = "150"; t.pMax = "19";
        m_system["6L6"] = t;
    }
    // KT88 – Pentode
    {
        DeviceTemplate t;
        t.deviceType = "Pentode"; t.testType = "Anode Characteristic";
        t.anodeStart = "0"; t.anodeStop = "600"; t.anodeStep = "5";
        t.gridStart  = "0"; t.gridStop  = "40";  t.gridStep  = "2.5";
        t.screenStart = "300"; t.screenStop = "300"; t.screenStep = "0";
        t.iaMax = "300"; t.pMax = "42";
        m_system["KT88"] = t;
    }
    // EF86 – Pentode (small-signal)
    {
        DeviceTemplate t;
        t.deviceType = "Pentode"; t.testType = "Anode Characteristic";
        t.anodeStart = "0"; t.anodeStop = "300"; t.anodeStep = "5";
        t.gridStart  = "0"; t.gridStop  = "4";   t.gridStep  = "0.5"; t.screenStop = "200"; t.screenStep = "0";
        t.iaMax = "3"; t.pMax = "1";
        m_system["EF86"] = t;
    }
}

// ─── Persistence ─────────────────────────────────────────────────────────────

DeviceTemplate TemplateStore::fromJson(const QJsonObject &o)
{
    DeviceTemplate t;
    t.deviceType  = o["deviceType"].toString();
    t.testType    = o["testType"].toString();
    t.anodeStart  = o["anodeStart"].toString();
    t.anodeStop   = o["anodeStop"].toString();
    t.anodeStep   = o["anodeStep"].toString();
    t.gridStart   = o["gridStart"].toString();
    t.gridStop    = o["gridStop"].toString();
    t.gridStep    = o["gridStep"].toString();
    t.screenStart = o["screenStart"].toString();
    t.screenStop  = o["screenStop"].toString();
    t.screenStep  = o["screenStep"].toString();
    t.iaMax       = o["iaMax"].toString();
    t.pMax        = o["pMax"].toString();
    return t;
}

QJsonObject TemplateStore::toJson(const DeviceTemplate &t)
{
    return {
        {"deviceType",  t.deviceType},
        {"testType",    t.testType},
        {"anodeStart",  t.anodeStart},
        {"anodeStop",   t.anodeStop},
        {"anodeStep",   t.anodeStep},
        {"gridStart",   t.gridStart},
        {"gridStop",    t.gridStop},
        {"gridStep",    t.gridStep},
        {"screenStart", t.screenStart},
        {"screenStop",  t.screenStop},
        {"screenStep",  t.screenStep},
        {"iaMax",       t.iaMax},
        {"pMax",        t.pMax}
    };
}

void TemplateStore::load(const QString &filePath)
{
    m_filePath = filePath;
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        save(); // create the file with an empty user section
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return;
    const QJsonObject root = doc.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        if (it.value().isObject())
            m_user[it.key()] = fromJson(it.value().toObject());
    }
}

void TemplateStore::save() const
{
    if (m_filePath.isEmpty()) return;
    QJsonObject root;
    for (auto it = m_user.begin(); it != m_user.end(); ++it)
        root[it.key()] = toJson(it.value());
    QFile f(m_filePath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(root).toJson());
}

// ─── Queries ─────────────────────────────────────────────────────────────────

QStringList TemplateStore::systemNames() const { return m_system.keys(); }
QStringList TemplateStore::userNames()   const { return m_user.keys(); }

bool TemplateStore::hasSystem(const QString &name) const { return m_system.contains(name); }
bool TemplateStore::hasUser  (const QString &name) const { return m_user.contains(name); }
bool TemplateStore::hasAny   (const QString &name) const { return hasSystem(name) || hasUser(name); }

DeviceTemplate TemplateStore::get(const QString &name) const
{
    if (m_user.contains(name))   return m_user.value(name);
    if (m_system.contains(name)) return m_system.value(name);
    return {};
}

void TemplateStore::saveUser(const QString &name, const DeviceTemplate &tmpl)
{
    m_user[name] = tmpl;
    save();
}
