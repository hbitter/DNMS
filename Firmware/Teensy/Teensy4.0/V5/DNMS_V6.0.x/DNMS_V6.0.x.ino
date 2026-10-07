/************************************************************************
* DNMS - Digital Noise Measurement Sensor
*
* Board : Teensy 4.0 (ARM Cortex-M7 @ 600 MHz)
* Input : I2S from ICS-43434 or IM72D128 microphone
* Output : I2C slave to external controller (NodeMCU or Raspberry Pi Zero 2W and above)
*
* Copyright (C) 2021-2026 Helmut Bitter
* Copyright (C) 2021-2026 Burkhard Volkemer (FFT parts)
* GNU GPL v3 or later
************************************************************************/
/* 
Memory Usage on Teensy 4.0:
  FLASH: code:74288, data:67528, headers:8708   free for files:1881092
   RAM1: variables:238752, code:72104, padding:26200   free for local variables:187232
   RAM2: variables:16064  free for malloc/new:508224
*/

#include <Arduino.h>
#include <math.h>
#include <Audio.h>
#include <dnms_audio_lib.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

#include "DNMS_def.h"
#include "FIR_decimation_filter.h"
#include "Micro_calib_data.h"   // replaces the four large per-bin correction headers
#include "A_weighting.h"
#include "C_weighting.h"

// ── firmware versionstring ─────────────────────────────────────────────────────
static const char SOFTWARE_VERSION[] = "DNMS Version 6.0.x ";

// ── Timing ────────────────────────────────────────────────────────────
static const uint32_t BLINK_PERIOD_ICS43434 = 100u;
static const uint32_t BLINK_PERIOD_IM72D128 = 500u;
static const uint32_t BLINK_PERIOD_MICRO3   = 200u;
static const uint32_t BLINK_PERIOD_MICRO4   = 1000u;

// ── FFT-Configuration ─────────────────────────────────────────────────
static const uint8_t NUMBER_OF_FFTS = 1u;
static const uint8_t FFT_CALCULATIONS_IN_START_PHASE = 44u;
static const uint8_t BLK_LEN = 128u;
static const uint8_t DECI_FACTOR = 32u;
static const double WINDOW_HANNING = 0.3750;

// ── I2C-Configuration ─────────────────────────────────────────────────
static const uint8_t DNMS_I2C_ADDRESS = 0x55u;
static const uint8_t DNMS_WORD_SIZE = 2u;
static const uint8_t CRC8_POLYNOMIAL = 0x31u;
static const uint8_t CRC8_INIT_VAL = 0xFFu;
static const uint8_t I2C_BUF_SIZE = 64u;

// ── Microphone-constants ──────────────────────────────────────
static const double MICRO_CONST_ICS43434 = 123.0102999565;
static const double MICRO_CONST_IM72D128 = 133.0102999565;

// ── Debug-Pins ────────────────────────────────────────────────────────
#define DNMS_DEBUG

static const uint8_t PIN_FFT4096 = 1u;
static const uint8_t PIN_FFT1024 = 3u;
static const uint8_t PIN_I2C_1ST = 5u;
static const uint8_t PIN_I2C_2ND = 7u;

#ifdef DNMS_DEBUG
#define DBG_HIGH(p) digitalWrite((p), HIGH)
#define DBG_LOW(p) digitalWrite((p), LOW)
#else
#define DBG_HIGH(p) ((void)0)
#define DBG_LOW(p) ((void)0)
#endif

// Frequency used for bin 0 (DC) in the calibration interpolation.
// DC has no physical frequency, so we use the first calibration point's
// frequency to avoid log(0) and to apply a sensible low-frequency value.
static constexpr float pts_guard = 1.0f;  // 1 Hz placeholder for bin 0

// ── Mathematische Hilfskonstanten ─────────────────────────────────────
static const double LN10_DIV_10 = 0.23025850929940456840;
static const double TEN_OVER_LN10 = 4.342944819032518;

static inline double delog(double x) {
  return exp(x * LN10_DIV_10);
}
static inline double ten_log10(double x) {
  return TEN_OVER_LN10 * log(x);
}
static inline uint16_t swap16(uint16_t s) {
  return static_cast<uint16_t>((s << 8u) | (s >> 8u));
}

// ── Band-Größen als enum – verhindert Doppeldefinition ────────────────
// FIX: Ursprünglich war BAND4096_COUNT und BAND1024_COUNT zweimal als
// static const uint8_t definiert (einmal ganz oben für die Struct,
// einmal weiter unten), was einen "redefinition"-Fehler ergab.
// Lösung: Ein einziger anonymer enum ersetzt beide Vorkommen.
enum : uint8_t {
  BAND4096_COUNT = 32u,
  BAND1024_COUNT = 18u
};

// ─────────────────────────────────────────────────────────────────────
// LeqInterval
// Enthält alle Akkumulatoren und Snapshot-Werte für eine Messperiode.
// FIX: Struct nur EINMAL definiert (vorher war sie doppelt vorhanden).
// Enum-Konstanten oben machen die Inline-Initialisierung möglich.
// ──────────────────────────────────────────────────────��──────────────
struct LeqInterval {
  uint32_t loop_count = 0u;

  float leq_A_min = 0.0f, leq_A_max = 0.0f;
  double leq_g_A = 0.0, sum_g_A = 0.0;
  float last_g_A = 0.0f, last_A_min = 0.0f, last_A_max = 0.0f;

  float leq_Z_min = 0.0f, leq_Z_max = 0.0f;
  double leq_g_Z = 0.0, sum_g_Z = 0.0;
  float last_g_Z = 0.0f, last_Z_min = 0.0f, last_Z_max = 0.0f;

  float leq_C_min = 0.0f, leq_C_max = 0.0f;
  double leq_g_C = 0.0, sum_g_C = 0.0;
  float last_g_C = 0.0f, last_C_min = 0.0f, last_C_max = 0.0f;

  uint32_t loop_count_fft = 0u;
  double sum_band_Z[BAND4096_COUNT] = {};
  double sum_band_A[BAND4096_COUNT] = {};
  double sum_band_C[BAND4096_COUNT] = {};
  double leq_band_g_Z[BAND4096_COUNT] = {};
  double leq_band_g_A[BAND4096_COUNT] = {};
  double leq_band_g_C[BAND4096_COUNT] = {};
  double last_band_g_Z[BAND4096_COUNT] = {};
  double last_band_g_A[BAND4096_COUNT] = {};
  double last_band_g_C[BAND4096_COUNT] = {};

  uint32_t loop_count_fft1024 = 0u;
  double sum_band1024_Z[BAND1024_COUNT] = {};
  double sum_band1024_A[BAND1024_COUNT] = {};
  double sum_band1024_C[BAND1024_COUNT] = {};
  double leq_band1024_g_Z[BAND1024_COUNT] = {};
  double leq_band1024_g_A[BAND1024_COUNT] = {};
  double leq_band1024_g_C[BAND1024_COUNT] = {};
  double last_band1024_g_Z[BAND1024_COUNT] = {};
  double last_band1024_g_A[BAND1024_COUNT] = {};
  double last_band1024_g_C[BAND1024_COUNT] = {};
};

// ── Vorwärtsdeklarationen ──────────────────────────────────────────────
// FIX: Explizite Prototypen verhindern fehlerhafte Auto-Prototypen
// des Arduino-Preprocessors bei Funktionen mit Referenz-Parametern.
static void reset_interval(LeqInterval& iv);
static void update_broadband_leq(LeqInterval& iv,
                                 double leq_A, double leq_Z, double leq_C);
static void update_band4096_leq(LeqInterval& iv);
static void update_band1024_leq(LeqInterval& iv);
static void finalize_interval(LeqInterval& iv,
                              volatile uint8_t& calc_flag,
                              volatile uint16_t& ready_flag);
void i2c_receive_from_master(int num_bytes);
void i2c_request_from_master(void);
void DNMS_reset(void);
static double corr_interp_dB(float f_hz, const CalibPoint* pts, uint8_t n);
static void   compute_corrections(const CalibPoint* calib, uint8_t n);
static void   set_microphone(const CalibPoint* calib, uint8_t calib_count,
                              uint8_t type, uint32_t blink, double mic_const);

