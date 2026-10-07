#pragma once
/*
 * DNMS_def.h - Central definitions for DNMS
 * Using constexpr instead of #define for type safety
 */

static constexpr uint16_t number_FIR_taps_decimation = 481u;
static constexpr uint16_t number_FFT_bins             = 2048u;
static constexpr uint16_t number_ZoomFFT_bins         = 512u;
