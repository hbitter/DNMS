/************************************************************************
 *                                                                      *
 *  Driver for SCD30 - CO2 Sensor from Sensirion                        *
 *                                                                      *
 *  This source code is for the raspberry pi                            *
 *                                                                      *
 *  based on Sensirion Software                                         *
 *                                                                      *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *                                                                      *
 ************************************************************************
*/

#include "./scd30_rpi_i2c.h"

int file_i2c_scd30;
int16_t number_scd30;
unsigned char i2c_buffer_scd30[64] = {0};
char *dev_name_scd30 = (char*)"/dev/i2c-1";


int16_t open_i2c_scd30() {
    if ((file_i2c_scd30 = open(dev_name_scd30, O_RDWR)) < 0) {
		//ERROR HANDLING: you can check errno to see what went wrong
        printf("Fehler bei Open i2c bus\n");
        printf("\n");
		return -1;
    }
	if (ioctl(file_i2c_scd30, I2C_SLAVE, SCD30_I2C_ADDRESS) < 0) {
        printf("Fehler bei ioctl SCD30 nicht gefunden\n");
        printf("\n");
        return -2;    
    }
    return 0;   
}


int16_t scd30_probe(void) {
    uint16_t data_ready;

    /* try to read data-ready state */
    return scd30_get_data_ready(&data_ready);
}


int16_t scd30_get_data_ready(uint16_t* data_ready) {
    return scd30_i2c_delayed_read_cmd(
        SCD30_I2C_ADDRESS, SCD30_CMD_GET_DATA_READY, 3000, data_ready,
        SCD30_NUM_WORDS(*data_ready));
}


int16_t scd30_i2c_delayed_read_cmd(uint8_t address, uint16_t cmd,
                                       uint32_t delay_us, uint16_t* data_words,
                                       uint16_t num_words) {
    int16_t ret;
    uint8_t buf[SCD30_COMMAND_SIZE];

    scd30_fill_cmd_send_buf(buf, cmd, NULL, 0);
    ret = scd30_i2c_write(address, buf, SCD30_COMMAND_SIZE);
    if (ret != STATUS_OK)
        return ret;

    if (delay_us)
        usleep(delay_us);

    return scd30_i2c_read_words(address, data_words, num_words);
}


uint16_t scd30_fill_cmd_send_buf(uint8_t* buf, uint16_t cmd, const uint16_t* args, uint8_t num_args) {
  uint8_t crc;
  uint8_t i;
  uint16_t idx = 0;

  buf[idx++] = (uint8_t)((cmd & 0xFF00) >> 8);
  buf[idx++] = (uint8_t)((cmd & 0x00FF) >> 0);

  for (i = 0; i < num_args; ++i) {
    buf[idx++] = (uint8_t)((args[i] & 0xFF00) >> 8);
    buf[idx++] = (uint8_t)((args[i] & 0x00FF) >> 0);

    crc = scd30_common_generate_crc((uint8_t *)&buf[idx - 2], SCD30_WORD_SIZE);
    buf[idx++] = crc;
  }
  return idx;
}


