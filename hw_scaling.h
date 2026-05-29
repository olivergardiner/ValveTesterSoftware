#ifndef HW_SCALING_H
#define HW_SCALING_H

// ─── Hardware scaling constants ───────────────────────────────────────────────
// Converts between raw firmware protocol values and engineering units (V, mA).
//
// Firmware protocol (all values are raw ADC/DAC counts):
//   S0 <code>  – Grid 1 DAC code  (0–4095)
//   S1 <code>  – Grid 2 DAC code  (0–4095)
//   S2 <code>  – Target HT1       (10-bit ADC counts, 0–1023)
//   S3 <code>  – Target HT2       (10-bit ADC counts, 0–1023)
//
//   Response CSV (12 values):
//     grid1, grid2,
//     targetHT1, targetHT2,
//     measuredHT1, measuredHT2,
//     currentLo1, currentMid1, currentHi1,
//     currentLo2, currentMid2, currentHi2

namespace Hw {

// ─── Grid DAC (MCP4725, one per channel) ─────────────────────────────────────
// 12-bit range (0–4095), Vref = 4.096 V, output drives an inverting HV stage
// with gain magnitude 16.5.
//
// Grid voltage = -(code / 4095) × 4.096 × 16.5   →   max ≈ -67.6 V
// Resolution   ≈ 16.5 mV per DAC step

inline constexpr int    DAC_BITS      = 12;
inline constexpr int    DAC_MAX       = (1 << DAC_BITS) - 1;   // 4095
inline constexpr double DAC_VREF      = 4.096;                  // V
inline constexpr double GRID_AMP_GAIN = 16.5;                   // magnitude (inverting)

// ─── HT voltage sense ────────────────────────────────────────────────────────
// Potential divider: 3 × 470 kΩ (top rail) / 2 × 4.7 kΩ (bottom rail),
// buffered by an op-amp follower into a 10-bit ADC, Vref = 4.096 V.
//
// V_HT = count × (4.096 / 1024) × (1 419 400 / 9 400)  ≈  count × 0.604 V/count
// Max measurable HT at count 1023 ≈ 618 V

inline constexpr double HT_R_TOP    = 3.0 * 470'000.0;         // 1 410 000 Ω
inline constexpr double HT_R_BOT    = 2.0 *   4'700.0;         //     9 400 Ω
inline constexpr int    HT_ADC_BITS = 10;
inline constexpr int    HT_ADC_MAX  = (1 << HT_ADC_BITS) - 1;  // 1023
inline constexpr double HT_ADC_VREF = 4.096;                    // V

// ─── Anode current sense ─────────────────────────────────────────────────────
// Three ranges per channel; 10-bit ADC, Vref = 4.096 V.
//
//   Hi  channel – medium sense resistor followed by × 4 op-amp  (most sensitive)
//   Mid channel – medium sense resistor only
//   Lo  channel – small sense resistor  (highest current range)
//
// Hi  resolution  ≈ 0.030 mA / count   →  full scale ≈  30.7 mA
// Mid resolution  ≈ 0.120 mA / count   →  full scale ≈ 122.8 mA
// Lo  resolution  ≈ 1.200 mA / count   →  full scale ≈  1.23 A

inline constexpr double RSENSE_MED    = 100.0 / 3.0;            // 33⅓ Ω
inline constexpr double RSENSE_LO     = 10.0  / 3.0;            // 3⅓ Ω
inline constexpr double HI_SENSE_GAIN = 4.0;                     // extra op-amp on hi channel
inline constexpr int    IA_ADC_BITS   = 10;
inline constexpr int    IA_ADC_MAX    = (1 << IA_ADC_BITS) - 1; // 1023
inline constexpr double IA_ADC_VREF   = 4.096;                   // V

// Threshold (raw ADC counts) below which a channel is considered not saturated.
// Must be ≤ 1023 so that the Nano's sentinel value of 1023 (hi channel unavailable)
// is treated as saturated, causing the logic to fall through to the mid channel.
#define CURRENT_THRESHOLD 1000

// ─── Conversion functions ─────────────────────────────────────────────────────

/// Grid voltage (V, must be ≤ 0) → DAC code to send in S0/S1 command (0–4095)
inline int gridVoltageToDacCode(double gridV)
{
    if (gridV > 0.0) gridV = 0.0;
    const double minV = -(DAC_VREF * GRID_AMP_GAIN);
    if (gridV < minV) gridV = minV;
    return static_cast<int>(-gridV / (DAC_VREF * GRID_AMP_GAIN) * DAC_MAX + 0.5);
}

/// DAC code (0–4095) → grid voltage (V, ≤ 0)
inline double dacCodeToGridVoltage(int code)
{
    return -static_cast<double>(code) / DAC_MAX * DAC_VREF * GRID_AMP_GAIN;
}

/// HT ADC count (0–1023) → HT bus voltage (V)
inline double htCountToVolts(int count)
{
    const double vAdc = static_cast<double>(count) * HT_ADC_VREF / (HT_ADC_MAX + 1);
    return vAdc * (HT_R_TOP + HT_R_BOT) / HT_R_BOT;
}

/// Target HT voltage (V) → ADC count to send in S2/S3 command (0–1023, clamped)
inline int htVoltsToCount(double volts)
{
    if (volts < 0.0) volts = 0.0;
    const double count = volts / (HT_ADC_VREF / (HT_ADC_MAX + 1))
                                * HT_R_BOT / (HT_R_TOP + HT_R_BOT);
    if (count > HT_ADC_MAX) return HT_ADC_MAX;
    return static_cast<int>(count + 0.5);
}

/// Convert the three raw current-sense ADC readings for one channel to mA.
/// Selects the most sensitive unsaturated range.
inline double adcToCurrentMa(int hiAdc, int midAdc, int loAdc)
{
    const double vPerStep = IA_ADC_VREF / (IA_ADC_MAX + 1);

    if (hiAdc < CURRENT_THRESHOLD)
        return hiAdc  * vPerStep / (RSENSE_MED * HI_SENSE_GAIN) * 1000.0;

    if (midAdc < CURRENT_THRESHOLD)
        return midAdc * vPerStep / RSENSE_MED * 1000.0;

    return     loAdc  * vPerStep / RSENSE_LO  * 1000.0;
}

} // namespace Hw

#endif // HW_SCALING_H