// ── Audio-Pipeline ────────────────────────────────────────────────────
AudioInputI2S_F32 i2s_in;
AudioAnalyzeFFT4096_F32 dnms_fft;
AudioAnalyzeFFT1024_F32 dnms_fft1024;
AudioDecimationFilter_F32 dnms_decimation;

AudioConnection_F32 patchCord4(i2s_in, 0, dnms_fft, 0);
AudioConnection_F32 patchCord5(i2s_in, 0, dnms_decimation, 0);
AudioConnection_F32 patchCord6(dnms_decimation, 0, dnms_fft1024, 0);

// ── Dezimierungsfilter-Puffer ─────────────────────────────────────────
static const uint16_t DECI_BUF_LEN = static_cast<uint16_t>(BLK_LEN) * DECI_FACTOR;
static const uint16_t DECI_STATE_LEN = number_FIR_taps_decimation + DECI_BUF_LEN - 1u;

static float32_t decimation_buffer[DECI_BUF_LEN];
static float32_t decimation_state_buffer[DECI_STATE_LEN];

// ── Fenster-Normalisierungsfaktoren ───────────────────────────────────
static double window_norm4096 = 0.0;
static double window_norm1024 = 0.0;

// ── Globale Intervall-Instanzen ───────────────────────────────────────
static LeqInterval g_iv1;
static LeqInterval g_iv2;

// ── Mikrofon-Korrektur-Zeiger ─────────────────────────────────────────
// All correction and weighting tables are float[].
// corr_Z / corr1024_Z point to the arrays computed at startup.
// corr_A / corr_C / corr1024_A / corr1024_C point to the static weighting tables.
static float computed_corr_Z    [number_FFT_bins]     = {};
static float computed_corr1024_Z[number_ZoomFFT_bins] = {};

static const float* corr_Z      = computed_corr_Z;
static const float* corr_A      = nullptr;
static const float* corr_C      = nullptr;
static const float* corr1024_Z  = computed_corr1024_Z;
static const float* corr1024_A  = nullptr;
static const float* corr1024_C  = nullptr;

// ── Microphone frequency-response correction ──────────────────────────
// Log-frequency linear interpolation of dB values from CalibPoint table.
// Returns the dB correction for frequency f_hz, clamping to the table edges.
static double corr_interp_dB(float f_hz,
                              const CalibPoint* pts, uint8_t n) {
    if (n == 0u) return 0.0;
    if (f_hz <= pts[0].freq_Hz)       return (double)pts[0].dB_correction;
    if (f_hz >= pts[n-1u].freq_Hz)    return (double)pts[n-1u].dB_correction;
    for (uint8_t k = 0u; k < n - 1u; ++k) {
        if (f_hz <= pts[k+1u].freq_Hz) {
            double t = log((double)f_hz            / (double)pts[k].freq_Hz)
                     / log((double)pts[k+1u].freq_Hz / (double)pts[k].freq_Hz);
            return (double)pts[k].dB_correction
                 + t * ((double)pts[k+1u].dB_correction
                      - (double)pts[k].dB_correction);
        }
    }
    return (double)pts[n-1u].dB_correction;
}

// Compute both correction arrays from a single calibration table.
// Called once in setup() and again whenever the microphone type changes.
static void compute_corrections(const CalibPoint* calib, uint8_t n) {
    // FFT4096: fs = 44100 Hz, N = 4096  →  10.7666… Hz / bin
    constexpr float fs4 = 44100.0f / 4096.0f;
    for (uint16_t b = 0u; b < number_FFT_bins; ++b) {
        float f = (b == 0u) ? pts_guard : (float)b * fs4;
        computed_corr_Z[b] = static_cast<float>(pow(10.0, corr_interp_dB(f, calib, n) / 10.0));
    }
    // FFT1024 (after ÷32 decimation): fs = 44100/32 = 1378.125 Hz, N = 1024
    //   → 1.34583… Hz / bin, Nyquist ≈ 689 Hz
    constexpr float fs10 = (44100.0f / 32.0f) / 1024.0f;
    for (uint16_t b = 0u; b < number_ZoomFFT_bins; ++b) {
        float f = (b == 0u) ? pts_guard : (float)b * fs10;
        computed_corr1024_Z[b] = static_cast<float>(pow(10.0, corr_interp_dB(f, calib, n) / 10.0));
    }
}

static uint8_t micro_type = 1u;
static uint32_t blink_period = BLINK_PERIOD_ICS43434;
static double micro_constant = MICRO_CONST_ICS43434;

// Solid ON until the first measurement trigger (cmd 3) arrives from Pi.
// Calibration is a prerequisite, but blink starts only when measurement begins.
static volatile bool measurement_started = false;

// ── Calibration receive buffer (Pi → Teensy upload, cmds 42/43) ──────
static constexpr uint8_t CALIB_MAX_POINTS = 31u;
static CalibPoint calib_rx[CALIB_MAX_POINTS] = {};

static volatile uint8_t  pending_reset = 0u;
static volatile uint8_t  pending_calib_apply = 0u;
static uint8_t  pending_calib_count = 0u;
static uint8_t  pending_calib_type  = 0u;
static uint32_t pending_calib_blink = 0u;
static float    pending_calib_const = 0.0f;

// ── I2C-Zustandsvariablen (volatile: ISR-Zugriff) ─────────────────────
static volatile uint8_t i2c_state = 0u;
static volatile uint16_t data_ready = 0u;
static volatile uint16_t data_ready_2nd = 0u;
static volatile uint8_t calculate_leq = 0u;
static volatile uint8_t calculate_leq_2nd = 0u;

static uint8_t i2c_buf[I2C_BUF_SIZE] = {};
static uint16_t command_received = 0u;

// ── Anlaufphase ───────────────────────────────────────────────────────
static bool start_phase = true;
static uint16_t start_phase_cnt = 0u;

// ── LED-Blink-Zustand ──────────────────────────────────���──────────────
static uint32_t last_blink_ms = 0u;
static bool led_on = false;
static uint8_t led_blink_cnt = 0u;

// ── FFT-Spektral-Arbeitspuffer ────────────────────────────────────────
// float: passt direkt zum FFT-Ausgabetyp der Audio-Bibliothek
static float freq_Z[number_FFT_bins] = {};
static float freq_A[number_FFT_bins] = {};
static float freq_C[number_FFT_bins] = {};
static float freq1024_Z[number_ZoomFFT_bins] = {};
static float freq1024_A[number_ZoomFFT_bins] = {};
static float freq1024_C[number_ZoomFFT_bins] = {};

// ── FFT4096-Breitband-Akkumulatoren ───────────────────────────────────
static float fft_all_A = 0.0f, mean_fft_all_A = 0.0f;
static float fft_all_Z = 0.0f, mean_fft_all_Z = 0.0f;
static float fft_all_C = 0.0f, mean_fft_all_C = 0.0f;
static uint32_t mean_count_fft = 0u;

static float freq_band_Z[BAND4096_COUNT] = {};
static float freq_band_A[BAND4096_COUNT] = {};
static float freq_band_C[BAND4096_COUNT] = {};
static float mean_freq_band_Z[BAND4096_COUNT] = {};
static float mean_freq_band_A[BAND4096_COUNT] = {};
static float mean_freq_band_C[BAND4096_COUNT] = {};

static float freq_band1024_Z[BAND1024_COUNT] = {};
static float freq_band1024_A[BAND1024_COUNT] = {};
static float freq_band1024_C[BAND1024_COUNT] = {};
static float mean_freq_band1024_Z[BAND1024_COUNT] = {};
static float mean_freq_band1024_A[BAND1024_COUNT] = {};
static float mean_freq_band1024_C[BAND1024_COUNT] = {};
static uint32_t mean_count_fft1024 = 0u;

// ── Band-Bin-Mapping-Tabellen ─────────────────────────────────────────
// FIX: Struct BandMap einmalig definiert, Längen per constexpr berechnet.
struct BandMap {
  uint8_t idx;
  uint16_t first;
  uint16_t last;
};

