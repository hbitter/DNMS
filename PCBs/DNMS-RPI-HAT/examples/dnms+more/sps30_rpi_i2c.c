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

#include "./sps30_rpi_i2c.h"

int file_i2c_sps30;
int16_t number_sps30;
unsigned char i2c_buffer_sps30[64] = {0};
char *dev_name_sps30 = (char*)"/dev/i2c-1";




int16_t open_i2c_sps30() {
    if ((file_i2c_sps30 = open(dev_name_sps30, O_RDWR)) < 0) {
		//ERROR HANDLING: you can check errno to see what went wrong
        printf("Fehler bei Open i2c bus\n");
        printf("\n");
		return -1;
    }
	if (ioctl(file_i2c_sps30, I2C_SLAVE, SPS30_I2C_ADDRESS) < 0) {
        printf("Fehler bei ioctl SPS30 nicht gefunden\n");
        printf("\n");
        return -2;    
    }
    return 0;   
}


/*
  void sps30_reset() {
  set_pointer(SPS30_CMD_RESET);
  }
*/

int16_t sps30_reset() {
  return sps30_i2c_write_cmd(SPS30_I2C_ADDRESS, SPS30_CMD_RESET);
}



/*
int16_t sps30_probe() {
  char serial[SPS30_MAX_SERIAL_LEN];

  // 

  return sps30_get_serial(serial);
}
*/

int16_t sps30_get_serial(char *sps30_serial) {
  uint16_t i;
  int16_t ret;
  union {
    char sps30_serial[SPS30_MAX_SERIAL_LEN];
    uint16_t __enforce_alignment;
  } buffer;

  ret = sps30_i2c_read_cmd(SPS30_I2C_ADDRESS, SPS30_CMD_GET_SERIAL, (uint16_t *)buffer.sps30_serial, SPS30_NUM_WORDS(buffer.sps30_serial));
  if (ret != STATUS_OK) {
    return ret;
  }
  SPS30_WORDS_TO_BYTES(buffer.sps30_serial, SPS30_NUM_WORDS(buffer.sps30_serial));
  for (i = 0; i < SPS30_MAX_SERIAL_LEN; ++i) {
    sps30_serial[i] = buffer.sps30_serial[i];
    if (sps30_serial[i] == '\0')
      return 0;
  }
  return 0;
}


int16_t sps30_read_product_type(char *sps30_type) {
  uint16_t i;
  int16_t ret;
  union {
    char sps30_type[SPS30_MAX_PRODUCT_TYPE_LEN];
    uint16_t __enforce_alignment;
  } buffer;
    
  ret = sps30_i2c_read_cmd(SPS30_I2C_ADDRESS, SPS30_CMD_READ_PRODUCT_TYPE, (uint16_t *)buffer.sps30_type, SPS30_NUM_WORDS(buffer.sps30_type));
  if (ret != STATUS_OK) {
    return ret;
  }
  SPS30_WORDS_TO_BYTES(buffer.sps30_type, SPS30_NUM_WORDS(buffer.sps30_type));
  for (i = 0; i < SPS30_MAX_PRODUCT_TYPE_LEN; ++i) {
    sps30_type[i] = buffer.sps30_type[i];
    if (sps30_type[i] == '\0')
      return 0;
  }
  return 0;

}


int16_t sps30_read_version(uint8_t *major, uint8_t *minor) {
  uint16_t version;
  int16_t ret;

  ret = sps30_i2c_read_cmd(SPS30_I2C_ADDRESS, SPS30_CMD_READ_VERSION, &version, 1);
  *major = (version & 0xff00) >> 8;
  *minor = (version & 0x00ff);
  return ret;

}



int16_t sps30_start_measurement() {
  const uint16_t arg = SPS30_CMD_START_MEASUREMENT_ARG;

  return sps30_i2c_write_cmd_with_args(SPS30_I2C_ADDRESS, SPS30_CMD_START_MEASUREMENT, &arg, SPS30_NUM_WORDS(arg));
}



