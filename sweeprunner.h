#ifndef SWEEPRUNNER_H
#define SWEEPRUNNER_H

#include <QObject>
#include <QVector>
#include "measurement.h"
#include "devicetemplate.h"

class ValveTesterDevice;

enum class SweepType { Anode, Transfer };

// ─── SweepParams ─────────────────────────────────────────────────────────────
// All sweep parameters parsed from the UI, in engineering units.

struct SweepParams
{
    QString deviceName;
    QString deviceType;
    QString testType;

    SweepType sweepType = SweepType::Anode;

    // Anode voltage range (swept in Anode char; fixed at vaStart in Transfer char)
    double vaStart  = 0.0;  // V
    double vaStop   = 0.0;  // V
    double vaStep   = 1.0;  // V

    // Grid 1 voltage range (positive magnitude)
    //   Anode char  — outer loop (one curve per Vg1 step)
    //   Transfer char — inner loop (x-axis of each curve)
    double vgStart  = 0.0;  // V
    double vgStop   = 0.0;  // V
    double vgStep   = 1.0;  // V

    // Grid 2 / screen voltage
    //   Anode char  — single fixed value (vg2)
    //   Transfer char — outer loop (one curve per Vg2 step)
    double vg2      = 0.0;  // V, fixed screen voltage for Anode char
    double vg2Start = 0.0;  // V, Transfer char outer loop start
    double vg2Stop  = 0.0;  // V
    double vg2Step  = 1.0;  // V

    double iaMax    = 0.0;  // mA — overload protection
    double pMax     = 0.0;  // W  — overload protection

    // When false (e.g. in simulator/test mode) M0 is not sent at the end of
    // the measurement so the HT supply is not unnecessarily discharged.
    bool dischargeOnComplete = true;
};

// ─── SweepRunner ─────────────────────────────────────────────────────────────
// Drives ValveTesterDevice through a complete Measurement.
// Sends S0 (grid DAC), S2 (HT target) and M1 for each point in sequence,
// waiting for measurementReady before advancing.

class SweepRunner : public QObject
{
    Q_OBJECT

public:
    explicit SweepRunner(ValveTesterDevice *device, QObject *parent = nullptr);

    bool isRunning() const { return m_running; }

public slots:
    void start(const SweepParams &params);
    void abort();

signals:
    // Emitted after every M1 response — allows the UI to update the chart live.
    void sampleAcquired(const Sweep &currentSweep, const SweepSample &sample);

    // Progress: 0–100.
    void progressChanged(int percent);

    // Emitted when the full measurement is complete.
    void measurementComplete(const Measurement &measurement);

    // Emitted if the sweep is aborted or the device reports an error.
    void sweepAborted(const QString &reason);

private slots:
    void onMeasurementReady(double vaVolts, double iaMa, double ig2Ma);
    void onMeasurementFailed();

private:
    void sendNextPoint();

    ValveTesterDevice *m_device;
    SweepParams        m_params;
    Measurement        m_measurement;
    bool               m_running = false;

    // Iteration state
    int    m_gridIndex  = 0;
    int    m_anodeIndex = 0;
    double m_currentVg  = 0.0;
    Sweep  m_currentSweep;

    int gridSteps()  const;
    int anodeSteps() const;
    int totalPoints() const;
};

#endif // SWEEPRUNNER_H
