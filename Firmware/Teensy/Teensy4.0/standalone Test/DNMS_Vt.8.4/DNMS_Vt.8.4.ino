/************************************************************************
 *                                                                      *
    DNMS - Digital Noise Measurement Sensor
 *                                                                      *
    This source code is for the board Teensy 4.0
    and ICS-43434 microphone with I²S output connected to Teensy4
    
    
    standalone-testprogramm 
 *                                                                      *
 ************************************************************************
 *                                                                      *
      DNMS - Digital Noise Measurement Sensor
      Copyright (C) 2021, 2023  Helmut Bitter
      Copyright (C) 2021, 2022  Burkhard Volkemer for FFT software parts
 *                                                                      *
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
 *                                                                      *
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 *                                                                      *
   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
 *                                                                      *
 ************************************************************************
*/
#include <Arduino.h>
#include <math.h>
#include <Audio.h>
#include <dnms_audio_lib.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>

#include "./FIR_decimation_filter.h"
#include "./Micro_corr_values.h"


#define SOFTWARE_VERSION "DNMS Version t.8.4"  // standalone testprogram for Teensy4 and ICS-43434 micro based on FFT
/**********************************************************************************
 *                                                                                *
 *      Standalone testprogram                                                    *
 *      version for Teensy4 and ICS-43434 microphone                               *
 *      all calculations are based von FFT                                        *
 *                                                                                *
 *      version that allows two different measurement                             *
 *      interval times.                                                           *#

 *      1. for LAeq, LAmin and LAmax as the versions before                       *
 *      2. for a second LAeq, LAmin and LAmax and the 1/3 octaves                 *
 *      values                                                                    *
 *                                                                                *
 *                                                                                *
 *                                                                                *
 *                                                                                *
 **********************************************************************************
*/



/*

   Attention: in Wire.h set BUFFER_LENGTH to 64 !!

   #define BUFFER_LENGTH 64

*/
#define DEFAULT_TIME_FOR_TEST_OUTPUT 1000  // default time in ms for measurement interval and output of data

#define NUMBER_OF_FFTs 1  // Number of FFTs to calculate Leq from
#define BLINK_PERIOD 200  // Time  ins ms for blinking of internal red LED

#define FFT_CALCULATIONS_IN_START_PHASE 25  // define number of first FFT calculations to throw away
#define BLK_LEN 128                         // block length of audio blocks in Audio Lib
#define DECI_FACTOR 32                      // decimation factor for 2nd FFT

#define DNMS_I2C_ADDRESS 0x55
#define DNMS_MAX_VERSION_LEN 32
#define DNMS_WORD_SIZE 2
#define DNMS_NUM_WORDS(x) (sizeof(x) / DNMS_WORD_SIZE)
#define STATUS_OK 0
#define STATUS_FAIL (-1)
#define DNMS_COMMAND_SIZE 2
#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xFF
#define CRC8_LEN 1
#define DNMS_MAX_BUFFER_WORDS 32
#define DNMS_IS_ERR_STATE(err_code) (((err_code) | 0xff) == 0x1ff)

#define DNMS_CMD_RESET 0x0001
#define DNMS_CMD_READ_VERSION 0x0002
#define DNMS_CMD_CALCULATE_LEQ 0x0003
#define DNMS_CMD_READ_DATA_READY 0x0004
#define DNMS_CMD_READ_LEQ 0x0005


#define be16_to_cpu(s) (((uint16_t)(s) << 8) | (0xff & ((uint16_t)(s)) >> 8))
#define be32_to_cpu(s) (((uint32_t)be16_to_cpu(s) << 16) | (0xffff & (be16_to_cpu((s) >> 16))))
#define be64_to_cpu(s) (((uint64_t)be32_to_cpu(s) << 32) | (0xffffffff & ((uint64_t)be32_to_cpu((s) >> 32))))
/**
   Convert a word-array to a bytes-array, effectively reverting the
   host-endianness to big-endian
   @a:  word array to change (must be (u16 *) castable)
   @w:  number of word-sized elements in the array (DNMS_NUM_WORDS(a)).
*/
#define DNMS_WORDS_TO_BYTES(a, w) \
  for (uint16_t *__a = (uint16_t *)(a), __e = (w), __w = 0; __w < __e; ++__w) { \
    __a[__w] = be16_to_cpu(__a[__w]); \
  }

// Function prototypes
void i2c_receive_from_master(int num_bytes);
void i2c_request_from_master(void);


// GUItool: begin automatically generated code
AudioInputI2S_F32 i2s_in;
//AudioOutputUSB            usb_out;
AudioConvert_F32toI16 convert_fp_to_int;
AudioAnalyzeFFT1024_F32 dnms_fft1024;
AudioAnalyzeFFT4096_F32 dnms_fft;
AudioDecimationFilter_F32 dnms_decimation;
AudioConnection_F32 patchCord4(i2s_in, 0, dnms_fft, 0);
AudioConnection_F32 patchCord5(i2s_in, 0, dnms_decimation, 0);
AudioConnection_F32 patchCord6(dnms_decimation, 0, dnms_fft1024, 0);
//AudioConnection           patchCord8(convert_fp_to_int, 0, usb_out, 0);
//AudioConnection           patchCord9(convert_fp_to_int, 0, usb_out, 1);
// GUItool: end automatically generated code

// Create an IntervalTimer object 
  IntervalTimer OutputTimer;

unsigned long startMillis;
unsigned long currentMillis;

uint16_t interval_time;
char s1[8];

int led = LED_BUILTIN;
bool led_on;
uint16_t led_count = 0;

uint8_t i2c_state = 0;
uint8_t buf[64];
uint16_t data_ready = 0;
volatile bool calculate_leq = false;
uint16_t command_received;

uint16_t data_ready_2nd = 0;
bool calculate_leq_2nd = false;

