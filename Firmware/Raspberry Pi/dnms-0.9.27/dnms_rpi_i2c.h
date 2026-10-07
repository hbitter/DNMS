#pragma once
/************************************************************************
 *                                                                      *
 *  Driver for DNMS - Digital Noise Measurement Sensor                  *
 *                                                                      *
 *  This source code is for the raspberry pi        *
 *                                                                      *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 *    DNMS - Digital Noise Measurement Sensor                           *
 *    Copyright (C) 2022, 2023, 2024  Helmut Bitter                                 *
 *                                                                      *
 * This program is free software: you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program. If not, see <http://www.gnu.org/licenses/>. *
 *                                                                      *
 ************************************************************************
*/

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>         	// needed for I2C port
#include <fcntl.h>          	// needed for I2C port
#include <sys/ioctl.h>      	// needed for I2C port
#include <linux/i2c-dev.h>  	// needed for I2C port


#define DNMS_I2C_ADDRESS 0x55
#define DNMS_MAX_VERSION_LEN 18
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
#define DNMS_CMD_READ_FFT_PART1 0x0006
#define DNMS_CMD_READ_FFT_PART2 0x0007
#define DNMS_CMD_READ_FFT_PART3 0x0008
#define DNMS_CMD_READ_FFT_PART4 0x0009
#define DNMS_CMD_CALCULATE_LEQ_2nd 0x000a
#define DNMS_CMD_READ_DATA_READY_2nd 0x000b
#define DNMS_CMD_READ_LEQ_2nd 0x000c
#define DNMS_CMD_READ_FFT_PART1_2nd 0x000d
#define DNMS_CMD_READ_FFT_PART2_2nd 0x000e
#define DNMS_CMD_READ_FFT_PART3_2nd 0x000f
#define DNMS_CMD_READ_FFT_PART4_2nd 0x0010
#define DNMS_CMD_READ_LEQ_Z 0x0011
#define DNMS_CMD_READ_FFT_Z_PART1 0x0012
#define DNMS_CMD_READ_FFT_Z_PART2 0x0013
#define DNMS_CMD_READ_FFT_Z_PART3 0x0014
#define DNMS_CMD_READ_FFT_Z_PART4 0x0015
#define DNMS_CMD_READ_LEQ_Z_2nd 0x0016
#define DNMS_CMD_READ_FFT_Z_PART1_2nd 0x0017
#define DNMS_CMD_READ_FFT_Z_PART2_2nd 0x0018
#define DNMS_CMD_READ_FFT_Z_PART3_2nd 0x0019
#define DNMS_CMD_READ_FFT_Z_PART4_2nd 0x001A
#define DNMS_CMD_SET_ICS43434 0x001B
#define DNMS_CMD_SET_IM72D128 0x001C
#define DNMS_CMD_SET_IM72D128_DLR_case 0x001D
#define DNMS_CMD_SET_ICS43434_no_correction 0x001E
#define DNMS_CMD_SET_IM72D128_no_correction 0x001F
#define DNMS_CMD_READ_LEQ_C 0x0020
#define DNMS_CMD_READ_FFT_C_PART1 0x0021
#define DNMS_CMD_READ_FFT_C_PART2 0x0022
#define DNMS_CMD_READ_FFT_C_PART3 0x0023
#define DNMS_CMD_READ_FFT_C_PART4 0x0024
#define DNMS_CMD_READ_LEQ_C_2nd 0x0025
#define DNMS_CMD_READ_FFT_C_PART1_2nd 0x0026
#define DNMS_CMD_READ_FFT_C_PART2_2nd 0x0027
#define DNMS_CMD_READ_FFT_C_PART3_2nd 0x0028
#define DNMS_CMD_READ_FFT_C_PART4_2nd 0x0029
#define DNMS_CMD_WRITE_CALIB_POINT    0x002A  /* Pi→Teensy: write one calibration point */
#define DNMS_CMD_APPLY_CALIB          0x002B  /* Pi→Teensy: commit table, trigger compute */

#define I2C_CLOCK_SET 0x0702

#define be16_to_cpu(s) (((uint16_t)(s) << 8) | (0xff & ((uint16_t)(s)) >> 8))
#define be32_to_cpu(s) (((uint32_t)be16_to_cpu(s) << 16) | (0xffff & (be16_to_cpu((s) >> 16))))
#define be64_to_cpu(s) (((uint64_t)be32_to_cpu(s) << 32) | (0xffffffff & ((uint64_t)be32_to_cpu((s) >> 32))))
/**
   Convert a word-array to a bytes-array, effectively reverting the
   host-endianness to big-endian
   @a:  word array to change (must be (uint16_t *) castable)
   @w:  number of word-sized elements in the array (DNMS_NUM_WORDS(a)).
*/
#define DNMS_WORDS_TO_BYTES(a, w) \
  for (uint16_t *__a = (uint16_t *)(a), __e = (w), __w = 0; __w < __e; ++__w) { \
    __a[__w] = be16_to_cpu(__a[__w]); \
  }


struct dnms_measurements {
  float leq_x;
  float leq_x_min;
  float leq_x_max;
  float leq_freq_spec_x[31];
};