uint8_t scd30_common_generate_crc(uint8_t *data, uint16_t count) {
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


int8_t scd30_common_check_crc(uint8_t *data, uint16_t count, uint8_t checksum) {
  uint8_t crc;
  crc = scd30_common_generate_crc(data, count);
  if (crc != checksum) {
    return STATUS_FAIL;
   }
  return STATUS_OK;
}



int16_t scd30_i2c_read_words(uint8_t address, uint16_t *data_words, uint16_t num_words) {
  int16_t ret;
  uint8_t i;

  ret = scd30_i2c_read_bytes(address, (uint8_t *)data_words, num_words);
  if (ret != STATUS_OK) {
    return ret;
  }
  for (i = 0; i < num_words; ++i) {
    data_words[i] = be16_to_cpu(data_words[i]);
  }
  return STATUS_OK;
}


int8_t scd30_i2c_write(uint8_t address, const uint8_t* data, uint16_t count) {
	if (write(file_i2c_scd30, data, count) != count) {
        return -1;
    }
    return 0;
} 	    


int16_t scd30_i2c_read_bytes(uint8_t address, uint8_t *data, uint16_t num_words) {
  int16_t ret;
  uint16_t i, j;
  uint16_t size = num_words * (SCD30_WORD_SIZE + CRC8_LEN);
  uint16_t word_buf[SCD30_MAX_BUFFER_WORDS];
  uint8_t * const buf8 = (uint8_t *)word_buf;
  
  ret = scd30_i2c_read(address, buf8, size);
  if (ret != STATUS_OK) {
    return ret;
  }
  /* check the CRC for each word */
  for (i = 0, j = 0; i < size; i += SCD30_WORD_SIZE + CRC8_LEN) {
    ret = scd30_common_check_crc(&buf8[i], SCD30_WORD_SIZE, buf8[i + SCD30_WORD_SIZE]);
    if (ret != STATUS_OK) {
      return ret;
    }
    data[j++] = buf8[i];
    data[j++] = buf8[i + 1];
  }

  return STATUS_OK;
}


int8_t scd30_i2c_read(uint8_t address, uint8_t* data, uint16_t count) {
    if (read(file_i2c_scd30, data, count) != count) {
        return -1;
    }
    return 0;
}


int16_t scd30_read_serial_number(char* serial) {
    int16_t error;

    error = scd30_i2c_write_cmd(SCD30_I2C_ADDRESS, SCD30_CMD_READ_SERIAL);
    if (error)
        return error;

    usleep(SCD30_WRITE_DELAY_US);
    error = scd30_i2c_read_words_as_bytes(
        SCD30_I2C_ADDRESS, (uint8_t*)serial, SCD30_SERIAL_NUM_WORDS);
    serial[2 * SCD30_SERIAL_NUM_WORDS] = '\0';
    return error;
}


int16_t scd30_i2c_write_cmd(uint8_t address, uint16_t command) {
  uint8_t buf[SCD30_COMMAND_SIZE];

  scd30_fill_cmd_send_buf(buf, command, NULL, 0);
  return scd30_i2c_write(address, buf, SCD30_COMMAND_SIZE);
}



int16_t scd30_i2c_read_words_as_bytes(uint8_t address, uint8_t* data,
                                          uint16_t num_words) {
    int16_t ret;
    uint16_t i, j;
    uint16_t size = num_words * (SCD30_WORD_SIZE + CRC8_LEN);
    uint16_t word_buf[SCD30_MAX_BUFFER_WORDS];
    uint8_t* const buf8 = (uint8_t*)word_buf;

    ret = scd30_i2c_read(address, buf8, size);
    if (ret != STATUS_OK)
        return ret;

    /* check the CRC for each word */
    for (i = 0, j = 0; i < size; i += SCD30_WORD_SIZE + CRC8_LEN) {

        ret = scd30_common_check_crc(&buf8[i], SCD30_WORD_SIZE,
                                         buf8[i + SCD30_WORD_SIZE]);
        if (ret != STATUS_OK)
            return ret;

        data[j++] = buf8[i];
        data[j++] = buf8[i + 1];
    }

    return STATUS_OK;
}



int16_t scd30_reset() {
  return scd30_i2c_write_cmd(SCD30_I2C_ADDRESS, SCD30_CMD_SOFT_RESET);
}


int16_t scd30_read_version(uint8_t *major, uint8_t *minor) {
  uint16_t version;
  int16_t ret;

  ret = scd30_i2c_read_cmd(SCD30_I2C_ADDRESS, SCD30_CMD_READ_FIRMWARE_VERSION, &version, 1);
  *major = (version & 0xff00) >> 8;
  *minor = (version & 0x00ff);
  return ret;

}


int16_t scd30_read_measurement(float* co2_ppm, float* temperature,
                               float* humidity) {
    int16_t error;
    uint8_t data[3][4];

    error =
        scd30_i2c_write_cmd(SCD30_I2C_ADDRESS, SCD30_CMD_READ_MEASUREMENT);
    if (error != STATUS_OK)
        return error;

    error = scd30_i2c_read_words_as_bytes(SCD30_I2C_ADDRESS, &data[0][0],
                                              SCD30_NUM_WORDS(data));
    if (error != STATUS_OK)
        return error;

    *co2_ppm = scd30_bytes_to_float(data[0]);
    *temperature = scd30_bytes_to_float(data[1]);
    *humidity = scd30_bytes_to_float(data[2]);

    return STATUS_OK;
}



int16_t scd30_i2c_read_cmd(uint8_t address, uint16_t cmd,
                               uint16_t* data_words, uint16_t num_words) {
    return scd30_i2c_delayed_read_cmd(address, cmd, 0, data_words,
                                          num_words);
}




float scd30_bytes_to_float(const uint8_t* bytes) {
    union {
        uint32_t u32_value;
        float float32;
    } tmp;

    tmp.u32_value = scd30_bytes_to_uint32_t(bytes);
    return tmp.float32;
}


uint32_t scd30_bytes_to_uint32_t(const uint8_t* bytes) {
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
}




int16_t scd30_get_measurement_interval(uint16_t *interval_sec) {
    return scd30_i2c_delayed_read_cmd(SCD30_I2C_ADDRESS, SCD30_CMD_SET_MEASUREMENT_INTERVAL,
        3000, interval_sec, SCD30_NUM_WORDS(*interval_sec));
}



int16_t scd30_set_measurement_interval(uint16_t interval_sec) {
    int16_t ret;

    if (interval_sec < 2 || interval_sec > 1800) {
        /* out of allowable range */
        return STATUS_FAIL;
    }

    ret = scd30_i2c_write_cmd_with_args(
        SCD30_I2C_ADDRESS, SCD30_CMD_SET_MEASUREMENT_INTERVAL, &interval_sec,
        SCD30_NUM_WORDS(interval_sec));
    usleep(SCD30_WRITE_DELAY_US);

    return ret;
}


int16_t scd30_i2c_write_cmd_with_args(uint8_t address, uint16_t command,
                                          const uint16_t* data_words,
                                          uint16_t num_words) {
    uint8_t buf[SCD30_MAX_BUFFER_WORDS];
    uint16_t buf_size;

    buf_size = scd30_fill_cmd_send_buf(buf, command, data_words, num_words);
    return scd30_i2c_write(address, buf, buf_size);
}



int16_t scd30_set_temperature_offset(uint16_t temperature_offset) {
    int16_t ret;

    ret = scd30_i2c_write_cmd_with_args(
        SCD30_I2C_ADDRESS, SCD30_CMD_SET_TEMPERATURE_OFFSET,
        &temperature_offset, SCD30_NUM_WORDS(temperature_offset));
    usleep(SCD30_WRITE_DELAY_US);

    return ret;
}


int16_t scd30_get_temperature_offset(uint16_t *temperature_offset) {

    return scd30_i2c_delayed_read_cmd(SCD30_I2C_ADDRESS, SCD30_CMD_SET_TEMPERATURE_OFFSET,
        3000, temperature_offset, SCD30_NUM_WORDS(*temperature_offset));

}


int16_t scd30_set_altitude(uint16_t altitude) {
    int16_t error;

    error = scd30_i2c_write_cmd_with_args(SCD30_I2C_ADDRESS,
                                              SCD30_CMD_SET_ALTITUDE, &altitude,
                                              SCD30_NUM_WORDS(altitude));
    usleep(SCD30_WRITE_DELAY_US);

    return error;
}



int16_t scd30_get_altitude(uint16_t *altitude) {

    return scd30_i2c_delayed_read_cmd(SCD30_I2C_ADDRESS, SCD30_CMD_SET_ALTITUDE,
        3000, altitude, SCD30_NUM_WORDS(*altitude));

}



int16_t scd30_get_ambient_pressure(uint16_t *pressure) {

    return scd30_i2c_delayed_read_cmd(SCD30_I2C_ADDRESS, SCD30_CMD_START_PERIODIC_MEASUREMENT,
        3000, pressure, SCD30_NUM_WORDS(*pressure));

}



int16_t scd30_start_periodic_measurement(uint16_t pressure) {

    if (pressure && (pressure < 700 || pressure > 1400)) {
        /* out of allowable range */
        return STATUS_FAIL;
    }

    return scd30_i2c_write_cmd_with_args(
        SCD30_I2C_ADDRESS, SCD30_CMD_START_PERIODIC_MEASUREMENT,
        &pressure, SCD30_NUM_WORDS(pressure));

}



int16_t scd30_stop_periodic_measurement(void) {
    
    return scd30_i2c_write_cmd(SCD30_I2C_ADDRESS, SCD30_CMD_STOP_PERIODIC_MEASUREMENT);
}


int16_t scd30_get_automatic_self_calibration(uint8_t* asc_enabled) {
    uint16_t word;
    int16_t ret;

    ret = scd30_i2c_read_cmd(SCD30_I2C_ADDRESS,
                                   SCD30_CMD_AUTO_SELF_CALIBRATION, &word,
                                   SCD30_NUM_WORDS(word));
    if (ret != STATUS_OK)
        return ret;

    *asc_enabled = (uint8_t)word;

    return STATUS_OK;

}



int16_t scd30_enable_automatic_self_calibration(uint8_t enable_asc) {
    int16_t ret;
    uint16_t asc = !!enable_asc;

    ret = scd30_i2c_write_cmd_with_args(SCD30_I2C_ADDRESS,
                                              SCD30_CMD_AUTO_SELF_CALIBRATION,
                                              &asc, SCD30_NUM_WORDS(asc));
    usleep(SCD30_WRITE_DELAY_US);

    return ret;

}


int16_t scd30_set_forced_recalibration(uint16_t co2_ppm) {
    int16_t ret;

    ret = scd30_i2c_write_cmd_with_args(
        SCD30_I2C_ADDRESS, SCD30_CMD_SET_FORCED_RECALIBRATION, &co2_ppm,
        SCD30_NUM_WORDS(co2_ppm));
    usleep(SCD30_WRITE_DELAY_US);

    return ret;

}


int16_t scd30_get_forced_recalibration(uint16_t *co2_ppm) {

    return scd30_i2c_delayed_read_cmd(SCD30_I2C_ADDRESS, SCD30_CMD_SET_FORCED_RECALIBRATION,
        3000, co2_ppm, SCD30_NUM_WORDS(*co2_ppm));

}