static const BandMap band4096_map[] = {
  { 14u, 33u, 41u }, { 15u, 42u, 51u }, { 16u, 52u, 65u }, { 17u, 66u, 82u }, { 18u, 83u, 103u }, { 19u, 104u, 130u }, { 20u, 131u, 164u }, { 21u, 165u, 207u }, { 22u, 208u, 261u }, { 23u, 262u, 328u }, { 24u, 329u, 414u }, { 25u, 415u, 521u }, { 26u, 522u, 656u }, { 27u, 657u, 826u }, { 28u, 827u, 1039u }, { 29u, 1040u, 1311u }, { 30u, 1312u, 1650u }, { 31u, 1651u, 2047u }
};
static const uint8_t BAND4096_MAP_LEN =
  static_cast<uint8_t>(sizeof(band4096_map) / sizeof(band4096_map[0]));

static const BandMap band1024_map[] = {
  { 1u, 13u, 16u }, { 2u, 17u, 20u }, { 3u, 21u, 25u }, { 4u, 26u, 32u }, { 5u, 33u, 41u }, { 6u, 42u, 52u }, { 7u, 53u, 65u }, { 8u, 66u, 82u }, { 9u, 83u, 104u }, { 10u, 105u, 131u }, { 11u, 132u, 166u }, { 12u, 167u, 209u }, { 13u, 210u, 263u }
};
static const uint8_t BAND1024_MAP_LEN =
  static_cast<uint8_t>(sizeof(band1024_map) / sizeof(band1024_map[0]));

// ── Min/Max-Hilfsfunktionen ───────────────────────────────────────────
// 0.0f als "noch nicht gesetzt"-Sentinel
static inline void update_min(float& stored_min, float new_val) {
  if (stored_min == 0.0f || new_val < stored_min) stored_min = new_val;
}
static inline void update_max(float& stored_max, float new_val) {
  if (stored_max == 0.0f || new_val > stored_max) stored_max = new_val;
}

// ─────────────────────────────────────────────────────────────────────
// sum_band_f()
// Summiert FFT-Bins [bin_start..bin_end] in dest_band[idx].
// Bleibt in float für FPU-Durchsatz auf Cortex-M7.
// ───────────────────────────────────���─────────────────────────────────
static void sum_band_f(float* dest_band, const float* src,
                       uint8_t band_idx,
                       uint16_t bin_start, uint16_t bin_end) {
  float acc = 0.0f;
  for (uint16_t k = bin_start; k <= bin_end; ++k) acc += src[k];
  dest_band[band_idx] = acc;
}

// ─────────────────────────────────────────────────────────────────────
// compute_leq()
// Breitband-Leq aus float-Summe und Anzahl Frames.
// ─────────────────────────────────────────────────────────────────────
static inline double compute_leq(float sum, uint32_t count, double norm) {
  double mean = (count > 0u)
                  ? (static_cast<double>(sum) * norm / static_cast<double>(count))
                  : 0.0;
  if (mean <= 0.0) mean = 1.0e-12;
  return micro_constant + ten_log10(mean);
}

// ──────────────────────────────────────────────────────────────���──────
// dnms_common_generate_crc()
// CRC-8 mit Polynom 0x31, Init 0xFF (Sensirion-Konvention).
// ─────────────────────────────────────────────────────────────────────
static uint8_t dnms_common_generate_crc(uint8_t* data, uint16_t count) {
  uint8_t crc = CRC8_INIT_VAL;
  for (uint16_t b = 0u; b < count; ++b) {
    crc ^= data[b];
    for (uint8_t bit = 8u; bit > 0u; --bit) {
      crc = (crc & 0x80u) ? static_cast<uint8_t>((crc << 1u) ^ CRC8_POLYNOMIAL)
                          : static_cast<uint8_t>(crc << 1u);
    }
  }
  return crc;
}

// ─────────────────────────────────────────────────────────────────────
// f2b()
// Interpretiert float als 4 rohe Bytes (little-endian,
// strict-aliasing-sicher via union + memcpy).
// ─────────────────────────────────────────────────────────────────────
static void f2b(float val, uint8_t* bytes_array) {
  union {
    float f;
    uint8_t b[4];
  } u;
  u.f = val;
  memcpy(bytes_array, u.b, 4u);
}

// Reconstruct a float from four I2C buffer bytes (big-endian, CRC slots skipped).
// a,b = high-word bytes (MSB first); c,d = low-word bytes (MSB first).
static inline float i2c_parse_float(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  uint32_t bits = (uint32_t(a) << 24u) | (uint32_t(b) << 16u)
                | (uint32_t(c) <<  8u) |  uint32_t(d);
  float f;
  memcpy(&f, &bits, 4u);
  return f;
}

// ─────────────────────────────────────────────────────────────────────
// write_float_with_crc()
// Kodiert einen float big-endian mit CRC-Bytes in i2c_buf.
// FIX: idx wird sequenziell erhöht – kein UB durch mehrfaches
// Modifizieren in einem Ausdruck.
// ─────────────────────────────────────────────────────────────────────
static void write_float_with_crc(float val, uint16_t& idx) {
  uint8_t b[4];
  f2b(val, b);

  // High-Word (Bytes 3, 2) + CRC
  i2c_buf[idx] = b[3];
  i2c_buf[idx + 1u] = b[2];
  i2c_buf[idx + 2u] = dnms_common_generate_crc(&i2c_buf[idx], DNMS_WORD_SIZE);
  idx += 3u;

  // Low-Word (Bytes 1, 0) + CRC
  i2c_buf[idx] = b[1];
  i2c_buf[idx + 1u] = b[0];
  i2c_buf[idx + 2u] = dnms_common_generate_crc(&i2c_buf[idx], DNMS_WORD_SIZE);
  idx += 3u;
}

// ─────────────────────────────────────────────────────────────────────
// write_band_values_i2c()
// Sendet Bänder [start_index..end_index] aus src[] über I2C.
// ─────────────────────────────────────────────────────────────────────
static void write_band_values_i2c(const double* src,
                                  uint32_t start_index,
                                  uint32_t end_index) {
  uint16_t idx = 0u;
  for (uint32_t i = start_index; i <= end_index; ++i) {
    write_float_with_crc(static_cast<float>(src[i]), idx);
  }
  Wire.write(i2c_buf, idx);
  i2c_state = 0u;
}

// ── Band-Writer-Wrapper ───────────────────────────────────────────────
// FIX: Inline-Wrapper halten den switch()-Block in
// i2c_request_from_master() kurz und lesbar.
static void fft_values_i2c_write(uint32_t s, uint32_t e) {
  write_band_values_i2c(g_iv1.last_band_g_A, s, e);
}
static void fft_Z_values_i2c_write(uint32_t s, uint32_t e) {
  write_band_values_i2c(g_iv1.last_band_g_Z, s, e);
}
static void fft_C_values_i2c_write(uint32_t s, uint32_t e) {
  write_band_values_i2c(g_iv1.last_band_g_C, s, e);
}
static void fft_values_2nd_i2c_write(uint32_t s, uint32_t e) {
  write_band_values_i2c(g_iv2.last_band_g_A, s, e);
}
static void fft_Z_values_2nd_i2c_write(uint32_t s, uint32_t e) {
  write_band_values_i2c(g_iv2.last_band_g_Z, s, e);
}
static void fft_C_values_2nd_i2c_write(uint32_t s, uint32_t e) {
  write_band_values_i2c(g_iv2.last_band_g_C, s, e);
}

// ─────────────────────────────────────────────────────────────────────
// reset_interval()
// Setzt alle Akkumulatoren eines LeqInterval auf null.
// FIX: memset auf die Arrays ist sicherer und kompakter als Einzelzuweisungen.
// ─────────────────────────────────────────────────────────────────────
static void reset_interval(LeqInterval& iv) {
  iv.loop_count = 0u;

  iv.leq_A_min = 0.0f;
  iv.leq_A_max = 0.0f;
  iv.leq_g_A = 0.0;
  iv.sum_g_A = 0.0;

  iv.leq_Z_min = 0.0f;
  iv.leq_Z_max = 0.0f;
  iv.leq_g_Z = 0.0;
  iv.sum_g_Z = 0.0;

  iv.leq_C_min = 0.0f;
  iv.leq_C_max = 0.0f;
  iv.leq_g_C = 0.0;
  iv.sum_g_C = 0.0;

  iv.loop_count_fft = 0u;
  for (uint8_t i = 14u; i < BAND4096_COUNT; ++i) {
    iv.sum_band_Z[i] = 0.0;
    iv.leq_band_g_Z[i] = 0.0;
    iv.sum_band_A[i] = 0.0;
    iv.leq_band_g_A[i] = 0.0;
    iv.sum_band_C[i] = 0.0;
    iv.leq_band_g_C[i] = 0.0;
  }

  iv.loop_count_fft1024 = 0u;
  for (uint8_t i = 1u; i < 14u; ++i) {
    iv.sum_band1024_Z[i] = 0.0;
    iv.leq_band1024_g_Z[i] = 0.0;
    iv.sum_band1024_A[i] = 0.0;
    iv.leq_band1024_g_A[i] = 0.0;
    iv.sum_band1024_C[i] = 0.0;
    iv.leq_band1024_g_C[i] = 0.0;
  }
}