boolean start_phase = true;
uint16_t number_of_start_phase_fft_calculations = 0;

double z;
uint16_t i;

uint32_t mean_count_A = 0;
uint32_t measure_loop_count = 0;
double leq_fft_A;
float leq_A_max = 0;
float leq_A_min = 0;
double leq_g_A = 0;
double sum_leq_g_A = 0;
double delog_leq_A;
float last_leq_g_A;
float last_leq_A_min;
float last_leq_A_max;

double leq_fft_Z;
float leq_Z_max = 0;
float leq_Z_min = 0;
double leq_g_Z = 0;
double sum_leq_g_Z = 0;
double delog_leq_Z;
float last_leq_g_Z;
float last_leq_Z_min;
float last_leq_Z_max;

uint32_t mean_count_A_2nd = 0;
uint32_t measure_loop_count_2nd = 0;
double leq_fft_A_2nd;
float leq_A_max_2nd = 0;
float leq_A_min_2nd = 0;
double leq_g_A_2nd = 0;
double sum_leq_g_A_2nd = 0;
float last_leq_g_A_2nd;
float last_leq_A_min_2nd;
float last_leq_A_max_2nd;

double leq_fft_Z_2nd;
float leq_Z_max_2nd = 0;
float leq_Z_min_2nd = 0;
double leq_g_Z_2nd = 0;
double sum_leq_g_Z_2nd = 0;
float last_leq_g_Z_2nd;
float last_leq_Z_min_2nd;
float last_leq_Z_max_2nd;


double freq_Z[2048];
double freq_A[2048];
double freq1024_Z[512];
double freq1024_A[512];

float fft_all_A;
float mean_fft_all_A = 0;
float fft_all_Z;
float mean_fft_all_Z = 0;

uint32_t mean_count_fft = 0;
uint32_t measure_loop_count_fft = 0;
double freq_band_Z[32];
double mean_freq_band_Z[32] = { 0 };
double leq_freq_band_Z[32];
double sum_leq_freq_band_Z[32] = { 0 };
double leq_freq_band_g_Z[32] = { 0 };
double last_leq_freq_band_g_Z[32] = { 0 };
double freq_band_A[32];
double mean_freq_band_A[32] = { 0 };
double leq_freq_band_A[32];
double sum_leq_freq_band_A[32] = { 0 };
double leq_freq_band_g_A[32] = { 0 };
double last_leq_freq_band_g_A[32] = { 0 };

static double window_norm4096;
uint16_t fft_average;

double f1;
uint32_t measure_loop_count_fft_2nd = 0;
double mean_freq_band_Z_2nd[32] = { 0 };
double leq_freq_band_Z_2nd[32];
double sum_leq_freq_band_Z_2nd[32] = { 0 };
double leq_freq_band_g_Z_2nd[32] = { 0 };
double last_leq_freq_band_g_Z_2nd[32] = { 0 };
double mean_freq_band_A_2nd[32] = { 0 };
double leq_freq_band_A_2nd[32];
double sum_leq_freq_band_A_2nd[32] = { 0 };
double leq_freq_band_g_A_2nd[32] = { 0 };
double last_leq_freq_band_g_A_2nd[32] = { 0 };

uint32_t mean_count_fft1024 = 0;
uint32_t measure_loop_count_fft1024 = 0;
double freq_band1024_Z[18];
double mean_freq_band1024_Z[18] = { 0 };
double leq_freq_band1024_Z[18];
double sum_leq_freq_band1024_Z[18] = { 0 };
double leq_freq_band1024_g_Z[18] = { 0 };
double freq_band1024_A[18];
double mean_freq_band1024_A[18] = { 0 };
double leq_freq_band1024_A[18];
double sum_leq_freq_band1024_A[18] = { 0 };
double leq_freq_band1024_g_A[18] = { 0 };

static double window_norm1024;
uint16_t fft1024_average;

uint32_t measure_loop_count_fft1024_2nd = 0;
double mean_freq_band1024_Z_2nd[18] = { 0 };
double leq_freq_band1024_Z_2nd[18];
double sum_leq_freq_band1024_Z_2nd[18] = { 0 };
double leq_freq_band1024_g_Z_2nd[18] = { 0 };
double mean_freq_band1024_A_2nd[18] = { 0 };
double leq_freq_band1024_A_2nd[18];
double sum_leq_freq_band1024_A_2nd[18] = { 0 };
double leq_freq_band1024_g_A_2nd[18] = { 0 };

float32_t decimation_buffer[BLK_LEN * DECI_FACTOR];
float32_t decimation_state_buffer[number_FIR_taps_decimation + (BLK_LEN * DECI_FACTOR) - 1];


void sum_Z(int x, int y, int z) {
  freq_band_Z[x] = 0.0f;
  do {
    freq_band_Z[x] += freq_Z[y++];
  } while (y <= z);
}

void sum_A(int x, int y, int z) {
  freq_band_A[x] = 0.0f;
  do {
    freq_band_A[x] += freq_A[y++];
  } while (y <= z);
}

void sum1024_A(int x, int y, int z) {
  freq_band1024_A[x] = 0.0f;
  do {
    freq_band1024_A[x] += freq1024_A[y++];
  } while (y <= z);
}

void sum1024_Z(int x, int y, int z) {
  freq_band1024_Z[x] = 0.0f;
  do {
    freq_band1024_Z[x] += freq1024_Z[y++];
  } while (y <= z);
}

