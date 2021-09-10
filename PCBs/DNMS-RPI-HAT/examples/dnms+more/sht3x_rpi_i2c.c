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

#include "./sht3x_rpi_i2c.h"

int file_i2c_sht3x;
int16_t number_sht3x;
unsigned char i2c_buffer_sht3x[64] = {0};
char *dev_name_sht3x = (char*)"/dev/i2c-1";


int16_t open_i2c_sht3x() {
    if ((file_i2c_sht3x = open(dev_name_sht3x, O_RDWR)) < 0) {
		//ERROR HANDLING: you can check errno to see what went wrong
        printf("Fehler bei Open i2c bus\n");
        printf("\n");
		return -1;
    }
	if (ioctl(file_i2c_sht3x, I2C_SLAVE, SHT3x_I2C_ADDRESS) < 0) {
        printf("Fehler bei ioctl SHT3x nicht gefunden\n");
        printf("\n");
        return -2;    
    }
    return 0;   
}


int16_t sht3x_reset() {
  int16_t ret;

  ret = sht3x_i2c_write_cmd(SHT3x_I2C_ADDRESS, SHT3x_CMD_RESET);
  sleep(1);  // wait 1 sec
  return ret;
}



int16_t sht3x_read_serial_number(uint32_t *serial) {
	uint16_t buf[2];
    int16_t ret;

    ret = sht3x_i2c_read_cmd(SHT3x_I2C_ADDRESS, SHT3x_CMD_READ_SERIAL_NUMBER, (uint16_t *)buf, 2);
    *serial = (buf[0] << 16) | buf[1];
    usleep(1000);  // wait 1 ms
    return ret;

}


int16_t sht3x_read_status(uint16_t *status) {
  int16_t ret;

  ret = sht3x_i2c_read_cmd(SHT3x_I2C_ADDRESS, SHT3x_CMD_READSTATUS, (uint16_t *)status, 1);
  usleep(1000);  // wait 1 ms
  return ret;
}


int16_t sht3x_clear_status() {
  int16_t ret;

  ret = sht3x_i2c_write_cmd(SHT3x_I2C_ADDRESS, SHT3x_CMD_CLEARSTATUS);
  usleep(1000);  // wait 1 ms
  return ret;
}


int16_t sht3x_read_t_rh(float *t, float *rh) {
    int16_t ret;
	uint16_t buf[2];
 
    ret = sht3x_i2c_read_cmd(SHT3x_I2C_ADDRESS, SHT31_CMD_T_RH_LOWREP_STRETCH, (uint16_t *)buf, 2);
    *t = 175.0f * (float)buf[0] / 65535.0f - 45.0f;
    *rh = 100.0f * buf[1] / 65535.0f;
    
    return ret;
}


int16_t sht3x_i2c_read_cmd(uint8_t address, uint16_t cmd, uint16_t *data_words, uint16_t num_words) {
  int16_t ret;
  uint8_t buf[SHT3x_COMMAND_SIZE];

  sht3x_fill_cmd_send_buf(buf, cmd, NULL, 0);
  ret = sht3x_i2c_write(address, buf, SHT3x_COMMAND_SIZE);
  if (ret != STATUS_OK) {
    return ret;
  }
  return sht3x_i2c_read_words(address, data_words, num_words);
}


int16_t sht3x_i2c_read_words(uint8_t address, uint16_t *data_words, uint16_t num_words) {
  int16_t ret;
  uint8_t i;

  ret = sht3x_i2c_read_bytes(address, (uint8_t *)data_words, num_words);
  if (ret != STATUS_OK) {
    return ret;
  }
  for (i = 0; i < num_words; ++i) {
    data_words[i] = be16_to_cpu(data_words[i]);
  }
  return STATUS_OK;
}


int16_t sht3x_i2c_read_bytes(uint8_t address, uint8_t *data, uint16_t num_words) {
  int16_t ret;
  uint16_t i, j;
  uint16_t size = num_words * (SHT3x_WORD_SIZE + CRC8_LEN);
  uint16_t word_buf[SHT3x_MAX_BUFFER_WORDS];
  uint8_t * const buf8 = (uint8_t *)word_buf;
  
  ret = sht3x_i2c_read(address, buf8, size);
  if (ret != STATUS_OK) {
    return ret;
  }
  /* check the CRC for each word */
  for (i = 0, j = 0; i < size; i += SHT3x_WORD_SIZE + CRC8_LEN) {
    ret = sht3x_common_check_crc(&buf8[i], SHT3x_WORD_SIZE, buf8[i + SHT3x_WORD_SIZE]);
    if (ret != STATUS_OK) {
      return ret;
    }
    data[j++] = buf8[i];
    data[j++] = buf8[i + 1];
  }

  return STATUS_OK;
}



int16_t sht3x_i2c_write_cmd(uint8_t address, uint16_t command) {
  uint8_t buf[SHT3x_COMMAND_SIZE];
  int16_t ret;

  sht3x_fill_cmd_send_buf(buf, command, NULL, 0);
  ret = sht3x_i2c_write(address, buf, SHT3x_COMMAND_SIZE);
  usleep(1000);  // wait 1 ms
  return ret;
}


int8_t sht3x_i2c_write(uint8_t address, const uint8_t* data, uint16_t count) {
	if (write(file_i2c_sht3x, data, count) != count) {
        return -1;
    }
    return 0;
} 	    


int8_t sht3x_i2c_read(uint8_t address, uint8_t* data, uint16_t count) {
    if (read(file_i2c_sht3x, data, count) != count) {
        return -1;
    }
    return 0;
}


uint16_t sht3x_fill_cmd_send_buf(uint8_t *buf, uint16_t cmd, const uint16_t *args, uint8_t num_args) {
  uint8_t crc;
  uint8_t i;
  uint16_t idx = 0;

  buf[idx++] = (uint8_t)((cmd & 0xFF00) >> 8);
  buf[idx++] = (uint8_t)((cmd & 0x00FF) >> 0);

  for (i = 0; i < num_args; ++i) {
    buf[idx++] = (uint8_t)((args[i] & 0xFF00) >> 8);
    buf[idx++] = (uint8_t)((args[i] & 0x00FF) >> 0);

    crc = sht3x_common_generate_crc((uint8_t *)&buf[idx - 2], SHT3x_WORD_SIZE);
    buf[idx++] = crc;
  }
  return idx;
}


uint8_t sht3x_common_generate_crc(uint8_t *data, uint16_t count) {
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


int8_t sht3x_common_check_crc(uint8_t *data, uint16_t count, uint8_t checksum) {
  uint8_t crc;
  crc = sht3x_common_generate_crc(data, count);
  if (crc != checksum) {
    return STATUS_FAIL;
   }
  return STATUS_OK;
}


