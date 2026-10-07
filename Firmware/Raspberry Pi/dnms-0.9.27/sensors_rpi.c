/*
 * sensors_rpi.c — Environment sensor drivers for Raspi (pigpio + termios).
 *
 * Sensor overview:
 *   SDS011  — Nova PM, UART 9600-8N1, 10-byte frames; reads via termios /dev/tty*
 *   SPS30   — Sensirion PM2.5+, I²C 0x69; Sensirion CRC8 wire protocol
 *   SHT3x   — Sensirion T/RH, I²C 0x44/0x45; single-shot 2-byte command + 6-byte read
 *   SEN5x   — Sensirion PM+T+RH+VOC+NOx, I²C 0x69; standard command-read protocol
 *   BME280  — Bosch T/P/RH, I²C 0x76/0x77; register-based reads + compensation formulas
 *
 * SPS30 and SEN5x share I²C address 0x69 — the user may enable only one.
 */

#include "sensors_rpi.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <pigpio.h>

/* ── Config variables (declared in dnms.c) ─────────────────────────────── */
extern int         enable_sds011;
extern const char *sds011_uart_port;
extern int         enable_sps30;
extern int         enable_sht3x;
extern int         enable_sen5x;
extern int         enable_bme280;
extern int         enable_scd30;
extern int         enable_scd4x;
extern int         sht3x_i2c_addr;
extern int         bme280_i2c_addr;
extern double      temp_correction_sht3x;
extern double      temp_correction_sen5x;
extern double      temp_correction_bme280;
extern double      temp_correction_scd30;
extern double      temp_correction_scd4x;

/* ── Sensor value storage ─────────────────────────────────────────────── */
float last_value_SDS_P1   = -1.0f;
float last_value_SDS_P2   = -1.0f;

float last_value_SPS30_P0  = -1.0f;
float last_value_SPS30_P2  = -1.0f;
float last_value_SPS30_P4  = -1.0f;
float last_value_SPS30_P1  = -1.0f;
float last_value_SPS30_N05 = -1.0f;
float last_value_SPS30_N1  = -1.0f;
float last_value_SPS30_N25 = -1.0f;
float last_value_SPS30_N4  = -1.0f;
float last_value_SPS30_N10 = -1.0f;
float last_value_SPS30_TS  = -1.0f;

float last_value_SHT3X_T  = -128.0f;
float last_value_SHT3X_H  = -1.0f;

float last_value_SEN5X_P0  = -1.0f;
float last_value_SEN5X_P2  = -1.0f;
float last_value_SEN5X_P4  = -1.0f;
float last_value_SEN5X_P1  = -1.0f;
float last_value_SEN5X_T   = -128.0f;
float last_value_SEN5X_H   = -1.0f;
float last_value_SEN5X_VOC = -1.0f;
float last_value_SEN5X_NOX = -1.0f;

float last_value_BME280_T  = -128.0f;
float last_value_BME280_P  = -1.0f;
float last_value_BME280_H  = -1.0f;

float last_value_SCD30_CO2 = -1.0f;
float last_value_SCD30_T   = -128.0f;
float last_value_SCD30_H   = -1.0f;

float last_value_SCD4X_CO2 = -1.0f;
float last_value_SCD4X_T   = -128.0f;
float last_value_SCD4X_H   = -1.0f;

int sensors_i2c_bus = 1;   /* default I²C bus */

/* ── Per-sensor error counters ─────────────────────────────────────────── */
uint32_t sensor_err_sds011 = 0;
uint32_t sensor_err_sps30  = 0;
uint32_t sensor_err_sht3x  = 0;
uint32_t sensor_err_sen5x  = 0;
uint32_t sensor_err_bme280 = 0;
uint32_t sensor_err_scd30  = 0;
uint32_t sensor_err_scd4x  = 0;

/* ── Sensor init status (set in sensors_rpi_init, extern-visible) ───────── */
int sds011_ok = 0;
int sps30_ok  = 0;
int sht3x_ok  = 0;
int sen5x_ok  = 0;
int bme280_ok = 0;
int scd30_ok  = 0;
int scd4x_ok  = 0;

/* ── Private state ─────────────────────────────────────────────────────── */
static int sds_fd    = -1;   /* SDS011 serial fd                  */
static int sps30_h   = -1;   /* pigpio I²C handle for SPS30       */
static int sht3x_h   = -1;   /* pigpio I²C handle for SHT3x       */
static int sen5x_h   = -1;   /* pigpio I²C handle for SEN5x       */
static int bme280_h  = -1;   /* pigpio I²C handle for BME280/BMP280 */
static int scd30_h   = -1;   /* pigpio I²C handle for SCD30       */
static int scd4x_h   = -1;   /* pigpio I²C handle for SCD4x       */
int bme280_has_humidity = 0;

/* BME280 calibration data */
static uint16_t bme_dig_T1;
static  int16_t bme_dig_T2, bme_dig_T3;
static uint16_t bme_dig_P1;
static  int16_t bme_dig_P2, bme_dig_P3, bme_dig_P4, bme_dig_P5,
                bme_dig_P6, bme_dig_P7, bme_dig_P8, bme_dig_P9;
