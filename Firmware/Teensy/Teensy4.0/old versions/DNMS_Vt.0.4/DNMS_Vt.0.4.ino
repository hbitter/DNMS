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
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>


#define SOFTWARE_VERSION "DNMS Version t.0.4"  // its for Teensy4 Version 1.0 without USB-Audio

/*
 * 
 * Attention: in Wire.h set BUFFER_LENGTH to 64 !!
 *
 * #define BUFFER_LENGTH 64
 * 
 */
#define leq_single_loop_time 34

#define output_loop_time 10000   // Ausgabe der aktuelle LAeq, LAmin und LAmax Werte alle 10000 ms

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




// GUItool: begin automatically generated code
AudioInputI2S            i2s_in;
//AudioOutputUSB           usb_out; // must set Tools > USB Type to Audio
AudioFilterStateVariable filter_LP;
AudioFilterBiquad        filter_A;
AudioAnalyzeRMS          aa_rms_A;
//AudioAnalyzeFFT1024      fft_A;;
AudioConnection          patchCord1(i2s_in, 0, filter_LP, 0);
AudioConnection          patchCord2(filter_LP, 0, filter_A, 0); 
AudioConnection          patchCord3(filter_A, aa_rms_A);
//AudioConnection          patchCord4(filter_A, fft_A);
//AudioConnection          patchCord5(filter_A, 0, usb_out, 0);
//AudioConnection          patchCord6(filter_A, 0, usb_out, 1);

// GUItool: end automatically generated code


int led = LED_BUILTIN;
bool led_on;
uint16_t led_count = 0;

uint8_t i2c_state = 0;
uint8_t buf[64];
uint16_t data_ready = 0;
bool calculate_leq = false;
uint16_t command_received;

elapsedMillis leq_loop;
elapsedMillis measure_loop;
elapsedMillis fft_millis;
elapsedMillis output_loop;

double rms_A;
double rms_mean_A = 0;
uint32_t mean_count_A = 0;
uint32_t measure_loop_count = 0;

boolean first_loop = true;

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

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  led_on = false;  

  first_loop = 1;
  delay(1000);
  AudioMemory(64);

  filter_LP.frequency(15000);              // State Variable LP Filter
  filter_LP.resonance(0.7);

  filter_A.setHighpass(0, 280, 0.3655);      // A-Filter HP  
  filter_A.setLowpass(1, 10000, 0.55);       // A-Filter LP  
  filter_A.setLowpass(2, 12500, 0.55);
  filter_A.setLowpass(3, 14000, 0.7071);     

  leq_loop = 0;
  output_loop = 0;

  Serial.println(" "); Serial.println("test Teensy 4.0 microphone connection and LAeq calculation");
}


void loop() {
  
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'o') c = 'o';
    if (c == 's') c = 's';
  }

  if (first_loop) {
    first_loop = false;
    while (!aa_rms_A.available())
      rms_A = aa_rms_A.read();
  }
 
  if (aa_rms_A.available()) {
    rms_A = aa_rms_A.read(); // * konstante;
    rms_mean_A += rms_A * rms_A;
    ++mean_count_A;
  }


  if (leq_loop > leq_single_loop_time) {
    leq_loop = 0;

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
//    leq_A = 120 + 20 * log10(z) + 5.1;
    leq_A = 120 + 20 * log10(z) + 5.0;     

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

    if (output_loop > output_loop_time) {
      output_loop = 0;
      calculate_leq = true;
    }

    if (calculate_leq) {
      calculate_leq = false;
      measure_loop_count = 0;
      last_leq_g_A = leq_g_A;
      last_leq_A_min = leq_A_min;
      last_leq_A_max = leq_A_max;
      data_ready = 1;

      Serial.println("-------------");
      Serial.print("LAeq    "); Serial.println(last_leq_g_A);
      Serial.print("LAmin   "); Serial.println(last_leq_A_min);
      Serial.print("LAmax   "); Serial.println(last_leq_A_max);

      sum_leq_g_A = 0;
      leq_A_min = 0;
      leq_A_max = 0;
    }
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