// ─────────────────────────────────────────────────────────────────────
// DNMS_reset()
// Vollständiger Reset beider Intervalle; aktiviert Audio-Interrupts.
// ─────────────────────────────────────────────────────────────────────
void DNMS_reset(void) {
  // Clear pending-calculation flags before zeroing accumulators.
  // Without this, a calculate flag that was set just before the reset
  // survives into the next loop() and immediately snapshots zeros.
  calculate_leq     = 0u;
  calculate_leq_2nd = 0u;
  data_ready        = 0u;
  data_ready_2nd    = 0u;
  reset_interval(g_iv1);
  reset_interval(g_iv2);
  AudioInterrupts();
}

// ─────────────────────────────────────────────────────────────────────
// set_microphone()
// Zentralisierte Mikrofon-Auswahl.
// Berechnet aus der Kalibriertabelle (30 Punkte) per log-freq. Interpolation
// die 2048 + 512 Korrekturwerte, stoppt Audio, aktualisiert Zeiger,
// setzt Akkumulatoren zurück und startet Audio neu.
// ─────────────────────────────────────────────────────────────────────
static void set_microphone(const CalibPoint* calib,
                           uint8_t           calib_count,
                           uint8_t           type,
                           uint32_t          blink,
                           double            mic_const) {
  start_phase = true;
  start_phase_cnt = 0u;
  AudioNoInterrupts();
  compute_corrections(calib, calib_count);
  corr_Z     = computed_corr_Z;
  corr1024_Z = computed_corr1024_Z;
  micro_type    = type;
  blink_period  = blink;
  micro_constant = mic_const;
  DNMS_reset();  // ruft AudioInterrupts() intern auf
  start_phase = false;
}

// ─────────────────────────────────────────────────────────────────────
// setup()
// ─────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

#ifdef DNMS_DEBUG
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_FFT4096, OUTPUT);
  pinMode(PIN_FFT1024, OUTPUT);
  pinMode(PIN_I2C_1ST, OUTPUT);
  pinMode(PIN_I2C_2ND, OUTPUT);
  digitalWrite(PIN_FFT4096, LOW);
  digitalWrite(PIN_FFT1024, LOW);
  digitalWrite(PIN_I2C_1ST, LOW);
  digitalWrite(PIN_I2C_2ND, LOW);
  digitalWrite(LED_BUILTIN, HIGH);  // solid ON — waiting for Pi calibration
#else
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // solid ON — waiting for Pi calibration
#endif

  // ── Wait for calibration from Raspberry Pi ────────────────────
  // corr_Z / corr1024_Z stay null; process_fft4096/1024 return
  // early on the null-pointer guard until Pi sends APPLY_CALIB.
  // A/C weighting tables are static and never change.
  corr_A     = A_weighting4096;
  corr1024_A = A_weighting1024;
  corr_C     = C_weighting4096;
  corr1024_C = C_weighting1024;

  // ── I2C-Slave-Setup ───────────────────────────────────────────
  Wire.begin(DNMS_I2C_ADDRESS);
  Wire.onReceive(i2c_receive_from_master);
  Wire.onRequest(i2c_request_from_master);

  // ── Audio-Speicher ────────────────────────────────────────────
  AudioMemory(10);
  AudioMemory_F32(64);

  dnms_decimation.end();  // Dezimierung erst nach Anlaufphase aktiv
  AudioNoInterrupts();

  // ── FFT4096-Konfiguration ──────────────────────────────���──────
  dnms_fft.setOutputType(FFT_POWER);
  dnms_fft.windowFunction(AudioWindowHanning4096);
  dnms_fft.setNAverage(1u);

  // ── FFT1024-Konfiguration ─────────────────────────────────────
  dnms_fft1024.setOutputType(FFT_POWER);
  dnms_fft1024.windowFunction(AudioWindowHanning1024);
  dnms_fft1024.setNAverage(1u);

  // ── Fenster-Normalisierung ────────────────────────────────────
  // Formel: 1 / (window_coeff * 2 * N * N), N = reale Bins (halbe FFT-Größe)
  window_norm4096 = 1.0 / (WINDOW_HANNING * 2.0 * 2048.0 * 2048.0);
  window_norm1024 = 1.0 / (WINDOW_HANNING * 2.0 * 512.0 * 512.0);

  // ── Dezimierungsfilter-Init ───────────────────────────────────
  dnms_decimation.begin(
    dnms_decimate_coeffs,
    number_FIR_taps_decimation,
    DECI_FACTOR,
    BLK_LEN,
    &decimation_buffer[0],
    &decimation_state_buffer[0]);

  AudioInterrupts();

  last_blink_ms = millis();
}

// ──────────────────────────────────────────────���──────────────────────
// update_blink()
// Nicht-blockierendes LED-Blinken mit millis().
// FIX: Kein delay() – vollständig nicht-blockierend.
// ─────────────────────────────────────────────────────────────────────
static void update_blink(void) {
  if (!measurement_started) return;  // LED stays solid ON until first measurement trigger
  uint32_t now = millis();
  if ((now - last_blink_ms) < blink_period) return;
  last_blink_ms = now;
  if (++led_blink_cnt >= 2u) {
    led_blink_cnt = 0u;
    led_on = !led_on;
    digitalWrite(LED_BUILTIN, led_on ? HIGH : LOW);
  }
}

// ─────────────────────────────────────────────────────────────────────
// process_fft4096()
// Liest einen FFT4096-Frame, wendet Mikrofon-Korrektur und Gewichtung
// an, akkumuliert Breitband- und Bandleistungen.
// FIX: Null-Pointer-Guard für corr_Z/corr_A/corr_C hinzugefügt.
// ─────────────────────────────────────────────────────────────────────
static void process_fft4096(void) {
  if (!dnms_fft.available()) return;
  // Sicherheitscheck: Korrektur-Zeiger müssen gesetzt sein
  if (!corr_Z || !corr_A || !corr_C) return;

  DBG_HIGH(PIN_FFT4096);

  fft_all_Z = 0.0f;
  fft_all_A = 0.0f;
  fft_all_C = 0.0f;

  // Bin 0 (DC) lesen, aber nicht aufsummieren
  freq_Z[0] = dnms_fft.read(0);

  for (uint16_t i = 1u; i < number_FFT_bins; ++i) {
    float raw = dnms_fft.read(i);
    float fz = raw * corr_Z[i];
    float fa = fz  * corr_A[i];
    float fc = fz  * corr_C[i];

    freq_Z[i] = fz;
    freq_A[i] = fa;
    freq_C[i] = fc;

    fft_all_Z += fz;
    fft_all_A += fa;
    fft_all_C += fc;
  }

  mean_fft_all_Z += fft_all_Z;
  mean_fft_all_A += fft_all_A;
  mean_fft_all_C += fft_all_C;
  ++mean_count_fft;

  // 1/3-Oktav-Bandsummen über Mapping-Tabelle
  for (uint8_t m = 0u; m < BAND4096_MAP_LEN; ++m) {
    const BandMap& bm = band4096_map[m];
    sum_band_f(freq_band_Z, freq_Z, bm.idx, bm.first, bm.last);
    sum_band_f(freq_band_A, freq_A, bm.idx, bm.first, bm.last);
    sum_band_f(freq_band_C, freq_C, bm.idx, bm.first, bm.last);
  }

  for (uint8_t i = 14u; i < BAND4096_COUNT; ++i) {
    mean_freq_band_Z[i] += freq_band_Z[i];
    mean_freq_band_A[i] += freq_band_A[i];
    mean_freq_band_C[i] += freq_band_C[i];
  }

  DBG_LOW(PIN_FFT4096);
}

