/************************************************************************
 *                                                                      *
 *  Driver for Sensirion SHT3x - Temperature and Humidity Sensor        *
 *                                                                      *
 *  This source code is for the raspberry pi                            *
 *                                                                      *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 *    SHT3x driver                                                      *
 *    Copyright (C) 2021  Helmut Bitter                                 *
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

#define SHT3x_I2C_ADDRESS                0x44
#define SHT3x_MAX_SERIAL_LEN             32
#define SHT3x_MAX_PRODUCT_TYPE_LEN       8
#define SHT3x_WORD_SIZE                  2
#define SHT3x_NUM_WORDS(x)               (sizeof(x) / SHT3x_WORD_SIZE)
#define STATUS_OK                        0
#define STATUS_FAIL                      (-1)
#define SHT3x_COMMAND_SIZE               2
#define CRC8_POLYNOMIAL                  0x31
#define CRC8_INIT                        0xFF
#define CRC8_LEN                         1
#define SHT3x_MAX_BUFFER_WORDS           32
#define SHT3x_IS_ERR_STATE(err_code)     (((err_code) | 0xff) == 0x1ff)


#define SHT3x_CMD_RESET                  0x30A2  // Soft Reset
#define SHT3x_CMD_READSTATUS             0xF32D  // Read Out of Status Register
#define SHT3x_CMD_CLEARSTATUS            0x3041  // Clear Status 
#define SHT3x_CMD_HEATEREN               0x306D  // Heater Enable
#define SHT3x_CMD_HEATERDIS              0x3066  // Heater Disable
#define SHT3x_CMD_READ_SERIAL_NUMBER     0x3780
#define SHT31_CMD_T_RH_LOWREP_STRETCH    0x2C10  // Measurement Low Repeatability with Clock Stretch Enabled

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


/**
   open_i2c() - open i2c bus on Raspberry Pi
   
   Return:  0 on success, an error code otherwise
*/

int16_t open_i2c_sht3x();

int16_t sht3x_reset();

int16_t sht3x_read_serial_number(uint32_t *serial);

int16_t sht3x_read_status(uint16_t *status);

int16_t sht3x_clear_status();

int16_t sht3x_read_t_rh(float *t, float *rh);


int16_t sht3x_i2c_read_cmd(uint8_t address, uint16_t cmd, uint16_t *data_words, uint16_t num_words);

int16_t sht3x_i2c_read_words(uint8_t address, uint16_t *data_words, uint16_t num_words);

int16_t sht3x_i2c_read_bytes(uint8_t address, uint8_t *data, uint16_t num_words);

int8_t sht3x_i2c_read(uint8_t address, uint8_t* data, uint16_t count);

int16_t sht3x_i2c_write_cmd(uint8_t address, uint16_t command);

int8_t sht3x_i2c_write(uint8_t address, const uint8_t* data, uint16_t count);

uint8_t sht3x_common_generate_crc(uint8_t *data, uint16_t count);

int8_t sht3x_common_check_crc(uint8_t *data, uint16_t count, uint8_t checksum);

uint16_t sht3x_fill_cmd_send_buf(uint8_t *buf, uint16_t cmd, const uint16_t *args, uint8_t num_args);