static  uint8_t bme_dig_H1, bme_dig_H3;
static  int16_t bme_dig_H2, bme_dig_H4, bme_dig_H5;
static   int8_t bme_dig_H6;
static int bme280_calib_ok = 0;


/* ══════════════════════════════════════════════════════════════════════════
 *  Sensirion CRC-8 helpers (polynomial 0x31, init 0xFF)
 * ══════════════════════════════════════════════════════════════════════════ */

static uint8_t sensirion_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
    return crc;
}

/* Read `nwords` 16-bit words from handle h after issuing cmd.
   Each word is 2 data bytes + 1 CRC byte in the read stream.
   out[] receives the raw uint16_t values. Returns 0 on success. */
static int sensirion_cmd_read_words(int h, uint16_t cmd,
                                    uint16_t *out, int nwords) {
    uint8_t cbuf[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
    if (i2cWriteDevice(h, (char *)cbuf, 2) != 0) return -1;
    usleep(10000);  /* 10 ms turnaround */

    int nbytes = nwords * 3;
    char rbuf[60];
    if (nbytes > (int)sizeof(rbuf)) return -1;
    if (i2cReadDevice(h, rbuf, nbytes) != nbytes) return -1;

    for (int i = 0; i < nwords; i++) {
        uint8_t msb = (uint8_t)rbuf[i * 3];
        uint8_t lsb = (uint8_t)rbuf[i * 3 + 1];
        uint8_t crc = (uint8_t)rbuf[i * 3 + 2];
        uint8_t expected = sensirion_crc8((uint8_t *)&rbuf[i * 3], 2);
        if (crc != expected) return -1;
        out[i] = ((uint16_t)msb << 8) | lsb;
    }
    return 0;
}

/* Send command without data and without read (e.g. start measurement). */
static int sensirion_cmd_write(int h, uint16_t cmd) {
    uint8_t cbuf[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
    return (i2cWriteDevice(h, (char *)cbuf, 2) == 0) ? 0 : -1;
}

/* Decode a Sensirion IEEE-754 float from 6-byte word-pair (word0 MSW, word1 LSW).
   Layout: MSB0 LSB0 CRC0 MSB1 LSB1 CRC1 → float. */
static int sensirion_decode_float(const uint8_t *p, float *out) {
    uint8_t crc0 = sensirion_crc8(p,     2);
    uint8_t crc1 = sensirion_crc8(p + 3, 2);
    if (p[2] != crc0 || p[5] != crc1) return -1;
    uint32_t v = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16)
               | ((uint32_t)p[3] <<  8) | (uint32_t)p[4];
    memcpy(out, &v, sizeof(float));
    return 0;
}


/* ══════════════════════════════════════════════════════════════════════════
 *  SDS011 (UART)
 * ══════════════════════════════════════════════════════════════════════════ */

static int sds_open(const char *port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "[sensors] SDS011: cannot open %s: %s\n",
                port, strerror(errno));
        return -1;
    }
    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    cfmakeraw(&tio);
    cfsetispeed(&tio, B9600);
    cfsetospeed(&tio, B9600);
    tio.c_cc[VMIN]  = 0;
    tio.c_cc[VTIME] = 0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &tio);
    return fd;
}

/* Returns number of valid packets received (0 = no data / error). */
static uint32_t sds_drain_and_average(void) {
    uint8_t buf[10];
    int     pos = 0;
    uint32_t pm25_sum = 0, pm10_sum = 0, count = 0;

    while (1) {
        uint8_t b;
        int n = (int)read(sds_fd, &b, 1);
        if (n <= 0) break;

        if (pos == 0) {
            if (b != 0xAA) continue;
        } else if (pos == 1) {
            if (b != 0xC0) { pos = 0; continue; }
        }
        buf[pos++] = b;

        if (pos == 10) {
            pos = 0;
            if (buf[9] != 0xAB) continue;
            uint8_t cs = 0;
            for (int i = 2; i < 8; i++) cs ^= buf[i];
            if (cs != buf[8]) continue;
            pm25_sum += (uint32_t)buf[2] | ((uint32_t)buf[3] << 8);
            pm10_sum += (uint32_t)buf[4] | ((uint32_t)buf[5] << 8);
            count++;
        }
    }

    if (count > 0) {
        last_value_SDS_P2 = (float)pm25_sum / (count * 10.0f);
        last_value_SDS_P1 = (float)pm10_sum / (count * 10.0f);
    }
    return count;
}


/* ══════════════════════════════════════════════════════════════════════════
 *  SPS30 (I²C 0x69)
 * ══════════════════════════════════════════════════════════════════════════ */

#define SPS30_ADDR             0x69
#define SPS30_CMD_RESET        0xD304
#define SPS30_CMD_START        0x0010
#define SPS30_CMD_READY        0x0202
#define SPS30_CMD_READ         0x0300
#define SPS30_ARG_START_FLOAT  0x0300  /* IEEE-754 float output mode */

