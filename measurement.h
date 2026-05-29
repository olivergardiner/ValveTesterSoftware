#ifndef MEASUREMENT_H
#define MEASUREMENT_H

#include <QString>
#include <QDateTime>
#include <QVector>

// ─── SweepSample ─────────────────────────────────────────────────────────────
// One measured point: anode voltage swept, bias voltages held fixed.
// All values are in engineering units (V, mA).

struct SweepSample
{
    double vaVolts  = 0.0;  // Anode voltage (measured)
    double vg1Volts = 0.0;  // Grid 1 voltage (set) — x-axis for transfer characteristic
    double iaMa     = 0.0;  // Anode current, mA
    double ig2Ma    = 0.0;  // Screen (g2) current, mA  — 0 for triodes
};

// ─── Sweep ───────────────────────────────────────────────────────────────────
// One characteristic curve: Ia vs Va at a fixed grid (and screen) bias.

struct Sweep
{
    double vgVolts  = 0.0;  // Grid 1 bias voltage (negative, e.g. -2.0 V)
    double vg2Volts = 0.0;  // Grid 2 (screen) voltage — 0 for triodes

    QVector<SweepSample> samples;
};

// ─── Measurement ─────────────────────────────────────────────────────────────
// A complete family of characteristic curves for one device under test.

struct Measurement
{
    QString   deviceName;
    QString   deviceType;   // "Triode", "Double Triode", "Pentode"
    QString   testType;     // "Anode Characteristic", etc.
    QDateTime timestamp;

    QVector<Sweep> sweeps;

    bool isEmpty() const { return sweeps.isEmpty(); }
};

#endif // MEASUREMENT_H
