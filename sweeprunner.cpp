#include "sweeprunner.h"
#include "valvetesterdevice.h"
#include "hw_scaling.h"

#include <QDateTime>
#include <cmath>

SweepRunner::SweepRunner(ValveTesterDevice *device, QObject *parent)
    : QObject(parent)
    , m_device(device)
{
    connect(m_device, &ValveTesterDevice::measurementReady,
            this,     &SweepRunner::onMeasurementReady);
    connect(m_device, &ValveTesterDevice::measurementFailed,
            this,     &SweepRunner::onMeasurementFailed);
}

// ─── Public slots ─────────────────────────────────────────────────────────────

void SweepRunner::start(const SweepParams &params)
{
    if (m_running || !m_device->isConnected())
        return;

    m_params      = params;
    m_running     = true;
    m_gridIndex   = 0;
    m_anodeIndex  = 0;
    m_measurement = Measurement{};
    m_measurement.deviceName = params.deviceName;
    m_measurement.deviceType = params.deviceType;
    m_measurement.testType   = params.testType;
    m_measurement.timestamp  = QDateTime::currentDateTime();

    m_currentSweep = Sweep{};
    if (params.sweepType == SweepType::Anode) {
        m_currentVg             = params.vgStart;
        m_currentSweep.vgVolts  = -m_currentVg;
        m_currentSweep.vg2Volts = params.vg2;
    } else {
        m_currentVg             = params.vg2Start;
        m_currentSweep.vgVolts  = 0.0;
        m_currentSweep.vg2Volts = m_currentVg;
    }

    sendNextPoint();
}

void SweepRunner::abort()
{
    if (!m_running)
        return;
    m_running = false;
    emit sweepAborted("Aborted by user");
}

// ─── Private ─────────────────────────────────────────────────────────────────

void SweepRunner::sendNextPoint()
{
    if (m_params.sweepType == SweepType::Anode) {
        // Outer loop: Vg1 (m_currentVg), Inner loop: Va
        const double va   = m_params.vaStart + m_anodeIndex * m_params.vaStep;
        const int htCode  = Hw::htVoltsToCount(va);
        const int dacCode = Hw::gridVoltageToDacCode(-m_currentVg);

        if (m_anodeIndex == 0) {
            m_device->setGrid(dacCode);
            if (m_params.deviceType == "Pentode")
                m_device->setScreen(Hw::htVoltsToCount(m_params.vg2));
            else
                m_device->setGrid2(dacCode); // Double Triode: both sections same grid bias
        }

        m_device->setHtTarget(htCode);
        if (m_params.deviceType == "Double Triode")
            m_device->setScreen(htCode);   // S3 = HT2 target, tracks HT1 for both sections
    } else {
        // Transfer characteristic: Outer loop: Vg2 (m_currentVg), Inner loop: Vg1
        // Transfer characteristic: sweep Vg1 from vgStop (most negative = min current)
        // toward vgStart, so pMax overload clips the high-current tail rather than the start.
        const double vg1     = m_params.vgStop - m_anodeIndex * m_params.vgStep;
        const int dacCodeVg1 = Hw::gridVoltageToDacCode(-vg1);

        if (m_anodeIndex == 0) {
            m_device->setHtTarget(Hw::htVoltsToCount(m_params.vaStart));  // Va fixed
            if (m_params.deviceType == "Pentode")
                m_device->setScreen(Hw::htVoltsToCount(m_currentVg));  // stepped Vg2
            else if (m_params.deviceType == "Double Triode")
                m_device->setGrid2(Hw::gridVoltageToDacCode(-m_currentVg));
        }

        m_device->setGrid(dacCodeVg1);
    }

    m_device->runTest();

    emit progressChanged(static_cast<int>(
        100.0 * (m_gridIndex * anodeSteps() + m_anodeIndex) / totalPoints()));
}

int SweepRunner::gridSteps() const
{
    // Outer loop step count
    if (m_params.sweepType == SweepType::Anode) {
        if (m_params.vgStep <= 0.0) return 1;
        return static_cast<int>(std::round((m_params.vgStop - m_params.vgStart) / m_params.vgStep)) + 1;
    } else {
        // Transfer characteristic outer loop is Vg2 (screen voltage)
        if (m_params.vg2Step <= 0.0) return 1;
        return static_cast<int>(std::round((m_params.vg2Stop - m_params.vg2Start) / m_params.vg2Step)) + 1;
    }
}

int SweepRunner::anodeSteps() const
{
    // Inner loop step count
    if (m_params.sweepType == SweepType::Anode) {
        if (m_params.vaStep <= 0.0) return 1;
        return static_cast<int>(std::round((m_params.vaStop - m_params.vaStart) / m_params.vaStep)) + 1;
    } else {
        if (m_params.vgStep <= 0.0) return 1;
        return static_cast<int>(std::round((m_params.vgStop - m_params.vgStart) / m_params.vgStep)) + 1;
    }
}

int SweepRunner::totalPoints() const
{
    return gridSteps() * anodeSteps();
}

