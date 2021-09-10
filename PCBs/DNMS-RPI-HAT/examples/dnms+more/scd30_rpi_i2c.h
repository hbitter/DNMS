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

#include <stdint.h>
#include <stdio.h> 
#include <unistd.h>				//Needed for I2C port
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port


#define SCD30_I2C_ADDRESS                0x61
#define SCD30_WORD_SIZE                  2
#define SCD30_NUM_WORDS(x)               (sizeof(x) / SCD30_WORD_SIZE)
#define SCD30_MAX_BUFFER_WORDS           32
#define SCD30_CMD_SINGLE_WORD_BUF_LEN    (SCD30_COMMAND_SIZE + SCD30_WORD_SIZE + CRC8_LEN)
#define STATUS_OK                        0
#define STATUS_FAIL                      (-1)
#define SCD30_COMMAND_SIZE               2
#define CRC8_POLYNOMIAL                  0x31
#define CRC8_INIT                        0xFF
#define CRC8_LEN                         1

#define SCD30_CMD_START_PERIODIC_MEASUREMENT    0x0010
#define SCD30_CMD_STOP_PERIODIC_MEASUREMENT     0x0104
#define SCD30_CMD_READ_MEASUREMENT              0x0300
#define SCD30_CMD_SET_MEASUREMENT_INTERVAL      0x4600
#define SCD30_CMD_GET_DATA_READY                0x0202
#define SCD30_CMD_SET_TEMPERATURE_OFFSET        0x5403
#define SCD30_CMD_SET_ALTITUDE                  0x5102
#define SCD30_CMD_SET_FORCED_RECALIBRATION      0x5204
#define SCD30_CMD_AUTO_SELF_CALIBRATION         0x5306
#define SCD30_CMD_READ_SERIAL                   0xD033
#define SCD30_CMD_SOFT_RESET                    0xD304
#define SCD30_CMD_READ_FIRMWARE_VERSION         0xD100
#define SCD30_SERIAL_NUM_WORDS                  16
#define SCD30_WRITE_DELAY_US                    20000


#define be16_to_cpu(s) (((uint16_t)(s) << 8) | (0xff & ((uint16_t)(s)) >> 8))
#define be32_to_cpu(s) (((uint32_t)be16_to_cpu(s) << 16) | \
                        (0xffff & (be16_to_cpu((s) >> 16))))
#define be64_to_cpu(s) (((uint64_t)be32_to_cpu(s) << 32) | \
                        (0xffffffff & ((uint64_t)be32_to_cpu((s) >> 32))))
/**
   Convert a word-array to a bytes-array, effectively reverting the
   host-endianness to big-endian
   @a:  word array to change (must be (uint16_t *) castable)
   @w:  number of word-sized elements in the array (scd30_NUM_WORDS(a)).
*/
#define SCD30_WORDS_TO_BYTES(a, w) \
  for (uint16_t *__a = (uint16_t *)(a), __e = (w), __w = 0; __w < __e; ++__w) { \
    __a[__w] = be16_to_cpu(__a[__w]); \
  }




/**
   open_i2c() - open i2c bus on Raspberry Pi
   
   Return:  0 on success, an error code otherwise
*/

int16_t open_i2c_scd30();


/**
 * scd30_probe() - check if the SCD sensor is available and initialize it
 *
 * @return  0 on success, an error code otherwise.
 */
int16_t scd30_probe(void);


/**
 * Read out the serial number
 *
 * @param serial    the address for the result of the serial number.
 *                  --------------------------------------
 *                  THE BUFFER MUST HOLD AT LEAST 33 BYTES
 *                  --------------------------------------
 *                  Usage example:
 *                  ```
 *                  char scd30_serial[33];
 *                  if (scd30_read_serial(scd30_serial) == 0) {
 *                      printf("SCD30 serial: %s\n", scd30_serial);
 *                  } else {
 *                      printf("Error reading SCD30 serial\n");
 *                  }
 *                  ```
 *                  Contains a zero-terminated string.
 * @return          0 if the command was successful, else an error code.
 */
int16_t scd30_read_serial(char* serial);


/**
 * scd30_get_data_ready() - Get data ready status
 *
 * Data ready command is used to determine if a measurement can be read from the
 * sensor's buffer. Whenever there is a measurement available from the internal
 * buffer this command returns 1 and 0 otherwise. As soon as the measurement has
 * been read by the return value changes to 0. It is recommended to use the data
 * ready status byte before readout of the measurement values with
 * scd30_read_measurement().
 *
 * @param data_ready    Pointer to memory of where to set the data ready bit.
 *                      The memory is set to 1 if a measurement is ready to be
 *                      fetched, 0 otherwise.
 *
 * @return              0 if the command was successful, an error code otherwise
 */
int16_t scd30_get_data_ready(uint16_t* data_ready);


