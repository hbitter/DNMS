/************************************************************************
 *                                                                      *
    DNMS - Digital Noise Measurement Sensor
 *                                                                      *
    This source code is for the board Teensy 3.6
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

#include <math.h>
#include <Audio.h>
#include <i2c_t3.h>
#include <Adafruit_SleepyDog.h>

#define SOFTWARE_VERSION "DNMS Version 1.1.1"

/* 
 ************************************************************************  
 *  
 *  Version history
 *  
 *  Version 1.0.1 - initial version
 *  
 *  Version 1.1.1 
 *   - change of I²C Library, now it is: i2c_t3  https://github.com/nox771/i2c_t3, which is more stable 
 *  
 *  
 *  
 *  
 ************************************************************************  
 */

/*
 * 
 * Attention: in Wire.h set BUFFER_LENGTH to 64 !!
 *
 * #define BUFFER_LENGTH 64
 * 
 */
#define leq_single_loop_time 34

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
AudioOutputUSB           usb_out; // must set Tools > USB Type to Audio
AudioFilterStateVariable filter_LP;
AudioFilterBiquad        filter_A;
AudioAnalyzeRMS          aa_rms_A;
//AudioAnalyzeFFT1024      fft_A;;
AudioConnection          patchCord1(i2s_in, 0, filter_LP, 0);
AudioConnection          patchCord2(filter_LP, 0, filter_A, 0); 
AudioConnection          patchCord3(filter_A, aa_rms_A);
//AudioConnection          patchCord4(filter_A, fft_A);
AudioConnection          patchCord5(filter_A, 0, usb_out, 0);
AudioConnection          patchCord6(filter_A, 0, usb_out, 1);

// GUItool: end automatically generated code


uint8_t i2c_state = 0;
uint8_t buf[64];
uint16_t data_ready = 0;
bool calculate_leq = false;
uint16_t command_received;

elapsedMillis leq_loop;
elapsedMillis measure_loop;
elapsedMillis disp_loop;
elapsedMillis fft_millis;

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
  // Setup for Slave mode, address 0x55, pins 18/19, external pullups, 100kHz
  Wire.begin(I2C_SLAVE, 0x55, I2C_PINS_18_19, I2C_PULLUP_EXT, 100000);
  Wire.onRequest(i2c_request_from_master);
  Wire.onReceive(i2c_receive_from_master);

  Serial.begin(115200);

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

}


void loop() {

  if (first_loop) {
    first_loop = false;
    while (!aa_rms_A.available())
      rms_A = aa_rms_A.read();
  }
 
  if (aa_rms_A.available()) {
    rms_A = aa_rms_A.read(); 
    rms_mean_A += rms_A * rms_A;
    ++mean_count_A;
  }
 

  if (leq_loop > leq_single_loop_time) {
    leq_loop = 0;

    z = sqrt(rms_mean_A / mean_count_A);
    if (z == 0) {
      z = 0.000001;
    }

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


void i2c_request_from_master() {
  uint16_t i;
  uint16_t idx;
  uint8_t crc;
  uint8_t b[4];

  switch (i2c_state) {
    case 2: // Read DNMS Version
//      Serial.println("request from master, Read DNMS Version");
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
//      Serial.println("request from master, Read Data Ready");    
      idx = 0;
      buf[idx++] = uint8_t ((data_ready >> 8) & 0xff);
      buf[idx++] = uint8_t (data_ready & 0xff);
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      Wire.write(buf, 3);
      i2c_state = 0;
      break;

    case 5: // Read Leq
//      Serial.println("request from master, Read Leq");    
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
//      Serial.println("request from master, default");
      break;
  }
}


void i2c_receive_from_master(size_t num_bytes) {

  i2c_state = 0;
  Wire.read(buf, num_bytes);

//  Serial.print("receive from master, num_bytes: "); Serial.println(num_bytes);
  command_received = (buf[0] << 8) | buf[1];

  switch (command_received) {
    case 1: // Reset DNMS
//      Serial.println("receive from master, CMD Reset DNMS");
      Watchdog.enable(10);
      for (;;) {
        // do nothing and wait for the wdt reset
      }
      break;

    case 2: // Read DNMS Version
//      Serial.println("receive from master, CMD Read DNMS Version");    
      i2c_state = 2;

      break;

    case 3: // Calculate Leq
//      Serial.println("receive from master, CMD Calculate DNMS");    
      i2c_state = 3;
      data_ready = 0;
      calculate_leq = true;

      break;

    case 4: // Read Data (Leq) Ready
      i2c_state = 4;
//      Serial.println("receive from master, CMD Read Data Ready");
      break;

    case 5: // Read Leq
      i2c_state = 5;
//      Serial.println("receive from master, CMD Read Leq");
      break;

    default: // sonst
//      Serial.println("receive from master, CMD default");
      break;
  }
}


void f2b(float val, uint8_t* bytes_array) {
  // Create union of shared memory space
  union {
    float float_variable;      
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