static int sps30_init(void) {
    int h = i2cOpen(sensors_i2c_bus, SPS30_ADDR, 0);
    if (h < 0) {
        fprintf(stderr, "[sensors] SPS30: i2cOpen failed: %d\n", h);
        return -1;
    }
    /* reset */
    uint8_t rst[2] = { 0xD3, 0x04 };
    i2cWriteDevice(h, (char *)rst, 2);
    usleep(100000);  /* 100 ms boot */

    /* start measurement with arg 0x0300 (float mode) + CRC */
    uint8_t arg[2] = { 0x03, 0x00 };
    uint8_t crc = sensirion_crc8(arg, 2);
    uint8_t start[5] = { 0x00, 0x10, arg[0], arg[1], crc };
    if (i2cWriteDevice(h, (char *)start, 5) != 0) {
        fprintf(stderr, "[sensors] SPS30: start measurement failed\n");
        i2cClose(h);
        return -1;
    }
    usleep(20000);  /* SPS30 needs ~20 ms to accept start */
    fprintf(stderr, "[sensors] SPS30 started on I²C bus %d\n", sensors_i2c_bus);
    return h;
}

static void sps30_read(void) {
    /* poll data ready */
    uint16_t ready = 0;
    if (sensirion_cmd_read_words(sps30_h, SPS30_CMD_READY, &ready, 1) != 0) {
        sensor_err_sps30++;
        last_value_SPS30_P0 = last_value_SPS30_P2 = last_value_SPS30_P4 = last_value_SPS30_P1  = -1.0f;
        last_value_SPS30_N05 = last_value_SPS30_N1 = last_value_SPS30_N25 = last_value_SPS30_N4 = -1.0f;
        last_value_SPS30_N10 = last_value_SPS30_TS = -1.0f;
        fprintf(stderr, "[sensors] SPS30: data-ready read failed\n"); fflush(stderr);
        return;
    }
    if ((ready & 0x01) == 0) return;

    /* read measurement: 10 floats × 6 bytes = 60 bytes */
    uint8_t cbuf[2] = { 0x03, 0x00 };
    i2cWriteDevice(sps30_h, (char *)cbuf, 2);
    usleep(20000);
    char raw[60];
    if (i2cReadDevice(sps30_h, raw, 60) != 60) {
        sensor_err_sps30++;
        last_value_SPS30_P0 = last_value_SPS30_P2 = last_value_SPS30_P4 = last_value_SPS30_P1  = -1.0f;
        last_value_SPS30_N05 = last_value_SPS30_N1 = last_value_SPS30_N25 = last_value_SPS30_N4 = -1.0f;
        last_value_SPS30_N10 = last_value_SPS30_TS = -1.0f;
        fprintf(stderr, "[sensors] SPS30: measurement read failed\n"); fflush(stderr);
        return;
    }

    const uint8_t *p = (const uint8_t *)raw;
    float vals[10];
    for (int i = 0; i < 10; i++) {
        if (sensirion_decode_float(p + i * 6, &vals[i]) != 0) {
            sensor_err_sps30++;
            last_value_SPS30_P0 = last_value_SPS30_P2 = last_value_SPS30_P4 = last_value_SPS30_P1  = -1.0f;
            last_value_SPS30_N05 = last_value_SPS30_N1 = last_value_SPS30_N25 = last_value_SPS30_N4 = -1.0f;
            last_value_SPS30_N10 = last_value_SPS30_TS = -1.0f;
            fprintf(stderr, "[sensors] SPS30: float decode failed at word %d\n", i); fflush(stderr);
            return;
        }
    }
    last_value_SPS30_P0  = vals[0];
    last_value_SPS30_P2  = vals[1];
    last_value_SPS30_P4  = vals[2];
    last_value_SPS30_P1  = vals[3];
    last_value_SPS30_N05 = vals[4];
    last_value_SPS30_N1  = vals[5];
    last_value_SPS30_N25 = vals[6];
    last_value_SPS30_N4  = vals[7];
    last_value_SPS30_N10 = vals[8];
    last_value_SPS30_TS  = vals[9];
}


/* ══════════════════════════════════════════════════════════════════════════
 *  SHT3x (I²C 0x44 or 0x45)
 *  Uses periodic measurement mode (1 mps, high repeatability) so that
 *  sht3x_read() needs no blocking sleep — a simple "Fetch Data" is enough.
 * ══════════════════════════════════════════════════════════════════════════ */

/* SHT3x command bytes */
#define SHT3X_CMD_START_PERIODIC_HI_1MPS_0 0x21   /* 1 mps, high repeatability */
#define SHT3X_CMD_START_PERIODIC_HI_1MPS_1 0x30
#define SHT3X_CMD_FETCH_DATA_0             0xE0   /* Fetch Data */
#define SHT3X_CMD_FETCH_DATA_1             0x00
#define SHT3X_CMD_STOP_PERIODIC_0         0x30   /* Stop Periodic Data Acquisition */
#define SHT3X_CMD_STOP_PERIODIC_1         0x93