/**
 * scd30_i2c_delayed_read_cmd() - send a command, wait for the sensor to
 *                                    process and read data back
 * @address:    Sensor i2c address
 * @cmd:        Command
 * @delay:      Time in microseconds to delay sending the read request
 * @data_words: Allocated buffer to store the read data
 * @num_words:  Data words to read (without CRC bytes)
 *
 * @return      NO_ERROR on success, an error code otherwise
 */
int16_t scd30_i2c_delayed_read_cmd(uint8_t address, uint16_t cmd,
                                       uint32_t delay_us, uint16_t* data_words,
                                       uint16_t num_words);


/**
 * scd30_fill_cmd_send_buf() - create the i2c send buffer for a command and
 *                                 a set of argument words. The output buffer
 *                                 interleaves argument words with their
 *                                 checksums.
 * @buf:        The generated buffer to send over i2c. Then buffer length must
 *              be at least SENSIRION_COMMAND_LEN + num_args *
 *              (SENSIRION_WORD_SIZE + CRC8_LEN).
 * @cmd:        The i2c command to send. It already includes a checksum.
 * @args:       The arguments to the command. Can be NULL if none.
 * @num_args:   The number of word arguments in args.
 *
 * @return      The number of bytes written to buf
 */
uint16_t scd30_fill_cmd_send_buf(uint8_t* buf, uint16_t cmd, const uint16_t* args, uint8_t num_args);


uint8_t scd30_common_generate_crc(uint8_t *data, uint16_t count);


int8_t scd30_common_check_crc(uint8_t *data, uint16_t count, uint8_t checksum);


int16_t scd30_i2c_read_words(uint8_t address, uint16_t *data_words, uint16_t num_words);


int8_t scd30_i2c_write(uint8_t address, const uint8_t* data, uint16_t count);


int16_t scd30_i2c_read_bytes(uint8_t address, uint8_t *data, uint16_t num_words);


int8_t scd30_i2c_read(uint8_t address, uint8_t* data, uint16_t count);


/**
 * Read out the serial number
 *
 * @param serial    the address for the result of the serial number.
 *                  --------------------------------------
 *                  THE BUFFER MUST HOLD AT LEAST 33 BYTES
 *                  --------------------------------------
 *                  Usage example:
 *                  ```
 *                  char scd30_serial[33];
 *                  if (scd30_read_serial(scd30_serial) == 0) {
 *                      printf("SCD30 serial: %s\n", scd30_serial);
 *                  } else {
 *                      printf("Error reading SCD30 serial\n");
 *                  }
 *                  ```
 *                  Contains a zero-terminated string.
 * @return          0 if the command was successful, else an error code.
 */
int16_t scd30_read_serial_number(char* serial);


int16_t scd30_i2c_write_cmd(uint8_t address, uint16_t command);


/**
 * scd30_i2c_read_words_as_bytes() - read data words as byte-stream from
 *                                       sensor
 *
 * Read bytes without adjusting values to the uP's word-order.
 *
 * @address:    Sensor i2c address
 * @data:       Allocated buffer to store the read bytes.
 *              The buffer may also have been modified on STATUS_FAIL return.
 * @num_words:  Number of data words(!) to read (without CRC bytes)
 *              Since only word-chunks can be read from the sensor the size
 *              is still specified in sensor-words (num_words = num_bytes *
 *              SENSIRION_WORD_SIZE)
 *
 * @return      NO_ERROR on success, an error code otherwise
 */
int16_t scd30_i2c_read_words_as_bytes(uint8_t address, uint8_t* data,
                                          uint16_t num_words);


int16_t scd30_reset();


int16_t scd30_read_version(uint8_t *major, uint8_t *minor);


int16_t scd30_read_measurement(float* co2_ppm, float* temperature, float* humidity);


int16_t scd30_i2c_read_cmd(uint8_t address, uint16_t cmd, uint16_t* data_words, uint16_t num_words);


int16_t scd30_i2c_delayed_read_cmd(uint8_t address, uint16_t cmd,
                                       uint32_t delay_us, uint16_t* data_words,
                                       uint16_t num_words);

/**
 * scd30_bytes_to_float() - Convert an array of bytes to a float
 *
 * Convert an array of bytes received from the sensor in big-endian/MSB-first
 * format to an float value in the correct system-endianness.
 *
 * @param bytes An array of at least four bytes (MSB first)
 * @return      The byte array represented as float
 */
float scd30_bytes_to_float(const uint8_t* bytes);


uint32_t scd30_bytes_to_uint32_t(const uint8_t* bytes);


int16_t scd30_get_measurement_interval(uint16_t *interval_sec);


int16_t scd30_set_measurement_interval(uint16_t interval_sec);


int16_t scd30_i2c_write_cmd_with_args(uint8_t address, uint16_t command,
                                          const uint16_t* data_words,
                                          uint16_t num_words);


