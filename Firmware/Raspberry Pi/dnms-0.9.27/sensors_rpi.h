#pragma once
#include <stdint.h>
/*
 * sensors_rpi.h — Environment sensor drivers for Raspi (pigpio).
 *
 * Supported sensors (individually enable in dnms.conf):
 *   SDS011  — UART PM sensor via Linux serial (termios)
 *   SPS30   — Sensirion I²C PM sensor (pigpio, address 0x69)
 *   SHT3x   — Sensirion I²C T+RH (pigpio, address 0x44 or 0x45, periodic mode)
 *   SEN5x   — Sensirion I²C PM+T+RH+VOC+NOx (pigpio, address 0x69)
 *   BME280  — Bosch I²C T+P+RH (pigpio, address 0x76 or 0x77)
 *
 * Note: SPS30 and SEN5x share I²C address 0x69 — use only one at a time.
 *
 * Call sensors_rpi_init() once at startup (after pigpio gpioInitialise).
 * Call sensors_rpi_read() while holding the dnms_mutex, after DNMS reads.
 * Call sensors_rpi_close() at shutdown.
 */

/* ── SDS011 (UART PM sensor) ─────────────────────────────────────────────── */
extern float last_value_SDS_P1;    /* PM10  µg/m³ */
extern float last_value_SDS_P2;    /* PM2.5 µg/m³ */

/* ── SPS30 (Sensirion I²C PM) ────────────────────────────────────────────── */
extern float last_value_SPS30_P0;   /* PM1.0   µg/m³ */
extern float last_value_SPS30_P2;   /* PM2.5   µg/m³ */
extern float last_value_SPS30_P4;   /* PM4.0   µg/m³ */
extern float last_value_SPS30_P1;   /* PM10    µg/m³ */
extern float last_value_SPS30_N05;  /* NC0.5   #/cm³ */
extern float last_value_SPS30_N1;   /* NC1.0   #/cm³ */
extern float last_value_SPS30_N25;  /* NC2.5   #/cm³ */
extern float last_value_SPS30_N4;   /* NC4.0   #/cm³ */
extern float last_value_SPS30_N10;  /* NC10    #/cm³ */
extern float last_value_SPS30_TS;   /* Typical particle size µm */

/* ── SHT3x (I²C T+RH) ───────────────────────────────────────────────────── */
extern float last_value_SHT3X_T;   /* Temperature °C */
extern float last_value_SHT3X_H;   /* Humidity    %RH */

/* ── SEN5x (I²C PM+T+RH+VOC+NOx) ───────────────────────────────────────── */
extern float last_value_SEN5X_P0;   /* PM1.0   µg/m³ */
extern float last_value_SEN5X_P2;   /* PM2.5   µg/m³ */
extern float last_value_SEN5X_P4;   /* PM4.0   µg/m³ */
extern float last_value_SEN5X_P1;   /* PM10    µg/m³ */
extern float last_value_SEN5X_T;    /* Temperature °C */
extern float last_value_SEN5X_H;    /* Humidity    %RH */
extern float last_value_SEN5X_VOC;  /* VOC index */
extern float last_value_SEN5X_NOX;  /* NOx index */

/* ── BME280 (I²C T+P+RH) ────────────────────────────────────────────────── */
extern float last_value_BME280_T;   /* Temperature °C */
extern float last_value_BME280_P;   /* Pressure    hPa */
extern float last_value_BME280_H;   /* Humidity    %RH  (-1 if BMP280) */

/* ── SCD30 (I²C CO₂ + T + RH) ───────────────────────────────────────────── */
extern float last_value_SCD30_CO2;  /* CO₂       ppm   (-1 = not valid)  */
extern float last_value_SCD30_T;    /* Temperature °C   (-128 = not valid) */
extern float last_value_SCD30_H;    /* Humidity    %RH  (-1 = not valid)  */

/* ── SCD4x (I²C CO₂ + T + RH) ───────────────────────────────────────────── */
extern float last_value_SCD4X_CO2;  /* CO₂       ppm   (-1 = not valid)  */
extern float last_value_SCD4X_T;    /* Temperature °C   (-128 = not valid) */
extern float last_value_SCD4X_H;    /* Humidity    %RH  (-1 = not valid)  */

/* I2C bus number (usually 1) */
extern int sensors_i2c_bus;

/* Sensor init status: 1 = initialised OK, 0 = not found / not enabled */
extern int sds011_ok;
extern int sps30_ok;
extern int sht3x_ok;
extern int sen5x_ok;
extern int bme280_ok;
extern int scd30_ok;
extern int scd4x_ok;
extern int bme280_has_humidity;  /* 1 = BME280 (has humidity), 0 = BMP280 */

/* Per-sensor read error counters (incremented on each failed read) */
extern uint32_t sensor_err_sds011;
extern uint32_t sensor_err_sps30;
extern uint32_t sensor_err_sht3x;
extern uint32_t sensor_err_sen5x;
extern uint32_t sensor_err_bme280;
extern uint32_t sensor_err_scd30;
extern uint32_t sensor_err_scd4x;

void sensors_rpi_init(void);   /* call once after pigpio init */
void sensors_rpi_read(void);   /* call with dnms_mutex held   */
void sensors_rpi_close(void);  /* call at shutdown             */
