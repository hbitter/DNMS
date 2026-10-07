#pragma once
/*
 * Micro_calib_data.h
 *
 * Microphone calibration tables — one set of ~30 measurement points
 * (1/3-octave centre frequencies, 20 Hz … 20 kHz) per microphone type.
 *
 * Each entry: { frequency_Hz,  dB_correction }
 *   dB_correction = SPL_reference − SPL_DUT  (positive = DUT reads low)
 *   The interpolation code converts this to a linear power multiplier:
 *     corr_Z[bin] = 10 ^ (dB_correction / 10)
 *
 * Values were derived from the original per-bin correction tables by
 * sampling at the nearest FFT4096 bin for each 1/3-octave centre frequency
 * (fs = 44100 Hz, N = 4096  →  10.767 Hz/bin).
 *
 * The same calibration points are used for both FFT4096 and FFT1024.
 * The interpolation function maps them to the correct bin for each FFT
 * (FFT1024 after ÷32 decimation: 1.346 Hz/bin, Nyquist ≈ 689 Hz).
 *
 * To calibrate a new microphone: measure its SPL response against a
 * calibrated reference at each listed frequency and fill in the dB values.
 * Add a new block following the same pattern and add a command in
 * handle_pending_mic_change().
 */

struct CalibPoint {
    float freq_Hz;
    float dB_correction;
};

/* ── No correction (flat response) ────────────────────────────────────── */
static const CalibPoint calib_no_corr[] = {
    {    20.0f,  0.0f },
    { 20000.0f,  0.0f },
};
static constexpr uint8_t CALIB_NO_CORR_COUNT =
    (uint8_t)(sizeof(calib_no_corr) / sizeof(calib_no_corr[0]));

/* ── ICS-43434 ─────────────────────────────────────────────────────────── */
static const CalibPoint calib_ICS43434[] = {
    {    20.00f,  +3.1679f },  // FFT1024 bin 15 (20.19 Hz)
    {    25.00f,  +4.5908f },
    {    31.50f,  +7.0490f },
    {    40.00f,  +3.5131f },
    {    50.00f,  +2.4759f },
    {    63.00f,  +2.0689f },
    {    80.00f,  +1.6867f },
    {   100.00f,  +0.7981f },
    {   125.00f,  +0.2619f },
    {   160.00f,  +0.4961f },
    {   200.00f,  +0.4757f },
    {   250.00f,  +0.2569f },
    {   315.00f,  +0.2552f },
    {   400.00f,  +0.1139f },
    {   500.00f,  -0.1494f },
    {   630.00f,  +0.2407f },
    {   800.00f,  +0.7788f },
    {  1000.00f,  +0.0683f },
    {  1250.00f,  -0.0570f },
    {  1600.00f,  -0.1534f },
    {  2000.00f,  -0.3387f },
    {  2500.00f,  -0.2782f },
    {  3150.00f,  -0.7201f },
    {  4000.00f,  -1.3934f },
    {  5000.00f,  -2.0110f },
    {  6300.00f,  -2.2938f },
    {  8000.00f,  -3.2128f },
    { 10000.00f,  -7.8251f },
    { 12500.00f, -11.2090f },
    { 16000.00f, -16.4307f },
    { 20000.00f, -25.2478f },
};
static constexpr uint8_t CALIB_ICS43434_COUNT =
    (uint8_t)(sizeof(calib_ICS43434) / sizeof(calib_ICS43434[0]));

/* ── IM72D128 ──────────────────────────────────────────────────────────── */
static const CalibPoint calib_IM72D128[] = {
    {    20.00f,  +3.2826f },  // FFT1024 bin 15 (20.19 Hz)
    {    25.00f,  +3.0568f },
    {    31.50f,  +4.0383f },
    {    40.00f,  +0.9893f },
    {    50.00f,  +0.9373f },
    {    63.00f,  +1.1978f },
    {    80.00f,  +1.0414f },
    {   100.00f,  +0.3559f },
    {   125.00f,  +0.0399f },
    {   160.00f,  +0.3603f },
    {   200.00f,  +0.5653f },
    {   250.00f,  +0.2596f },
    {   315.00f,  +0.2142f },
    {   400.00f,  +0.1807f },
    {   500.00f,  -0.0171f },
    {   630.00f,  +0.3713f },
    {   800.00f,  +0.6080f },
    {  1000.00f,  +0.4057f },
    {  1250.00f,  -0.0141f },
    {  1600.00f,  -0.1656f },
    {  2000.00f,  +0.1020f },
    {  2500.00f,  -0.1345f },
    {  3150.00f,  -0.9334f },
    {  4000.00f,  -0.9785f },
    {  5000.00f,  -1.1592f },
    {  6300.00f,  -1.6668f },
    {  8000.00f,  -3.2580f },
    { 10000.00f,  -6.4877f },
    { 12500.00f,  -8.8333f },
    { 16000.00f, -11.3088f },
    { 20000.00f, -15.2310f },
};
static constexpr uint8_t CALIB_IM72D128_COUNT =
    (uint8_t)(sizeof(calib_IM72D128) / sizeof(calib_IM72D128[0]));

/* ── IM72D128 with DLR housing correction ─────────────────────────────── */
static const CalibPoint calib_IM72D128_DLR[] = {
    {    20.00f,  +2.3860f },  // FFT1024 bin 15 (20.19 Hz)
    {    25.00f,  +2.3469f },
    {    31.50f,  +3.0703f },
    {    40.00f,  +0.3928f },
    {    50.00f,  +0.5531f },
    {    63.00f,  +0.8455f },
    {    80.00f,  +0.6703f },
    {   100.00f,  +0.0053f },
    {   125.00f,  -0.3025f },
    {   160.00f,  +0.0228f },
    {   200.00f,  +0.2858f },
    {   250.00f,  +0.0507f },
    {   315.00f,  +0.0274f },
    {   400.00f,  -0.0315f },
    {   500.00f,  -0.1850f },
    {   630.00f,  +0.3670f },
    {   800.00f,  +0.5836f },
    {  1000.00f,  +0.3783f },
    {  1250.00f,  +0.2513f },
    {  1600.00f,  +0.4471f },
    {  2000.00f,  +0.0848f },
    {  2500.00f,  +0.4052f },
    {  3150.00f,  +0.9752f },
    {  4000.00f,  -1.5955f },
    {  5000.00f,  -5.4558f },
    {  6300.00f,  -7.2532f },
    {  8000.00f,  -6.4252f },
    { 10000.00f,  -7.6011f },
    { 12500.00f,  -9.7074f },
    { 16000.00f,  -8.4933f },
    { 20000.00f, -15.6855f },
};
static constexpr uint8_t CALIB_IM72D128_DLR_COUNT =
    (uint8_t)(sizeof(calib_IM72D128_DLR) / sizeof(calib_IM72D128_DLR[0]));