void text_at_start() {

  interval_time = DEFAULT_TIME_FOR_TEST_OUTPUT;

  Serial.println(" ");
  Serial.println("DNMS Teensy4.0 standalone test program");
  Serial.println(" ");
  Serial.print(SOFTWARE_VERSION);
  Serial.println("  for ICS-43434 microphone");
  Serial.println(" ");
  Serial.println("Please enter the appropriate interval/output time in ms and press the 'Enter' key.");
  Serial.println("The smallest possible interval time is 125 ms.");
  Serial.println("If you do not enter an interval time within 10 seconds, the output will start with the default interval time of 1000ms.");
  Serial.println(" ");
  Serial.println("The values shown from left to right are LZeq, LZmin, LZmax, followed by the 31 1/3 octave values from 20Hz to 20,000Hz.");
  Serial.println("The 1st line is marked with 'LZ,'. The 2nd line is marked with 'LA,' and shows the LA values."); 
  Serial.println("All values are separated by a comma and the next measurement is separated by a blank line.");
  Serial.println(" ");
  Serial.println("Attention: if the interval/output time is less than 500ms, 1/3 octave values will not be output!");
  Serial.println(" ");
  Serial.println("To stop the current measurement and change the interval time, type '#' and then press 'Enter'.");
  Serial.println("The system starts from the beginning.");
  Serial.println(" ");

  Serial.setTimeout(10000);
  long eingabe = Serial.parseInt();
  if (eingabe == 0) {
    eingabe = interval_time;
  }
  if (eingabe < 125) {
    eingabe = 125;
  }

  interval_time = eingabe;
  Serial.print("measurement interval time and output period is: ");
  Serial.print(eingabe);
  Serial.println(" ms");

  OutputTimer.begin(SetOutput, interval_time * 1000);

  AudioInterrupts();  // start Audio Interrupts again
  startMillis = millis();
}

void setup() {
  // Für Testausgaben
  Serial.begin(115200);
  delay(500);

//  Wire.begin(0x55);
//  Wire.onReceive(i2c_receive_from_master);
//  Wire.onRequest(i2c_request_from_master);

  AudioMemory(10);
  AudioMemory_F32(64);

  dnms_decimation.end();

  AudioNoInterrupts();

  delay(500);

  fft_average = 1;
  dnms_fft.setOutputType(FFT_POWER);
  //  dnms_fft.windowFunction(AudioWindowNone);
  dnms_fft.windowFunction(AudioWindowHanning4096);
  dnms_fft.setNAverage(fft_average);

  fft1024_average = 1;
  dnms_fft1024.setOutputType(FFT_POWER);
  //  dnms_fft1024.windowFunction(AudioWindowNone);
  dnms_fft1024.windowFunction(AudioWindowHanning1024);
  dnms_fft1024.setNAverage(fft1024_average);

  delay(100);

  // pure window function normalization part for non coherent gain
  // source: http://www.gigalogchip.com/uploads/3/4/9/4/34940339/2009fftnoise.pdf page 2, table 1
#define window 0.3750  // Hanning
  // #define window 0.2580 // Blackman-Harris
  // technical normalization:
  // factor 2 due to overlapping windows
  window_norm4096 = 1.0 / (window * 2 * 2048.0 * 2048.0);
  window_norm1024 = 1.0 / (window * 2 * 512.0 * 512.0);

  dnms_decimation.begin(&dnms_decimate_coeffs[0], number_FIR_taps_decimation, DECI_FACTOR, BLK_LEN, &decimation_buffer[0], &decimation_state_buffer[0]);

  text_at_start();
}


// timer interrupt every ... ms, depends on input at beginning
void SetOutput() {
  calculate_leq = true;
}