static int sht3x_init(void) {
    int h = i2cOpen(sensors_i2c_bus, sht3x_i2c_addr, 0);
    if (h < 0) {
        int alt = (sht3x_i2c_addr == 0x44) ? 0x45 : 0x44;
        h = i2cOpen(sensors_i2c_bus, alt, 0);
        if (h >= 0)
            fprintf(stderr, "[sensors] SHT3x: not at 0x%02x, found at 0x%02x\n", sht3x_i2c_addr, alt);
        else {
            fprintf(stderr, "[sensors] SHT3x: no device at 0x%02x or 0x%02x\n", sht3x_i2c_addr, alt);
            return -1;
        }
    } else {
        fprintf(stderr, "[sensors] SHT3x found at 0x%02x on I\xc2\xb2""C bus %d\n", sht3x_i2c_addr, sensors_i2c_bus);
    }
    /* start periodic measurement (1 mps, high repeatability: command 0x2130) */
    uint8_t start[2] = { SHT3X_CMD_START_PERIODIC_HI_1MPS_0,
                         SHT3X_CMD_START_PERIODIC_HI_1MPS_1 };
    if (i2cWriteDevice(h, (char *)start, 2) != 0) {
        fprintf(stderr, "[sensors] SHT3x: failed to start periodic measurement\n");
        i2cClose(h);
        return -1;
    }
    usleep(20000);  /* wait for the first measurement to complete (~15 ms) */
    fprintf(stderr, "[sensors] SHT3x: periodic measurement running\n");
    return h;
}

static void sht3x_read(void) {
    /* Fetch Data from periodic measurement buffer — no sleep needed */
    uint8_t fetch[2] = { SHT3X_CMD_FETCH_DATA_0, SHT3X_CMD_FETCH_DATA_1 };
    if (i2cWriteDevice(sht3x_h, (char *)fetch, 2) != 0) {
        sensor_err_sht3x++;
        last_value_SHT3X_T = -128.0f; last_value_SHT3X_H = -1.0f;
        fprintf(stderr, "[sensors] SHT3x: fetch command failed\n");
        fflush(stderr);
        return;
    }

    char raw[6];
    if (i2cReadDevice(sht3x_h, raw, 6) != 6) {
        sensor_err_sht3x++;
        last_value_SHT3X_T = -128.0f; last_value_SHT3X_H = -1.0f;
        fprintf(stderr, "[sensors] SHT3x: read failed\n");
        fflush(stderr);
        return;
    }

    const uint8_t *p = (const uint8_t *)raw;
    if (sensirion_crc8(p,     2) != p[2] ||
        sensirion_crc8(p + 3, 2) != p[5]) {
        sensor_err_sht3x++;
        last_value_SHT3X_T = -128.0f; last_value_SHT3X_H = -1.0f;
        fprintf(stderr, "[sensors] SHT3x: CRC error\n");
        fflush(stderr);
        return;
    }

    uint16_t t_raw = ((uint16_t)p[0] << 8) | p[1];
    uint16_t h_raw = ((uint16_t)p[3] << 8) | p[4];
    last_value_SHT3X_T = 175.0f * t_raw / 65535.0f - 45.0f + (float)temp_correction_sht3x;
    last_value_SHT3X_H = 100.0f * h_raw / 65535.0f;
}


/* ══════════════════════════════════════════════════════════════════════════
 *  SEN5x (I²C 0x69)
 *  Uses standard Sensirion command protocol.
 *  Read Measured Values: cmd 0x03C4 → 24 bytes (8 uint16 words with CRC).
 *  PM values: raw uint16 × 0.1 = µg/m³.
 *  T: raw int16 / 200.0, RH: raw int16 / 100.0, VOC: /10.0, NOx: /10.0.
 * ══════════════════════════════════════════════════════════════════════════ */

#define SEN5X_ADDR  0x69
#define SEN5X_CMD_RESET  0xD304
#define SEN5X_CMD_START  0x0021
#define SEN5X_CMD_READ   0x03C4

static int sen5x_init(void) {
    int h = i2cOpen(sensors_i2c_bus, SEN5X_ADDR, 0);
    if (h < 0) {
        fprintf(stderr, "[sensors] SEN5x: i2cOpen failed: %d\n", h);
        return -1;
    }
    /* soft reset */
    if (sensirion_cmd_write(h, SEN5X_CMD_RESET) != 0) {
        fprintf(stderr, "[sensors] SEN5x: reset failed\n");
        i2cClose(h);
        return -1;
    }
    usleep(1200000);  /* SEN5x needs up to 1.2 s after reset */

    if (sensirion_cmd_write(h, SEN5X_CMD_START) != 0) {
        fprintf(stderr, "[sensors] SEN5x: start measurement failed\n");
        i2cClose(h);
        return -1;
    }
    fprintf(stderr, "[sensors] SEN5x started on I²C bus %d\n", sensors_i2c_bus);
    return h;
}