/* One frequency-response calibration point (1/3-octave centre frequency + dB correction). */
struct dnms_calib_point {
  float freq_hz;
  float db_correction;
};


/**
   open_i2c(_dnms (char * dev_name) - open i2c bus on Raspberry Pi
   
   Return:  0 on success, an error code otherwise
*/
int16_t open_i2c_dnms(char *dev_name);


/**
   close_i2c () - open i2c bus on Raspberry Pi
   
   Return:  0 on success, an error code otherwise
*/
int16_t close_i2c_dnms();

/**
   dnms_reset() - reset the dnms system
   
   Return:  0 on success, an error code otherwise
*/
int16_t dnms_reset();

/**
   dnms_set_ICS43434() - set microphone ICS-43434 for dnms system
   
   Return:  0 on success, an error code otherwise
*/
int16_t dnms_set_ICS43434();

/**
   dnms_set_IM72D128() - set microphone IM72D128 for dnms system
   
   Return:  0 on success, an error code otherwise
*/
int16_t dnms_set_IM72D128();

/**
   dnms_set_IM72D128_DLR_case() - set microphone IM72D128 with DLR case for dnms system
   
   Return:  0 on success, an error code otherwise
*/
int16_t dnms_set_IM72D128_DLR_case();

/**
   dnms_set_ICS43434_no_correction() - set microphone ICS-43434 no frequency correction for dnms system
   
   Return:  0 on success, an error code otherwise
*/
int16_t dnms_set_ICS43434_no_correction();

/**
   dnms_set_IM72D128_no_correction() - set microphone IM72D128 no frequency correction for dnms system
   
   Return:  0 on success, an error code otherwise
*/
int16_t dnms_set_IM72D128_no_correction();

/**
   dnms_read_version() - retrieve the version of the dnms

   Note that version must be discarded when the return code is non-zero.

   @dnms_version: Memory where the serial number is written into as hex string (zero(
            terminated). Must be at least DNMS_MAX_VERSION_LEN long.
   Return:  0 on success, an error code otherwise
*/
int16_t dnms_read_version(char *dnms_version);


int16_t dnms_calculate_leq();

int16_t dnms_calculate_leq_2nd();

int16_t dnms_read_data_ready(uint16_t *data_ready);

int16_t dnms_read_data_ready_2nd(uint16_t *data_ready);

int16_t dnms_read_leq(uint32_t dnms_command, struct dnms_measurements *leq);

int16_t dnms_read_freq_spec(uint32_t dnms_command, struct dnms_measurements *leq);

int16_t dnms_i2c_read_cmd(uint16_t cmd, uint16_t *data_words, uint16_t num_words);

int8_t dnms_i2c_read(uint8_t *data, uint16_t count);

int8_t dnms_i2c_write(const uint8_t *data, uint16_t count);


uint8_t dnms_common_generate_crc(uint8_t *data, uint16_t count);

int8_t dnms_common_check_crc(uint8_t *data, uint16_t count,
                             uint8_t checksum);

uint16_t dnms_fill_cmd_send_buf(uint8_t *buf, uint16_t cmd,
                                const uint16_t *args, uint8_t num_args);

int16_t dnms_i2c_read_bytes(uint8_t *data,
                            uint16_t num_words);

int16_t dnms_i2c_read_words(uint16_t *data_words,
                            uint16_t num_words);

int16_t dnms_i2c_write_cmd(uint16_t command);

int16_t dnms_i2c_write_cmd_with_args(uint16_t command,
                                     const uint16_t *data_words,
                                     uint16_t num_words);

/**
   dnms_write_calib_point() - transmit one calibration point (Pi → Teensy).
   @index:         position in the Teensy's receive buffer (0 … 30)
   @freq_hz:       1/3-octave centre frequency in Hz
   @db_correction: dB correction value (positive = DUT reads low)
   Return: 0 on success, error code otherwise.
*/
int16_t dnms_write_calib_point(uint16_t index, float freq_hz, float db_correction);

/**
   dnms_apply_calib() - finalise a calibration upload.
   Causes the Teensy to call compute_corrections() and update all parameters.
   @count:    number of valid calibration points in the receive buffer
   @mic_type: 1 = ICS-43434 family, 2 = IM72D128 family
   @blink_ms: LED blink period in ms (100 for ICS-43434, 500 for IM72D128)
   @mic_const: microphone sensitivity constant (dB)
   Return: 0 on success, error code otherwise.
*/
int16_t dnms_apply_calib(uint8_t count, uint8_t mic_type,
                         uint32_t blink_ms, float mic_const);

/**
   dnms_send_calibration() - transmit a full calibration table and apply it.
   Sends all @n points via dnms_write_calib_point(), then calls dnms_apply_calib().
   @pts:      pointer to array of dnms_calib_point
   @n:        number of points (max 31)
   @mic_type: 1 = ICS-43434 family, 2 = IM72D128 family
   @blink_ms: LED blink period in ms
   @mic_const: microphone sensitivity constant (dB)
   Return: 0 on success, first error code encountered otherwise.
*/
int16_t dnms_send_calibration(const struct dnms_calib_point *pts, uint8_t n,
                              uint8_t mic_type, uint32_t blink_ms,
                              float mic_const);