int16_t sps30_stop_measurement() {
  return sps30_i2c_write_cmd(SPS30_I2C_ADDRESS, SPS30_CMD_STOP_MEASUREMENT);
}



int16_t sps30_read_data_ready(uint16_t *data_ready) {
  return sps30_i2c_read_cmd(SPS30_I2C_ADDRESS, SPS30_CMD_GET_DATA_READY, data_ready, SPS30_NUM_WORDS(*data_ready));
}


int16_t sps30_read_measurement(struct sps30_measurement *measurement) {
  int16_t ret;
  uint16_t idx;
  union {
    uint16_t uu[2];
    uint32_t u;
    float f;
  } val, data[10];

  ret = sps30_i2c_read_cmd(SPS30_I2C_ADDRESS, SPS30_CMD_READ_MEASUREMENT, data->uu, SPS30_NUM_WORDS(data));
  if (ret != STATUS_OK)
    return ret;

  SPS30_WORDS_TO_BYTES(data->uu, SPS30_NUM_WORDS(data));

  idx = 0;
  val.u = be32_to_cpu(data[idx].u);
  measurement->mc_1p0 = val.f;
  ++idx;
  val.u = be32_to_cpu(data[idx].u);
  measurement->mc_2p5 = val.f;
  ++idx;
  val.u = be32_to_cpu(data[idx].u);
  measurement->mc_4p0 = val.f;
  ++idx;
  val.u = be32_to_cpu(data[idx].u);
  measurement->mc_10p0 = val.f;
  ++idx;
  val.u = be32_to_cpu(data[idx].u);
  measurement->nc_0p5 = val.f;
  ++idx;
  val.u = be32_to_cpu(data[idx].u);
  measurement->nc_1p0 = val.f;
  ++idx;
  val.u = be32_to_cpu(data[idx].u);
  measurement->nc_2p5 = val.f;
  ++idx;
  val.u = be32_to_cpu(data[idx].u);
  measurement->nc_4p0 = val.f;
  ++idx;
  val.u = be32_to_cpu(data[idx].u);
  measurement->nc_10p0 = val.f;
  ++idx;
  val.u = be32_to_cpu(data[idx].u);
  measurement->tps = val.f;
  ++idx;

  return 0;
}


int16_t sps30_get_fan_auto_cleaning_interval(uint32_t *interval_seconds) {
  union {
    uint16_t u[2];
    uint32_t uu;
  } data;
  int16_t ret = sps30_i2c_read_cmd(SPS30_I2C_ADDRESS, SPS30_CMD_AUTOCLEAN_INTERVAL, data.u, SPS30_NUM_WORDS(data.u));
  if (ret != STATUS_OK) {
    return ret;
  }
  SPS30_WORDS_TO_BYTES(data.u, SPS30_NUM_WORDS(data.u));
  *interval_seconds = be32_to_cpu(data.uu);
  return 0;
}


int16_t sps30_set_fan_auto_cleaning_interval(uint32_t interval_seconds) {
  int16_t ret;
  const uint16_t data[] = {(uint16_t)((interval_seconds & 0xFFFF0000) >> 16),
                           (uint16_t)((interval_seconds & 0x0000FFFF) >> 0)};

  ret = sps30_i2c_write_cmd_with_args(SPS30_I2C_ADDRESS,  SPS30_CMD_AUTOCLEAN_INTERVAL, data, SPS30_NUM_WORDS(data));
//  sps30_sleep_usec(SPS30_WRITE_DELAY_US);
  usleep(SPS30_WRITE_DELAY_US);
  return ret;
}


int16_t sps30_read_status_reg(uint32_t *status_reg) {

}


int16_t sps30_clear_status_reg() {

}


int16_t sps30_i2c_read_cmd(uint8_t address, uint16_t cmd, uint16_t *data_words, uint16_t num_words) {
  int16_t ret;
  uint8_t buf[SPS30_COMMAND_SIZE];

  sps30_fill_cmd_send_buf(buf, cmd, NULL, 0);
  ret = sps30_i2c_write(address, buf, SPS30_COMMAND_SIZE);
  if (ret != STATUS_OK) {
    printf("\nAusgabe aus sps30_i2c_read_cmd ret: %3d\n", ret);
    return ret;
  }
  return sps30_i2c_read_words(address, data_words, num_words);
}