static void sen5x_read(void) {
    uint16_t words[8];
    if (sensirion_cmd_read_words(sen5x_h, SEN5X_CMD_READ, words, 8) != 0) {
        sensor_err_sen5x++;
        last_value_SEN5X_P0 = last_value_SEN5X_P2 = last_value_SEN5X_P4 = last_value_SEN5X_P1 = -1.0f;
        last_value_SEN5X_T = -128.0f; last_value_SEN5X_H = -1.0f;
        last_value_SEN5X_VOC = -1.0f; last_value_SEN5X_NOX = -1.0f;
        fprintf(stderr, "[sensors] SEN5x: read failed\n"); fflush(stderr);
        return;
    }

    /* PM values: uint16, scale 0.1 µg/m³; 0xFFFF = invalid */
    if (words[0] != 0xFFFF) last_value_SEN5X_P0 = words[0] * 0.1f;
    if (words[1] != 0xFFFF) last_value_SEN5X_P2 = words[1] * 0.1f;
    if (words[2] != 0xFFFF) last_value_SEN5X_P4 = words[2] * 0.1f;
    if (words[3] != 0xFFFF) last_value_SEN5X_P1 = words[3] * 0.1f;

    /* humidity: int16, scale 0.01 %RH */
    if (words[4] != 0x7FFF) last_value_SEN5X_H = (int16_t)words[4] * 0.01f;

    /* temperature: int16, scale 0.005 °C */
    if (words[5] != 0x7FFF) last_value_SEN5X_T = (int16_t)words[5] * 0.005f + (float)temp_correction_sen5x;

    /* VOC index: int16, scale 0.1 */
    if (words[6] != 0x7FFF) last_value_SEN5X_VOC = (int16_t)words[6] * 0.1f;

    /* NOx index: int16, scale 0.1 */
    if (words[7] != 0x7FFF) last_value_SEN5X_NOX = (int16_t)words[7] * 0.1f;
}


/* ══════════════════════════════════════════════════════════════════════════
 *  BME280 / BMP280 (I²C 0x76 or 0x77)
 *  Full register-based implementation with compensation formulas from the
 *  Bosch BME280 datasheet (4.2.3).
 * ══════════════════════════════════════════════════════════════════════════ */


#define BME280_REG_ID          0xD0
#define BME280_REG_RESET       0xE0
#define BME280_REG_CTRL_HUM    0xF2
#define BME280_REG_CTRL_MEAS   0xF4
#define BME280_REG_CONFIG      0xF5
#define BME280_REG_DATA        0xF7   /* 0xF7..0xFC = press+temp+hum (6+3 bytes) */
#define BME280_REG_CALIB_A     0x88  /* 0x88..0x9F: T/P calibration */
#define BME280_REG_CALIB_B     0xE1  /* 0xE1..0xF0: H2..H6 calibration */
#define BME280_CHIP_ID_280     0x60
#define BMP280_CHIP_ID         0x58
#define BME280_FORCED_MODE     0x01  /* CTRL_MEAS mode bits: 01 or 10 = forced */

static uint8_t bme_read8(int h, uint8_t reg) {
    return (uint8_t)i2cReadByteData(h, reg);
}

static uint16_t bme_read16_le(int h, uint8_t reg) {
    uint8_t lo = bme_read8(h, reg);
    uint8_t hi = bme_read8(h, reg + 1);
    return (uint16_t)lo | ((uint16_t)hi << 8);
}

static int16_t bme_read16s_le(int h, uint8_t reg) {
    return (int16_t)bme_read16_le(h, reg);
}

static void bme_write8(int h, uint8_t reg, uint8_t val) {
    i2cWriteByteData(h, reg, val);
}

static void bme280_read_calib(int h, int is_bme280) {
    bme_dig_T1 = bme_read16_le(h, 0x88);
    bme_dig_T2 = bme_read16s_le(h, 0x8A);
    bme_dig_T3 = bme_read16s_le(h, 0x8C);

    bme_dig_P1 = bme_read16_le(h, 0x8E);
    bme_dig_P2 = bme_read16s_le(h, 0x90);
    bme_dig_P3 = bme_read16s_le(h, 0x92);
    bme_dig_P4 = bme_read16s_le(h, 0x94);
    bme_dig_P5 = bme_read16s_le(h, 0x96);
    bme_dig_P6 = bme_read16s_le(h, 0x98);
    bme_dig_P7 = bme_read16s_le(h, 0x9A);
    bme_dig_P8 = bme_read16s_le(h, 0x9C);
    bme_dig_P9 = bme_read16s_le(h, 0x9E);

    if (is_bme280) {
        bme_dig_H1 = bme_read8(h, 0xA1);
        bme_dig_H2 = (int16_t)((bme_read8(h, 0xE2) << 8) | bme_read8(h, 0xE1));
        bme_dig_H3 = bme_read8(h, 0xE3);
        int16_t e4 = bme_read8(h, 0xE4);
        int16_t e5 = bme_read8(h, 0xE5);
        int16_t e6 = bme_read8(h, 0xE6);
        bme_dig_H4 = (int16_t)((e4 << 4) | (e5 & 0x0F));
        bme_dig_H5 = (int16_t)((e6 << 4) | ((e5 >> 4) & 0x0F));
        bme_dig_H6 = (int8_t)bme_read8(h, 0xE7);
    }
    bme280_calib_ok = 1;
}

/* Bosch compensation formulas (64-bit integer as in the datasheet). */
static float bme280_comp_temp(int32_t adc_T, int32_t *t_fine) {
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)bme_dig_T1 << 1)))
                    * (int32_t)bme_dig_T2) >> 11;
    int32_t var2 = (((((adc_T >> 4) - (int32_t)bme_dig_T1)
                      * ((adc_T >> 4) - (int32_t)bme_dig_T1)) >> 12)
                    * (int32_t)bme_dig_T3) >> 14;
    *t_fine = var1 + var2;
    return (float)((*t_fine * 5 + 128) >> 8) / 100.0f;
}