void loop() {
  currentMillis = millis();
  if (start_phase) {
    if (dnms_fft.available()) {
      freq_band_Z[0] = dnms_fft.read(0, 2047);
      number_of_start_phase_fft_calculations++;
      if (number_of_start_phase_fft_calculations >= FFT_CALCULATIONS_IN_START_PHASE) {
        start_phase = false;
        if (dnms_fft1024.available()) {
          freq_band1024_Z[0] = dnms_fft1024.read(0, 511);
        }
      }
    }
  } else { /*** here starts the normal loop besides the start phase ***/
    // ***** FFT4096 available?
    if (dnms_fft.available()) {
      fft_all_Z = 0;
      fft_all_A = 0;
      // read FFT values, correct frequency response of micro and do A-weighting
      // sum up all FFT values to calculate LZeq and LAeq
      freq_Z[0] = dnms_fft.read(0);  // skip bin 0 to eleminate DC offsets, works like a high pass filter
      for (i = 1; i < 2048; i++) {
        freq_Z[i] = dnms_fft.read(i) * corr_Z[i];
        fft_all_Z += freq_Z[i];
        freq_A[i] = freq_Z[i] * corr_A[i];
        fft_all_A += freq_A[i];
      }
      mean_fft_all_Z += fft_all_Z;
      mean_fft_all_A += fft_all_A;
      mean_count_fft++;

      // from here on used for 1/3 octave values
      sum_Z(14, 33, 41);
      sum_Z(15, 42, 51);
      sum_Z(16, 52, 65);
      sum_Z(17, 66, 82);
      sum_Z(18, 83, 103);
      sum_Z(19, 104, 130);
      sum_Z(20, 131, 164);
      sum_Z(21, 165, 207);
      sum_Z(22, 208, 261);
      sum_Z(23, 262, 328);
      sum_Z(24, 329, 414);
      sum_Z(25, 415, 521);
      sum_Z(26, 522, 656);
      sum_Z(27, 657, 826);
      sum_Z(28, 827, 1039);
      sum_Z(29, 1040, 1311);
      sum_Z(30, 1312, 1650);
      sum_Z(31, 1651, 2047);

      sum_A(14, 33, 41);
      sum_A(15, 42, 51);
      sum_A(16, 52, 65);
      sum_A(17, 66, 82);
      sum_A(18, 83, 103);
      sum_A(19, 104, 130);
      sum_A(20, 131, 164);
      sum_A(21, 165, 207);
      sum_A(22, 208, 261);
      sum_A(23, 262, 328);
      sum_A(24, 329, 414);
      sum_A(25, 415, 521);
      sum_A(26, 522, 656);
      sum_A(27, 657, 826);
      sum_A(28, 827, 1039);
      sum_A(29, 1040, 1311);
      sum_A(30, 1312, 1650);
      sum_A(31, 1651, 2047);

      for (i = 14; i < 32; i++) {
        mean_freq_band_Z[i] += freq_band_Z[i];
        mean_freq_band_Z_2nd[i] += freq_band_Z[i];
        mean_freq_band_A[i] += freq_band_A[i];
        mean_freq_band_A_2nd[i] += freq_band_A[i];
      }
    }

    // **** FFT1024 available?
    if (dnms_fft1024.available()) {

      // read FFT values, correct frequency response of micro and do A-weighting
      for (i = 0; i < 512; i++) {
        freq1024_Z[i] = dnms_fft1024.read(i) * corr1024_Z[i];
        freq1024_A[i] = freq1024_Z[i] * corr1024_A[i];
      }

      // from here used for 1/3 octave values
      sum1024_Z(1, 13, 16);
      sum1024_Z(2, 17, 20);
      sum1024_Z(3, 21, 25);
      sum1024_Z(4, 26, 32);
      sum1024_Z(5, 33, 41);
      sum1024_Z(6, 42, 52);
      sum1024_Z(7, 53, 65);
      sum1024_Z(8, 66, 82);
      sum1024_Z(9, 83, 104);
      sum1024_Z(10, 105, 131);
      sum1024_Z(11, 132, 166);
      sum1024_Z(12, 167, 209);
      sum1024_Z(13, 210, 263);
      // up to here used for 1/3 octave values

      // from here used for 1/3 octave values
      sum1024_A(1, 13, 16);
      sum1024_A(2, 17, 20);
      sum1024_A(3, 21, 25);
      sum1024_A(4, 26, 32);
      sum1024_A(5, 33, 41);
      sum1024_A(6, 42, 52);
      sum1024_A(7, 53, 65);
      sum1024_A(8, 66, 82);
      sum1024_A(9, 83, 104);
      sum1024_A(10, 105, 131);
      sum1024_A(11, 132, 166);
      sum1024_A(12, 167, 209);
      sum1024_A(13, 210, 263);
      // up to here used for 1/3 octave values

      for (i = 1; i < 14; i++) {  // only for the used ones
        mean_freq_band1024_Z[i] += freq_band1024_Z[i];
        mean_freq_band1024_Z_2nd[i] += freq_band1024_Z[i];
        mean_freq_band1024_A[i] += freq_band1024_A[i];
        mean_freq_band1024_A_2nd[i] += freq_band1024_A[i];
      }
      mean_count_fft1024++;
    }


    /**** BLINK_PERIOD (standard 200ms) over? ***/
    if (currentMillis - startMillis >= BLINK_PERIOD) {
      startMillis = currentMillis;
      // blinking of internal led ( 2 x BLINK_PERIOD on and 2 x BLINK_PERIOD off)
      led_count++;
      if (led_count == 2) {
        led_count = 0;
        if (led_on == false) {
          led_on = true;
          digitalWrite(led, HIGH);
        } else {
          led_on = false;
          digitalWrite(led, LOW);
        }
      }
    }

    if (Serial.available() > 0) {  // input of '# + Enter' to start from beginning??
      if (Serial.read() == 35) {   // if yes let's output text and input interval time
        AudioNoInterrupts();
        text_at_start();
//        SCB_AIRCR = 0x05FA0004;
      }
    }

    // calculate LZeq and LAeq over CALC_PERIOD interval from FFT and store/check min and max values
    if (mean_count_fft > 0) {

      // recalculate FFT LAeq for sum of alls bins and for all 1/3 octave values after x FFTs
      // LAeq aus FFT berechnen
      z = mean_fft_all_A * window_norm4096 / mean_count_fft;
      if (z == 0) {
        z = 1.0e-12;
      }
      leq_fft_A = 123.0102999565 + 10 * log10(z);

      //        Serial.print("z: "); Serial.print(z, 12); Serial.print("  leq_fft_A: "); Serial.println(leq_fft_A, 12);

      z = mean_fft_all_Z * window_norm4096 / mean_count_fft;
      if (z == 0) {
        z = 1.0e-12;
      }
      leq_fft_Z = 123.0102999565 + 10 * log10(z);

      // compare new LAeq to saved LAeq min and save, if it is smaller than old LAeq min
      if (leq_A_min == 0) {
        leq_A_min = leq_fft_A;
      } else {
        if (leq_fft_A < leq_A_min) {
          leq_A_min = leq_fft_A;
        }
      }

      // now for 2nd: compare new LAeq to saved LAeq min and save, if it is smaller than old LAeq min
      if (leq_A_min_2nd == 0) {
        leq_A_min_2nd = leq_fft_A;
      } else {
        if (leq_fft_A < leq_A_min_2nd) {
          leq_A_min_2nd = leq_fft_A;
        }
      }

      // compare new LZeq to saved LAeq min and save, if it is smaller than old LZeq min
      if (leq_Z_min == 0) {
        leq_Z_min = leq_fft_Z;
      } else {
        if (leq_fft_Z < leq_Z_min) {
          leq_Z_min = leq_fft_Z;
        }
      }

      // now for 2nd: compare new LZeq to saved LZeq min and save, if it is smaller than old LZeq min
      if (leq_Z_min_2nd == 0) {
        leq_Z_min_2nd = leq_fft_Z;
      } else {
        if (leq_fft_Z < leq_Z_min_2nd) {
          leq_Z_min_2nd = leq_fft_Z;
        }
      }

      // compare new LAeq to saved LAeq max and save, if it is bigger than old LAeq max
      if (leq_A_max == 0) {
        leq_A_max = leq_fft_A;
      } else {
        if (leq_fft_A > leq_A_max) {
          leq_A_max = leq_fft_A;
        }
      }

      // now for 2nd: compare new LAeq to saved LAeq max and save, if it is bigger than old LAeq max
      if (leq_A_max_2nd == 0) {
        leq_A_max_2nd = leq_fft_A;
      } else {
        if (leq_fft_A > leq_A_max_2nd) {
          leq_A_max_2nd = leq_fft_A;
        }
      }

      // compare new LZeq to saved LZeq max and save, if it is bigger than old LZeq max
      if (leq_Z_max == 0) {
        leq_Z_max = leq_fft_Z;
      } else {
        if (leq_fft_Z > leq_Z_max) {
          leq_Z_max = leq_fft_Z;
        }
      }

      // now for 2nd: compare new LZeq to saved LZeq max and save, if it is bigger than old LZeq max
      if (leq_Z_max_2nd == 0) {
        leq_Z_max_2nd = leq_fft_Z;
      } else {
        if (leq_fft_Z > leq_Z_max_2nd) {
          leq_Z_max_2nd = leq_fft_Z;
        }
      }

      mean_fft_all_A = 0;
      mean_fft_all_Z = 0;

      // recalculate the overall LAeq
      delog_leq_A = pow(10, leq_fft_A / 10);
      sum_leq_g_A += delog_leq_A;
      measure_loop_count++;
      leq_g_A = 10 * log10(sum_leq_g_A / measure_loop_count);  // neuer gesamt LAeq

      // recalculate the overall LZeq
      delog_leq_Z = pow(10, leq_fft_Z / 10);
      sum_leq_g_Z += delog_leq_Z;
      leq_g_Z = 10 * log10(sum_leq_g_Z / measure_loop_count);  // neuer gesamt LZeq

      // recalculate the overall LAeq_2nd
      sum_leq_g_A_2nd += delog_leq_A;
      measure_loop_count_2nd++;
      leq_g_A_2nd = 10 * log10(sum_leq_g_A_2nd / measure_loop_count_2nd);  // neuer gesamt LAeq_2nd

      // recalculate the overall LZeq_2nd
      sum_leq_g_Z_2nd += delog_leq_Z;
      leq_g_Z_2nd = 10 * log10(sum_leq_g_Z_2nd / measure_loop_count_2nd);  // neuer gesamt LZeq_2nd

      // now LZeq and LAeq calculation for 1/3 octave bands
      measure_loop_count_fft++;
      measure_loop_count_fft_2nd++;
      for (i = 14; i < 32; i++) {
        z = mean_freq_band_Z[i] * window_norm4096 / mean_count_fft;
        if (z == 0) {
          z = 0.0000001;
        }
        leq_freq_band_Z[i] = 123.0102999565 + 10 * log10(z);
        f1 = pow(10, leq_freq_band_Z[i] / 10);
        sum_leq_freq_band_Z[i] += f1;  // gesamt LZeq weiterrechnen
        sum_leq_freq_band_Z_2nd[i] += f1;
        leq_freq_band_g_Z[i] = 10 * log10(sum_leq_freq_band_Z[i] / measure_loop_count_fft);              // neuer gesamt LZeq
        leq_freq_band_g_Z_2nd[i] = 10 * log10(sum_leq_freq_band_Z_2nd[i] / measure_loop_count_fft_2nd);  // neuer gesamt LZeq_2nd
        mean_freq_band_Z[i] = 0;

        z = mean_freq_band_A[i] * window_norm4096 / mean_count_fft;
        if (z == 0) {
          z = 0.0000001;
        }
        leq_freq_band_A[i] = 123.0102999565 + 10 * log10(z);
        f1 = pow(10, leq_freq_band_A[i] / 10);
        sum_leq_freq_band_A[i] += f1;  // gesamt LAeq weiterrechnen
        sum_leq_freq_band_A_2nd[i] += f1;
        leq_freq_band_g_A[i] = 10 * log10(sum_leq_freq_band_A[i] / measure_loop_count_fft);              // neuer gesamt LAeq
        leq_freq_band_g_A_2nd[i] = 10 * log10(sum_leq_freq_band_A_2nd[i] / measure_loop_count_fft_2nd);  // neuer gesamt LAeq_2nd
        mean_freq_band_A[i] = 0;
      }
      mean_count_fft = 0;
    }
    // ***** ZoomFFT1024
    if (mean_count_fft1024 > 0) {
      measure_loop_count_fft1024++;
      measure_loop_count_fft1024_2nd++;
      for (i = 1; i < 14; i++) {
        z = mean_freq_band1024_Z[i] * window_norm1024 / mean_count_fft1024;
        if (z == 0) {
          z = 0.0000001;
        }
        leq_freq_band1024_Z[i] = 123.0102999565 + 10 * log10(z);
        f1 = pow(10, leq_freq_band1024_Z[i] / 10);
        sum_leq_freq_band1024_Z[i] += f1;  // gesamt Leq weiterrechnen
        sum_leq_freq_band1024_Z_2nd[i] += f1;
        leq_freq_band1024_g_Z[i] = 10 * log10(sum_leq_freq_band1024_Z[i] / measure_loop_count_fft1024);              // neuer gesamt Leq
        leq_freq_band1024_g_Z_2nd[i] = 10 * log10(sum_leq_freq_band1024_Z_2nd[i] / measure_loop_count_fft1024_2nd);  // neuer gesamt Leq
        mean_freq_band1024_Z[i] = 0;

        z = mean_freq_band1024_A[i] * window_norm1024 / mean_count_fft1024;
        if (z == 0) {
          z = 0.0000001;
        }
        leq_freq_band1024_A[i] = 123.0102999565 + 10 * log10(z);
        f1 = pow(10, leq_freq_band1024_A[i] / 10);
        sum_leq_freq_band1024_A[i] += f1;  // gesamt Leq weiterrechnen
        sum_leq_freq_band1024_A_2nd[i] += f1;
        leq_freq_band1024_g_A[i] = 10 * log10(sum_leq_freq_band1024_A[i] / measure_loop_count_fft1024);              // neuer gesamt Leq
        leq_freq_band1024_g_A_2nd[i] = 10 * log10(sum_leq_freq_band1024_A_2nd[i] / measure_loop_count_fft1024_2nd);  // neuer gesamt Leq
        mean_freq_band1024_A[i] = 0;
      }
      mean_count_fft1024 = 0;
    }

    if (calculate_leq) {
      measure_loop_count = 0;
      last_leq_g_A = leq_g_A;
      last_leq_A_min = leq_A_min;
      last_leq_A_max = leq_A_max;
      leq_g_A = 0;
      sum_leq_g_A = 0;
      leq_A_min = 0;
      leq_A_max = 0;

      last_leq_g_Z = leq_g_Z;
      last_leq_Z_min = leq_Z_min;
      last_leq_Z_max = leq_Z_max;
      leq_g_Z = 0;
      sum_leq_g_Z = 0;
      leq_Z_min = 0;
      leq_Z_max = 0;

      // ***** FFT4096
      measure_loop_count_fft = 0;
      for (i = 14; i < 32; i++) {
        last_leq_freq_band_g_A[i] = leq_freq_band_g_A[i];
        sum_leq_freq_band_A[i] = 0;
        leq_freq_band_g_A[i] = 0;
        last_leq_freq_band_g_Z[i] = leq_freq_band_g_Z[i];
        sum_leq_freq_band_Z[i] = 0;
        leq_freq_band_g_Z[i] = 0;
      }

      // **** FFT1024
      measure_loop_count_fft1024 = 0;
      for (i = 1; i < 14; i++) {
        last_leq_freq_band_g_A[i] = leq_freq_band1024_g_A[i];
        sum_leq_freq_band1024_A[i] = 0;
        leq_freq_band1024_g_A[i] = 0;
        last_leq_freq_band_g_Z[i] = leq_freq_band1024_g_Z[i];
        sum_leq_freq_band1024_Z[i] = 0;
        leq_freq_band1024_g_Z[i] = 0;
      }

      calculate_leq = false;
      data_ready = 1;

// testprog output of LZ values
      Serial.println(" ");
      Serial.print("LZ,");
      sprintf(s1, "%6.1f,", last_leq_g_Z);
      Serial.print(s1);
      //Max
      sprintf(s1, "%6.1f,", last_leq_Z_min);
      Serial.print(s1);
      //Original
      sprintf(s1, "%6.1f", last_leq_Z_max);
      Serial.print(s1);

      if (interval_time >= 500) {  // Output of 1/3 Octaves only if output period is >= 500ms
        Serial.print(",");
        for (i = 1; i < 14; i++) {
          sprintf(s1, "%6.1f,", last_leq_freq_band_g_Z[i]);
          Serial.print(s1);
        }
        for (i = 14; i < 31; i++) {
          sprintf(s1, "%6.1f,", last_leq_freq_band_g_Z[i]);
          Serial.print(s1);
        }
        sprintf(s1, "%6.1f", last_leq_freq_band_g_Z[31]);
        Serial.print(s1);
      }

// testprog output of LA values
      Serial.println(" ");
      Serial.print("LA,");
      sprintf(s1, "%6.1f,", last_leq_g_A);
      Serial.print(s1);
      //Max
      sprintf(s1, "%6.1f,", last_leq_A_min);
      Serial.print(s1);
      //Original
      sprintf(s1, "%6.1f", last_leq_A_max);
      Serial.print(s1);

      if (interval_time >= 500) {  // Output of 1/3 Octaves only if output period is >= 500ms
        Serial.print(",");
        for (i = 1; i < 14; i++) {
          sprintf(s1, "%6.1f,", last_leq_freq_band_g_A[i]);
          Serial.print(s1);
        }
        for (i = 14; i < 31; i++) {
          sprintf(s1, "%6.1f,", last_leq_freq_band_g_A[i]);
          Serial.print(s1);
        }
        sprintf(s1, "%6.1f", last_leq_freq_band_g_A[31]);
        Serial.print(s1);
      }
      Serial.println(" ");
    }

    /*** and now for 2nd Interval if it was requested ***/
    if (calculate_leq_2nd) {

      measure_loop_count_2nd = 0;
      last_leq_g_A_2nd = leq_g_A_2nd;
      last_leq_A_min_2nd = leq_A_min_2nd;
      last_leq_A_max_2nd = leq_A_max_2nd;
      sum_leq_g_A_2nd = 0;
      leq_g_A_2nd = 0;
      leq_A_min_2nd = 0;
      leq_A_max_2nd = 0;

      last_leq_g_Z_2nd = leq_g_Z_2nd;
      last_leq_Z_min_2nd = leq_Z_min_2nd;
      last_leq_Z_max_2nd = leq_Z_max_2nd;
      sum_leq_g_Z_2nd = 0;
      leq_g_Z_2nd = 0;
      leq_Z_min_2nd = 0;
      leq_Z_max_2nd = 0;

      // ***** FFT4096
      measure_loop_count_fft_2nd = 0;
      for (i = 14; i < 32; i++) {
        last_leq_freq_band_g_A_2nd[i] = leq_freq_band_g_A_2nd[i];
        sum_leq_freq_band_A_2nd[i] = 0;
        leq_freq_band_g_A_2nd[i] = 0;
        last_leq_freq_band_g_Z_2nd[i] = leq_freq_band_g_Z_2nd[i];
        sum_leq_freq_band_Z_2nd[i] = 0;
        leq_freq_band_g_Z_2nd[i] = 0;
      }

      // **** FFT1024
      measure_loop_count_fft1024_2nd = 0;
      for (i = 1; i < 14; i++) {
        last_leq_freq_band_g_A_2nd[i] = leq_freq_band1024_g_A_2nd[i];
        sum_leq_freq_band1024_A_2nd[i] = 0;
        leq_freq_band1024_g_A_2nd[i] = 0;
        last_leq_freq_band_g_Z_2nd[i] = leq_freq_band1024_g_Z_2nd[i];
        sum_leq_freq_band1024_Z_2nd[i] = 0;
        leq_freq_band1024_g_Z_2nd[i] = 0;
      }

      calculate_leq_2nd = false;
      data_ready_2nd = 1;
    }
  }
}

