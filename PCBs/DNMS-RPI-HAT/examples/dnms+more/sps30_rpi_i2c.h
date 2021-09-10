/************************************************************************
 *                                                                      *
 *  Driver for SPS30 - PM Sensor from Sensirion                          *
 *                                                                      *
 *  This source code is for the raspberry pi                            *
 *                                                                      *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 *                                                                      *
 *    Copyright (C) 2019  Helmut Bitter                                 *
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
#include <unistd.h>				//Needed for I2C port
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port


#define SPS30_I2C_ADDRESS                0x69
#define SPS30_MAX_SERIAL_LEN             32
#define SPS30_MAX_PRODUCT_TYPE_LEN       8
#define SPS30_WORD_SIZE                  2
#define SPS30_NUM_WORDS(x)               (sizeof(x) / SPS30_WORD_SIZE)
#define STATUS_OK                        0
#define STATUS_FAIL                      (-1)
#define SPS30_COMMAND_SIZE               2
#define CRC8_POLYNOMIAL                  0x31
#define CRC8_INIT                        0xFF
#define CRC8_LEN                         1
#define SPS30_MAX_BUFFER_WORDS           32
#define SPS30_IS_ERR_STATE(err_code)     (((err_code) | 0xff) == 0x1ff)

#define SPS30_CMD_START_MEASUREMENT      0x0010
#define SPS30_CMD_START_MEASUREMENT_ARG  0x0300
#define SPS30_CMD_STOP_MEASUREMENT       0x0104
#define SPS30_CMD_READ_MEASUREMENT       0x0300
#define SPS30_CMD_GET_DATA_READY         0x0202
#define SPS30_CMD_AUTOCLEAN_INTERVAL     0x8004
#define SPS30_CMD_GET_SERIAL             0xD033
#define SPS30_CMD_RESET                  0xD304
#define SPS30_CMD_SLEEP                  0x1001
#define SPS30_CMD_WAKE_UP                0x1103
#define SPS30_CMD_START_FAN_CLEANING     0x5607
#define SPS30_CMD_READ_PRODUCT_TYPE      0xD002
#define SPS30_CMD_READ_VERSION           0xD100
#define SPS30_CMD_READ_DEVICE_STATUS_R   0xD206
#define SPS30_CMD_CLEAR_DEVICE_STATUS_R  0xD210
#define SPS30_WRITE_DELAY_US             20000


#define be16_to_cpu(s) (((uint16_t)(s) << 8) | (0xff & ((uint16_t)(s)) >> 8))
#define be32_to_cpu(s) (((uint32_t)be16_to_cpu(s) << 16) | \
                        (0xffff & (be16_to_cpu((s) >> 16))))
#define be64_to_cpu(s) (((uint64_t)be32_to_cpu(s) << 32) | \
                        (0xffffffff & ((uint64_t)be32_to_cpu((s) >> 32))))
/**
   Convert a word-array to a bytes-array, effectively reverting the
   host-endianness to big-endian
   @a:  word array to change (must be (uint16_t *) castable)
   @w:  number of word-sized elements in the array (SPS30_NUM_WORDS(a)).
*/
#define SPS30_WORDS_TO_BYTES(a, w) \
  for (uint16_t *__a = (uint16_t *)(a), __e = (w), __w = 0; __w < __e; ++__w) { \
    __a[__w] = be16_to_cpu(__a[__w]); \
  }


struct sps30_measurement {
  float mc_1p0;
  float mc_2p5;
  float mc_4p0;
  float mc_10p0;
  float nc_0p5;
  float nc_1p0;
  float nc_2p5;
  float nc_4p0;
  float nc_10p0;
  float tps;
};


/**
   open_i2c() - open i2c bus on Raspberry Pi
   
   Return:  0 on success, an error code otherwise
*/

int16_t open_i2c_sps30();


int16_t sps30_reset();

/**
   sps30_probe() - check if SPS sensor is available and initialize it

   Note that Pin 4 must be pulled to ground for the sensor to operate in i2c
   mode (this driver). When left floating, the sensor operates in UART mode
   which is not compatible with this i2c driver.

   Return:  0 on success, an error code otherwise
*/
/*
int16_t sps30_probe();
*/

/**
   sps30_get_serial() - retrieve the serial number

   Note that serial must be discarded when the return code is non-zero.

   @serial: Memory where the serial number is written into as hex string (zero
            terminated). Must be at least SPS30_MAX_SERIAL_LEN long.
   Return:  0 on success, an error code otherwise
*/
int16_t sps30_get_serial(char *serial);


int16_t sps30_read_product_type(char *type);


int16_t sps30_read_version(uint8_t *major, uint8_t *minor);


int16_t sps30_start_measurement();


int16_t sps30_stop_measurement();


int16_t sps30_get_fan_auto_cleaning_interval(uint32_t *interval_seconds);


int16_t sps30_set_fan_auto_cleaning_interval(uint32_t interval_seconds);


int16_t sps30_read_status_reg(uint32_t *status_reg);


int16_t sps30_clear_status_reg();


int16_t sps30_read_data_ready(uint16_t *data_ready);


int16_t sps30_read_measurement(struct sps30_measurement *measurement);


int16_t sps30_i2c_read_cmd(uint8_t address, uint16_t cmd, uint16_t *data_words, uint16_t num_words);


int8_t sps30_i2c_read(uint8_t address, uint8_t* data, uint16_t count);

int8_t sps30_i2c_write(uint8_t address, const uint8_t* data, uint16_t count);


uint8_t sps30_common_generate_crc(uint8_t *data, uint16_t count);

int8_t sps30_common_check_crc(uint8_t *data, uint16_t count, uint8_t checksum);

uint16_t sps30_fill_cmd_send_buf(uint8_t *buf, uint16_t cmd, const uint16_t *args, uint8_t num_args);

int16_t sps30_i2c_read_bytes(uint8_t address, uint8_t *data, uint16_t num_words);

int16_t sps30_i2c_read_words(uint8_t address, uint16_t *data_words, uint16_t num_words);

int16_t sps30_i2c_write_cmd(uint8_t address, uint16_t command);

int16_t sps30_i2c_write_cmd_with_args(uint8_t address, uint16_t command, const uint16_t *data_words, uint16_t num_words);