static float bme280_comp_press(int32_t adc_P, int32_t t_fine) {
    int64_t var1 = (int64_t)t_fine - 128000LL;
    int64_t var2 = var1 * var1 * (int64_t)bme_dig_P6;
    var2 = var2 + ((var1 * (int64_t)bme_dig_P5) << 17);
    var2 = var2 + ((int64_t)bme_dig_P4 << 35);
    var1 = ((var1 * var1 * (int64_t)bme_dig_P3) >> 8)
         + ((var1 * (int64_t)bme_dig_P2) << 12);
    var1 = (((int64_t)1 << 47) + var1) * (int64_t)bme_dig_P1 >> 33;
    if (var1 == 0) return 0.0f;
    int64_t p = 1048576LL - adc_P;
    p = (((p << 31) - var2) * 3125LL) / var1;
    var1 = ((int64_t)bme_dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)bme_dig_P8 * p) >> 19;
    p = ((p + var1 + var2) >> 8) + ((int64_t)bme_dig_P7 << 4);
    return (float)p / 25600.0f;  /* Pa / 100 = hPa */
}

static float bme280_comp_hum(int32_t adc_H, int32_t t_fine) {
    int32_t x = t_fine - 76800;
    x = (((((adc_H << 14) - ((int32_t)bme_dig_H4 << 20)
            - ((int32_t)bme_dig_H5 * x)) + 16384) >> 15)
         * (((((((x * (int32_t)bme_dig_H6) >> 10)
                * (((x * (int32_t)bme_dig_H3) >> 11) + 32768)) >> 10)
               + 2097152) * (int32_t)bme_dig_H2 + 8192) >> 14));
    x = x - (((((x >> 15) * (x >> 15)) >> 7) * (int32_t)bme_dig_H1) >> 4);
    if (x < 0) x = 0;
    if (x > 419430400) x = 419430400;
    return (float)(x >> 12) / 1024.0f;
}

static int bme280_init(void) {
    int h = i2cOpen(sensors_i2c_bus, bme280_i2c_addr, 0);
    if (h < 0) {
        int alt = (bme280_i2c_addr == 0x76) ? 0x77 : 0x76;
        h = i2cOpen(sensors_i2c_bus, alt, 0);
        if (h >= 0)
            fprintf(stderr, "[sensors] BME280: not at 0x%02x, found at 0x%02x\n", bme280_i2c_addr, alt);
        else {
            fprintf(stderr, "[sensors] BME280: no device at 0x%02x or 0x%02x\n", bme280_i2c_addr, alt);
            return -1;
        }
    } else {
        fprintf(stderr, "[sensors] BME280 found at 0x%02x on I\xc2\xb2""C bus %d\n", bme280_i2c_addr, sensors_i2c_bus);
    }
    /* soft reset */
    bme_write8(h, BME280_REG_RESET, 0xB6);
    usleep(10000);

    uint8_t id = bme_read8(h, BME280_REG_ID);
    if (id == BME280_CHIP_ID_280) {
        bme280_has_humidity = 1;
        fprintf(stderr, "[sensors] BME280 found (id=0x%02X), bus %d\n",
                id, sensors_i2c_bus);
        bme_write8(h, BME280_REG_CTRL_HUM, 0x05);  /* humidity ×16 */
    } else if (id == BMP280_CHIP_ID) {
        bme280_has_humidity = 0;
        fprintf(stderr, "[sensors] BMP280 found (id=0x%02X, no humidity), bus %d\n",
                id, sensors_i2c_bus);
    } else {
        fprintf(stderr, "[sensors] BME280: unexpected chip ID 0x%02X\n", id);
        i2cClose(h);
        return -1;
    }
    /* settings: temperature ×16, pressure ×16, filter off, no standby (forced mode) */
    bme_write8(h, BME280_REG_CONFIG, 0x00);
    bme_write8(h, BME280_REG_CTRL_MEAS, (0x05 << 5) | (0x05 << 2));  /* osrs_t=×16, osrs_p=×16, mode=sleep for now */

    bme280_read_calib(h, bme280_has_humidity);
    return h;
}

static void bme280_read(void) {
    /* trigger forced measurement: set mode bits to 01 */
    uint8_t ctrl = bme_read8(bme280_h, BME280_REG_CTRL_MEAS);
    bme_write8(bme280_h, BME280_REG_CTRL_MEAS, (ctrl & 0xFC) | BME280_FORCED_MODE);
    usleep(100000);  /* 100 ms should be more than enough for ×16 oversampling */

    /* read 8 bytes: 0xF7..0xFE */
    char raw[8];
    uint8_t reg = BME280_REG_DATA;
    i2cWriteDevice(bme280_h, (char *)&reg, 1);
    usleep(1000);
    if (i2cReadDevice(bme280_h, raw, bme280_has_humidity ? 8 : 6) < 0) {
        sensor_err_bme280++;
        last_value_BME280_T = -128.0f; last_value_BME280_P = -1.0f; last_value_BME280_H = -1.0f;
        fprintf(stderr, "[sensors] BME280: read failed\n"); fflush(stderr);
        return;
    }

    const uint8_t *p = (const uint8_t *)raw;
    int32_t adc_P = (((int32_t)p[0] << 12) | ((int32_t)p[1] << 4) | (p[2] >> 4));
    int32_t adc_T = (((int32_t)p[3] << 12) | ((int32_t)p[4] << 4) | (p[5] >> 4));

    int32_t t_fine = 0;
    last_value_BME280_T = bme280_comp_temp(adc_T, &t_fine) + (float)temp_correction_bme280;
    last_value_BME280_P = bme280_comp_press(adc_P, t_fine);

    if (bme280_has_humidity) {
        int32_t adc_H = ((int32_t)p[6] << 8) | p[7];
        last_value_BME280_H = bme280_comp_hum(adc_H, t_fine);
    }
}