void SweepRunner::onMeasurementReady(double vaVolts, double iaMa, double ig2Ma)
{
    if (!m_running)
        return;

    SweepSample sample;
    sample.vaVolts  = vaVolts;
    sample.iaMa     = iaMa;
    sample.ig2Ma    = ig2Ma;

    // Record the set Vg1 for this point
    if (m_params.sweepType == SweepType::Anode)
        sample.vg1Volts = -m_currentVg;
    else
        sample.vg1Volts = -(m_params.vgStop - m_anodeIndex * m_params.vgStep);

    m_currentSweep.samples.append(sample);
    emit sampleAcquired(m_currentSweep, sample);

    // ── Overload protection ───────────────────────────────────────────────────
    // iaMax is a Y-axis display scale only.  pMax is the datasheet rated peak
    // dissipation.  Because each measurement point is applied for only a few
    // milliseconds — far shorter than the anode's thermal time constant — the
    // actual trip threshold is scaled up from the datasheet value.  3× gives
    // adequate protection against genuine faults (shorts, deep saturation)
    // while not clipping normal characteristic curves near the rated operating
    // point.  Commercial curve tracers typically use 3–5×.
    constexpr double kPMaxScale = 3.0;
    const double powerW  = vaVolts * iaMa / 1000.0;
    const bool overPower = m_params.pMax > 0.0 && powerW > m_params.pMax * kPMaxScale;

    if (overPower) {
        // Store the partial curve that was collected so far
        m_measurement.sweeps.append(m_currentSweep);

        // Advance outer loop
        ++m_gridIndex;
        if (m_gridIndex < gridSteps()) {
            m_anodeIndex  = 0;
            if (m_params.sweepType == SweepType::Anode) {
                m_currentVg            = m_params.vgStart + m_gridIndex * m_params.vgStep;
                m_currentSweep         = Sweep{};
                m_currentSweep.vgVolts  = -m_currentVg;
                m_currentSweep.vg2Volts = m_params.vg2;
            } else {
                m_currentVg            = m_params.vg2Start + m_gridIndex * m_params.vg2Step;
                m_currentSweep         = Sweep{};
                m_currentSweep.vgVolts  = 0.0;
                m_currentSweep.vg2Volts = m_currentVg;
            }
            sendNextPoint();
            return;
        }

        // No more grid steps — measurement is complete
        m_running = false;
        if (m_params.dischargeOnComplete) m_device->safeMode();
        emit progressChanged(100);
        emit measurementComplete(m_measurement);
        return;
    }

    // Advance inner loop
    ++m_anodeIndex;
    const bool innerContinues = (m_params.sweepType == SweepType::Anode)
        ? (m_params.vaStart + m_anodeIndex * m_params.vaStep <= m_params.vaStop + 1e-9)
        : (m_anodeIndex < anodeSteps());

    if (innerContinues) {
        sendNextPoint();
        return;
    }

    // Inner loop done — store completed curve
    m_measurement.sweeps.append(m_currentSweep);

    // Advance outer loop
    ++m_gridIndex;
    if (m_gridIndex < gridSteps()) {
        m_anodeIndex  = 0;
        if (m_params.sweepType == SweepType::Anode) {
            m_currentVg            = m_params.vgStart + m_gridIndex * m_params.vgStep;
            m_currentSweep         = Sweep{};
            m_currentSweep.vgVolts  = -m_currentVg;
            m_currentSweep.vg2Volts = m_params.vg2;
        } else {
            m_currentVg            = m_params.vg2Start + m_gridIndex * m_params.vg2Step;
            m_currentSweep         = Sweep{};
            m_currentSweep.vgVolts  = 0.0;
            m_currentSweep.vg2Volts = m_currentVg;
        }
        sendNextPoint();
        return;
    }

    // All sweeps done
    m_running = false;
    if (m_params.dischargeOnComplete) m_device->safeMode();
    emit progressChanged(100);
    emit measurementComplete(m_measurement);
}

// ─── onMeasurementFailed ─────────────────────────────────────────────────────
// Called when the firmware returns ERR: for an M1 command (e.g. HT voltage
// could not be regulated to the target).  Skip the failed point and continue.

void SweepRunner::onMeasurementFailed()
{
    if (!m_running)
        return;

    // Skip this point — advance the inner loop without adding a sample.
    ++m_anodeIndex;
    const bool innerContinues = (m_params.sweepType == SweepType::Anode)
        ? (m_params.vaStart + m_anodeIndex * m_params.vaStep <= m_params.vaStop + 1e-9)
        : (m_anodeIndex < anodeSteps());

    if (innerContinues) {
        sendNextPoint();
        return;
    }

    // Inner loop exhausted — store the curve (may be empty if all points failed).
    m_measurement.sweeps.append(m_currentSweep);

    ++m_gridIndex;
    if (m_gridIndex < gridSteps()) {
        m_anodeIndex  = 0;
        if (m_params.sweepType == SweepType::Anode) {
            m_currentVg            = m_params.vgStart + m_gridIndex * m_params.vgStep;
            m_currentSweep         = Sweep{};
            m_currentSweep.vgVolts  = -m_currentVg;
            m_currentSweep.vg2Volts = m_params.vg2;
        } else {
            m_currentVg            = m_params.vg2Start + m_gridIndex * m_params.vg2Step;
            m_currentSweep         = Sweep{};
            m_currentSweep.vgVolts  = 0.0;
            m_currentSweep.vg2Volts = m_currentVg;
        }
        sendNextPoint();
        return;
    }

    m_running = false;
    if (m_params.dischargeOnComplete) m_device->safeMode();
    emit progressChanged(100);
    emit measurementComplete(m_measurement);
}