// ─────────────────────────────────────────────────────────────────────
// process_fft1024()
// Liest einen dezimier ten FFT1024-Frame, wendet Korrektur und
// Gewichtung an, akkumuliert Niederfrequenz-Bandsummen (Bänder 1..13).
// FIX: Null-Pointer-Guard für corr1024_Z/A/C hinzugefügt.
// ─────────────────────────────────────────────────────────────────────
static void process_fft1024(void) {
  if (!dnms_fft1024.available()) return;
  // Sicherheitscheck: Korrektur-Zeiger müssen gesetzt sein
  if (!corr1024_Z || !corr1024_A || !corr1024_C) return;

  DBG_HIGH(PIN_FFT1024);

  for (uint16_t i = 0u; i < number_ZoomFFT_bins; ++i) {
    float raw = dnms_fft1024.read(i);
    float fz = raw * corr1024_Z[i];
    float fa = fz  * corr1024_A[i];
    float fc = fz  * corr1024_C[i];

    freq1024_Z[i] = fz;
    freq1024_A[i] = fa;
    freq1024_C[i] = fc;
  }

  for (uint8_t m = 0u; m < BAND1024_MAP_LEN; ++m) {
    const BandMap& bm = band1024_map[m];
    sum_band_f(freq_band1024_Z, freq1024_Z, bm.idx, bm.first, bm.last);
    sum_band_f(freq_band1024_A, freq1024_A, bm.idx, bm.first, bm.last);
    sum_band_f(freq_band1024_C, freq1024_C, bm.idx, bm.first, bm.last);
  }

  for (uint8_t i = 1u; i < 14u; ++i) {
    mean_freq_band1024_Z[i] += freq_band1024_Z[i];
    mean_freq_band1024_A[i] += freq_band1024_A[i];
    mean_freq_band1024_C[i] += freq_band1024_C[i];
  }

  ++mean_count_fft1024;

  DBG_LOW(PIN_FFT1024);
}

// ─────────────────────────────────────────────────────────────────────
// update_broadband_leq()
// Aktualisiert laufenden Breitband-Leq (A, Z, C) für ein Intervall.
// FIX: Division durch loop_count nur einmal als Kehrwert berechnet.
// ───────────────────────────────────���─────────────────────────────────
static void update_broadband_leq(LeqInterval& iv,
                                 double leq_A, double leq_Z, double leq_C) {
  update_min(iv.leq_A_min, static_cast<float>(leq_A));
  update_max(iv.leq_A_max, static_cast<float>(leq_A));
  update_min(iv.leq_Z_min, static_cast<float>(leq_Z));
  update_max(iv.leq_Z_max, static_cast<float>(leq_Z));
  update_min(iv.leq_C_min, static_cast<float>(leq_C));
  update_max(iv.leq_C_max, static_cast<float>(leq_C));

  iv.sum_g_A += delog(leq_A);
  iv.sum_g_Z += delog(leq_Z);
  iv.sum_g_C += delog(leq_C);
  ++iv.loop_count;

  // Einen Kehrwert, drei Multiplikationen (statt drei Divisionen)
  const double inv = 1.0 / static_cast<double>(iv.loop_count);
  iv.leq_g_A = ten_log10(iv.sum_g_A * inv);
  iv.leq_g_Z = ten_log10(iv.sum_g_Z * inv);
  iv.leq_g_C = ten_log10(iv.sum_g_C * inv);
}

// ─────────────────────────────────────────────────────────────────────
// update_band4096_leq()
// Aktualisiert 1/3-Oktav-Leq für FFT4096-Bänder (14..31).
// Akkumuliert normierte lineare Leistung direkt (kein log/exp-Umweg).
// FIX: Division durch 0 abgesichert; Kehrwerte vorab berechnet.
// ─────────────────────────────────────────────────────────────────────
static void update_band4096_leq(LeqInterval& iv) {
  ++iv.loop_count_fft;

  if (mean_count_fft == 0u) return;  // Division durch 0 verhindern

  const double inv_count = 1.0 / static_cast<double>(mean_count_fft);
  const double inv_loop = 1.0 / static_cast<double>(iv.loop_count_fft);
  const double norm = window_norm4096 * inv_count;

  for (uint8_t i = 14u; i < BAND4096_COUNT; ++i) {
    double lin_Z = static_cast<double>(mean_freq_band_Z[i]) * norm;
    if (lin_Z <= 0.0) lin_Z = 1.0e-7;
    iv.sum_band_Z[i] += lin_Z;
    iv.leq_band_g_Z[i] = micro_constant
                         + ten_log10(iv.sum_band_Z[i] * inv_loop);

    double lin_A = static_cast<double>(mean_freq_band_A[i]) * norm;
    if (lin_A <= 0.0) lin_A = 1.0e-7;
    iv.sum_band_A[i] += lin_A;
    iv.leq_band_g_A[i] = micro_constant
                         + ten_log10(iv.sum_band_A[i] * inv_loop);

    double lin_C = static_cast<double>(mean_freq_band_C[i]) * norm;
    if (lin_C <= 0.0) lin_C = 1.0e-7;
    iv.sum_band_C[i] += lin_C;
    iv.leq_band_g_C[i] = micro_constant
                         + ten_log10(iv.sum_band_C[i] * inv_loop);
  }
}

// ─────────────────────────────────────────────────────────────────────
// update_band1024_leq()
// Aktualisiert 1/3-Oktav-Leq für FFT1024-Bänder (1..13).
// FIX: Division durch 0 abgesichert; Kehrwerte vorab berechnet.
// ─────────────────────────────────────────────────────────────────────
static void update_band1024_leq(LeqInterval& iv) {
  ++iv.loop_count_fft1024;

  if (mean_count_fft1024 == 0u) return;  // Division durch 0 verhindern

  const double inv_count = 1.0 / static_cast<double>(mean_count_fft1024);
  const double inv_loop = 1.0 / static_cast<double>(iv.loop_count_fft1024);
  const double norm = window_norm1024 * inv_count;

  for (uint8_t i = 1u; i < 14u; ++i) {
    double lin_Z = static_cast<double>(mean_freq_band1024_Z[i]) * norm;
    if (lin_Z <= 0.0) lin_Z = 1.0e-7;
    iv.sum_band1024_Z[i] += lin_Z;
    iv.leq_band1024_g_Z[i] = micro_constant
                             + ten_log10(iv.sum_band1024_Z[i] * inv_loop);

    double lin_A = static_cast<double>(mean_freq_band1024_A[i]) * norm;
    if (lin_A <= 0.0) lin_A = 1.0e-7;
    iv.sum_band1024_A[i] += lin_A;
    iv.leq_band1024_g_A[i] = micro_constant
                             + ten_log10(iv.sum_band1024_A[i] * inv_loop);

    double lin_C = static_cast<double>(mean_freq_band1024_C[i]) * norm;
    if (lin_C <= 0.0) lin_C = 1.0e-7;
    iv.sum_band1024_C[i] += lin_C;
    iv.leq_band1024_g_C[i] = micro_constant
                             + ten_log10(iv.sum_band1024_C[i] * inv_loop);
  }
}

// ─────────────────────────────────────────────────────────────────────
// compute_leq_interval()
// Berechnet Breitband-Leq wenn genug FFT4096-Frames vorliegen.
// FIX: mean_count_fft wird erst nach update_band4096_leq() auf 0
// gesetzt, damit update_band4096_leq() den korrekten Zähler nutzt.
// ─────────────────────────────────────────────────────────────────────
static void compute_leq_interval(void) {
  if (mean_count_fft < NUMBER_OF_FFTS) return;

  double leq_fft_A = compute_leq(mean_fft_all_A, mean_count_fft, window_norm4096);
  double leq_fft_Z = compute_leq(mean_fft_all_Z, mean_count_fft, window_norm4096);
  double leq_fft_C = compute_leq(mean_fft_all_C, mean_count_fft, window_norm4096);

  update_broadband_leq(g_iv1, leq_fft_A, leq_fft_Z, leq_fft_C);
  update_broadband_leq(g_iv2, leq_fft_A, leq_fft_Z, leq_fft_C);

  // Band-Leq VOR dem Reset der Mittelwert-Arrays aktualisieren
  update_band4096_leq(g_iv1);
  update_band4096_leq(g_iv2);

  // Jetzt erst Akkumulatoren zurücksetzen
  mean_fft_all_A = 0.0f;
  mean_fft_all_Z = 0.0f;
  mean_fft_all_C = 0.0f;

  for (uint8_t i = 14u; i < BAND4096_COUNT; ++i) {
    mean_freq_band_Z[i] = 0.0f;
    mean_freq_band_A[i] = 0.0f;
    mean_freq_band_C[i] = 0.0f;
  }

  mean_count_fft = 0u;
}