/*  SCD30 (I²C 0x61) — Sensirion CO₂ + T + RH
 * ══════════════════════════════════════════════════════════════════════════ */

#define SCD30_ADDR           0x61
#define SCD30_CMD_START      0x0010
#define SCD30_CMD_DATA_READY 0x0202
#define SCD30_CMD_READ_MEAS  0x0300

static int scd30_init(void) {
    int h = i2cOpen(sensors_i2c_bus, SCD30_ADDR, 0);
    if (h < 0) {
        fprintf(stderr, "[sensors] SCD30: i2cOpen failed: %d\n", h);
        return -1;
    }
    /* Start continuous measurement, pressure arg = 0x0000 (no compensation) */
    uint8_t arg[2] = { 0x00, 0x00 };
    uint8_t crc = sensirion_crc8(arg, 2);
    uint8_t start[5] = { 0x00, 0x10, arg[0], arg[1], crc };
    if (i2cWriteDevice(h, (char *)start, 5) != 0) {
        fprintf(stderr, "[sensors] SCD30: start measurement failed\n");
        i2cClose(h);
        return -1;
    }
    usleep(20000);
    fprintf(stderr, "[sensors] SCD30 started on I\xc2\xb2""C bus %d\n", sensors_i2c_bus);
    return h;
}

static void scd30_read(void) {
    uint16_t ready = 0;
    if (sensirion_cmd_read_words(scd30_h, SCD30_CMD_DATA_READY, &ready, 1) != 0) {
        sensor_err_scd30++;
        last_value_SCD30_CO2 = -1.0f; last_value_SCD30_T = -128.0f; last_value_SCD30_H = -1.0f;
        fprintf(stderr, "[sensors] SCD30: data-ready check failed\n"); fflush(stderr);
        return;
    }
    if ((ready & 0x01) == 0) return;  /* not ready yet */

    uint8_t cmd[2] = { 0x03, 0x00 };
    i2cWriteDevice(scd30_h, (char *)cmd, 2);
    usleep(3000);
    char raw[18];
    if (i2cReadDevice(scd30_h, raw, 18) != 18) {
        sensor_err_scd30++;
        last_value_SCD30_CO2 = -1.0f; last_value_SCD30_T = -128.0f; last_value_SCD30_H = -1.0f;
        fprintf(stderr, "[sensors] SCD30: read failed\n"); fflush(stderr);
        return;
    }
    const uint8_t *p = (const uint8_t *)raw;
    float co2, t, h;
    if (sensirion_decode_float(p +  0, &co2) != 0 ||
        sensirion_decode_float(p +  6, &t)   != 0 ||
        sensirion_decode_float(p + 12, &h)   != 0) {
        sensor_err_scd30++;
        last_value_SCD30_CO2 = -1.0f; last_value_SCD30_T = -128.0f; last_value_SCD30_H = -1.0f;
        fprintf(stderr, "[sensors] SCD30: CRC error\n"); fflush(stderr);
        return;
    }
    last_value_SCD30_CO2 = co2;
    last_value_SCD30_T   = t + (float)temp_correction_scd30;
    last_value_SCD30_H   = h;
}


/* ══════════════════════════════════════════════════════════════════════════
 *  SCD4x (I²C 0x62) — Sensirion CO₂ + T + RH
 * ══════════════════════════════════════════════════════════════════════════ */

#define SCD4X_ADDR           0x62
#define SCD4X_CMD_STOP       0x3F86
#define SCD4X_CMD_START      0x21B1
#define SCD4X_CMD_DATA_READY 0xE4B8
#define SCD4X_CMD_READ_MEAS  0xEC05

static int scd4x_init(void) {
    int h = i2cOpen(sensors_i2c_bus, SCD4X_ADDR, 0);
    if (h < 0) {
        fprintf(stderr, "[sensors] SCD4x: i2cOpen failed: %d\n", h);
        return -1;
    }
    sensirion_cmd_write(h, SCD4X_CMD_STOP);
    usleep(500000);  /* 500 ms after stop before next command */
    if (sensirion_cmd_write(h, SCD4X_CMD_START) != 0) {
        fprintf(stderr, "[sensors] SCD4x: start measurement failed\n");
        i2cClose(h);
        return -1;
    }
    fprintf(stderr, "[sensors] SCD4x started on I\xc2\xb2""C bus %d\n", sensors_i2c_bus);
    return h;
}

