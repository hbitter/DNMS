/************************************************************************
 *                                                                      *
    DNMS - Digital Noise Measurement Sensor
 *                                                                      *
    This source code is for the board Teensy 4.0
    I2S Input from IvenSense Microphone ICS-43434
    Output as I2C Slave to other Microcontroller (e.g. NodeMCU)
 *                                                                      *
 ************************************************************************
 *                                                                      *
      DNMS - Digital Noise Measurement Sensor
      Copyright (C) 2019  Helmut Bitter
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


#define SOFTWARE_VERSION "DNMS Version 2.0.4"  // Version 2.0 for Teensy4 

/*

   Attention: in Wire.h set BUFFER_LENGTH to 64 !!

   #define BUFFER_LENGTH 64

*/
#define number_of_loops 12        // number of 2,9ms loops (128 PCM values in a block if sample rate is 44,1 kHz)

#define limit_of_rms_calculations_in_start_phase 20 // define number of first RMS calculations to throw away

#define fft_loops 1

#define DNMS_I2C_ADDRESS                0x55
#define DNMS_MAX_VERSION_LEN            32
#define DNMS_WORD_SIZE                  2
#define DNMS_NUM_WORDS(x)               (sizeof(x) / DNMS_WORD_SIZE)
#define STATUS_OK                       0
#define STATUS_FAIL                     (-1)
#define DNMS_COMMAND_SIZE               2
#define CRC8_POLYNOMIAL                 0x31
#define CRC8_INIT                       0xFF
#define CRC8_LEN                        1
#define DNMS_MAX_BUFFER_WORDS           32
#define DNMS_IS_ERR_STATE(err_code)     (((err_code) | 0xff) == 0x1ff)

#define DNMS_CMD_RESET                  0x0001
#define DNMS_CMD_READ_VERSION           0x0002
#define DNMS_CMD_CALCULATE_LEQ          0x0003
#define DNMS_CMD_READ_DATA_READY        0x0004
#define DNMS_CMD_READ_LEQ               0x0005


#define be16_to_cpu(s) (((uint16_t)(s) << 8) | (0xff & ((uint16_t)(s)) >> 8))
#define be32_to_cpu(s) (((uint32_t)be16_to_cpu(s) << 16) | \
                        (0xffff & (be16_to_cpu((s) >> 16))))
#define be64_to_cpu(s) (((uint64_t)be32_to_cpu(s) << 32) | \
                        (0xffffffff & ((uint64_t)be32_to_cpu((s) >> 32))))
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
AudioInputI2S_F32         i2s_in;
//AudioOutputUSB            usb_out;
//AudioConvert_F32toI16     convert_fp_to_int;
AudioFilterBiquad_F32     filter_LP;
AudioFilterBiquad_F32     filter_A;
AudioAnalyzeRMS_F32       aa_rms_A;
AudioConnection_F32       patchCord1(i2s_in, 0, filter_LP, 0);
AudioConnection_F32       patchCord2(filter_LP, 0, filter_A, 0);
AudioConnection_F32       patchCord3(filter_A, aa_rms_A);
//AudioConnection_F32       patchCord5(i2s_in, 0, convert_fp_to_int, 0);
//AudioConnection           patchCord6(convert_fp_to_int, 0, usb_out, 0);
//AudioConnection           patchCord7(convert_fp_to_int, 0, usb_out, 1);

// GUItool: end automatically generated code


int led = LED_BUILTIN;
bool led_on;
uint16_t led_count = 0;

uint8_t i2c_state = 0;
uint8_t buf[64];
uint16_t data_ready = 0;
bool calculate_leq = false;
uint16_t command_received;

//elapsedMillis leq_loop;
elapsedMillis measure_loop;
elapsedMillis disp_loop;
elapsedMillis fft_millis;

double rms_A;
double rms_mean_A = 0;
uint32_t mean_count_A = 0;
uint32_t measure_loop_count = 0;

boolean start_phase = true;
uint16_t number_of_start_phase_rms_calculations = 0;

double leq_A;
float leq_A_max = 0;
float leq_A_min = 0;
double leq_g_A = 0;
double sum_leq_g_A = 0;
double delog_leq_A;
double z;