// ──────────────────────────────────���──────────────────────────────────
// compute_leq_interval_1024()
// Aktualisiert Niederfrequenz-Bänder (1..13) wenn genug FFT1024-Frames
// vorliegen, dann Reset der gemeinsamen Mittelwert-Arrays.
// FIX: mean_count_fft1024 wird erst nach update_band1024_leq() auf 0
// gesetzt, damit der Zähler korrekt genutzt wird.
// ─────────────────────────────────────────────────────────────────────
static void compute_leq_interval_1024(void) {
  if (mean_count_fft1024 < NUMBER_OF_FFTS) return;

  // Band-Leq VOR dem Reset der Mittelwert-Arrays aktualisieren
  update_band1024_leq(g_iv1);
  update_band1024_leq(g_iv2);

  // Jetzt erst Akkumulatoren zurücksetzen
  for (uint8_t i = 1u; i < 14u; ++i) {
    mean_freq_band1024_Z[i] = 0.0f;
    mean_freq_band1024_A[i] = 0.0f;
    mean_freq_band1024_C[i] = 0.0f;
  }

  mean_count_fft1024 = 0u;
}

// ─────────────────────────────────────────────────────────────────────
// finalize_interval()
// Kopiert laufende Leq-Werte in Snapshot-Felder, setzt Akkumulatoren
// zurück, signalisiert Bereitschaft an I2C-Master.
// FIX: calc_flag wird atomar gelesen – kein doppeltes Auslösen möglich.
// ─────────────────────────────────────────────────────────────────────
static void finalize_interval(LeqInterval& iv,
                              volatile uint8_t& calc_flag,
                              volatile uint16_t& ready_flag) {
  if (!calc_flag) return;

  // Guard: if the ISR re-fired calc_flag between the previous calc_flag=0
  // and reset_interval(), loop_count is 0 and leq_g_A is 0.  Keep the flag
  // set and retry on the next loop() call — compute_leq_interval() will
  // populate the interval within one FFT frame (~93 ms).
  if (iv.loop_count == 0u) return;

  // ── Breitband-Snapshot ────────────────────────────────────────
  iv.last_g_A = static_cast<float>(iv.leq_g_A);
  iv.last_A_min = iv.leq_A_min;
  iv.last_A_max = iv.leq_A_max;

  iv.last_g_Z = static_cast<float>(iv.leq_g_Z);
  iv.last_Z_min = iv.leq_Z_min;
  iv.last_Z_max = iv.leq_Z_max;

  iv.last_g_C = static_cast<float>(iv.leq_g_C);
  iv.last_C_min = iv.leq_C_min;
  iv.last_C_max = iv.leq_C_max;

  // ── FFT4096-Band-Snapshot (Bänder 14..31) ─────────────────────
  for (uint8_t i = 14u; i < BAND4096_COUNT; ++i) {
    iv.last_band_g_A[i] = iv.leq_band_g_A[i];
    iv.last_band_g_Z[i] = iv.leq_band_g_Z[i];
    iv.last_band_g_C[i] = iv.leq_band_g_C[i];
  }

  // ── FFT1024-Band-Snapshot in gemeinsamen last_band-Arrays
  // (Bänder 1..13, gleicher Index-Raum wie FFT4096-Bänder) ────
  for (uint8_t i = 1u; i < 14u; ++i) {
    iv.last_band_g_A[i] = iv.leq_band1024_g_A[i];
    iv.last_band_g_Z[i] = iv.leq_band1024_g_Z[i];
    iv.last_band_g_C[i] = iv.leq_band1024_g_C[i];
  }

  // Clear calc_flag before reset_interval so that if the ISR fires a
  // new cmd-3/cmd-10 in the window between flag clear and accumulator
  // reset, that new trigger starts a fresh accumulation cycle instead
  // of immediately re-snapshotting zeros.
  calc_flag = 0u;

  // ── Intervall für nächste Messperiode zurücksetzen ────────────
  reset_interval(iv);

  ready_flag = 1u;
}


// ─────────────────────────────────────────────────────────────────────
// write_leq_triplet()
// Kodiert (Leq, min, max) mit CRC in i2c_buf und sendet über Wire.
// ─────────────────────────────────────────���───────────────────────────
static void write_leq_triplet(float leq, float leq_min, float leq_max) {
  uint16_t idx = 0u;
  write_float_with_crc(leq, idx);
  write_float_with_crc(leq_min, idx);
  write_float_with_crc(leq_max, idx);
  Wire.write(i2c_buf, idx);
  i2c_state = 0u;
}

// ─────────────────────────────────────────────────────────────────────
// i2c_receive_from_master()
// I2C-onReceive-Callback – läuft im Interrupt-Kontext.
// FIX: Nur Flags und i2c_state setzen; keine schwere Berechnung hier.
// set_microphone() aus ISR-Kontext aufzurufen ist problematisch
// (AudioNoInterrupts innerhalb ISR); daher wird ein pending-Flag
// gesetzt und der Aufruf in loop() erledigt.
// ─────────────────────────────────────────────────────────────────────

// Pending-Mikrofon-Wechsel: 0 = kein Wechsel, sonst Befehlsnummer
static volatile uint8_t pending_mic_change = 0u;