static void scd4x_read(void) {
    uint16_t ready = 0;
    if (sensirion_cmd_read_words(scd4x_h, SCD4X_CMD_DATA_READY, &ready, 1) != 0) {
        sensor_err_scd4x++;
        last_value_SCD4X_CO2 = -1.0f; last_value_SCD4X_T = -128.0f; last_value_SCD4X_H = -1.0f;
        fprintf(stderr, "[sensors] SCD4x: data-ready check failed\n"); fflush(stderr);
        return;
    }
    if ((ready & 0x07FF) == 0) return;  /* lower 11 bits = 0 means not ready */

    uint16_t words[3];
    if (sensirion_cmd_read_words(scd4x_h, SCD4X_CMD_READ_MEAS, words, 3) != 0) {
        sensor_err_scd4x++;
        last_value_SCD4X_CO2 = -1.0f; last_value_SCD4X_T = -128.0f; last_value_SCD4X_H = -1.0f;
        fprintf(stderr, "[sensors] SCD4x: read failed\n"); fflush(stderr);
        return;
    }
    uint16_t co2 = words[0];
    float    t   = -45.0f + 175.0f * (float)words[1] / 65535.0f;
    float    h   = 100.0f * (float)words[2] / 65535.0f;
    if (co2 == 0) {
        sensor_err_scd4x++;
        last_value_SCD4X_CO2 = -1.0f; last_value_SCD4X_T = -128.0f; last_value_SCD4X_H = -1.0f;
        fprintf(stderr, "[sensors] SCD4x: CO2 = 0 (invalid)\n"); fflush(stderr);
        return;
    }
    last_value_SCD4X_CO2 = (float)co2;
    last_value_SCD4X_T   = t + (float)temp_correction_scd4x;
    last_value_SCD4X_H   = h;
}


/* ══════════════════════════════════════════════════════════════════════════
 *  Public API
 * ══════════════════════════════════════════════════════════════════════════ */

void sensors_rpi_init(void) {
    if (enable_sds011 && sds011_uart_port) {
        sds_fd = sds_open(sds011_uart_port);
    }
    if (enable_sps30) {
        sps30_h = sps30_init();
        if (sps30_h >= 0)
            usleep(1200000);  /* SPS30 needs 1 s to produce first valid measurement */
    }
    if (enable_sht3x) {
        sht3x_h = sht3x_init();
    }
    if (enable_sen5x) {
        if (enable_sps30 && sps30_h >= 0) {
            fprintf(stderr, "[sensors] WARNING: SPS30 and SEN5x both enabled — "
                            "they share I2C address 0x69! Skipping SEN5x.\n");
        } else {
            sen5x_h = sen5x_init();
            if (sen5x_h >= 0)
                usleep(1200000);
        }
    }
    if (enable_bme280) {
        bme280_h = bme280_init();
    }
    if (enable_scd30) {
        scd30_h = scd30_init();
    }
    if (enable_scd4x) {
        scd4x_h = scd4x_init();
    }
    /* set public status flags */
    sds011_ok = (sds_fd  >= 0);
    sps30_ok  = (sps30_h >= 0);
    sht3x_ok  = (sht3x_h >= 0);
    sen5x_ok  = (sen5x_h >= 0);
    bme280_ok = (bme280_h >= 0);
    scd30_ok  = (scd30_h >= 0);
    scd4x_ok  = (scd4x_h >= 0);
}

void sensors_rpi_read(void) {
    if (enable_sds011 && sds_fd >= 0) {
        if (sds_drain_and_average() == 0) {
            sensor_err_sds011++;
            last_value_SDS_P1 = -1.0f; last_value_SDS_P2 = -1.0f;
            fprintf(stderr, "[sensors] SDS011: no data\n"); fflush(stderr);
        }
    }
    if (enable_sps30 && sps30_h >= 0) {
        sps30_read();
    }
    if (enable_sht3x && sht3x_h >= 0) {
        sht3x_read();
    }
    if (enable_sen5x && sen5x_h >= 0) {
        sen5x_read();
    }
    if (enable_bme280 && bme280_h >= 0) {
        bme280_read();
    }
    if (enable_scd30 && scd30_h >= 0) {
        scd30_read();
    }
    if (enable_scd4x && scd4x_h >= 0) {
        scd4x_read();
    }
}

void sensors_rpi_close(void) {
    if (sds_fd  >= 0) { close(sds_fd);      sds_fd   = -1; }
    if (sps30_h >= 0) { i2cClose(sps30_h);  sps30_h  = -1; }
    if (sht3x_h >= 0) {
        uint8_t stop[2] = { SHT3X_CMD_STOP_PERIODIC_0, SHT3X_CMD_STOP_PERIODIC_1 };
        i2cWriteDevice(sht3x_h, (char *)stop, 2);
        i2cClose(sht3x_h);
        sht3x_h = -1;
    }
    if (sen5x_h >= 0) { i2cClose(sen5x_h);  sen5x_h  = -1; }
    if (bme280_h >= 0){ i2cClose(bme280_h); bme280_h = -1; }
    if (scd30_h >= 0) { i2cClose(scd30_h); scd30_h = -1; }
    if (scd4x_h >= 0) { sensirion_cmd_write(scd4x_h, SCD4X_CMD_STOP); usleep(500000); i2cClose(scd4x_h); scd4x_h = -1; }
}