float last_leq_g_A;
float last_leq_A_min;
float last_leq_A_max;

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(0x55);
  Wire.onReceive(i2c_receive_from_master);
  Wire.onRequest(i2c_request_from_master);

  delay(500);

  AudioMemory(10);
  AudioMemory_F32(32);

  filter_LP.setLowpass(0, 18000, 0.11, false);
  filter_LP.setLowpass(1, 21800, 0.03, true);

  filter_A.setBandpass(0, 2500, 0.33, false);      // A-Filter Bandpass
  filter_A.setHighpass(1, 8, 0.12, true);       // A-Filter Highpass

}


void loop() {


  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'o') c = 'o';
    if (c == 's') c = 's';
  }

  if (start_phase && aa_rms_A.available()) {
      rms_A = aa_rms_A.read();
      number_of_start_phase_rms_calculations++;
      if (number_of_start_phase_rms_calculations >= limit_of_rms_calculations_in_start_phase) {
        start_phase = false;
      }
  } else {

    if (aa_rms_A.available()) {
      rms_A = aa_rms_A.read(); // * konstante;
      rms_mean_A += rms_A * rms_A;
      ++mean_count_A;
    }

    if (mean_count_A >= number_of_loops) {

      // blinking of internal led ( 7 x 35ms on and 7 x 35ms off)
      ++led_count;
      if (led_count == 7) {
        led_count = 0;
        if (led_on == false) {
          led_on = true;
          digitalWrite(led, HIGH);
        } else {
          led_on = false;
          digitalWrite(led, LOW);
        }
      }


      z = sqrt(rms_mean_A / mean_count_A);
      if (z == 0) {
        z = 0.000001;
      }

      leq_A = 125.0 + 20 * log10(z);

      if (leq_A_min == 0) {
        leq_A_min = leq_A;
      } else {
        if (leq_A < leq_A_min) {
          leq_A_min = leq_A;
        }
      }

      if (leq_A_max == 0) {
        leq_A_max = leq_A;
      } else {
        if (leq_A > leq_A_max) {
          leq_A_max = leq_A;
        }
      }

      delog_leq_A = pow(10, leq_A / 10);  // gesamt Leq weiterrechnen
      sum_leq_g_A += delog_leq_A;

      ++measure_loop_count;

      leq_g_A = 10 * log10(sum_leq_g_A / measure_loop_count); // neuer gesamt Leq

      rms_mean_A = 0;
      mean_count_A = 0;

      if (calculate_leq) {
        calculate_leq = false;
        measure_loop_count = 0;
        last_leq_g_A = leq_g_A;
        last_leq_A_min = leq_A_min;
        last_leq_A_max = leq_A_max;
        data_ready = 1;

        sum_leq_g_A = 0;
        leq_A_min = 0;
        leq_A_max = 0;
      }
    }
  }
}

  void i2c_request_from_master() {
    uint16_t i;
    uint16_t idx;
    uint8_t crc;
    uint8_t b[4];

    switch (i2c_state) {
      case 2: // Read DNMS Version
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

      case 4: // Read Data Ready
        idx = 0;
        buf[idx++] = uint8_t ((data_ready >> 8) & 0xff);
        buf[idx++] = uint8_t (data_ready & 0xff);
        crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
        buf[idx++] = crc;
        Wire.write(buf, 3);
        i2c_state = 0;
        break;

      case 5: // Read Leq
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
        break;

      default: // sonst
        break;
    }
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
      case 1: // Reset DNMS
        SCB_AIRCR = 0x05FA0004;
        break;

      case 2: // Read DNMS Version
        i2c_state = 2;

        break;

      case 3: // Calculate Leq
        i2c_state = 3;
        data_ready = 0;
        calculate_leq = true;

        break;

      case 4: // Read Data (Leq) Ready
        i2c_state = 4;
        break;

      case 5: // Read Leq
        i2c_state = 5;
        break;

      default: // sonst
        break;
    }
  }


  void f2b(float val, uint8_t* bytes_array) {
    // Create union of shared memory space
    union {
      float float_variable;      //      leq_HP_min = 0;
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
        if (crc & 0x80)
          crc = (crc << 1) ^ CRC8_POLYNOMIAL;
        else
          crc = (crc << 1);
      }
    }
    return crc;
  }