void i2c_receive_from_master(int num_bytes) {
  (void)num_bytes;

  i2c_state = 0u;
  uint8_t idx = 0u;
  while (Wire.available() && idx < I2C_BUF_SIZE) {
    i2c_buf[idx++] = static_cast<uint8_t>(Wire.read());
  }

  command_received = static_cast<uint16_t>(
    (static_cast<uint16_t>(i2c_buf[0]) << 8u) | i2c_buf[1]);

  switch (command_received) {

    case 1u:  // Reset DNMS — defer to loop(); AudioNoInterrupts() must not be called from ISR
      pending_reset = 1u;
      break;

    case 2u:  // Firmware-Version lesen
      i2c_state = 2u;
      break;

    case 3u:  // Leq-Berechnung auslösen (1. Intervall)
      i2c_state = 3u;
      data_ready = 0u;
      calculate_leq = 1u;
      if (!measurement_started) {
        measurement_started = true;
        last_blink_ms = millis();
      }
      break;

    case 4u:  // Daten bereit? (1. Intervall)
      i2c_state = 4u;
      break;

    case 5u:  // LAeq / LAmin / LAmax (1. Intervall)
      i2c_state = 5u;
      break;

    case 6u:  // A-gewichtet 1/3-Oktav Teil 1 – Bänder 1..8
      i2c_state = 6u;
      break;
    case 7u:  // A-gewichtet 1/3-Oktav Teil 2 – Bänder 9..16
      i2c_state = 7u;
      break;
    case 8u:  // A-gewichtet 1/3-Oktav Teil 3 – Bänder 17..24
      i2c_state = 8u;
      break;
    case 9u:  // A-gewichtet 1/3-Oktav Teil 4 – Bänder 25..31
      i2c_state = 9u;
      break;

    case 10u:  // Leq-Berechnung auslösen (2. Intervall)
      i2c_state = 10u;
      data_ready_2nd = 0u;
      calculate_leq_2nd = 1u;
      break;

    case 11u:  // Daten bereit? (2. Intervall)
      i2c_state = 11u;
      break;

    case 12u:  // LAeq / LAmin / LAmax (2. Intervall)
      i2c_state = 12u;
      break;

    case 13u: i2c_state = 13u; break;
    case 14u: i2c_state = 14u; break;
    case 15u: i2c_state = 15u; break;
    case 16u: i2c_state = 16u; break;

    case 17u:  // LZeq / LZmin / LZmax (1. Intervall)
      i2c_state = 17u;
      break;

    case 18u: i2c_state = 18u; break;
    case 19u: i2c_state = 19u; break;
    case 20u: i2c_state = 20u; break;
    case 21u: i2c_state = 21u; break;

    case 22u:  // LZeq / LZmin / LZmax (2. Intervall)
      i2c_state = 22u;
      break;

    case 23u: i2c_state = 23u; break;
    case 24u: i2c_state = 24u; break;
    case 25u: i2c_state = 25u; break;
    case 26u: i2c_state = 26u; break;

    // FIX: Mikrofon-Wechsel nur als pending-Flag setzen –
    // set_microphone() enthält AudioNoInterrupts(), das
    // nicht sicher aus dem ISR-Kontext aufrufbar ist.
    // loop() wertet pending_mic_change aus und führt den
    // Wechsel im normalen Task-Kontext durch.
    case 27u:  // ICS-43434 auswählen
      pending_mic_change = 27u;
      break;

    case 28u:  // IM72D128 auswählen
      pending_mic_change = 28u;
      break;

    case 29u:  // IM72D128 mit DLR-Gehäuse-Korrektur
      pending_mic_change = 29u;
      break;

    case 30u:  // ICS-43434 ohne Frequenzkorrektur
      pending_mic_change = 30u;
      break;

    case 31u:  // IM72D128 ohne Frequenzkorrektur
      pending_mic_change = 31u;
      break;

    case 32u:  // LCeq / LCmin / LCmax (1. Intervall)
      i2c_state = 32u;
      break;

    case 33u: i2c_state = 33u; break;
    case 34u: i2c_state = 34u; break;
    case 35u: i2c_state = 35u; break;
    case 36u: i2c_state = 36u; break;

    case 37u:  // LCeq / LCmin / LCmax (2. Intervall)
      i2c_state = 37u;
      break;

    case 38u: i2c_state = 38u; break;
    case 39u: i2c_state = 39u; break;
    case 40u: i2c_state = 40u; break;
    case 41u: i2c_state = 41u; break;

    // ── Calibration upload from Pi ──────────────────────────────────
    // Buffer layout after 2-byte command:
    //   Each arg = [hi_byte, lo_byte, CRC] — CRC bytes are at +2, +5, +8 … and skipped here.
    //   Float = hi_word (bytes +0,+1) followed by lo_word (bytes +3,+4).

    case 42u:  // WRITE_CALIB_POINT: store one point into calib_rx[]
    {
      uint8_t pt_idx = i2c_buf[3];                   // low byte of arg0 (index)
      if (pt_idx < CALIB_MAX_POINTS) {
        calib_rx[pt_idx].freq_Hz =
            i2c_parse_float(i2c_buf[5], i2c_buf[6],  // freq hi-word
                            i2c_buf[8], i2c_buf[9]);  // freq lo-word
        calib_rx[pt_idx].dB_correction =
            i2c_parse_float(i2c_buf[11], i2c_buf[12], // dB hi-word
                            i2c_buf[14], i2c_buf[15]); // dB lo-word
      }
      break;
    }

    case 43u:  // APPLY_CALIB: latch metadata, trigger set_microphone() in loop()
      pending_calib_count = i2c_buf[3];               // low byte of arg0
      pending_calib_type  = i2c_buf[6];               // low byte of arg1
      pending_calib_blink = (uint32_t(i2c_buf[8])  << 24u)   // arg2 hi-word
                          | (uint32_t(i2c_buf[9])  << 16u)
                          | (uint32_t(i2c_buf[11]) <<  8u)   // arg3 lo-word
                          |  uint32_t(i2c_buf[12]);
      pending_calib_const =
          i2c_parse_float(i2c_buf[14], i2c_buf[15],  // mic_const hi-word
                          i2c_buf[17], i2c_buf[18]); // mic_const lo-word
      pending_calib_apply = 1u;
      break;

    default:
      // Unbekannter Befehl: i2c_state bleibt 0, keine Antwort
      break;
  }
}

// ─────────────────────────────────────────────────────────────────────
// handle_pending_mic_change()
// Wertet pending_mic_change aus und führt den Mikrofon-Wechsel
// sicher im loop()-Kontext durch (nicht im ISR-Kontext).
// FIX: AudioNoInterrupts() darf nicht innerhalb einer ISR aufgerufen
// werden – deshalb wird der Wechsel hierher ausgelagert.
// ─────────────────────────────────────────────────────────────────────
static void handle_pending_mic_change(void) {
  // Deferred RESET (cmd 1) — safe to call AudioNoInterrupts() here, outside ISR
  if (pending_reset) {
    pending_reset = 0u;
    AudioNoInterrupts();
    DNMS_reset();
    start_phase = false;
    start_phase_cnt = 0u;
  }

  // New-protocol calibration upload (Pi sent cmds 42 + 43)
  if (pending_calib_apply) {
    pending_calib_apply = 0u;
    set_microphone(calib_rx, pending_calib_count,
                   pending_calib_type, pending_calib_blink,
                   (double)pending_calib_const);
    if (!measurement_started) {
      measurement_started = true;
      last_blink_ms = millis();
    }
  }

  if (pending_mic_change == 0u) return;

  uint8_t cmd = pending_mic_change;
  pending_mic_change = 0u;  // Flag sofort löschen

  switch (cmd) {
    case 27u:
      set_microphone(calib_ICS43434,     CALIB_ICS43434_COUNT,
                     1u, BLINK_PERIOD_ICS43434, MICRO_CONST_ICS43434);
      break;
    case 28u:
      set_microphone(calib_IM72D128,     CALIB_IM72D128_COUNT,
                     2u, BLINK_PERIOD_IM72D128, MICRO_CONST_IM72D128);
      break;
    case 29u:
      set_microphone(calib_IM72D128_DLR, CALIB_IM72D128_DLR_COUNT,
                     2u, BLINK_PERIOD_IM72D128, MICRO_CONST_IM72D128);
      break;
    case 30u:
      set_microphone(calib_no_corr, CALIB_NO_CORR_COUNT,
                     1u, BLINK_PERIOD_ICS43434, MICRO_CONST_ICS43434);
      break;
    case 31u:
      set_microphone(calib_no_corr, CALIB_NO_CORR_COUNT,
                     2u, BLINK_PERIOD_IM72D128, MICRO_CONST_IM72D128);
      break;
    default:
      break;
  }
  // Old-protocol (cmds 27-31): also start blinking once microphone is configured
  if (!measurement_started) {
    measurement_started = true;
    last_blink_ms = millis();
  }
}