int8_t sps30_i2c_read(uint8_t address, uint8_t* data, uint16_t count) {
    if (read(file_i2c_sps30, data, count) != count) {
        return -1;
    }
    return 0;
}


int8_t sps30_i2c_write(uint8_t address, const uint8_t* data, uint16_t count) {
	if (write(file_i2c_sps30, data, count) != count) {
        return -1;
    }
    return 0;
} 	    


uint8_t sps30_common_generate_crc(uint8_t *data, uint16_t count) {
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


int8_t sps30_common_check_crc(uint8_t *data, uint16_t count, uint8_t checksum) {
  uint8_t crc;
  crc = sps30_common_generate_crc(data, count);
  if (crc != checksum) {
    return STATUS_FAIL;
   }
  return STATUS_OK;
}


uint16_t sps30_fill_cmd_send_buf(uint8_t *buf, uint16_t cmd, const uint16_t *args, uint8_t num_args) {
  uint8_t crc;
  uint8_t i;
  uint16_t idx = 0;

  buf[idx++] = (uint8_t)((cmd & 0xFF00) >> 8);
  buf[idx++] = (uint8_t)((cmd & 0x00FF) >> 0);

  for (i = 0; i < num_args; ++i) {
    buf[idx++] = (uint8_t)((args[i] & 0xFF00) >> 8);
    buf[idx++] = (uint8_t)((args[i] & 0x00FF) >> 0);

    crc = sps30_common_generate_crc((uint8_t *)&buf[idx - 2], SPS30_WORD_SIZE);
    buf[idx++] = crc;
  }
  return idx;
}


int16_t sps30_i2c_read_bytes(uint8_t address, uint8_t *data, uint16_t num_words) {
  int16_t ret;
  uint16_t i, j;
  uint16_t size = num_words * (SPS30_WORD_SIZE + CRC8_LEN);
  uint16_t word_buf[SPS30_MAX_BUFFER_WORDS];
  uint8_t * const buf8 = (uint8_t *)word_buf;
  
  ret = sps30_i2c_read(address, buf8, size);
  if (ret != STATUS_OK) {
    return ret;
  }
  /* check the CRC for each word */
  for (i = 0, j = 0; i < size; i += SPS30_WORD_SIZE + CRC8_LEN) {
    ret = sps30_common_check_crc(&buf8[i], SPS30_WORD_SIZE, buf8[i + SPS30_WORD_SIZE]);
    if (ret != STATUS_OK) {
      return ret;
    }
    data[j++] = buf8[i];
    data[j++] = buf8[i + 1];
  }

  return STATUS_OK;
}


int16_t sps30_i2c_read_words(uint8_t address, uint16_t *data_words, uint16_t num_words) {
  int16_t ret;
  uint8_t i;

  ret = sps30_i2c_read_bytes(address, (uint8_t *)data_words, num_words);
  if (ret != STATUS_OK) {
    return ret;
  }
  for (i = 0; i < num_words; ++i) {
    data_words[i] = be16_to_cpu(data_words[i]);
  }
  return STATUS_OK;
}


int16_t sps30_i2c_write_cmd(uint8_t address, uint16_t command) {
  uint8_t buf[SPS30_COMMAND_SIZE];

  sps30_fill_cmd_send_buf(buf, command, NULL, 0);
  return sps30_i2c_write(address, buf, SPS30_COMMAND_SIZE);
}


int16_t sps30_i2c_write_cmd_with_args(uint8_t address, uint16_t command, const uint16_t *data_words, uint16_t num_words) {
  uint8_t buf[SPS30_MAX_BUFFER_WORDS];
  uint16_t buf_size;

  buf_size = sps30_fill_cmd_send_buf(buf, command, data_words, num_words);
  return sps30_i2c_write(address, buf, buf_size);
}
