#pragma once
/*
 * dnms_calib_data.h  —  Microphone calibration tables for ESP8266 firmware.
 *
 * Stored in PROGMEM to keep the data in flash instead of RAM.
 * Use memcpy_P() to read individual points into a local struct:
 *
 *   struct dnms_calib_point pt;
 *   memcpy_P(&pt, &calib_ICS43434[i], sizeof(pt));
 *
 * Tables and constants match the Raspberry Pi driver (dnms_calib_data.h)
 * and the Teensy firmware (Micro_calib_data.h).
 */

struct dnms_calib_point {
    float freq_hz;
    float db_correction;
};

/* ── No correction (flat response) ─────────────────────────────────────── */
static const struct dnms_calib_point PROGMEM calib_no_corr[] = {
    {    20.0f,  0.0f },
    { 20000.0f,  0.0f },
};
#define DNMS_CALIB_NO_CORR_COUNT  2

/* ── ICS-43434 ──────────────────────────────────────────────────────────── */
static const struct dnms_calib_point PROGMEM calib_ICS43434[] = {
    {    20.00f,  +3.1679f },
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
#define DNMS_CALIB_ICS43434_COUNT  31

/* ── IM72D128 ────────────────────────────────────────────────────────────── */
static const struct dnms_calib_point PROGMEM calib_IM72D128[] = {
    {    20.00f,  +3.2826f },
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
#define DNMS_CALIB_IM72D128_COUNT  31

/* ── IM72D128 with DLR housing correction ───────────────────────────────── */
static const struct dnms_calib_point PROGMEM calib_IM72D128_DLR[] = {
    {    20.00f,  +2.3860f },
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
#define DNMS_CALIB_IM72D128_DLR_COUNT  31