/**
 * scd30_set_temperature_offset() - Set the temperature offset
 *
 * The on-board RH/T sensor is influenced by thermal self-heating of SCD30 and
 * other electrical components. Design-in alters the thermal properties of SCD30
 * such that temperature and humidity offsets may occur when operating the
 * sensor in end-customer devices. Compensation of those effects is achievable
 * by writing the temperature offset found in continuous operation of the device
 * into the sensor.
 * The temperature offset value is saved in non-volatile memory. The last set
 * value will be used after repowering.
 *
 * @param temperature_offset    Temperature offset, unit [degrees Celsius * 100]
 *                              i.e. one tick corresponds to 0.01 degrees C
 *
 * @return                      0 if the command was successful, an error code
 *                              otherwise
 */
int16_t scd30_set_temperature_offset(uint16_t temperature_offset);


int16_t scd30_get_temperature_offset(uint16_t *temperature_offset);


/**
 * scd30_set_altitude() - Set the altitude above sea level
 *
 * Measurements of CO2 concentration are influenced by altitude. When a value is
 * set, the altitude-effect is compensated. The altitude setting is disregarded
 * when an ambient pressure is set on the sensor with
 * scd30_start_periodic_measurement.
 * The altitude is saved in non-volatile memory. The last set value will be used
 * after repowering.
 *
 * @param altitude  altitude in meters above sea level, 0 meters is the default
 *                  value and disables altitude compensation
 *
 * @return          0 if the command was successful, an error code otherwise
 */
int16_t scd30_set_altitude(uint16_t altitude);


int16_t scd30_get_altitude(uint16_t *altitude);



int16_t scd30_get_ambient_pressure(uint16_t *pressure);


/**
 * scd30_start_periodic_measurement() - Start continuous measurement to measure
 * CO2 concentration, relative humidity and temperature or updates the ambient
 * pressure if the periodic measurement is already running.
 *
 * Measurement data which is not read from the sensor is continuously
 * overwritten. The CO2 measurement value can be compensated for ambient
 * pressure by setting the pressure value in mBar. Setting the ambient pressure
 * overwrites previous and future settings of altitude compensation. Setting the
 * pressure to zero deactivates the ambient pressure compensation.
 * The continuous measurement status is saved in non-volatile memory. The last
 * measurement mode is resumed after repowering.
 *
 * @param pressure              Ambient pressure in millibars. 0 to deactivate
 *                              ambient pressure compensation (reverts to
 *                              altitude compensation, if set), 700-1200mBar
 *                              allowable range otherwise
 *
 * @return                      0 if the command was successful, an error code
 *                              otherwise
 */
int16_t scd30_start_periodic_measurement(uint16_t pressure);


/**
 * scd30_stop_periodic_measurement() - Stop the continuous measurement
 *
 * @return  0 if the command was successful, else an error code
 */
int16_t scd30_stop_periodic_measurement(void);


/**
 * scd30_get_automatic_self_calibration() - Read if the sensor's automatic self
 * calibration is enabled or disabled
 *
 * See scd30_enable_automatic_self_calibration() for more details.
 *
 * @param asc_enabled   Pointer to memory of where to set the self calibration
 *                      state. 1 if ASC is enabled, 0 if ASC disabled. Remains
 *                      untouched if return is non-zero.
 *
 * @return              0 if the command was successful, an error code otherwise
 */
int16_t scd30_get_automatic_self_calibration(uint8_t* asc_enabled);


/**
 * scd30_enable_automatic_self_calibration() - Enable or disable the sensor's
 * automatic self calibration
 *
 * When activated for the first time a period of minimum 7 days is needed so
 * that the algorithm can find its initial parameter set for ASC.
 * The sensor has to be exposed to fresh air for at least 1 hour every day.
 * Refer to the datasheet for further conditions
 *
 * ASC status is saved in non-volatile memory. When the sensor is powered down
 * while ASC is activated SCD30 will continue with automatic self-calibration
 * after repowering without sending the command.
 *
 * @param enable_asc    enable ASC if non-zero, disable otherwise
 *
 * @return              0 if the command was successful, an error code otherwise
 */
int16_t scd30_enable_automatic_self_calibration(uint8_t enable_asc);


/**
 * scd30_set_forced_recalibration() - Forcibly recalibrate the sensor to a known
 * value.
 *
 * Forced recalibration (FRC) is used to compensate for sensor drifts when a
 * reference value of the CO2 concentration in close proximity to the SCD30 is
 * available.
 *
 * For best results the sensor has to be run in a stable environment in
 * continuous mode at a measurement rate of 2s for at least two minutes before
 * applying the calibration command and sending the reference value.
 * Setting a reference CO2 concentration will overwrite the settings from ASC
 * (see scd30_enable_automatic_self_calibration) and vice-versa. The reference
 * CO2 concentration has to be in the range 400..2000 ppm.
 *
 * FRC value is saved in non-volatile memory, the last set FRC value will be
 * used for field-calibration after repowering.
 *
 * @param co2_ppm   recalibrate to this specific co2 concentration
 *
 * @return          0 if the command was successful, an error code otherwise
 */
int16_t scd30_set_forced_recalibration(uint16_t co2_ppm);


int16_t scd30_get_forced_recalibration(uint16_t *co2_ppm);