// ─────────────────────────────────────────────────────────────────────
// i2c_request_from_master()
// I2C-onRequest-Callback – läuft im Interrupt-Kontext.
// Bereitet Antwort-Buffer auf Basis von i2c_state vor und sendet.
// FIX: Jeder case setzt i2c_state = 0u am Ende (via write_*-Helfer
// oder explizit), um Doppel-Antworten zu verhindern.
// ─────────────────────────────────────────────────────────────────────
void i2c_request_from_master(void) {
  switch (i2c_state) {

    // ── Firmware-Version senden ───────────────────────────────
    case 2u:
      {
        uint16_t idx = 0u;
        for (size_t i = 0u; i + 1u < sizeof(SOFTWARE_VERSION); i += 2u) {
          i2c_buf[idx] = static_cast<uint8_t>(SOFTWARE_VERSION[i]);
          i2c_buf[idx + 1u] = static_cast<uint8_t>(SOFTWARE_VERSION[i + 1u]);
          i2c_buf[idx + 2u] = dnms_common_generate_crc(
            &i2c_buf[idx], DNMS_WORD_SIZE);
          idx += 3u;
        }
        Wire.write(i2c_buf, idx);
        i2c_state = 0u;
        break;
      }

    // ── Daten bereit? (1. Intervall) ──────────────────────────
    case 4u:
      {
        uint16_t idx = 0u;
        i2c_buf[idx] = static_cast<uint8_t>((data_ready >> 8u) & 0xFFu);
        i2c_buf[idx + 1u] = static_cast<uint8_t>(data_ready & 0xFFu);
        i2c_buf[idx + 2u] = dnms_common_generate_crc(&i2c_buf[idx], DNMS_WORD_SIZE);
        Wire.write(i2c_buf, 3u);
        i2c_state = 0u;
        break;
      }

    // ── LAeq / LAmin / LAmax (1. Intervall) ───────────────────
    case 5u:
      DBG_HIGH(PIN_I2C_1ST);
      write_leq_triplet(g_iv1.last_g_A, g_iv1.last_A_min, g_iv1.last_A_max);
      g_iv1.last_g_A = 0.0f;
      g_iv1.last_A_min = 0.0f;
      g_iv1.last_A_max = 0.0f;
      DBG_LOW(PIN_I2C_1ST);
      break;

    // ── A-gewichtet 1/3-Oktav (1. Intervall) ──────────────────
    case 6u:
      DBG_HIGH(PIN_I2C_1ST);
      fft_values_i2c_write(1u, 8u);
      break;
    case 7u:
      fft_values_i2c_write(9u, 16u);
      break;
    case 8u:
      fft_values_i2c_write(17u, 24u);
      break;
    case 9u:
      fft_values_i2c_write(25u, 31u);
      DBG_LOW(PIN_I2C_1ST);
      break;

    // ── Daten bereit? (2. Intervall) ──────────────────────────
    case 11u:
      {
        uint16_t idx = 0u;
        i2c_buf[idx] = static_cast<uint8_t>((data_ready_2nd >> 8u) & 0xFFu);
        i2c_buf[idx + 1u] = static_cast<uint8_t>(data_ready_2nd & 0xFFu);
        i2c_buf[idx + 2u] = dnms_common_generate_crc(&i2c_buf[idx], DNMS_WORD_SIZE);
        Wire.write(i2c_buf, 3u);
        i2c_state = 0u;
        break;
      }

    // ── LAeq / LAmin / LAmax (2. Intervall) ───────────────────
    case 12u:
      DBG_HIGH(PIN_I2C_2ND);
      write_leq_triplet(g_iv2.last_g_A, g_iv2.last_A_min, g_iv2.last_A_max);
      g_iv2.last_g_A = 0.0f;
      g_iv2.last_A_min = 0.0f;
      g_iv2.last_A_max = 0.0f;
      DBG_LOW(PIN_I2C_2ND);
      break;

    // ── A-gewichtet 1/3-Oktav (2. Intervall) ──────────────────
    case 13u:
      DBG_HIGH(PIN_I2C_2ND);
      fft_values_2nd_i2c_write(1u, 8u);
      break;
    case 14u:
      fft_values_2nd_i2c_write(9u, 16u);
      break;
    case 15u:
      fft_values_2nd_i2c_write(17u, 24u);
      break;
    case 16u:
      fft_values_2nd_i2c_write(25u, 31u);
      DBG_LOW(PIN_I2C_2ND);
      break;

    // ── LZeq / LZmin / LZmax (1. Intervall) ───────────────────
    case 17u:
      DBG_HIGH(PIN_I2C_1ST);
      write_leq_triplet(g_iv1.last_g_Z, g_iv1.last_Z_min, g_iv1.last_Z_max);
      g_iv1.last_g_Z = 0.0f;
      g_iv1.last_Z_min = 0.0f;
      g_iv1.last_Z_max = 0.0f;
      DBG_LOW(PIN_I2C_1ST);
      break;

    // ── Z-gewichtet 1/3-Oktav (1. Intervall) ──────────────────
    case 18u:
      DBG_HIGH(PIN_I2C_1ST);
      fft_Z_values_i2c_write(1u, 8u);
      break;
    case 19u:
      fft_Z_values_i2c_write(9u, 16u);
      break;
    case 20u:
      fft_Z_values_i2c_write(17u, 24u);
      break;
    case 21u:
      fft_Z_values_i2c_write(25u, 31u);
      DBG_LOW(PIN_I2C_1ST);
      break;

    // ── LZeq / LZmin / LZmax (2. Intervall) ───────────────────
    case 22u:
      DBG_HIGH(PIN_I2C_2ND);
      write_leq_triplet(g_iv2.last_g_Z, g_iv2.last_Z_min, g_iv2.last_Z_max);
      g_iv2.last_g_Z = 0.0f;
      g_iv2.last_Z_min = 0.0f;
      g_iv2.last_Z_max = 0.0f;
      DBG_LOW(PIN_I2C_2ND);
      break;

    // ── Z-gewichtet 1/3-Oktav (2. Intervall) ──────────────────
    case 23u:
      DBG_HIGH(PIN_I2C_2ND);
      fft_Z_values_2nd_i2c_write(1u, 8u);
      break;
    case 24u:
      fft_Z_values_2nd_i2c_write(9u, 16u);
      break;
    case 25u:
      fft_Z_values_2nd_i2c_write(17u, 24u);
      break;
    case 26u:
      fft_Z_values_2nd_i2c_write(25u, 31u);
      DBG_LOW(PIN_I2C_2ND);
      break;

    // ── LCeq / LCmin / LCmax (1. Intervall) ───────────────────
    case 32u:
      DBG_HIGH(PIN_I2C_1ST);
      write_leq_triplet(g_iv1.last_g_C, g_iv1.last_C_min, g_iv1.last_C_max);
      g_iv1.last_g_C = 0.0f;
      g_iv1.last_C_min = 0.0f;
      g_iv1.last_C_max = 0.0f;
      DBG_LOW(PIN_I2C_1ST);
      break;

    // ── C-gewichtet 1/3-Oktav (1. Intervall) ──────────────────
    case 33u:
      DBG_HIGH(PIN_I2C_1ST);
      fft_C_values_i2c_write(1u, 8u);
      break;
    case 34u:
      fft_C_values_i2c_write(9u, 16u);
      break;
    case 35u:
      fft_C_values_i2c_write(17u, 24u);
      break;
    case 36u:
      fft_C_values_i2c_write(25u, 31u);
      DBG_LOW(PIN_I2C_1ST);
      break;

    // ── LCeq / LCmin / LCmax (2. Intervall) ───────────────────
    case 37u:
      DBG_HIGH(PIN_I2C_2ND);
      write_leq_triplet(g_iv2.last_g_C, g_iv2.last_C_min, g_iv2.last_C_max);
      g_iv2.last_g_C = 0.0f;
      g_iv2.last_C_min = 0.0f;
      g_iv2.last_C_max = 0.0f;
      DBG_LOW(PIN_I2C_2ND);
      break;

    // ── C-gewichtet 1/3-Oktav (2. Intervall) ──────────────────
    case 38u:
      DBG_HIGH(PIN_I2C_2ND);
      fft_C_values_2nd_i2c_write(1u, 8u);
      break;
    case 39u:
      fft_C_values_2nd_i2c_write(9u, 16u);
      break;
    case 40u:
      fft_C_values_2nd_i2c_write(17u, 24u);
      break;
    case 41u:
      fft_C_values_2nd_i2c_write(25u, 31u);
      DBG_LOW(PIN_I2C_2ND);
      break;

    default:
      // Unbekannter Zustand: nichts senden, State löschen
      i2c_state = 0u;
      break;
  }
}

// ─────────────────────────────────────────────────────────────────────
// loop()
// FIX: handle_pending_mic_change() wird hier aufgerufen, damit
//      Mikrofon-Wechsel sicher außerhalb des ISR-Kontexts stattfinden.
// ─────────────────────────────────────────────────────────────────────
void loop() {
  // Ausstehenden Mikrofon-Wechsel zuerst behandeln
  handle_pending_mic_change();

  if (start_phase) {
    if (dnms_fft.available()) {
      // Anlauf-Frames verwerfen (DC-Offset, Einschwingen)
      (void)dnms_fft.read(0u, 2047u);
      if (++start_phase_cnt >= FFT_CALCULATIONS_IN_START_PHASE) {
        start_phase = false;
        if (dnms_fft1024.available())
          (void)dnms_fft1024.read(0u, 511u);
      }
    }
    update_blink();
    return;
  }

  // ── standard loop after start_phase ─────────────────────────────────────────────
  process_fft4096();
  process_fft1024();
  update_blink();
  compute_leq_interval();
  compute_leq_interval_1024();
  finalize_interval(g_iv1, calculate_leq, data_ready);
  finalize_interval(g_iv2, calculate_leq_2nd, data_ready_2nd);
}
