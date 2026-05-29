#pragma once
#include <QString>

struct DeviceTemplate
{
    QString deviceType;
    QString testType;     // default test type when template is loaded
    QString anodeStart,  anodeStop,  anodeStep;  // Va sweep (Anode char)
    QString vaFixed;                             // fixed Va (Transfer char)
    QString gridStart,   gridStop,   gridStep;   // Vg1 range (Anode char outer loop / Transfer char x-axis)
    QString gridStepTransfer;                    // finer Vg1 step for Transfer char x-axis (falls back to gridStep)
    QString screenStart;                         // Vg2 sweep start (Transfer char)
    QString screenFixed;                         // fixed Vg2 (Anode char); falls back to screenStart if empty
    QString screenStop,  screenStep;             // Vg2 sweep range (Transfer char)
    QString iaMax;                               // Y-axis scale for Anode char
    QString iaMaxTransfer;                       // Y-axis scale for Transfer char (falls back to iaMax)
    QString pMax;
};
