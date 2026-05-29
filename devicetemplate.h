#pragma once
#include <QString>

struct DeviceTemplate
{
    QString deviceType;
    QString testType;
    QString anodeStart,  anodeStop,  anodeStep;
    QString gridStart,   gridStop,   gridStep;
    QString screenStart, screenStop, screenStep;
    QString iaMax;
    QString pMax;
};