void i2c_request_from_master() {
  uint16_t i;
  uint16_t idx;
  uint8_t crc;
  uint8_t b[4];

  switch (i2c_state) {
    case 2:  // Read DNMS Version
      idx = 0;
      for (i = 0; i < sizeof(SOFTWARE_VERSION); ++i) {
        buf[idx++] = SOFTWARE_VERSION[i++];
        buf[idx++] = SOFTWARE_VERSION[i];
        crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
        buf[idx++] = crc;
      }
      i = Wire.write(buf, idx - 3);
      i2c_state = 0;
      break;

    case 4:  // Read Data Ready
      idx = 0;
      buf[idx++] = uint8_t((data_ready >> 8) & 0xff);
      buf[idx++] = uint8_t(data_ready & 0xff);
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      Wire.write(buf, 3);
      i2c_state = 0;
      break;

    case 5:  // Read LAeq
      idx = 0;

      f2b(last_leq_g_A, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;

      f2b(last_leq_A_min, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;

      f2b(last_leq_A_max, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      i = Wire.write(buf, idx);
      data_ready = 0;
      i2c_state = 0;

      last_leq_g_A = 0.0;
      last_leq_A_max = 0.0;
      last_leq_A_min = 0.0;

      break;

    case 6:  // Read 1/3 octav values part 1
      fft_values_i2c_write(1, 8);
      break;

    case 7:  // Read 1/3 octav values part 2
      fft_values_i2c_write(9, 16);
      break;

    case 8:  // Read 1/3 octav values part 3
      fft_values_i2c_write(17, 24);
      break;

    case 9:  // Read 1/3 octav values part 4
      fft_values_i2c_write(25, 31);
      break;

    case 11:  // Read Data Ready 2nd
      idx = 0;
      buf[idx++] = uint8_t((data_ready_2nd >> 8) & 0xff);
      buf[idx++] = uint8_t(data_ready_2nd & 0xff);
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      Wire.write(buf, 3);
      i2c_state = 0;
      break;

    case 12:  // Read LAeq 2nd
      idx = 0;

      f2b(last_leq_g_A_2nd, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;

      f2b(last_leq_A_min_2nd, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;

      f2b(last_leq_A_max_2nd, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      i = Wire.write(buf, idx);
      data_ready = 0;
      i2c_state = 0;
      break;

    case 13:  // Read 1/3 octav values 2nd part 1
      fft_values_2nd_i2c_write(1, 8);
      break;

    case 14:  // Read 1/3 octav values 2nd part 2
      fft_values_2nd_i2c_write(9, 16);
      break;

    case 15:  // Read 1/3 octav values 2nd part 3
      fft_values_2nd_i2c_write(17, 24);
      break;

    case 16:  // Read 1/3 octav values 2nd part 4
      fft_values_2nd_i2c_write(25, 31);
      break;

    case 17:  // Read LZeq
      idx = 0;

      f2b(last_leq_g_Z, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;

      f2b(last_leq_Z_min, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;

      f2b(last_leq_Z_max, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      i = Wire.write(buf, idx);
      data_ready = 0;
      i2c_state = 0;

      last_leq_g_Z = 0.0;
      last_leq_Z_max = 0.0;
      last_leq_Z_min = 0.0;
      break;

    case 18:  // Read Z 1/3 octav values part 1
      fft_Z_values_i2c_write(1, 8);
      break;

    case 19:  // Read Z 1/3 octav values part 2
      fft_Z_values_i2c_write(9, 16);
      break;

    case 20:  // Read Z 1/3 octav values part 3
      fft_Z_values_i2c_write(17, 24);
      break;

    case 21:  // Read Z 1/3 octav values part 4
      fft_Z_values_i2c_write(25, 31);
      break;

    case 22:  // Read LZeq 2nd
      idx = 0;

      f2b(last_leq_g_Z_2nd, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;

      f2b(last_leq_Z_min_2nd, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;

      f2b(last_leq_Z_max_2nd, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      i = Wire.write(buf, idx);
      data_ready = 0;
      i2c_state = 0;
      break;

    case 23:  // Read Z 1/3 octav 2nd values part 1
      fft_Z_values_2nd_i2c_write(1, 8);
      break;

    case 24:  // Read Z 1/3 octav 2nd values part 2
      fft_Z_values_2nd_i2c_write(9, 16);
      break;

    case 25:  // Read Z 1/3 octav 2nd values part 3
      fft_Z_values_2nd_i2c_write(17, 24);
      break;

    case 26:  // Read Z 1/3 octav 2nd values part 4
      fft_Z_values_2nd_i2c_write(25, 31);
      break;

    default:  // else
      break;
  }
}


void fft_values_i2c_write(uint32_t start_index, uint32_t end_index) {

  uint16_t i;
  uint16_t idx;
  uint8_t crc;
  uint8_t b[4];

  idx = 0;
  for (i = start_index; i < end_index + 1; i++) {
    f2b(last_leq_freq_band_g_A[i], &b[0]);
    buf[idx++] = b[3];
    buf[idx++] = b[2];
    crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
    buf[idx++] = crc;
    buf[idx++] = b[1];
    buf[idx++] = b[0];
    crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
    buf[idx++] = crc;
  }
  i = Wire.write(buf, idx);
  data_ready = 0;
  i2c_state = 0;
}

void fft_Z_values_i2c_write(uint32_t start_index, uint32_t end_index) {

  uint16_t i;
  uint16_t idx;
  uint8_t crc;
  uint8_t b[4];

  idx = 0;
  for (i = start_index; i < end_index + 1; i++) {
    f2b(last_leq_freq_band_g_Z[i], &b[0]);
    buf[idx++] = b[3];
    buf[idx++] = b[2];
    crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
    buf[idx++] = crc;
    buf[idx++] = b[1];
    buf[idx++] = b[0];
    crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
    buf[idx++] = crc;
  }
  i = Wire.write(buf, idx);
  data_ready = 0;
  i2c_state = 0;
}

void fft_values_2nd_i2c_write(uint32_t start_index, uint32_t end_index) {

  uint16_t i;
  uint16_t idx;
  uint8_t crc;
  uint8_t b[4];

  idx = 0;
  for (i = start_index; i < end_index + 1; i++) {
    f2b(last_leq_freq_band_g_A_2nd[i], &b[0]);
    buf[idx++] = b[3];
    buf[idx++] = b[2];
    crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
    buf[idx++] = crc;
    buf[idx++] = b[1];
    buf[idx++] = b[0];
    crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
    buf[idx++] = crc;
  }
  i = Wire.write(buf, idx);
  data_ready = 0;
  i2c_state = 0;
}

void fft_Z_values_2nd_i2c_write(uint32_t start_index, uint32_t end_index) {

  uint16_t i;
  uint16_t idx;
  uint8_t crc;
  uint8_t b[4];

  idx = 0;
  for (i = start_index; i < end_index + 1; i++) {
    f2b(last_leq_freq_band_g_Z_2nd[i], &b[0]);
    buf[idx++] = b[3];
    buf[idx++] = b[2];
    crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
    buf[idx++] = crc;
    buf[idx++] = b[1];
    buf[idx++] = b[0];
    crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
    buf[idx++] = crc;
  }
  i = Wire.write(buf, idx);
  data_ready = 0;
  i2c_state = 0;
}


void i2c_receive_from_master(int num_bytes) {
  uint16_t i;

  i2c_state = 0;
  i = 0;
  while (Wire.available()) {
    buf[i] = Wire.read();
    i++;
  }

  command_received = (buf[0] << 8) | buf[1];
  switch (command_received) {
    case 1:               // Reset DNMS
      AudioInterrupts();  // start Audio Interrupts again
      break;

    case 2:  // Read DNMS Version
      i2c_state = 2;
      break;

    case 3:  // Calculate Leq
      i2c_state = 3;
      data_ready = 0;
      calculate_leq = true;
      break;

    case 4:  // Read Data (LAeq) Ready
      i2c_state = 4;
      break;

    case 5:  // Read LAeq
      i2c_state = 5;
      break;

    case 6:  // Read 1/3 octav values part 1
      i2c_state = 6;
      break;

    case 7:  // Read 1/3 octav values part 2
      i2c_state = 7;
      break;

    case 8:  // Read 1/3 octav values part 3
      i2c_state = 8;
      break;

    case 9:  // Read 1/3 octav values part 4
      i2c_state = 9;
      break;

    case 10:  // Calculate Leq 2nd
      i2c_state = 10;
      data_ready_2nd = 0;
      calculate_leq_2nd = true;
      break;

    case 11:  // Read Data(LAeq) Ready 2nd
      i2c_state = 11;
      break;

    case 12:  // Read LAeq 2nd
      i2c_state = 12;
      break;

    case 13:  // Read 1/3 octav values 2nd part 1
      i2c_state = 13;
      break;

    case 14:  // Read 1/3 octav values 2nd part 2
      i2c_state = 14;
      break;

    case 15:  // Read 1/3 octav values 2nd part 3
      i2c_state = 15;
      break;

    case 16:  // Read 1/3 octav values 2nd part 4
      i2c_state = 16;
      break;

    case 17:  // Read LZeq
      i2c_state = 17;
      break;

    case 18:  // Read Z 1/3 octav values part 1
      i2c_state = 18;
      break;

    case 19:  // Read Z 1/3 octav values part 2
      i2c_state = 19;
      break;

    case 20:  // Read Z 1/3 octav values part 3
      i2c_state = 20;
      break;

    case 21:  // Read Z 1/3 octav values part 4
      i2c_state = 21;
      break;

    case 22:  // Read LZeq 2nd
      i2c_state = 22;
      break;

    case 23:  // Read Z 1/3 octav values 2nd part 1
      i2c_state = 23;
      break;

    case 24:  // Read Z 1/3 octav values 2nd part 2
      i2c_state = 24;
      break;

    case 25:  // Read Z 1/3 octav values 2nd part 3
      i2c_state = 25;
      break;

    case 26:  // Read Z 1/3 octav values 2nd part 4
      i2c_state = 26;
      break;

    default:  // else
      break;
  }
}


void f2b(float val, uint8_t *bytes_array) {
  // Create union of shared memory space
  union {
    float float_variable;  //      leq_HP_min = 0;
    //      leq_HP_max = 0;
    uint8_t temp_array[4];
  } u;
  // Overite bytes of union with float variable
  u.float_variable = val;
  // Assign bytes to input array
  memcpy(bytes_array, u.temp_array, 4);
}


uint8_t dnms_common_generate_crc(uint8_t *data, uint16_t count) {
  uint16_t current_byte;
  uint8_t crc = CRC8_INIT;
  uint8_t crc_bit;

  /* calculates 8-Bit checksum with given polynomial */
  for (current_byte = 0; current_byte < count; ++current_byte) {
    crc ^= (data[current_byte]);
    for (crc_bit = 8; crc_bit > 0; --crc_bit) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ CRC8_POLYNOMIAL;
      } else {
        crc = (crc << 1);
      }
    }
  }
  return crc;
}
