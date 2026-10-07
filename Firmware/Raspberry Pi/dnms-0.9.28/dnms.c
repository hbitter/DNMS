/*
 *  raspi-dnms
 *  
 *  Raspi program for noise measurement with a connected DNMS sensor
 *  It works from Zero W up to Raspi 4
 * 
 *  Program sends measurement to Sensor.Community and/or to an influxDB.
 *  Besides the fixed measurement interval of Sensor.Community (150 s) a 2nd measurement interval 
 *  for the influxDB is supported. The interval time of the 2nd measurement interval can be freely configured (1 - 3600 s).
 *  Transmission of data can be configured as http or https transmission.
 *                                                                      
 *  Copyright (C) 2022, 2023, 2024, 2025  Helmut Bitter
 *                                                                      
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *                                                                      
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *                                                                      
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

// if defined, than you can switch a gpio pin if a certain noise level is exceeded. The pigpio lib is includes and you have to start dnms with sudo (that depends on the pigpio lib)
// if you don't want to switch a gpio pin comment the define  gpio_switch out
#define gpio_switch false

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <sys/socket.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <libconfig.h>
#include <mosquitto.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iwlib.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/wireless.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "./dnms_rpi_i2c.h"
#include "./dnms_calib_data.h"
#include "./b64_encode.h"
#include "./data_logging.h"
#include "./webserver.h"
#include "./sensors_rpi.h"


#ifdef gpio_switch
#include <pigpio.h>
#endif


const char firmware_version[] = { "raspi-dnms-0.9.28" };  // Firmware Version for Raspberry Pi

config_t cfg;
config_setting_t *setting;


/* *************************************   start configuration values   ********************************************** */
// waiting time after program start - this depends on the raspi type, Pi Zero W should be 20 seconds, for Pi Zero 2W 10 seconds
int waiting_time_after_start;
// Priorities for threads
int prio_1st_timer;
int prio_1st_measurement;
int prio_2nd_timer;
int prio_2nd_measurement;
int prio_wlan;
int prio_print_ipc_and_log_1st;
int prio_print_ipc_and_log_2nd;
int prio_start_stop;

const char *dev_name_dnms;   // name of I²C device on Raspi
const char *interface_name;    // name of wlan or lan interface for transmission of data to Sensor.Community and/or other APIs
int enable_wlan_or_lan;             // enabble wlan or lan for transmission to Sensor.Community and/or InfluxDB
int enable_wifi_signal_strength_influxdb; // include WiFi RSSI in InfluxDB payload
int enable_wifi_signal_strength_mqtt;     // include WiFi RSSI in MQTT payload
int last_influxdb_transmission_time_to_payload; // include last InfluxDB tx duration (ms) in payload
int last_mqtt_transmission_time_to_payload;     // include last MQTT tx duration (ms) in payload
int long_id;								  // long or short ID


/* ********* Configuration of connected microphone  -- ICS-43434  or IM72D128  *********  */
int dnms_microphone;  // which microphone is connected?

/* ********* LED blink period (ms) per microphone type — transmitted to Teensy at startup *********  */
int dnms_blink_period_1 =  100;   /* ICS-43434 */
int dnms_blink_period_2 =  500;   /* IM72D128 */
int dnms_blink_period_3 =  500;   /* IM72D128 with DLR case */
int dnms_blink_period_4 =  100;   /* ICS-43434 no correction */
int dnms_blink_period_5 =  500;   /* IM72D128 no correction */
int dnms_blink_period_6 =  200;   /* micro_3 */
int dnms_blink_period_7 = 1000;   /* micro_4 */

/* ********* Microphone sensitivity constant (dB) per microphone type — transmitted to Teensy at startup *********  */
double dnms_micro_const_1 = 123.0102999565;   /* ICS-43434 */
double dnms_micro_const_2 = 133.0102999565;   /* IM72D128 */
double dnms_micro_const_3 = 133.0102999565;   /* IM72D128 with DLR case */
double dnms_micro_const_4 = 123.0102999565;   /* ICS-43434 no correction */
double dnms_micro_const_5 = 133.0102999565;   /* IM72D128 no correction */
double dnms_micro_const_6 =   0.0;            /* micro_3 — fill in actual value */
double dnms_micro_const_7 =   0.0;            /* micro_4 — fill in actual value */

/* ********* Correction value in dB for microphone for time domain dB(A) an dB(Z) values  -- for ICS-43434  or IM72D128  *********  */
double dnms_correction;

/* ********* Configuration of intervals for 1st and 2nd measurements ********* */
int enable_1st_interval;                   // enable or disable 1st measurement
int measurement_1st_interval_ms;           // 1st measurement interval, in ms
int enable_2nd_interval;                   // enable or disable 2nd measurement
int measurement_2nd_interval_ms;           // 2nd measurement interval, in ms
int threshold_2nd_interval_laeq_transmit;  // threshold for transmission of 2nd interval measurements to influxdb, transmission to influxdb is started if LAeq is equal or higher than threshold
int number_transmissions_after_exceeding;  // number of transmissions of 2nd interval measurement after exceeding threshold
int switch_output_pin;                     // switch output pin, if threshold of 2nd is exceeded, true or false
int gpio_output_pin;                       // pin that is switched on/off based on Broadcom GPIO numbering
int start_on_full_minute;                  // start measurements on the full minute (seconds = 00)
int start_on_full_hour;                    // start measurements on the full hour (minutes = 00 and seconds = 00)

/* *********  Configuration options what data is tranferred to InfluxDB/MQTT Broker *********  */
int data_transmit_laeq_1st_to_influxdb;           // 1st measurement interval LAeq data to influxDB, true or false
int data_transmit_lzeq_1st_to_influxdb;           // 1st measurement interval LZeq data to influxDB, true or false
int data_transmit_lceq_1st_to_influxdb;		 // 1st measurement interval LCeq data to influxDB, true or false
int data_transmit_laeq_1st_spectrum_to_influxdb;  // 1st measurement interval spectrum LAeq data to influxDB, true or false
int data_transmit_lzeq_1st_spectrum_to_influxdb;  //  1st measurement interval spectrum LZeq data to influxDB, true or false
int data_transmit_lceq_1st_spectrum_to_influxdb;  //  1st measurement interval spectrum LCeq data to influxDB, true or false
int data_transmit_laeq_2nd_to_influxdb;           // 2nd measurement interval LAeq data to influxDB, true or false
int data_transmit_lzeq_2nd_to_influxdb;           // 2nd measurement interval LZeq data to influxDB, true or false
int data_transmit_lceq_2nd_to_influxdb;           // 2nd measurement interval LCeq data to influxDB, true or false
int data_transmit_laeq_2nd_spectrum_to_influxdb;  // 2nd measurement interval LAeq spectrum data to influxDB, true or false
int data_transmit_lzeq_2nd_spectrum_to_influxdb;  // 2nd measurement interval LZeq spectrum data to influxDB, true or false
int data_transmit_lceq_2nd_spectrum_to_influxdb;  // 2nd measurement interval LCeq spectrum data to influxDB, true or false
int data_transmit_laeq_1st_to_mqtt              = 1;
int data_transmit_lzeq_1st_to_mqtt              = 1;
int data_transmit_lceq_1st_to_mqtt              = 0;
int data_transmit_laeq_1st_spectrum_to_mqtt     = 1;
int data_transmit_lzeq_1st_spectrum_to_mqtt     = 1;
int data_transmit_lceq_1st_spectrum_to_mqtt     = 0;
int data_transmit_laeq_2nd_to_mqtt              = 1;
int data_transmit_lzeq_2nd_to_mqtt              = 1;
int data_transmit_lceq_2nd_to_mqtt              = 0;
int data_transmit_laeq_2nd_spectrum_to_mqtt     = 1;
int data_transmit_lzeq_2nd_spectrum_to_mqtt     = 1;
int data_transmit_lceq_2nd_spectrum_to_mqtt     = 0;
int timestamp_to_influxdb;                        // send timestamp to InfluxDB, true or fase
int influxdb_transmit_http;                       // data transmission to influxDB as http, true or false (false means no http transmission)
int influxdb_transmit_https;                      // data transmission to InfluxDB as https, true or false (false means no https transmission)
const char *influxdb_server;                      // InfluxDB server address
const char *influxdb_port;                        // InfluxDB server port
const char *influxdb_pfad;                        // InfluxDB database path and name
const char *influxdb_user;                        // InfluxDB user name
const char *influxdb_passwort;                    // InfluxDB passwort for user
const char *influxdb_messung;                     // InfluxDB measurement name

/* *********  Configuration options for transmission to MQTT Broker  *********  */
int mqtt_transmit;             // data transmission to MQTT Broker, InfluxDB Line Protocol is used, true or false (false means no http transmission)
const char *mqtt_messung = "DNMS"; // MQTT measurement name (separate from InfluxDB measurement name)
const char *mqtt_user;         // MQTT user name
const char *mqtt_passwort;     // MQTT user passwort
const char *mqtt_broker;       // URL of MQRR Broker
int mqtt_port;                 // Port for MQTT Broker connection
int mqtt_keepalive;            // MQTT keepalive time
int mqtt_qos;                  // MQTT QoS
const char *mqtt_main_topic;   // MQTT Main Topic
int mqtt_use_tls      = 0;     // use TLS for MQTT connection
const char *mqtt_tls_cafile = NULL; // path to CA certificate file (or "" for system default)
int mqtt_tls_insecure = 0;     // skip hostname verification
int mqtt_use_id_as_sub_topic;  // use the Raspberry ID as MQTT Sub Topic


/* ********* Configuration options for transmission to Sensor.Community *********  */
int data_transmit_laeq_to_sc;      // transmission of LAeq 1st interval to Sensor.Community, true or false
const char *host_sc;               // Host Sensor.Community
const char *port_sc;               // Port Sensor.Community (legacy, no longer used)
const char *url_sc;                // URL to data store of Sensor.Community
const char *DNMS_API_PIN;          // API PIN für DNMS at Sensor.Community
int sc_transmit_https;             // use https (true) or http (false) for transmission to Sensor.Community
int data_transmit_laeq_to_madavi;  // transmission of LAeq 1st interval to Madavi

/* ********* Custom API — HTTP POST of JSON at each 1st-interval measurement ********* */
int  custom_api_enable        = 0;
char custom_api_server[128]   = "";
char custom_api_port[8]       = "80";
char custom_api_path[128]     = "/data";
char custom_api_hostname[140] = "";  /* server:port — built at send time */
int  custom_api_send_1st      = 1;
int  custom_api_send_2nd      = 0;
int  custom_api_spectrum_1st  = 1;
int  custom_api_spectrum_2nd  = 1;
int  custom_api_https         = 0;

/* ********* Configuration options for IPC - Inter Process Communication  using a named pipe ********* */
int data_transmit_via_pipe;
const char *name_of_pipe;


/*  ********* start/stop measurement from named pipe message  ********* */
const char *start_stop_name_of_pipe;  // name of named pipe for start/stop
int start_stop_extern;                // enable or disable external start/stop functionality (true =enable, false = disable)


/* ********* output of mesurement  values on terminal *********  */
int data_on_terminal;


/* ********* which mesurement values are outputed on terminal, named pipe or data logging *********  */
int data_laeq_1st_output_on_terminal;             // output of LAeq 1st interval measurements values on terminal, true or false
int data_la_spec_1st_output_on_terminal;          // output of LA spectrum 1st interval measurements values on terminal, true or false
int data_lzeq_1st_output_on_terminal;             // output of LZeq 1st interval measurements values on terminal, true or false
int data_lz_spec_1st_output_on_terminal;          // output of LZ spectrum 1st interval measurements values on terminal, true or false
int data_lceq_1st_output_on_terminal;             // output of LCeq 1st interval measurements values on terminal, true or false
int data_lc_spec_1st_output_on_terminal;          // output of LC spectrum 1st interval measurements values on terminal, true or false
int data_laeq_2nd_output_on_terminal;             // output of LAeq 2nd interval measurements values on terminal, true or false
int data_la_spec_2nd_output_on_terminal;          // output of LA spectrum 2nd interval measurements values on terminal, true or false
int data_lzeq_2nd_output_on_terminal;             // output of LZeq 2nd interval measurements values on terminal, true or false
int data_lz_spec_2nd_output_on_terminal;          // output of LZ spectrum 2nd interval measurements values on terminal, true or false
int data_lceq_2nd_output_on_terminal;             // output of LCeq 2nd interval measurements values on terminal, true or false
int data_lc_spec_2nd_output_on_terminal;          // output of LC spectrum 2nd interval measurements values on terminal, true or false

int last_influxdb_transmission_time_to_terminal;  // output to terminal of last transmission time to InfluxDB, true or false
int threshold_output_influxdb_ltt_to_terminal;    // threshold to output the last transmission time to InfluxDB to terminal if transmission time is higher than threshold, in ms
int last_mqtt_transmission_time_to_terminal;      // output to terminal of last transmission time to MQTT Broker, true or false
int threshold_output_mqtt_ltt_to_terminal;        // threshold to output the last transmission time to MQTT Broker to terminal if transmission time is higher than threshold, in ms
int last_sc_transmission_time_to_terminal;        // output to terminal of last transmission time to Sensor.Community, true or false
int threshold_output_sc_ltt_to_terminal;          // threshold to output last transmission time to influxdb to terminal if transmission time is higher than threshold, in ms
int output_thread_info;                           // output of thread information at program start


/* ********* data logging functionality ********* */
int data_logging;                    // enable/diasable data logging functionality
const char *data_logging_directory;  // directory for data logging (full path)

/* ********* environment sensors ********* */
int         enable_sds011      = 0;
const char *sds011_uart_port   = "/dev/ttyUSB0";
int         enable_sps30       = 0;
int         enable_sht3x       = 0;
int         sht3x_i2c_addr     = 0x44;     /* 0x44 (default) or 0x45 */
int         enable_sen5x       = 0;
int         enable_bme280      = 0;
int         bme280_i2c_addr    = 0x76;     /* 0x76 (default) or 0x77 */
double      temp_correction_sht3x  = 0.0;  /* °C offset added to SHT3x temperature */
double      temp_correction_sen5x  = 0.0;  /* °C offset added to SEN5x temperature */
double      temp_correction_bme280 = 0.0;  /* °C offset added to BME280 temperature */
int         enable_scd30       = 0;
double      temp_correction_scd30 = 0.0;
int         enable_scd4x       = 0;
double      temp_correction_scd4x = 0.0;

/* ********* webserver ********* */
int enable_webserver = 1;   // enable/disable the embedded webserver
int webserver_port   = 8080; // TCP port the webserver listens on

/* ********* measurement counters (reset at midnight) ********* */
uint32_t counter_measurements_1st = 0;
uint32_t counter_measurements_2nd = 0;

uint32_t dnms_error_count      = 0;
uint32_t influxdb_error_count  = 0;
uint32_t sc_error_count        = 0;
uint32_t madavi_error_count    = 0;
uint32_t mqtt_error_count      = 0;
uint32_t buffer_overflow_count = 0;
time_t measurement_start_time = 0;
time_t process_start_time = 0;
struct timespec process_start_mono = {0, 0};
struct timespec measurement_start_mono = {0, 0};
int date_format_iso = 1;
const char *fmt_dt  = NULL;
const char *fmt_d   = NULL;
const char *fmt_dl  = NULL;

/* *************************************   end configuration values   ******************************************************* */


#define msg_size 3072
#define timestamp_size 32
#define log_size 256
#define number_of_mac_loops 20

#define buffer_fail -1
#define buffer_success 0
#define buffer_size_sc 16
#define buffer_size_influx 512
#define dest_influxdb 2
#define dest_sc 1
#define dest_madavi 3
#define dest_mqtt 4

volatile bool transmission_enabled = true;  // transmission takes place only if flag "transmission_enabled" is set to true

bool interval_1st_active = false;   // 1st measurement interval active
bool interval_2nd_active = false;   // 2nd measurement interval active if available/supported by DNMS firmware
volatile bool interval_2nd_due = false;     // set by 2nd timer; cleared and forwarded by 1st measurement thread
uint16_t counter_threshold_2nd;     // counter for number of transmissions to InfluxDB after exceeding trhreshold
bool data_to_influxdb = false;
bool data_1st_to_influxdb = false;
bool data_2nd_to_influxdb = false;
bool data_to_mqtt = false;
bool data_1st_to_mqtt = false;
bool data_2nd_to_mqtt = false;

unsigned char mac_adr[6];
char mac_strg[13];
uint64_t raspi_id;
uint64_t zw;
char raspi_id_strg[32];

char influxdb_hostname[64];
char influxdb_user_passwd[64];
char *encoded_user_passwd;
char sc_hostname[64];
static const char host_madavi[] = "api-rrd.madavi.de";
static const char url_madavi[]  = "/data.php";
char madavi_hostname[64];
char madavi_header1[512];
char madavi_msg[msg_size];
char data_4_madavi[512];

char msg[msg_size];
char send_data[msg_size];
char send_data_mqtt[msg_size];

struct Ring_Buffer_sc {
  char b_data[buffer_size_sc][msg_size];
  char b_timestamp[buffer_size_sc][timestamp_size];
  long int b_last_tt[buffer_size_sc];      // last transmission time
  uint16_t b_destination[buffer_size_sc];  // destination of data: 1 = Sensor.Community, 2 = influxDB - has to be set explicitly
  uint16_t b_read;                         // pointer to last data
  uint16_t b_write;                        // pointer to next free buffer element
} buffer_sc = { { {}, {} }, { {}, {} }, {}, {}, 0, 0 };

struct Ring_Buffer_influx {
  char b_data[buffer_size_influx][msg_size];
  char b_timestamp[buffer_size_influx][timestamp_size];
  long int b_last_tt[buffer_size_influx];      // last transmission time
  uint16_t b_destination[buffer_size_influx];  // destination of data: 1 = Sensor.Community, 2 = influxDB - has to be set explicitly
  uint16_t b_read;                             // pointer to last data
  uint16_t b_write;                            // pointer to next free buffer element
} buffer_influx = { { {}, {} }, { {}, {} }, {}, {}, 0, 0 };

char msg_header1[256];
char msg_header2[256];
char msg_header3[] = { "\r\n\r\n" };

char sc_header1[512];
char sc_header2[] = { "\r\n\r\n" };
char sc_msg[1024];

uint16_t length_data_str;
uint16_t length_data_sc_str;
long unsigned int output_size;
char str_of_length_data_str[4];
char str_of_length_data_sc_str[4];

char timestamp_4_log_A_1st[log_size];
char timestamp_4_log_Z_1st[log_size];
char timestamp_4_log_C_1st[log_size];
char timestamp_4_log_A_2nd[log_size];
char timestamp_4_log_Z_2nd[log_size];
char timestamp_4_log_C_2nd[log_size];
char print_string_1st[log_size];
char part_string_1st[log_size];
char print_string_2nd[log_size];
char part_string_2nd[log_size];
char data_4_transmit[msg_size];
char data_4_influxdb_2nd[msg_size];
char data_4_mqtt[msg_size];
char data_4_mqtt_2nd[msg_size];
char value_2_string[16];
char value_2_string_2nd[16];
char value_2_string_wlan[16];

char data_sc_1[128];
char data_sc_2[128];
char data_sc_3[64];
char data_sc_4[16];

char terzen[31][23] = {
  ",DNMS_noise_LAeq20=", ",DNMS_noise_LAeq25=", ",DNMS_noise_LAeq31.5=", ",DNMS_noise_LAeq40=",
  ",DNMS_noise_LAeq50=", ",DNMS_noise_LAeq63=", ",DNMS_noise_LAeq80=", ",DNMS_noise_LAeq100=", ",DNMS_noise_LAeq125=",
  ",DNMS_noise_LAeq160=", ",DNMS_noise_LAeq200=", ",DNMS_noise_LAeq250=", ",DNMS_noise_LAeq315=",
  ",DNMS_noise_LAeq400=", ",DNMS_noise_LAeq500=", ",DNMS_noise_LAeq630=", ",DNMS_noise_LAeq800=",
  ",DNMS_noise_LAeq1000=", ",DNMS_noise_LAeq1250=", ",DNMS_noise_LAeq1600=", ",DNMS_noise_LAeq2000=",
  ",DNMS_noise_LAeq2500=", ",DNMS_noise_LAeq3150=", ",DNMS_noise_LAeq4000=", ",DNMS_noise_LAeq5000=",
  ",DNMS_noise_LAeq6300=", ",DNMS_noise_LAeq8000=", ",DNMS_noise_LAeq10000=", ",DNMS_noise_LAeq12500=",
  ",DNMS_noise_LAeq16000=", ",DNMS_noise_LAeq20000="
};
char terzen_z[31][23] = {
  ",DNMS_noise_LZeq20=", ",DNMS_noise_LZeq25=", ",DNMS_noise_LZeq31.5=", ",DNMS_noise_LZeq40=",
  ",DNMS_noise_LZeq50=", ",DNMS_noise_LZeq63=", ",DNMS_noise_LZeq80=", ",DNMS_noise_LZeq100=", ",DNMS_noise_LZeq125=",
  ",DNMS_noise_LZeq160=", ",DNMS_noise_LZeq200=", ",DNMS_noise_LZeq250=", ",DNMS_noise_LZeq315=",
  ",DNMS_noise_LZeq400=", ",DNMS_noise_LZeq500=", ",DNMS_noise_LZeq630=", ",DNMS_noise_LZeq800=",
  ",DNMS_noise_LZeq1000=", ",DNMS_noise_LZeq1250=", ",DNMS_noise_LZeq1600=", ",DNMS_noise_LZeq2000=",
  ",DNMS_noise_LZeq2500=", ",DNMS_noise_LZeq3150=", ",DNMS_noise_LZeq4000=", ",DNMS_noise_LZeq5000=",
  ",DNMS_noise_LZeq6300=", ",DNMS_noise_LZeq8000=", ",DNMS_noise_LZeq10000=", ",DNMS_noise_LZeq12500=",
  ",DNMS_noise_LZeq16000=", ",DNMS_noise_LZeq20000="
};
char terzen_c[31][23] = {
  ",DNMS_noise_LCeq20=", ",DNMS_noise_LCeq25=", ",DNMS_noise_LCeq31.5=", ",DNMS_noise_LCeq40=",
  ",DNMS_noise_LCeq50=", ",DNMS_noise_LCeq63=", ",DNMS_noise_LCeq80=", ",DNMS_noise_LCeq100=", ",DNMS_noise_LCeq125=",
  ",DNMS_noise_LCeq160=", ",DNMS_noise_LCeq200=", ",DNMS_noise_LCeq250=", ",DNMS_noise_LCeq315=",
  ",DNMS_noise_LCeq400=", ",DNMS_noise_LCeq500=", ",DNMS_noise_LCeq630=", ",DNMS_noise_LCeq800=",
  ",DNMS_noise_LCeq1000=", ",DNMS_noise_LCeq1250=", ",DNMS_noise_LCeq1600=", ",DNMS_noise_LCeq2000=",
  ",DNMS_noise_LCeq2500=", ",DNMS_noise_LCeq3150=", ",DNMS_noise_LCeq4000=", ",DNMS_noise_LCeq5000=",
  ",DNMS_noise_LCeq6300=", ",DNMS_noise_LCeq8000=", ",DNMS_noise_LCeq10000=", ",DNMS_noise_LCeq12500=",
  ",DNMS_noise_LCeq16000=", ",DNMS_noise_LCeq20000="
};

char terzen_2nd[31][27] = {
  ",DNMS_noise_LAeq20_2nd=", ",DNMS_noise_LAeq25_2nd=", ",DNMS_noise_LAeq31.5_2nd=", ",DNMS_noise_LAeq40_2nd=",
  ",DNMS_noise_LAeq50_2nd=", ",DNMS_noise_LAeq63_2nd=", ",DNMS_noise_LAeq80_2nd=", ",DNMS_noise_LAeq100_2nd=", ",DNMS_noise_LAeq125_2nd=",
  ",DNMS_noise_LAeq160_2nd=", ",DNMS_noise_LAeq200_2nd=", ",DNMS_noise_LAeq250_2nd=", ",DNMS_noise_LAeq315_2nd=",
  ",DNMS_noise_LAeq400_2nd=", ",DNMS_noise_LAeq500_2nd=", ",DNMS_noise_LAeq630_2nd=", ",DNMS_noise_LAeq800_2nd=",
  ",DNMS_noise_LAeq1000_2nd=", ",DNMS_noise_LAeq1250_2nd=", ",DNMS_noise_LAeq1600_2nd=", ",DNMS_noise_LAeq2000_2nd=",
  ",DNMS_noise_LAeq2500_2nd=", ",DNMS_noise_LAeq3150_2nd=", ",DNMS_noise_LAeq4000_2nd=", ",DNMS_noise_LAeq5000_2nd=",
  ",DNMS_noise_LAeq6300_2nd=", ",DNMS_noise_LAeq8000_2nd=", ",DNMS_noise_LAeq10000_2nd=", ",DNMS_noise_LAeq12500_2nd=",
  ",DNMS_noise_LAeq16000_2nd=", ",DNMS_noise_LAeq20000_2nd="
};
char terzen_z_2nd[31][27] = {
  ",DNMS_noise_LZeq20_2nd=", ",DNMS_noise_LZeq25_2nd=", ",DNMS_noise_LZeq31.5_2nd=", ",DNMS_noise_LZeq40_2nd=",
  ",DNMS_noise_LZeq50_2nd=", ",DNMS_noise_LZeq63_2nd=", ",DNMS_noise_LZeq80_2nd=", ",DNMS_noise_LZeq100_2nd=", ",DNMS_noise_LZeq125_2nd=",
  ",DNMS_noise_LZeq160_2nd=", ",DNMS_noise_LZeq200_2nd=", ",DNMS_noise_LZeq250_2nd=", ",DNMS_noise_LZeq315_2nd=",
  ",DNMS_noise_LZeq400_2nd=", ",DNMS_noise_LZeq500_2nd=", ",DNMS_noise_LZeq630_2nd=", ",DNMS_noise_LZeq800_2nd=",
  ",DNMS_noise_LZeq1000_2nd=", ",DNMS_noise_LZeq1250_2nd=", ",DNMS_noise_LZeq1600_2nd=", ",DNMS_noise_LZeq2000_2nd=",
  ",DNMS_noise_LZeq2500_2nd=", ",DNMS_noise_LZeq3150_2nd=", ",DNMS_noise_LZeq4000_2nd=", ",DNMS_noise_LZeq5000_2nd=",
  ",DNMS_noise_LZeq6300_2nd=", ",DNMS_noise_LZeq8000_2nd=", ",DNMS_noise_LZeq10000_2nd=", ",DNMS_noise_LZeq12500_2nd=",
  ",DNMS_noise_LZeq16000_2nd=", ",DNMS_noise_LZeq20000_2nd="
};
char terzen_c_2nd[31][27] = {
  ",DNMS_noise_LCeq20_2nd=", ",DNMS_noise_LCeq25_2nd=", ",DNMS_noise_LCeq31.5_2nd=", ",DNMS_noise_LCeq40_2nd=",
  ",DNMS_noise_LCeq50_2nd=", ",DNMS_noise_LCeq63_2nd=", ",DNMS_noise_LCeq80_2nd=", ",DNMS_noise_LCeq100_2nd=", ",DNMS_noise_LCeq125_2nd=",
  ",DNMS_noise_LCeq160_2nd=", ",DNMS_noise_LCeq200_2nd=", ",DNMS_noise_LCeq250_2nd=", ",DNMS_noise_LCeq315_2nd=",
  ",DNMS_noise_LCeq400_2nd=", ",DNMS_noise_LCeq500_2nd=", ",DNMS_noise_LCeq630_2nd=", ",DNMS_noise_LCeq800_2nd=",
  ",DNMS_noise_LCeq1000_2nd=", ",DNMS_noise_LCeq1250_2nd=", ",DNMS_noise_LCeq1600_2nd=", ",DNMS_noise_LCeq2000_2nd=",
  ",DNMS_noise_LCeq2500_2nd=", ",DNMS_noise_LCeq3150_2nd=", ",DNMS_noise_LCeq4000_2nd=", ",DNMS_noise_LCeq5000_2nd=",
  ",DNMS_noise_LCeq6300_2nd=", ",DNMS_noise_LCeq8000_2nd=", ",DNMS_noise_LCeq10000_2nd=", ",DNMS_noise_LCeq12500_2nd=",
  ",DNMS_noise_LCeq16000_2nd=", ",DNMS_noise_LCeq20000_2nd="
};

pthread_t measurement_1st_interval_thread;
pthread_t measurement_1st_interval_timer_thread;
pthread_t measurement_2nd_interval_thread;
pthread_t measurement_2nd_interval_timer_thread;
pthread_t print_ipc_and_log_1st_thread;
pthread_t print_ipc_and_log_2nd_thread;
pthread_t wlan_thread;
pthread_t start_stop_thread;

pthread_cond_t new_data_1st_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t new_data_1st_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t new_1st_measurement_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t new_1st_measurement_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t new_data_2nd_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t new_data_2nd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t new_2nd_measurement_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t new_2nd_measurement_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer_sc_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t buffer_influx_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t dnms_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_1st_measurement_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t start_1st_measurement_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_2nd_measurement_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t start_2nd_measurement_mutex = PTHREAD_MUTEX_INITIALIZER;

char *measurement_2nd_message = (char *)"measurement_2nd_interval_thread is up";
char *print_ipc_and_log_1st_message = (char *)"print ipc and_log 1st thread is up";
char *print_ipc_and_log_2nd_message = (char *)"print ipc and log 2nd thread is up";
char *measurement_2nd_timer_message = (char *)"measurement 2nd_interval_timer_thread is up";
char *wlan_message = (char *)"wlan_thread is up";
char *start_stop_message = (char *)"start_stop_thread is up";
char *measurement_1st_message = (char *)"measurement 1st_interval_thread is up";
char *measurement_1st_timer_message = (char *)"measurement 1st_interval_timer_thread is up";
int16_t i_measurement_1st_interval_thread;
int16_t i_measurement_1st_interval_timer_thread;
int16_t i_measurement_2nd_interval_thread;
int16_t i_measurement_2nd_interval_timer_thread;
int16_t i_print_ipc_and_log_1st_thread;
int16_t i_print_ipc_and_log_2nd_thread;
int16_t i_wlan_thread;
int16_t i_start_stop_thread;

void *measurement_2nd_interval_function(void *ptr);
void *measurement_2nd_interval_timer_function(void *ptr);
void *print_ipc_and_log_1st_function(void *ptr);
void *print_ipc_and_log_2nd_function(void *ptr);
void *wlan_function(void *ptr);
void *measurement_1st_interval_function(void *ptr);
void *measurement_1st_interval_timer_function(void *ptr);
void *start_stop_function(void *ptr);

char dnms_version[DNMS_MAX_VERSION_LEN + 1];
uint16_t data_ready;
uint16_t data_ready_2nd;
bool dnms_error = false;
bool dnms_error_2nd = false;
struct dnms_measurements dnms_values;
float last_value_dnms_laeq = 0.0;
float last_value_dnms_lamin = 0.0;
float last_value_dnms_lamax = 0.0;
float last_value_dnms_spectrum[31] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                       0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                       0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                       0.0 };
float last_value_dnms_laeq_2nd = 0.0;
float last_value_dnms_lamin_2nd = 0.0;
float last_value_dnms_lamax_2nd = 0.0;
float last_value_dnms_spectrum_2nd[31] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                           0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                           0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                           0.0 };

float last_value_dnms_lzeq = 0.0;
float last_value_dnms_lzmin = 0.0;
float last_value_dnms_lzmax = 0.0;
float last_value_dnms_z_spectrum[31] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                         0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                         0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                         0.0 };
float last_value_dnms_lzeq_2nd = 0.0;
float last_value_dnms_lzmin_2nd = 0.0;
float last_value_dnms_lzmax_2nd = 0.0;
float last_value_dnms_z_spectrum_2nd[31] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                             0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                             0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                             0.0 };

float last_value_dnms_lceq = 0.0;
float last_value_dnms_lcmin = 0.0;
float last_value_dnms_lcmax = 0.0;
float last_value_dnms_c_spectrum[31] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                         0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                         0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                         0.0 };
float last_value_dnms_lceq_2nd = 0.0;
float last_value_dnms_lcmin_2nd = 0.0;
float last_value_dnms_lcmax_2nd = 0.0;
float last_value_dnms_c_spectrum_2nd[31] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                             0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                             0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                             0.0 };

struct timeval tv;
struct timeval tv_1st;
struct timeval tv_2nd;
/* Timer-fire timestamps: set by each timer thread right after clock_nanosleep
   returns so the measurement threads can use the intended fire time instead of
   the post-I2C gettimeofday() which is 3-8ms later on Linux. */
static volatile struct timeval g_timer_fire_1st = {0, 0};
static volatile struct timeval g_timer_fire_2nd = {0, 0};
struct timeval mqtt_error_tv;
unsigned long long ns_mqtt;
unsigned long long ns_1st;
unsigned long long ns_2nd;
char t_string_1st[80];
char t_string_2nd[80];

char print_and_log_1st_A_buf[256];
char print_and_log_1st_Z_buf[256];
char print_and_log_1st_C_buf[256];
char print_and_log_2nd_A_buf[256];
char print_and_log_2nd_Z_buf[256];
char print_and_log_2nd_C_buf[256];

struct timezone tz;

time_t time_wlan;
time_t time_print_1;
time_t time_print_2;
struct tm *ts_wlan;
struct tm *ts_print_1;
struct tm *ts_print_2;
struct tm *ts_mqtt_1;
struct tm *ts_mqtt_2;
time_t sekunden;
time_t minuten;
time_t rawtime_1st;
time_t rawtime_2nd;
time_t rawtime_mqtt;
char zeit_string_mqtt[80];
char zeit_string_print[80];
char zeit_string_wlan[80];
char timestamp_1st[timestamp_size];
char timestamp_2nd[timestamp_size];
char send_timestamp[timestamp_size];
long int send_last_tt;
uint16_t destination;

char mqtt_error_timestamp[timestamp_size];

struct timeval t0, t1, t2;
long long elapsedTime;
long int last_sc_transmission_time;
long int last_influxdb_transmission_time;
long int last_mqtt_transmission_time;

SSL_CTX *ctx;
SSL *ssl;
BIO *bio;
const SSL_METHOD *method;

char name[128];

bool print_flag = false;

struct mosquitto *mosq;
int rc;
char mqtt_topic[128];

int dnms_pipe_fd;
int start_stop_pipe_fd;
#define start_stop_buffer_size 256
char start_stop_buffer[start_stop_buffer_size];

char data_logging_file_name[64];

int wifi_signal_strength;
char string_wifi_signal_strength[8];
int sockfd;
struct iwreq wreq;
struct iw_statistics stats;

int ret;

bool read_laeq_spec_1st = false;
bool read_lzeq_1st = false;
bool read_lzeq_spec_1st = false;
bool read_lceq_1st = false;
bool read_lceq_spec_1st = false;

bool read_laeq_spec_2nd = false;
bool read_lzeq_2nd = false;
bool read_lzeq_spec_2nd = false;
bool read_lceq_2nd = false;
bool read_lceq_spec_2nd = false;



/* ******************************************************************************************************************** */
int open_socket_to_get_signal_strength(void) {
	// Open a socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        return -100;
    }

    // Set interface name
    strncpy(wreq.ifr_name, (char *) interface_name, IFNAMSIZ);
    wreq.u.data.pointer = &stats;
    wreq.u.data.length = sizeof(stats);
//    sleep(1);
    return 0;
}


/* ******************************************************************************************************************** */
int get_signal_strength(void) {
	// Get wireless statistics
    if (ioctl(sockfd, SIOCGIWSTATS, &wreq) == -1) {
        return -299;
    }
    // Signal level in dBm (convert from arbitrary units if necessary)
    return stats.qual.level - 256;	
}

/* ******************************************************************************************************************** */
void sprintbig(char * sz, uint64_t value) {
    const int NUM_DIGITS    = log10(value) + 1;

    sz[NUM_DIGITS] =  0;
    for ( size_t i = NUM_DIGITS; i--; value /= 10)
    {
        sz[i] = '0' + (value % 10);
    }

}

/* ******************************************************************************************************************** */
void open_dnms(void) {

  printf("Open i2c bus: ");
  if (ret = open_i2c_dnms((char *)dev_name_dnms) == 0) {
	printf("OK\n\n");
  } else {
	printf("Error open I²C return:, %3d\n", ret);
  }
  /* Reset DNMS Sensor */
  printf("Reset DNMS: ");
  
  for (int i = 0; i < 20; i++) {
	 if ( ret = dnms_reset() == 0) {
		    printf("OK\n");
		    break;
	 } else {
		usleep (5000);
	 }
	 if (ret != 0) {
		printf("Error reset DNMS, check it!\n");
	}	 
  }
}

/* ******************************************************************************************************************** */
void close_dnms(void) {
  if (close_i2c_dnms() == 0) {
    // nothing to do
  } else {
    printf("Error closing i2c connection to DNMS!\n");
  }
}

/* ******************************************************************************************************************** */
int buffer_in_sc(char *msg, char *timestamp, uint16_t dest) {
  if ((buffer_sc.b_write + 1 == buffer_sc.b_read) || (buffer_sc.b_read == 0 && buffer_sc.b_write + 1 == buffer_size_sc)) {
    return buffer_fail;
  }
  strcpy(buffer_sc.b_data[buffer_sc.b_write], msg);
  if (dest == dest_influxdb) {
    strcpy(buffer_sc.b_timestamp[buffer_sc.b_write], timestamp);
  }
  buffer_sc.b_destination[buffer_sc.b_write] = dest;

  buffer_sc.b_write++;
  if (buffer_sc.b_write >= buffer_size_sc) {
    buffer_sc.b_write = 0;
  }

  return buffer_success;
}

/* ******************************************************************************************************************** */
int buffer_out_sc(char *msg, char *timestamp, long int *last_tt, uint16_t *dest) {
  uint16_t d;

  if (buffer_sc.b_read == buffer_sc.b_write) {
    pthread_mutex_unlock(&buffer_sc_mutex);
    return buffer_fail;  // buffer empty
  }
  strcpy(msg, buffer_sc.b_data[buffer_sc.b_read]);
  d = buffer_sc.b_destination[buffer_sc.b_read];
  *dest = d;

  if (d == dest_influxdb)  // is destination influxdb, than get the timestamp is
  {
    strcpy(timestamp, buffer_sc.b_timestamp[buffer_sc.b_read]);
  }
  *last_tt = buffer_sc.b_last_tt[buffer_sc.b_read];
  buffer_sc.b_read++;
  if (buffer_sc.b_read >= buffer_size_sc) {
    buffer_sc.b_read = 0;
  }

  return buffer_success;
}

/* ******************************************************************************************************************** */
int buffer_in_influx(char *msg, char *timestamp, uint16_t dest) {
  if ((buffer_influx.b_write + 1 == buffer_influx.b_read) || (buffer_influx.b_read == 0 && buffer_influx.b_write + 1 == buffer_size_influx)) {
    return buffer_fail;
  }
  strcpy(buffer_influx.b_data[buffer_influx.b_write], msg);
  if (dest == dest_influxdb || dest == dest_mqtt) {
    strcpy(buffer_influx.b_timestamp[buffer_influx.b_write], timestamp);
  }
  buffer_influx.b_destination[buffer_influx.b_write] = dest;

  buffer_influx.b_write++;
  if (buffer_influx.b_write >= buffer_size_influx) {
    buffer_influx.b_write = 0;
  }

  return buffer_success;
}

/* ******************************************************************************************************************** */
void buffer_in_influx_last_tt(long int last_tt) {
  buffer_influx.b_last_tt[buffer_influx.b_read] = last_tt;
}

/* ******************************************************************************************************************** */
int buffer_out_influx(char *msg, char *timestamp, long int *last_tt, uint16_t *dest) {
  uint16_t d;

  if (buffer_influx.b_read == buffer_influx.b_write) {
    pthread_mutex_unlock(&buffer_influx_mutex);
    return buffer_fail;  // buffer empty
  }
  strcpy(msg, buffer_influx.b_data[buffer_influx.b_read]);
  d = buffer_influx.b_destination[buffer_influx.b_read];
  *dest = d;

  if (d == dest_influxdb || d == dest_mqtt)
  {
    strcpy(timestamp, buffer_influx.b_timestamp[buffer_influx.b_read]);
  }
  *last_tt = buffer_influx.b_last_tt[buffer_influx.b_read];
  buffer_influx.b_read++;
  if (buffer_influx.b_read >= buffer_size_influx) {
    buffer_influx.b_read = 0;
  }
  return buffer_success;
}


/* ******************************************************************************************************************** */
/* Callback called when the client receives a CONNACK message from the broker. */
void on_mqtt_connect(struct mosquitto *mosq, void *obj, int reason_code) {
  /* Print out the connection result. mosquitto_connack_string() produces an
	 * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
	 * clients is mosquitto_reason_string().
	 */
  //	printf("MQTT on_connect: %s\n", mosquitto_connack_string(reason_code));
  if (reason_code != 0) {
    /* If the connection fails for any reason, we don't want to keep on
		 * retrying in this example, so disconnect. Without this, the client
		 * will attempt to reconnect. */
    mosquitto_disconnect(mosq);
  }
  /* You may wish to set a flag here to indicate to your application that the
	 * client is now connected. */
}


/* ******************************************************************************************************************** */
/* set priority and scheduling of thread */
static void setprio(pthread_t id, int policy, int prio) {
  struct sched_param param;
  param.sched_priority = prio;
  if ((pthread_setschedparam(id, policy, &param)) != 0) {
    //	report_and_exit("change of thread priority and scheduling policy not possible");
    printf("problem to set priority and scheduling policy id: %ld\n", id);
  }
}


/* ******************************************************************************************************************** */
/* get thread info and print the info */
static void getprio(pthread_t id, const char *name) {
  int policy;
  struct sched_param param;
  printf("thread: %s,", name);
  printf(" id: %ld,", id);
  if ((pthread_getschedparam(id, &policy, &param)) == 0) {
    printf(" scheduling policy: ");
    switch (policy) {
      case SCHED_OTHER: printf("SCHED_OTHER,"); break;
      case SCHED_FIFO: printf("SCHED_FIFO, "); break;
      case SCHED_RR: printf("SCHED_RR, "); break;
      default: printf("unknown, "); break;
    }
    printf("priority: %d", param.sched_priority);
  }
  printf("\n");
}


/* ******************************************************************************************************************** */
/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void on_mqtt_publish(struct mosquitto *mosq, void *obj, int mid) {
  /* endtime of WLAN transmission to influxdb */
  gettimeofday(&t2, NULL);
  /* Berechne die verbrauchte Zeit in Microsekunden */
  elapsedTime = ((t2.tv_sec * 1000000) + t2.tv_usec) - ((t0.tv_sec * 1000000) + t0.tv_usec);
  last_influxdb_transmission_time = elapsedTime / 1000;
  if (last_influxdb_transmission_time == 0) {
    last_influxdb_transmission_time = 1;
  }
  pthread_mutex_lock(&buffer_influx_mutex);
  buffer_in_influx_last_tt(last_influxdb_transmission_time);
  pthread_mutex_unlock(&buffer_influx_mutex);
}


/* ******************************************************************************************************************** */
void prep_msg_header_influxdb(void) {
  bzero((char *)&msg_header1, sizeof(msg_header1));
  strcat(msg_header1, "POST ");
  strcat(msg_header1, influxdb_pfad);
  strcat(msg_header1, " HTTP/1.1\r\nHost: ");
  strcat(msg_header1, influxdb_server);
  strcat(msg_header1, ":");
  strcat(msg_header1, influxdb_port);
  strcat(msg_header1, "\r\nUser-Agent: ");
  strcat(msg_header1, firmware_version);
  strcat(msg_header1, "/raspi-");
  strcat(msg_header1, raspi_id_strg);
  printf("raspi_id: raspi-%s\n", raspi_id_strg);
  strcat(msg_header1, "/");
  strcat(msg_header1, mac_strg);
  strcat(msg_header1, "\r\nAccept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\nAuthorization: Basic ");

  bzero((char *)&msg_header2, sizeof(msg_header2));
  strcat(msg_header2, "\r\nConnection: close\r\nContent-Type: application/x-www-form-urlencoded\r\nX-Sensor: raspi-");
  strcat(msg_header2, raspi_id_strg);
  strcat(msg_header2, "\r\nX-MAC-ID: raspi-");
  strcat(msg_header2, mac_strg);
  strcat(msg_header2, "\r\nContent-Length: ");

  bzero((char *)&influxdb_hostname, sizeof(influxdb_hostname));
  strcat(influxdb_hostname, influxdb_server);
  strcat(influxdb_hostname, ":");
  strcat(influxdb_hostname, influxdb_port);

  bzero((char *)&influxdb_user_passwd, sizeof(influxdb_user_passwd));
  strcat(influxdb_user_passwd, influxdb_user);
  strcat(influxdb_user_passwd, ":");
  strcat(influxdb_user_passwd, influxdb_passwort);
}


/* ******************************************************************************************************************** */
void prep_msg_header_sc(void) {
  bzero((char *)&sc_header1, sizeof(sc_header1));
  strcat(sc_header1, "POST ");
  strcat(sc_header1, url_sc);
  strcat(sc_header1, " HTTP/1.1\r\nHost: ");
  strcat(sc_header1, host_sc);
  strcat(sc_header1, "\r\nUser-Agent: ");
  strcat(sc_header1, firmware_version);
  strcat(sc_header1, "/");
  strcat(sc_header1, raspi_id_strg);
  strcat(sc_header1, "/");
  strcat(sc_header1, mac_strg);
  strcat(sc_header1, "\r\nAccept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\nConnection: close\r\n");
  strcat(sc_header1, "Content-Type: application/json\r\nX-Sensor: raspi-");
  strcat(sc_header1, raspi_id_strg);
  strcat(sc_header1, "\r\nX-MAC-ID: raspi-");
  strcat(sc_header1, mac_strg);
  strcat(sc_header1, "\r\nX-PIN: ");
  strcat(sc_header1, DNMS_API_PIN);
  strcat(sc_header1, "\r\nContent-Length: ");

  bzero((char *)&sc_hostname, sizeof(sc_hostname));
  strcat(sc_hostname, host_sc);
  strcat(sc_hostname, sc_transmit_https ? ":443" : ":80");

  bzero((char *)&madavi_hostname, sizeof(madavi_hostname));
  strcat(madavi_hostname, host_madavi);
  strcat(madavi_hostname, ":80");

  bzero((char *)&madavi_header1, sizeof(madavi_header1));
  strcat(madavi_header1, "POST ");
  strcat(madavi_header1, url_madavi);
  strcat(madavi_header1, " HTTP/1.1\r\nHost: ");
  strcat(madavi_header1, host_madavi);
  strcat(madavi_header1, "\r\nUser-Agent: ");
  strcat(madavi_header1, firmware_version);
  strcat(madavi_header1, "/");
  strcat(madavi_header1, raspi_id_strg);
  strcat(madavi_header1, "/");
  strcat(madavi_header1, mac_strg);
  strcat(madavi_header1, "\r\nAccept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\nConnection: close\r\n");
  strcat(madavi_header1, "Content-Type: application/json\r\nX-Sensor: raspi-");
  strcat(madavi_header1, raspi_id_strg);
  strcat(madavi_header1, "\r\nX-MAC-ID: raspi-");
  strcat(madavi_header1, mac_strg);
  strcat(madavi_header1, "\r\nContent-Length: ");
}


/* ******************************************************************************************************************** */
void prep_data_sc(void) {
  bzero((char *)&data_sc_1, sizeof(data_sc_1));
  strcat(data_sc_1, "{\"software_version\": \"");
  strcat(data_sc_1, firmware_version);
  strcat(data_sc_1, "\", \"sensordatavalues\":[{\"value_type\":\"noise_LAeq\",\"value\":\"");

  bzero((char *)&data_sc_2, sizeof(data_sc_2));
  strcat(data_sc_2, "\"},{\"value_type\":\"noise_LA_min\",\"value\":\"");

  bzero((char *)&data_sc_3, sizeof(data_sc_3));
  strcat(data_sc_3, "\"},{\"value_type\":\"noise_LA_max\",\"value\":\"");

  bzero((char *)&data_sc_4, sizeof(data_sc_4));
  strcat(data_sc_4, "\"}]}");
}


/* ******************************************************************************************************************** */
int get_mac(char *interface, char *mac_address) {
  int fd;
  struct ifreq ifr;
  char *iface = interface;
  unsigned char *mac = NULL;

  memset(&ifr, 0, sizeof(ifr));
  fd = socket(AF_INET, SOCK_STREAM, 0);
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);

  if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr)) {
    mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    close(fd);
    for (int i = 0; i < 6; i++) {
      mac_address[i] = mac[i];
    }
    return 0;
  } else {
    return 1;
  }
}


/* ******************************************************************************************************************** */
int get_ip(char *interface, char *ip_address) {
  int fd;
  struct ifreq ifr;

  memset(&ifr, 0, sizeof(ifr));
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

  if (0 == ioctl(fd, SIOCGIFADDR, &ifr)) {
    close(fd);
    strncpy(ip_address,
            inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr),
            INET_ADDRSTRLEN - 1);
    return 0;
  } else {
    close(fd);
    return 1;
  }
}


/* ******************************************************************************************************************** */
void report_and_exit(const char *msg) {
  perror(msg);
  ERR_print_errors_fp(stderr);
  exit(-1);
}


/* Non-blocking connect with timeout for plain HTTP BIOs.
   Returns > 0 on success, <= 0 on failure or timeout. */
/* Forward declaration — defined further below */
static int bio_do_connect_timed(BIO *b, int timeout_secs);

/* ── 1/3-octave band centre-frequency JSON keys ──────────────────────────── */
static const char *BAND_HZ_JSON[31] = {
  "20","25","31_5","40","50","63","80","100","125","160",
  "200","250","315","400","500","630","800","1000","1250","1600",
  "2000","2500","3150","4000","5000","6300","8000","10000","12500","16000","20000"
};

/* ── Build airrohr-compatible JSON of current measurement values ─────────── *
 * Returns a malloc'd NUL-terminated string; caller must free().             */
char *build_data_json(int include_spectrum, int include_1st, int include_2nd) {
    /* ISO 8601 local timestamp with millisecond resolution */
    struct timespec tspec;
    clock_gettime(CLOCK_REALTIME, &tspec);
    struct tm tm_buf;
    localtime_r(&tspec.tv_sec, &tm_buf);
    char ts[28];
    strftime(ts, 20, "%Y-%m-%dT%H:%M:%S", &tm_buf);
    snprintf(ts + 19, sizeof(ts) - 19, ".%03ld", tspec.tv_nsec / 1000000L);

    /* Growing output buffer */
    size_t cap = 16384;
    char  *buf = malloc(cap);
    if (!buf) return NULL;
    size_t len = 0;

#define J_APPEND(fmt, ...) do { \
    int _n = snprintf(buf + len, cap - len, fmt, ##__VA_ARGS__); \
    if (_n < 0) { free(buf); return NULL; } \
    if ((size_t)_n >= cap - len) { \
        cap = cap * 2 + (size_t)_n + 256; \
        char *_r = realloc(buf, cap); \
        if (!_r) { free(buf); return NULL; } \
        buf = _r; \
        snprintf(buf + len, cap - len, fmt, ##__VA_ARGS__); \
    } \
    len += (size_t)_n; \
} while(0)

    bool first = true;
    /* helper macros for sensordatavalues fields */
#define SDV(type, fmt, val) do { \
    J_APPEND("%s{\"value_type\":\"%s\",\"value\":\"" fmt "\"}", \
             first ? "" : ",", (type), (val)); \
    first = false; \
} while(0)
#define SDV1(type, val)  SDV(type, "%.1f", (double)(val))
#define SDV2(type, val)  SDV(type, "%.2f", (double)(val))
#define SDV0(type, val)  SDV(type, "%.0f", (double)(val))

    J_APPEND("{\"software_version\":\"%s\","
             "\"sensor_id\":\"raspi-%s\","
             "\"timestamp\":\"%s\","
             "\"sensordatavalues\":[",
             firmware_version, raspi_id_strg, ts);

    /* 1st interval DNMS — only when include_1st is set */
    if (include_1st && enable_1st_interval) {
        SDV1("DNMS_LAeq",  last_value_dnms_laeq);
        SDV1("DNMS_LAmin", last_value_dnms_lamin);
        SDV1("DNMS_LAmax", last_value_dnms_lamax);
        SDV1("DNMS_LZeq",  last_value_dnms_lzeq);
        SDV1("DNMS_LZmin", last_value_dnms_lzmin);
        SDV1("DNMS_LZmax", last_value_dnms_lzmax);
        SDV1("DNMS_LCeq",  last_value_dnms_lceq);
        SDV1("DNMS_LCmin", last_value_dnms_lcmin);
        SDV1("DNMS_LCmax", last_value_dnms_lcmax);
        if (include_spectrum) {
            char bt[40];
            for (int i = 0; i < 31; i++) {
                snprintf(bt, sizeof(bt), "DNMS_LA_spectrum_%s", BAND_HZ_JSON[i]);
                SDV1(bt, last_value_dnms_spectrum[i]);
            }
            for (int i = 0; i < 31; i++) {
                snprintf(bt, sizeof(bt), "DNMS_LZ_spectrum_%s", BAND_HZ_JSON[i]);
                SDV1(bt, last_value_dnms_z_spectrum[i]);
            }
            for (int i = 0; i < 31; i++) {
                snprintf(bt, sizeof(bt), "DNMS_LC_spectrum_%s", BAND_HZ_JSON[i]);
                SDV1(bt, last_value_dnms_c_spectrum[i]);
            }
        }
    }

    /* 2nd interval DNMS — only when include_2nd is set */
    if (include_2nd && enable_2nd_interval) {
        SDV1("DNMS_2nd_LAeq",  last_value_dnms_laeq_2nd);
        SDV1("DNMS_2nd_LAmin", last_value_dnms_lamin_2nd);
        SDV1("DNMS_2nd_LAmax", last_value_dnms_lamax_2nd);
        SDV1("DNMS_2nd_LZeq",  last_value_dnms_lzeq_2nd);
        SDV1("DNMS_2nd_LZmin", last_value_dnms_lzmin_2nd);
        SDV1("DNMS_2nd_LZmax", last_value_dnms_lzmax_2nd);
        SDV1("DNMS_2nd_LCeq",  last_value_dnms_lceq_2nd);
        SDV1("DNMS_2nd_LCmin", last_value_dnms_lcmin_2nd);
        SDV1("DNMS_2nd_LCmax", last_value_dnms_lcmax_2nd);
        if (include_spectrum) {
            char bt[44];
            for (int i = 0; i < 31; i++) {
                snprintf(bt, sizeof(bt), "DNMS_2nd_LA_spectrum_%s", BAND_HZ_JSON[i]);
                SDV1(bt, last_value_dnms_spectrum_2nd[i]);
            }
            for (int i = 0; i < 31; i++) {
                snprintf(bt, sizeof(bt), "DNMS_2nd_LZ_spectrum_%s", BAND_HZ_JSON[i]);
                SDV1(bt, last_value_dnms_z_spectrum_2nd[i]);
            }
            for (int i = 0; i < 31; i++) {
                snprintf(bt, sizeof(bt), "DNMS_2nd_LC_spectrum_%s", BAND_HZ_JSON[i]);
                SDV1(bt, last_value_dnms_c_spectrum_2nd[i]);
            }
        }
    }

    /* Environmental sensors — only included with 1st interval (that is when they are read) */
    if (include_1st) {
        if (enable_sds011 && sds011_ok) {
            SDV1("SDS011_PM10",  last_value_SDS_P1);
            SDV1("SDS011_PM2_5", last_value_SDS_P2);
        }
        if (enable_sps30 && sps30_ok) {
            SDV2("SPS30_PM1_0",   last_value_SPS30_P0);
            SDV2("SPS30_PM2_5",   last_value_SPS30_P2);
            SDV2("SPS30_PM4_0",   last_value_SPS30_P4);
            SDV2("SPS30_PM10",    last_value_SPS30_P1);
            SDV2("SPS30_NC0_5",   last_value_SPS30_N05);
            SDV2("SPS30_NC1_0",   last_value_SPS30_N1);
            SDV2("SPS30_NC2_5",   last_value_SPS30_N25);
            SDV2("SPS30_NC4_0",   last_value_SPS30_N4);
            SDV2("SPS30_NC10",    last_value_SPS30_N10);
            SDV2("SPS30_TypSize", last_value_SPS30_TS);
        }
        if (enable_sht3x && sht3x_ok) {
            SDV2("SHT3x_T", last_value_SHT3X_T);
            SDV2("SHT3x_H", last_value_SHT3X_H);
        }
        if (enable_sen5x && sen5x_ok) {
            SDV2("SEN5x_PM1_0", last_value_SEN5X_P0);
            SDV2("SEN5x_PM2_5", last_value_SEN5X_P2);
            SDV2("SEN5x_PM4_0", last_value_SEN5X_P4);
            SDV2("SEN5x_PM10",  last_value_SEN5X_P1);
            SDV2("SEN5x_T",     last_value_SEN5X_T);
            SDV2("SEN5x_H",     last_value_SEN5X_H);
            SDV1("SEN5x_VOC",   last_value_SEN5X_VOC);
            SDV1("SEN5x_NOX",   last_value_SEN5X_NOX);
        }
        if (enable_bme280 && bme280_ok) {
            SDV2("BME280_T", last_value_BME280_T);
            SDV2("BME280_P", last_value_BME280_P);
            if (bme280_has_humidity)
                SDV2("BME280_H", last_value_BME280_H);
        }
        if (enable_scd30 && scd30_ok && last_value_SCD30_CO2 >= 0.0f) {
            SDV0("SCD30_CO2", last_value_SCD30_CO2);
            SDV2("SCD30_T",   last_value_SCD30_T);
            SDV2("SCD30_H",   last_value_SCD30_H);
        }
        if (enable_scd4x && scd4x_ok && last_value_SCD4X_CO2 >= 0.0f) {
            SDV0("SCD4x_CO2", last_value_SCD4X_CO2);
            SDV2("SCD4x_T",   last_value_SCD4X_T);
            SDV2("SCD4x_H",   last_value_SCD4X_H);
        }
        if (enable_wifi_signal_strength_influxdb) {
            J_APPEND("%s{\"value_type\":\"signal\",\"value\":\"%d\"}",
                     first ? "" : ",", wifi_signal_strength);
            first = false;
        }
    }

    J_APPEND("]}");

#undef SDV0
#undef SDV1
#undef SDV2
#undef SDV
#undef J_APPEND

    (void)first;
    return buf;
}

/* ── Send JSON payload to the custom API endpoint via HTTP(S) POST ───────── */
static void send_to_custom_api(int include_spectrum, int include_1st, int include_2nd) {
    if (!custom_api_enable || custom_api_server[0] == '\0') return;
    if (!enable_wlan_or_lan) return;

    char *json = build_data_json(include_spectrum, include_1st, include_2nd);
    if (!json) return;

    const char *port_str = custom_api_port[0] ? custom_api_port
                                              : (custom_api_https ? "443" : "80");
    /* Build "server:port" for BIO_set_conn_hostname */
    snprintf(custom_api_hostname, sizeof(custom_api_hostname),
             "%s:%s", custom_api_server, port_str);

    /* Build the HTTP POST request */
    size_t json_len = strlen(json);
    size_t req_size = 256 + strlen(custom_api_path) + strlen(custom_api_server)
                      + strlen(firmware_version) + strlen(raspi_id_strg)
                      + json_len;
    char *req = (char *)malloc(req_size);
    if (!req) { free(json); return; }
    snprintf(req, req_size,
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: %s\r\n"
        "Content-Type: application/json\r\n"
        "X-Sensor: raspi-%s\r\n"
        "Connection: close\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
        custom_api_path[0] ? custom_api_path : "/data",
        custom_api_server,
        firmware_version,
        raspi_id_strg,
        json_len,
        json);
    free(json);

    if (custom_api_https) {
        /* HTTPS using a local SSL context (does not touch the global ctx/bio/ssl) */
        SSL_CTX *ssl_ctx = SSL_CTX_new(TLS_client_method());
        if (!ssl_ctx) { free(req); return; }
        SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION);
        SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, NULL);

        BIO *ssl_bio = BIO_new_ssl_connect(ssl_ctx);
        if (!ssl_bio) { SSL_CTX_free(ssl_ctx); free(req); return; }

        /* SNI — strip port from hostname */
        char sni[128];
        snprintf(sni, sizeof(sni), "%s", custom_api_server);
        char *colon = strchr(sni, ':');
        if (colon) *colon = '\0';
        SSL *ssl_obj = NULL;
        BIO_get_ssl(ssl_bio, &ssl_obj);
        if (ssl_obj) {
            SSL_set_mode(ssl_obj, SSL_MODE_AUTO_RETRY);
            SSL_set_tlsext_host_name(ssl_obj, sni);
        }

        BIO_set_conn_hostname(ssl_bio, custom_api_hostname);
        int res_h = BIO_do_connect(ssl_bio);
        if (res_h > 0) {
            BIO_puts(ssl_bio, req);
            usleep(50000);
        } else {
            printf("[custom_api] HTTPS connect to %s failed\n", custom_api_hostname);
        }
        BIO_free_all(ssl_bio);
        SSL_CTX_free(ssl_ctx);
    } else {
        /* Plain HTTP */
        BIO *bio = NULL;
        for (unsigned i = 0; i < 30; i++) {
            if ((bio = BIO_new(BIO_s_connect())) != NULL) break;
            usleep(2000);
        }
        if (!bio) {
            printf("[custom_api] BIO_new failed\n");
            free(req);
            return;
        }

        BIO_set_conn_hostname(bio, custom_api_hostname);
        int res_p = bio_do_connect_timed(bio, 5);
        if (res_p > 0) {
            BIO_puts(bio, req);
            usleep(50000);
        } else {
            printf("[custom_api] connect to %s failed\n", custom_api_hostname);
        }
        BIO_free_all(bio);
    }
    free(req);
}

static int bio_do_connect_timed(BIO *b, int timeout_secs) {
  int fd = -1;

  BIO_set_nbio(b, 1);
  BIO_do_connect(b);  /* starts non-blocking connect; returns immediately */

  if (BIO_get_fd(b, &fd) < 0 || fd < 0) {
    BIO_set_nbio(b, 0);
    return -1;
  }

  fd_set wfds;
  FD_ZERO(&wfds);
  FD_SET(fd, &wfds);
  struct timeval tv;
  tv.tv_sec  = timeout_secs;
  tv.tv_usec = 0;

  if (select(fd + 1, NULL, &wfds, NULL, &tv) <= 0) {
    BIO_set_nbio(b, 0);
    return -1;  /* timeout or select error */
  }

  int err = 0;
  socklen_t slen = sizeof(err);
  getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &slen);

  BIO_set_nbio(b, 0);  /* restore blocking for subsequent send/recv */
  return (err == 0) ? 1 : -1;
}

/* ******************************************************************************************************************** */
void init_ssl() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
  SSL_library_init();
#else
  OPENSSL_init_ssl(0, NULL);
#endif

  SSL_load_error_strings();

  method = TLS_client_method();
  if (NULL == method) {
    report_and_exit("TLS_client_method...");
  }
  ctx = SSL_CTX_new(method);
  if (NULL == ctx) {
    report_and_exit("SSL_CTX_new...");
  }
  SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
  SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
}


/* ******************************************************************************************************************** */
bool secure_connect(const char *hostname) {
  int res;
  char sni_host[128];

  init_ssl();

  bio = BIO_new_ssl_connect(ctx);
  if (NULL == bio) {
    printf("HTTPS: BIO_new_ssl_connect failed\n");
    SSL_CTX_free(ctx);
    bio = NULL;
    return false;
  }
  ssl = NULL;

  /* link bio channel, SSL session, and server endpoint */

  sprintf(name, "%s", hostname);
  BIO_get_ssl(bio, &ssl);                 /* session */
  SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY); /* robustness */

  /* set SNI hostname (strip port if present) */
  snprintf(sni_host, sizeof(sni_host), "%s", hostname);
  char *colon = strchr(sni_host, ':');
  if (colon) *colon = '\0';
  SSL_set_tlsext_host_name(ssl, sni_host);

  BIO_set_conn_hostname(bio, name); /* prepare to connect */

  for (unsigned i = 0; i < 30; i++) {
    /* try to connect */
    res = BIO_do_connect(bio);
    if (res > 0) {
      break;
    }
    usleep(2000);
  }
  if (res <= 0) {
    printf("HTTPS: connection to %s failed\n", hostname);
    SSL_CTX_free(ctx);
    BIO_free(bio);
    bio = NULL;
    return false;
  }
  /* secure connection established */
  return true;
}


/* ******************************************************************************************************************** */
/* Print actual time to stdout  */
void print_time() {
  time_t t;
  struct tm ts_local;
  time(&t);
  localtime_r(&t, &ts_local);
  strftime(zeit_string_print, 80, fmt_dt, &ts_local);
  printf("%s ", zeit_string_print);
}



/* ******************************************************************************************************************** */
/* Timer for 2nd interval to start measurement thread */
void *measurement_2nd_interval_timer_function(void *ptr) {
  setprio(pthread_self(), SCHED_RR, prio_2nd_timer);
  if (output_thread_info) {
    getprio(pthread_self(), "measurement_2nd_interval_timer_thread");
  }

  // wait for start
  pthread_mutex_lock(&start_2nd_measurement_mutex);
  pthread_cond_wait(&start_2nd_measurement_cond, &start_2nd_measurement_mutex);
  pthread_mutex_unlock(&start_2nd_measurement_mutex);

  for (;;) {
    /* Re-anchor to CLOCK_REALTIME each cycle so NTP corrections on either
       machine converge inter-device offset over time.  Both Raspis share
       the same measurement_start_time (integer second), so they target
       identical wall-clock boundaries; residual inter-device offset is
       bounded by NTP accuracy rather than growing with independent
       MONOTONIC drift. */
    struct timespec now_rt;
    clock_gettime(CLOCK_REALTIME, &now_rt);
    int64_t now_ms    = (int64_t)now_rt.tv_sec * 1000 + now_rt.tv_nsec / 1000000;
    int64_t start_ms  = (int64_t)measurement_start_time * 1000;
    int64_t n         = (now_ms - start_ms) / (int64_t)measurement_2nd_interval_ms;
    int64_t target_ms = start_ms + (n + 1) * (int64_t)measurement_2nd_interval_ms;
    struct timespec target_rt;
    target_rt.tv_sec  = (time_t)(target_ms / 1000);
    target_rt.tv_nsec = (long)(target_ms % 1000) * 1000000L;
    while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &target_rt, NULL) == EINTR)
      ;
    {
      struct timespec actual_rt;
      clock_gettime(CLOCK_REALTIME, &actual_rt);
      g_timer_fire_2nd.tv_sec  = actual_rt.tv_sec;
      g_timer_fire_2nd.tv_usec = (suseconds_t)(actual_rt.tv_nsec / 1000);
    }

    if (interval_1st_active && measurement_2nd_interval_ms >= measurement_1st_interval_ms) {
      /* 2nd >= 1st: let the 1st measurement thread trigger 2nd after its own
         measurement completes, so CALCULATE_LEQ always precedes CALCULATE_LEQ_2nd. */
      interval_2nd_due = true;
    } else {
      /* 2nd < 1st (or 1st not active): signal directly. */
      pthread_mutex_lock(&new_2nd_measurement_mutex);
      pthread_cond_signal(&new_2nd_measurement_cond);
      pthread_mutex_unlock(&new_2nd_measurement_mutex);
    }
  }
}


/* ******************************************************************************************************************** */
/* Measurement for 2nd interval */
void *measurement_2nd_interval_function(void *ptr) {
  int ret;
  /* Tracks when the previous CALCULATE_LEQ_2nd was sent so we can enforce a
     minimum gap of 100ms.  After finalize_interval() resets the Teensy
     accumulator to 0, the firmware needs up to one FFT block (~93ms) before
     leq_g_A is non-zero again.  If the 2nd timer fires too soon after a
     delayed measurement (e.g. when the 1st-interval thread held dnms_mutex
     across the coincident 500ms boundary), consecutive case-10 sends can be
     less than 93ms apart and the second finalize snapshots leq_g_A = 0. */
  static struct timeval prev_case10_tv = {0, 0};

  setprio(pthread_self(), SCHED_RR, prio_2nd_measurement);
  if (output_thread_info) {
    getprio(pthread_self(), "measurement_2nd_interval_thread");
  }


  for (;;) {
    /* wait until cyclic time kick from main */
    pthread_mutex_lock(&new_2nd_measurement_mutex);
    pthread_cond_wait(&new_2nd_measurement_cond, &new_2nd_measurement_mutex);
    pthread_mutex_unlock(&new_2nd_measurement_mutex);

    /* ── 100ms gap enforcement — sleep WITHOUT holding the mutex ──────
       Prevents "LAeq 2nd = 0": after finalize_interval() resets leq_g_A,
       the Teensy needs up to one FFT frame (~93ms) to repopulate it.
       Sleeping here rather than inside the mutex avoids blocking the
       1st-interval thread unnecessarily. */
    if (prev_case10_tv.tv_sec != 0) {
      struct timeval now_tv;
      gettimeofday(&now_tv, NULL);
      /* Use long long: on 32-bit ARM 'long' is 32-bit and overflows for
         elapsed times >= 2148 s (e.g. 3600 s 2nd interval gives -694967296,
         which falsely satisfies < 100000 and causes a 695-second sleep). */
      long long us_elapsed = (long long)(now_tv.tv_sec  - prev_case10_tv.tv_sec)  * 1000000LL
                           + (now_tv.tv_usec - prev_case10_tv.tv_usec);
      if (us_elapsed < 100000LL) {
        usleep((useconds_t)(100000LL - us_elapsed));
      }
    }

    /* ── Phase 1: send trigger (brief mutex hold) ───────────────────────
       The DNMS snapshots the 2nd interval the instant it receives cmd 10.
       tv_2nd is recorded here so the timestamp reflects the true end of the
       measurement window, not the (later) time when reads complete. */
    pthread_mutex_lock(&dnms_mutex);
    dnms_error_2nd = false;
    if (dnms_calculate_leq_2nd() != 0) {
      dnms_error_count++;
      dnms_error_2nd = true;
      print_time();
      printf("Error DNMS_CALCULATE_LEQ_2nd\n\n");
      close_i2c_dnms();
      open_dnms();
      print_time();
      printf("try to reset DNMS there was an Error interval 2nd\n\n");
      sleep(1);
      pthread_mutex_unlock(&dnms_mutex);
      pthread_mutex_lock(&new_data_2nd_mutex);
      if (transmission_enabled) {
        pthread_cond_broadcast(&new_data_2nd_cond);
      }
      pthread_mutex_unlock(&new_data_2nd_mutex);
      continue;
    }
    tv_2nd = g_timer_fire_2nd;             /* use timer-fire time, not post-I2C time */
    gettimeofday(&prev_case10_tv, NULL);
    pthread_mutex_unlock(&dnms_mutex);     /* release immediately after trigger */

    /* ── Phase 2: read results (separate mutex hold) ────────────────────
       The DNMS snapshot is already taken; reads just fetch buffered data.
       Re-acquiring the mutex here serialises reads with the 1st thread. */
    pthread_mutex_lock(&dnms_mutex);
    /* is data ready? */
    for (unsigned i = 0; i < 1000; i++) {
      ret = dnms_read_data_ready_2nd(&data_ready_2nd);
      if ((ret == 0) && (data_ready_2nd != 0)) {
        break;
      }
      usleep(20);
    }
    if (data_ready_2nd == 0) {
      dnms_error_2nd = true;
      print_time();
      printf("Timeout data_ready_2nd\n\n");
    }

    if (!dnms_error_2nd) {
      if (dnms_read_leq(DNMS_CMD_READ_LEQ_2nd, &dnms_values) == 0) {
        if (dnms_values.leq_x == 0) {
          dnms_error_2nd = true;
          print_time();
          printf("LAeq 2nd = 0!\n\n");
        }
        last_value_dnms_laeq_2nd = dnms_values.leq_x + dnms_correction;        
        last_value_dnms_lamin_2nd = dnms_values.leq_x_min + dnms_correction;
        last_value_dnms_lamax_2nd = dnms_values.leq_x_max + dnms_correction;

        if (read_laeq_spec_2nd) {
          if (dnms_read_freq_spec(DNMS_CMD_READ_FFT_PART1_2nd, &dnms_values) == 0) {
            for (unsigned i = 0; i < 31; i++) {
              last_value_dnms_spectrum_2nd[i] = dnms_values.leq_freq_spec_x[i];
            }
          } else {
            dnms_error_2nd = true;
            print_time();
            printf("Error DNMS_CMD_READ_FFT_2nd\n\n");
          }
        }
      } else {
        dnms_error_2nd = true;
        print_time();
        printf("Error DNMS_CMD_READ_LEQ_2nd\n\n");
      }
      
      if (read_lzeq_2nd) {
        if (dnms_read_leq(DNMS_CMD_READ_LEQ_Z_2nd, &dnms_values) == 0) {
          last_value_dnms_lzeq_2nd = dnms_values.leq_x + dnms_correction;
          last_value_dnms_lzmin_2nd = dnms_values.leq_x_min + dnms_correction;
          last_value_dnms_lzmax_2nd = dnms_values.leq_x_max + dnms_correction;
        } else {
          dnms_error_2nd = true;
          print_time();
          printf("Error DNMS_CMD_READ_LEQ_2nd\n\n");
        }
      }
      
      if (read_lzeq_spec_2nd) {
	if (dnms_read_freq_spec(DNMS_CMD_READ_FFT_Z_PART1_2nd, &dnms_values) == 0) {
	  for (unsigned i = 0; i < 31; i++) {
	      last_value_dnms_z_spectrum_2nd[i] = dnms_values.leq_freq_spec_x[i];
	  }
	} else {
	  dnms_error_2nd = true;
	  print_time();
	  printf("Error DNMS_CMD_READ_FFT_Z_2nd\n\n");
	}
      }
      
      if (read_lceq_2nd) {
        if (dnms_read_leq(DNMS_CMD_READ_LEQ_C_2nd, &dnms_values) == 0) {
          last_value_dnms_lceq_2nd = dnms_values.leq_x + dnms_correction;
          last_value_dnms_lcmin_2nd = dnms_values.leq_x_min + dnms_correction;
          last_value_dnms_lcmax_2nd = dnms_values.leq_x_max + dnms_correction;
        } else {
          dnms_error_2nd = true;
          print_time();
          printf("Error DNMS_CMD_READ_LEQ_2nd\n\n");
        }
      }
      
      if (read_lceq_spec_2nd) {
	if (dnms_read_freq_spec(DNMS_CMD_READ_FFT_C_PART1_2nd, &dnms_values) == 0) {
	  for (unsigned i = 0; i < 31; i++) {
	      last_value_dnms_c_spectrum_2nd[i] = dnms_values.leq_freq_spec_x[i];
	  }
	} else {
	  dnms_error_2nd = true;
	  print_time();
	  printf("Error DNMS_CMD_READ_FFT_C_2nd\n\n");
	}
      }
            
    }

    if (dnms_error_2nd) {
      dnms_error_count++;
      close_i2c_dnms();
      open_dnms();
      print_time();
      printf("try to reset DNMS there was an Error interval 2nd\n\n");
      sleep(1);
      /* unlock DNMS access (Phase 2 mutex) */
      pthread_mutex_unlock(&dnms_mutex);
    } else {
      /* unlock DNMS access (Phase 2 mutex) */
      pthread_mutex_unlock(&dnms_mutex);

      counter_measurements_2nd++;

      /* tv_2nd was set in Phase 1 right after cmd 10 — no new gettimeofday */
      bzero((char *)&timestamp_2nd, sizeof(timestamp_2nd));
      bzero((char *)&timestamp_4_log_A_2nd, sizeof(timestamp_4_log_A_2nd));

      ns_2nd = (uint64_t)tv_2nd.tv_sec * 1000000000 + (uint64_t)tv_2nd.tv_usec * 1000;
      rawtime_2nd = ns_2nd / 1000000000;
      struct tm ts_local_1_2nd, ts_local_2_2nd;
      localtime_r(&rawtime_2nd, &ts_local_1_2nd);
      localtime_r(&tv_2nd.tv_sec, &ts_local_2_2nd);

      sprintf(timestamp_2nd, "%llu", ns_2nd);

      strftime(timestamp_4_log_A_2nd, 80, fmt_dl, &ts_local_1_2nd);

      sprintf(value_2_string_2nd, " %02d:%02d:%02d.%03d, ", ts_local_2_2nd.tm_hour, ts_local_2_2nd.tm_min, ts_local_2_2nd.tm_sec, (int)(tv_2nd.tv_usec / 1000));
      strcat(timestamp_4_log_A_2nd, value_2_string_2nd);

      strcpy(timestamp_4_log_Z_2nd, timestamp_4_log_A_2nd);
      strcpy(timestamp_4_log_C_2nd, timestamp_4_log_A_2nd);     

      if (last_value_dnms_laeq_2nd >= threshold_2nd_interval_laeq_transmit) {
        /* retrigger counter */
        counter_threshold_2nd = number_transmissions_after_exceeding;
#ifdef gpio_switch
        /* set output if configured */
        if (switch_output_pin) {
          gpioWrite(gpio_output_pin, 1);
        }
#endif
      }

      if (counter_threshold_2nd > 0) {
        /* timestamp  for this measurement */
        time(&sekunden);

        /* format the string for influxdb */
        bzero((char *)&data_4_influxdb_2nd, sizeof(data_4_influxdb_2nd));

        strcat(data_4_influxdb_2nd, influxdb_messung);
        strcat(data_4_influxdb_2nd, ",node=raspi-");
        strcat(data_4_influxdb_2nd, raspi_id_strg);
        strcat(data_4_influxdb_2nd, " ");

        if (data_transmit_laeq_2nd_to_influxdb) {
          strcat(data_4_influxdb_2nd, "DNMS_noise_LAeq_2nd=");
          sprintf(value_2_string_2nd, "%.1f", last_value_dnms_laeq_2nd);
          strcat(data_4_influxdb_2nd, value_2_string_2nd);

          strcat(data_4_influxdb_2nd, ",DNMS_noise_LA_min_2nd=");
          sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lamin_2nd);
          strcat(data_4_influxdb_2nd, value_2_string_2nd);

          strcat(data_4_influxdb_2nd, ",DNMS_noise_LA_max_2nd=");
          sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lamax_2nd);
          strcat(data_4_influxdb_2nd, value_2_string_2nd);
        }

        if (data_transmit_laeq_2nd_spectrum_to_influxdb) {
          for (unsigned i = 0; i < 31; i++) {
            strcat(data_4_influxdb_2nd, terzen_2nd[i]);
            sprintf(value_2_string_2nd, "%.1f", last_value_dnms_spectrum_2nd[i]);
            strcat(data_4_influxdb_2nd, value_2_string_2nd);
          }
        }

        if (data_transmit_lzeq_2nd_to_influxdb) {
          if (data_transmit_laeq_2nd_spectrum_to_influxdb || data_transmit_laeq_2nd_to_influxdb) {
            strcat(data_4_influxdb_2nd, ",");
          }
          strcat(data_4_influxdb_2nd, "DNMS_noise_LZeq_2nd=");
          sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lzeq_2nd);
          strcat(data_4_influxdb_2nd, value_2_string_2nd);

          strcat(data_4_influxdb_2nd, ",DNMS_noise_LZ_min_2nd=");
          sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lzmin_2nd);
          strcat(data_4_influxdb_2nd, value_2_string_2nd);

          strcat(data_4_influxdb_2nd, ",DNMS_noise_LZ_max_2nd=");
          sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lzmax_2nd);
          strcat(data_4_influxdb_2nd, value_2_string_2nd);
        }

        if (data_transmit_lzeq_2nd_spectrum_to_influxdb) {
          for (unsigned i = 0; i < 31; i++) {
            strcat(data_4_influxdb_2nd, terzen_z_2nd[i]);
            sprintf(value_2_string_2nd, "%.1f", last_value_dnms_z_spectrum_2nd[i]);
            strcat(data_4_influxdb_2nd, value_2_string_2nd);
          }
        }

        if (data_transmit_lceq_2nd_to_influxdb) {
          if (data_transmit_laeq_2nd_spectrum_to_influxdb || data_transmit_laeq_2nd_to_influxdb || data_transmit_lzeq_2nd_to_influxdb || data_transmit_lzeq_2nd_spectrum_to_influxdb ) {
            strcat(data_4_influxdb_2nd, ",");
          }
          strcat(data_4_influxdb_2nd, "DNMS_noise_LCeq_2nd=");
          sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lceq_2nd);
          strcat(data_4_influxdb_2nd, value_2_string_2nd);

          strcat(data_4_influxdb_2nd, ",DNMS_noise_LC_min_2nd=");
          sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lcmin_2nd);
          strcat(data_4_influxdb_2nd, value_2_string_2nd);

          strcat(data_4_influxdb_2nd, ",DNMS_noise_LC_max_2nd=");
          sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lcmax_2nd);
          strcat(data_4_influxdb_2nd, value_2_string_2nd);
        }

        if (data_transmit_lceq_2nd_spectrum_to_influxdb) {
          for (unsigned i = 0; i < 31; i++) {
            strcat(data_4_influxdb_2nd, terzen_c_2nd[i]);
            sprintf(value_2_string_2nd, "%.1f", last_value_dnms_c_spectrum_2nd[i]);
            strcat(data_4_influxdb_2nd, value_2_string_2nd);
          }
        }

        /* is transmission enabled or stopped ?? '*/
        if (transmission_enabled && enable_wlan_or_lan) {
          pthread_mutex_lock(&buffer_influx_mutex);
          {
            int backlog = (buffer_influx.b_read != buffer_influx.b_write);
            if (buffer_in_influx((char *)data_4_influxdb_2nd, (char *)timestamp_2nd, dest_influxdb) == buffer_fail) {
              print_time();
              buffer_overflow_count++;
              printf("buffer overflow for InfluxDB 2nd, dropping entry\n");
              fflush(stdout);
            } else if (backlog) {
              print_time();
              printf("connection may be down, buffering 2nd-interval InfluxDB\n");
              fflush(stdout);
            }
          }
          pthread_mutex_unlock(&buffer_influx_mutex);
        }

        if (data_2nd_to_mqtt && mqtt_transmit) {
          bzero((char *)&data_4_mqtt_2nd, sizeof(data_4_mqtt_2nd));
          strcat(data_4_mqtt_2nd, mqtt_messung);
          strcat(data_4_mqtt_2nd, ",node=raspi-");
          strcat(data_4_mqtt_2nd, raspi_id_strg);
          strcat(data_4_mqtt_2nd, " ");
          if (data_transmit_laeq_2nd_to_mqtt) {
            strcat(data_4_mqtt_2nd, "DNMS_noise_LAeq_2nd=");
            sprintf(value_2_string_2nd, "%.1f", last_value_dnms_laeq_2nd);
            strcat(data_4_mqtt_2nd, value_2_string_2nd);
            strcat(data_4_mqtt_2nd, ",DNMS_noise_LA_min_2nd=");
            sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lamin_2nd);
            strcat(data_4_mqtt_2nd, value_2_string_2nd);
            strcat(data_4_mqtt_2nd, ",DNMS_noise_LA_max_2nd=");
            sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lamax_2nd);
            strcat(data_4_mqtt_2nd, value_2_string_2nd);
          }
          if (data_transmit_laeq_2nd_spectrum_to_mqtt) {
            for (unsigned i = 0; i < 31; i++) {
              strcat(data_4_mqtt_2nd, terzen_2nd[i]);
              sprintf(value_2_string_2nd, "%.1f", last_value_dnms_spectrum_2nd[i]);
              strcat(data_4_mqtt_2nd, value_2_string_2nd);
            }
          }
          if (data_transmit_lzeq_2nd_to_mqtt) {
            if (data_transmit_laeq_2nd_spectrum_to_mqtt || data_transmit_laeq_2nd_to_mqtt)
              strcat(data_4_mqtt_2nd, ",");
            strcat(data_4_mqtt_2nd, "DNMS_noise_LZeq_2nd=");
            sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lzeq_2nd);
            strcat(data_4_mqtt_2nd, value_2_string_2nd);
            strcat(data_4_mqtt_2nd, ",DNMS_noise_LZ_min_2nd=");
            sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lzmin_2nd);
            strcat(data_4_mqtt_2nd, value_2_string_2nd);
            strcat(data_4_mqtt_2nd, ",DNMS_noise_LZ_max_2nd=");
            sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lzmax_2nd);
            strcat(data_4_mqtt_2nd, value_2_string_2nd);
          }
          if (data_transmit_lzeq_2nd_spectrum_to_mqtt) {
            for (unsigned i = 0; i < 31; i++) {
              strcat(data_4_mqtt_2nd, terzen_z_2nd[i]);
              sprintf(value_2_string_2nd, "%.1f", last_value_dnms_z_spectrum_2nd[i]);
              strcat(data_4_mqtt_2nd, value_2_string_2nd);
            }
          }
          if (data_transmit_lceq_2nd_to_mqtt) {
            if (data_transmit_laeq_2nd_spectrum_to_mqtt || data_transmit_laeq_2nd_to_mqtt ||
                data_transmit_lzeq_2nd_to_mqtt || data_transmit_lzeq_2nd_spectrum_to_mqtt)
              strcat(data_4_mqtt_2nd, ",");
            strcat(data_4_mqtt_2nd, "DNMS_noise_LCeq_2nd=");
            sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lceq_2nd);
            strcat(data_4_mqtt_2nd, value_2_string_2nd);
            strcat(data_4_mqtt_2nd, ",DNMS_noise_LC_min_2nd=");
            sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lcmin_2nd);
            strcat(data_4_mqtt_2nd, value_2_string_2nd);
            strcat(data_4_mqtt_2nd, ",DNMS_noise_LC_max_2nd=");
            sprintf(value_2_string_2nd, "%.1f", last_value_dnms_lcmax_2nd);
            strcat(data_4_mqtt_2nd, value_2_string_2nd);
          }
          if (data_transmit_lceq_2nd_spectrum_to_mqtt) {
            for (unsigned i = 0; i < 31; i++) {
              strcat(data_4_mqtt_2nd, terzen_c_2nd[i]);
              sprintf(value_2_string_2nd, "%.1f", last_value_dnms_c_spectrum_2nd[i]);
              strcat(data_4_mqtt_2nd, value_2_string_2nd);
            }
          }
          if (transmission_enabled && enable_wlan_or_lan) {
            pthread_mutex_lock(&buffer_influx_mutex);
            if (buffer_in_influx((char *)data_4_mqtt_2nd, (char *)timestamp_2nd, dest_mqtt) == buffer_fail) {
              buffer_overflow_count++;
              printf("buffer overflow for MQTT 2nd, try to increase buffer!\n");
            }
            pthread_mutex_unlock(&buffer_influx_mutex);
          }
        }

        counter_threshold_2nd--;  // count down to zero
        if (counter_threshold_2nd == 0) {
#ifdef gpio_switch
          /* is switch_output_pin set? than reset output pin */
          if (switch_output_pin) {
            gpioWrite(gpio_output_pin, 0);
          }
#endif
        }
      }
      fflush(stdout);
      pthread_mutex_lock(&new_data_2nd_mutex);
      if (transmission_enabled) {
        pthread_cond_broadcast(&new_data_2nd_cond);
      }
      pthread_mutex_unlock(&new_data_2nd_mutex);
    }
  }
}



/* ******************************************************************************************************************** */
void *print_ipc_and_log_1st_function(void *ptr) {
  setprio(pthread_self(), SCHED_RR, prio_print_ipc_and_log_1st);
  if (output_thread_info) {
    getprio(pthread_self(), "print_ipc_and_log_1st_thread");
  }
  for (;;) {
    /* wait until a 1st interval measurement is done and a signal/broadcast for new data arrives */
    pthread_mutex_lock(&new_data_1st_mutex);
    pthread_cond_wait(&new_data_1st_cond, &new_data_1st_mutex);
    pthread_mutex_unlock(&new_data_1st_mutex);

    /* ── InfluxDB / Sensor.Community buffering ──────────────────────────
       Moved here from measurement_1st_interval_function so that the ~88ms
       of string formatting runs at prio_print_ipc_and_log_1st (= 1) instead
       of prio_1st_measurement (= 7).  This lets the 2nd-interval timer
       (prio = 6) preempt and deliver its signal on time, eliminating the
       systematic ~25ms delay in 2nd-interval delivery at coincidences. */
    if (data_1st_to_influxdb) {
      bzero((char *)&data_4_transmit, sizeof(data_4_transmit));
      strcat(data_4_transmit, influxdb_messung);
      strcat(data_4_transmit, ",node=raspi-");
      strcat(data_4_transmit, raspi_id_strg);
      strcat(data_4_transmit, " ");
      if (data_transmit_laeq_1st_to_influxdb) {
        strcat(data_4_transmit, "DNMS_noise_LAeq=");
        sprintf(value_2_string, "%.1f", last_value_dnms_laeq);
        strcat(data_4_transmit, value_2_string);
        strcat(data_4_transmit, ",DNMS_noise_LA_min=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lamin);
        strcat(data_4_transmit, value_2_string);
        strcat(data_4_transmit, ",DNMS_noise_LA_max=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lamax);
        strcat(data_4_transmit, value_2_string);
      }
      if (data_transmit_laeq_1st_spectrum_to_influxdb) {
        for (unsigned i = 0; i < 31; i++) {
          strcat(data_4_transmit, terzen[i]);
          sprintf(value_2_string, "%.1f", last_value_dnms_spectrum[i]);
          strcat(data_4_transmit, value_2_string);
        }
      }
      if (data_transmit_lzeq_1st_to_influxdb) {
        if (data_transmit_laeq_1st_spectrum_to_influxdb || data_transmit_laeq_1st_to_influxdb)
          strcat(data_4_transmit, ",");
        strcat(data_4_transmit, "DNMS_noise_LZeq=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lzeq);
        strcat(data_4_transmit, value_2_string);
        strcat(data_4_transmit, ",DNMS_noise_LZ_min=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lzmin);
        strcat(data_4_transmit, value_2_string);
        strcat(data_4_transmit, ",DNMS_noise_LZ_max=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lzmax);
        strcat(data_4_transmit, value_2_string);
      }
      if (data_transmit_lzeq_1st_spectrum_to_influxdb) {
        for (unsigned i = 0; i < 31; i++) {
          strcat(data_4_transmit, terzen_z[i]);
          sprintf(value_2_string, "%.1f", last_value_dnms_z_spectrum[i]);
          strcat(data_4_transmit, value_2_string);
        }
      }
      if (data_transmit_lceq_1st_to_influxdb) {
        if (data_transmit_laeq_1st_spectrum_to_influxdb || data_transmit_laeq_1st_to_influxdb ||
            data_transmit_lzeq_1st_spectrum_to_influxdb || data_transmit_lzeq_1st_to_influxdb)
          strcat(data_4_transmit, ",");
        strcat(data_4_transmit, "DNMS_noise_LCeq=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lceq);
        strcat(data_4_transmit, value_2_string);
        strcat(data_4_transmit, ",DNMS_noise_LC_min=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lcmin);
        strcat(data_4_transmit, value_2_string);
        strcat(data_4_transmit, ",DNMS_noise_LC_max=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lcmax);
        strcat(data_4_transmit, value_2_string);
      }
      if (data_transmit_lceq_1st_spectrum_to_influxdb) {
        for (unsigned i = 0; i < 31; i++) {
          strcat(data_4_transmit, terzen_c[i]);
          sprintf(value_2_string, "%.1f", last_value_dnms_c_spectrum[i]);
          strcat(data_4_transmit, value_2_string);
        }
      }
      if (transmission_enabled && enable_wlan_or_lan) {
        pthread_mutex_lock(&buffer_influx_mutex);
        {
          int backlog = (buffer_influx.b_read != buffer_influx.b_write);
          if (buffer_in_influx((char *)data_4_transmit, (char *)timestamp_1st, dest_influxdb) == buffer_fail) {
            buffer_overflow_count++;
            printf("buffer overflow for InfluxDB, dropping entry\n");
          } else if (backlog) {
            print_time();
            printf("connection may be down, buffering 1st-interval InfluxDB\n");
            fflush(stdout);
          }
        }
        pthread_mutex_unlock(&buffer_influx_mutex);
      }
    }

    if (data_1st_to_mqtt && mqtt_transmit) {
      bzero((char *)&data_4_mqtt, sizeof(data_4_mqtt));
      strcat(data_4_mqtt, mqtt_messung);
      strcat(data_4_mqtt, ",node=raspi-");
      strcat(data_4_mqtt, raspi_id_strg);
      strcat(data_4_mqtt, " ");
      if (data_transmit_laeq_1st_to_mqtt) {
        strcat(data_4_mqtt, "DNMS_noise_LAeq=");
        sprintf(value_2_string, "%.1f", last_value_dnms_laeq);
        strcat(data_4_mqtt, value_2_string);
        strcat(data_4_mqtt, ",DNMS_noise_LA_min=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lamin);
        strcat(data_4_mqtt, value_2_string);
        strcat(data_4_mqtt, ",DNMS_noise_LA_max=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lamax);
        strcat(data_4_mqtt, value_2_string);
      }
      if (data_transmit_laeq_1st_spectrum_to_mqtt) {
        for (unsigned i = 0; i < 31; i++) {
          strcat(data_4_mqtt, terzen[i]);
          sprintf(value_2_string, "%.1f", last_value_dnms_spectrum[i]);
          strcat(data_4_mqtt, value_2_string);
        }
      }
      if (data_transmit_lzeq_1st_to_mqtt) {
        if (data_transmit_laeq_1st_spectrum_to_mqtt || data_transmit_laeq_1st_to_mqtt)
          strcat(data_4_mqtt, ",");
        strcat(data_4_mqtt, "DNMS_noise_LZeq=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lzeq);
        strcat(data_4_mqtt, value_2_string);
        strcat(data_4_mqtt, ",DNMS_noise_LZ_min=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lzmin);
        strcat(data_4_mqtt, value_2_string);
        strcat(data_4_mqtt, ",DNMS_noise_LZ_max=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lzmax);
        strcat(data_4_mqtt, value_2_string);
      }
      if (data_transmit_lzeq_1st_spectrum_to_mqtt) {
        for (unsigned i = 0; i < 31; i++) {
          strcat(data_4_mqtt, terzen_z[i]);
          sprintf(value_2_string, "%.1f", last_value_dnms_z_spectrum[i]);
          strcat(data_4_mqtt, value_2_string);
        }
      }
      if (data_transmit_lceq_1st_to_mqtt) {
        if (data_transmit_laeq_1st_spectrum_to_mqtt || data_transmit_laeq_1st_to_mqtt ||
            data_transmit_lzeq_1st_spectrum_to_mqtt || data_transmit_lzeq_1st_to_mqtt)
          strcat(data_4_mqtt, ",");
        strcat(data_4_mqtt, "DNMS_noise_LCeq=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lceq);
        strcat(data_4_mqtt, value_2_string);
        strcat(data_4_mqtt, ",DNMS_noise_LC_min=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lcmin);
        strcat(data_4_mqtt, value_2_string);
        strcat(data_4_mqtt, ",DNMS_noise_LC_max=");
        sprintf(value_2_string, "%.1f", last_value_dnms_lcmax);
        strcat(data_4_mqtt, value_2_string);
      }
      if (data_transmit_lceq_1st_spectrum_to_mqtt) {
        for (unsigned i = 0; i < 31; i++) {
          strcat(data_4_mqtt, terzen_c[i]);
          sprintf(value_2_string, "%.1f", last_value_dnms_c_spectrum[i]);
          strcat(data_4_mqtt, value_2_string);
        }
      }
      if (transmission_enabled && enable_wlan_or_lan) {
        pthread_mutex_lock(&buffer_influx_mutex);
        if (buffer_in_influx((char *)data_4_mqtt, (char *)timestamp_1st, dest_mqtt) == buffer_fail) {
          buffer_overflow_count++;
          printf("buffer overflow for MQTT, try to increase buffer!\n");
        }
        pthread_mutex_unlock(&buffer_influx_mutex);
      }
    }

    if (data_transmit_laeq_to_sc) {
      bzero((char *)&data_4_transmit, sizeof(data_4_transmit));
      strcat(data_4_transmit, data_sc_1);
      sprintf(value_2_string, "%.1f", last_value_dnms_laeq);
      strcat(data_4_transmit, value_2_string);
      strcat(data_4_transmit, data_sc_2);
      sprintf(value_2_string, "%.1f", last_value_dnms_lamin);
      strcat(data_4_transmit, value_2_string);
      strcat(data_4_transmit, data_sc_3);
      sprintf(value_2_string, "%.1f", last_value_dnms_lamax);
      strcat(data_4_transmit, value_2_string);
      strcat(data_4_transmit, data_sc_4); /* close LA_max value string and JSON array */
      length_data_sc_str = strlen(data_4_transmit);
      sprintf(str_of_length_data_sc_str, "%d", length_data_sc_str);
      bzero((char *)&sc_msg, sizeof(sc_msg));
      strcat(sc_msg, sc_header1);
      strcat(sc_msg, str_of_length_data_sc_str);
      strcat(sc_msg, sc_header2);
      strcat(sc_msg, data_4_transmit);
      if (transmission_enabled && enable_wlan_or_lan) {
        pthread_mutex_lock(&buffer_sc_mutex);
        if (buffer_in_sc((char *)sc_msg, (char *)timestamp_1st, dest_sc) == buffer_fail) {
          print_time();
          buffer_overflow_count++;
          printf("buffer overflow for Sensor.Community, dropping entry\n");
        }
        pthread_mutex_unlock(&buffer_sc_mutex);
      }
    }

    if (data_transmit_laeq_to_madavi) {
      char madavi_rssi_str[8];
      sprintf(madavi_rssi_str, "%d", get_signal_strength());
      bzero((char *)&data_4_madavi, sizeof(data_4_madavi));
      strcat(data_4_madavi, "{\"software_version\": \"");
      strcat(data_4_madavi, firmware_version);
      strcat(data_4_madavi, "\", \"sensordatavalues\":[{\"value_type\":\"DNMS_noise_LAeq\",\"value\":\"");
      sprintf(value_2_string, "%.1f", last_value_dnms_laeq);
      strcat(data_4_madavi, value_2_string);
      strcat(data_4_madavi, "\"},{\"value_type\":\"DNMS_noise_LA_min\",\"value\":\"");
      sprintf(value_2_string, "%.1f", last_value_dnms_lamin);
      strcat(data_4_madavi, value_2_string);
      strcat(data_4_madavi, "\"},{\"value_type\":\"DNMS_noise_LA_max\",\"value\":\"");
      sprintf(value_2_string, "%.1f", last_value_dnms_lamax);
      strcat(data_4_madavi, value_2_string);
      strcat(data_4_madavi, "\"},{\"value_type\":\"signal\",\"value\":\"");
      strcat(data_4_madavi, madavi_rssi_str);
      strcat(data_4_madavi, "\"}]}");
      length_data_sc_str = strlen(data_4_madavi);
      sprintf(str_of_length_data_sc_str, "%d", length_data_sc_str);
      bzero((char *)&madavi_msg, sizeof(madavi_msg));
      strcat(madavi_msg, madavi_header1);
      strcat(madavi_msg, str_of_length_data_sc_str);
      strcat(madavi_msg, sc_header2);
      strcat(madavi_msg, data_4_madavi);
      if (transmission_enabled && enable_wlan_or_lan) {
        pthread_mutex_lock(&buffer_sc_mutex);
        if (buffer_in_sc((char *)madavi_msg, (char *)timestamp_1st, dest_madavi) == buffer_fail) {
          print_time();
          buffer_overflow_count++;
          printf("buffer overflow for Madavi, try to increase buffer!\n");
        }
        pthread_mutex_unlock(&buffer_sc_mutex);
      }
    }

    /* ── environment sensor transmission (only when interval > 29 s) ── */
    if (measurement_1st_interval_ms > 29000 && transmission_enabled && enable_wlan_or_lan) {
      static char sensor_sc_hdr[600];
      static char sensor_sc_msg_buf[1300];
      static char sensor_influx_line[512];
      static char sensor_sc_json[512];
      char sensor_len_str[12];
      int sensor_body_len;

#define SENSOR_SC_POST(pin, json_body) do { \
        sensor_body_len = (int)strlen(json_body); \
        snprintf(sensor_len_str, sizeof(sensor_len_str), "%d", sensor_body_len); \
        bzero(sensor_sc_hdr, sizeof(sensor_sc_hdr)); \
        strcat(sensor_sc_hdr, "POST "); strcat(sensor_sc_hdr, url_sc); \
        strcat(sensor_sc_hdr, " HTTP/1.1\r\nHost: "); strcat(sensor_sc_hdr, host_sc); \
        strcat(sensor_sc_hdr, "\r\nUser-Agent: "); strcat(sensor_sc_hdr, firmware_version); \
        strcat(sensor_sc_hdr, "/"); strcat(sensor_sc_hdr, raspi_id_strg); \
        strcat(sensor_sc_hdr, "/"); strcat(sensor_sc_hdr, mac_strg); \
        strcat(sensor_sc_hdr, "\r\nAccept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\nConnection: close\r\n"); \
        strcat(sensor_sc_hdr, "Content-Type: application/json\r\nX-Sensor: raspi-"); \
        strcat(sensor_sc_hdr, raspi_id_strg); \
        strcat(sensor_sc_hdr, "\r\nX-MAC-ID: raspi-"); strcat(sensor_sc_hdr, mac_strg); \
        strcat(sensor_sc_hdr, "\r\nX-PIN: "); strcat(sensor_sc_hdr, pin); \
        strcat(sensor_sc_hdr, "\r\nContent-Length: "); strcat(sensor_sc_hdr, sensor_len_str); \
        strcat(sensor_sc_hdr, "\r\n\r\n"); \
        bzero(sensor_sc_msg_buf, sizeof(sensor_sc_msg_buf)); \
        strcat(sensor_sc_msg_buf, sensor_sc_hdr); \
        strcat(sensor_sc_msg_buf, json_body); \
        pthread_mutex_lock(&buffer_sc_mutex); \
        if (buffer_in_sc(sensor_sc_msg_buf, (char *)timestamp_1st, dest_sc) == buffer_fail) { \
          print_time(); buffer_overflow_count++; printf("sensor SC buffer overflow\n"); \
        } \
        pthread_mutex_unlock(&buffer_sc_mutex); \
      } while(0)

#define SENSOR_MADAVI_POST(pin, json_body) do { \
        sensor_body_len = (int)strlen(json_body); \
        snprintf(sensor_len_str, sizeof(sensor_len_str), "%d", sensor_body_len); \
        bzero(sensor_sc_hdr, sizeof(sensor_sc_hdr)); \
        strcat(sensor_sc_hdr, "POST "); strcat(sensor_sc_hdr, url_madavi); \
        strcat(sensor_sc_hdr, " HTTP/1.1\r\nHost: "); strcat(sensor_sc_hdr, host_madavi); \
        strcat(sensor_sc_hdr, "\r\nUser-Agent: "); strcat(sensor_sc_hdr, firmware_version); \
        strcat(sensor_sc_hdr, "/"); strcat(sensor_sc_hdr, raspi_id_strg); \
        strcat(sensor_sc_hdr, "/"); strcat(sensor_sc_hdr, mac_strg); \
        strcat(sensor_sc_hdr, "\r\nAccept-Encoding: identity;q=1,chunked;q=0.1,*;q=0\r\nConnection: close\r\n"); \
        strcat(sensor_sc_hdr, "Content-Type: application/json\r\nX-Sensor: raspi-"); \
        strcat(sensor_sc_hdr, raspi_id_strg); \
        strcat(sensor_sc_hdr, "\r\nX-MAC-ID: raspi-"); strcat(sensor_sc_hdr, mac_strg); \
        strcat(sensor_sc_hdr, "\r\nX-PIN: "); strcat(sensor_sc_hdr, pin); \
        strcat(sensor_sc_hdr, "\r\nContent-Length: "); strcat(sensor_sc_hdr, sensor_len_str); \
        strcat(sensor_sc_hdr, "\r\n\r\n"); \
        bzero(sensor_sc_msg_buf, sizeof(sensor_sc_msg_buf)); \
        strcat(sensor_sc_msg_buf, sensor_sc_hdr); \
        strcat(sensor_sc_msg_buf, json_body); \
        pthread_mutex_lock(&buffer_sc_mutex); \
        if (buffer_in_sc(sensor_sc_msg_buf, (char *)timestamp_1st, dest_madavi) == buffer_fail) { \
          print_time(); buffer_overflow_count++; printf("sensor Madavi buffer overflow\n"); \
        } \
        pthread_mutex_unlock(&buffer_sc_mutex); \
      } while(0)

#define SENSOR_INFLUX_QUEUE(fields) do { \
        char _siq_fields[512]; \
        strncpy(_siq_fields, (fields), sizeof(_siq_fields) - 1); \
        _siq_fields[sizeof(_siq_fields) - 1] = '\0'; \
        if (data_1st_to_influxdb) { \
          bzero(sensor_influx_line, sizeof(sensor_influx_line)); \
          strcat(sensor_influx_line, influxdb_messung); \
          strcat(sensor_influx_line, ",node=raspi-"); \
          strcat(sensor_influx_line, raspi_id_strg); \
          strcat(sensor_influx_line, " "); \
          strcat(sensor_influx_line, _siq_fields); \
          pthread_mutex_lock(&buffer_influx_mutex); \
          if (buffer_in_influx(sensor_influx_line, (char *)timestamp_1st, dest_influxdb) == buffer_fail) { \
            print_time(); buffer_overflow_count++; printf("sensor InfluxDB buffer overflow\n"); \
          } \
          pthread_mutex_unlock(&buffer_influx_mutex); \
        } \
        if (data_1st_to_mqtt) { \
          bzero(sensor_influx_line, sizeof(sensor_influx_line)); \
          strcat(sensor_influx_line, mqtt_messung); \
          strcat(sensor_influx_line, ",node=raspi-"); \
          strcat(sensor_influx_line, raspi_id_strg); \
          strcat(sensor_influx_line, " "); \
          strcat(sensor_influx_line, _siq_fields); \
          pthread_mutex_lock(&buffer_influx_mutex); \
          if (buffer_in_influx(sensor_influx_line, (char *)timestamp_1st, dest_mqtt) == buffer_fail) { \
            print_time(); buffer_overflow_count++; printf("sensor MQTT buffer overflow\n"); \
          } \
          pthread_mutex_unlock(&buffer_influx_mutex); \
        } \
      } while(0)

      /* SDS011 (SC PIN 1) */
      if (enable_sds011 && last_value_SDS_P2 >= 0.0f) {
        snprintf(sensor_influx_line, sizeof(sensor_influx_line),
                 "SDS_P1=%.1f,SDS_P2=%.1f",
                 last_value_SDS_P1, last_value_SDS_P2);
        SENSOR_INFLUX_QUEUE(sensor_influx_line);
        if (data_transmit_laeq_to_sc || data_transmit_laeq_to_madavi) {
          snprintf(sensor_sc_json, sizeof(sensor_sc_json),
                   "{\"software_version\":\"%s\","
                   "\"sensordatavalues\":["
                   "{\"value_type\":\"SDS_P1\",\"value\":\"%.1f\"},"
                   "{\"value_type\":\"SDS_P2\",\"value\":\"%.1f\"}"
                   "]}",
                   firmware_version, last_value_SDS_P1, last_value_SDS_P2);
          if (data_transmit_laeq_to_sc)     SENSOR_SC_POST("1", sensor_sc_json);
          if (data_transmit_laeq_to_madavi) SENSOR_MADAVI_POST("1", sensor_sc_json);
        }
      }

      /* SPS30 (SC PIN 1) */
      if (enable_sps30 && last_value_SPS30_P2 >= 0.0f) {
        snprintf(sensor_influx_line, sizeof(sensor_influx_line),
                 "SPS_P0=%.2f,SPS_P2=%.2f,SPS_P4=%.2f,SPS_P1=%.2f,"
                 "SPS_N05=%.2f,SPS_N1=%.2f,SPS_N25=%.2f,SPS_N4=%.2f,SPS_N10=%.2f,SPS_TS=%.2f",
                 last_value_SPS30_P0, last_value_SPS30_P2, last_value_SPS30_P4, last_value_SPS30_P1,
                 last_value_SPS30_N05, last_value_SPS30_N1, last_value_SPS30_N25,
                 last_value_SPS30_N4, last_value_SPS30_N10, last_value_SPS30_TS);
        SENSOR_INFLUX_QUEUE(sensor_influx_line);
        if (data_transmit_laeq_to_sc || data_transmit_laeq_to_madavi) {
          snprintf(sensor_sc_json, sizeof(sensor_sc_json),
                   "{\"software_version\":\"%s\","
                   "\"sensordatavalues\":["
                   "{\"value_type\":\"SPS30_P0\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SPS30_P2\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SPS30_P4\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SPS30_P1\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SPS30_N05\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SPS30_N1\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SPS30_N25\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SPS30_N4\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SPS30_N10\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SPS30_TS\",\"value\":\"%.2f\"}"
                   "]}",
                   firmware_version,
                   last_value_SPS30_P0, last_value_SPS30_P2, last_value_SPS30_P4, last_value_SPS30_P1,
                   last_value_SPS30_N05, last_value_SPS30_N1, last_value_SPS30_N25,
                   last_value_SPS30_N4, last_value_SPS30_N10, last_value_SPS30_TS);
          if (data_transmit_laeq_to_sc)     SENSOR_SC_POST("1", sensor_sc_json);
          if (data_transmit_laeq_to_madavi) SENSOR_MADAVI_POST("1", sensor_sc_json);
        }
      }

      /* SHT3x (SC PIN 7) */
      if (enable_sht3x && last_value_SHT3X_H >= 0.0f) {
        snprintf(sensor_influx_line, sizeof(sensor_influx_line),
                 "SHT3X_temperature=%.2f,SHT3X_humidity=%.2f",
                 last_value_SHT3X_T, last_value_SHT3X_H);
        SENSOR_INFLUX_QUEUE(sensor_influx_line);
        if (data_transmit_laeq_to_sc || data_transmit_laeq_to_madavi) {
          snprintf(sensor_sc_json, sizeof(sensor_sc_json),
                   "{\"software_version\":\"%s\","
                   "\"sensordatavalues\":["
                   "{\"value_type\":\"SHT3X_temperature\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SHT3X_humidity\",\"value\":\"%.2f\"}"
                   "]}",
                   firmware_version, last_value_SHT3X_T, last_value_SHT3X_H);
          if (data_transmit_laeq_to_sc)     SENSOR_SC_POST("7", sensor_sc_json);
          if (data_transmit_laeq_to_madavi) SENSOR_MADAVI_POST("7", sensor_sc_json);
        }
      }

      /* SEN5x (SC PIN 16) */
      if (enable_sen5x && last_value_SEN5X_P2 >= 0.0f) {
        snprintf(sensor_influx_line, sizeof(sensor_influx_line),
                 "SEN5X_P0=%.2f,SEN5X_P2=%.2f,SEN5X_P4=%.2f,SEN5X_P1=%.2f,"
                 "SEN5X_temperature=%.2f,SEN5X_humidity=%.2f,SEN5X_voc=%.1f,SEN5X_nox=%.1f",
                 last_value_SEN5X_P0, last_value_SEN5X_P2, last_value_SEN5X_P4, last_value_SEN5X_P1,
                 last_value_SEN5X_T, last_value_SEN5X_H, last_value_SEN5X_VOC, last_value_SEN5X_NOX);
        SENSOR_INFLUX_QUEUE(sensor_influx_line);
        if (data_transmit_laeq_to_sc || data_transmit_laeq_to_madavi) {
          snprintf(sensor_sc_json, sizeof(sensor_sc_json),
                   "{\"software_version\":\"%s\","
                   "\"sensordatavalues\":["
                   "{\"value_type\":\"SEN5X_P0\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SEN5X_P2\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SEN5X_P4\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SEN5X_P1\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SEN5X_temperature\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SEN5X_humidity\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SEN5X_voc\",\"value\":\"%.1f\"},"
                   "{\"value_type\":\"SEN5X_nox\",\"value\":\"%.1f\"}"
                   "]}",
                   firmware_version,
                   last_value_SEN5X_P0, last_value_SEN5X_P2, last_value_SEN5X_P4, last_value_SEN5X_P1,
                   last_value_SEN5X_T, last_value_SEN5X_H, last_value_SEN5X_VOC, last_value_SEN5X_NOX);
          if (data_transmit_laeq_to_sc)     SENSOR_SC_POST("16", sensor_sc_json);
          if (data_transmit_laeq_to_madavi) SENSOR_MADAVI_POST("16", sensor_sc_json);
        }
      }

      /* BME280 (SC PIN 11) */
      if (enable_bme280 && last_value_BME280_T > -100.0f) {
        if (last_value_BME280_H >= 0.0f) {
          snprintf(sensor_influx_line, sizeof(sensor_influx_line),
                   "BME280_temperature=%.2f,BME280_pressure=%.2f,BME280_humidity=%.2f",
                   last_value_BME280_T, last_value_BME280_P, last_value_BME280_H);
        } else {
          snprintf(sensor_influx_line, sizeof(sensor_influx_line),
                   "BME280_temperature=%.2f,BME280_pressure=%.2f",
                   last_value_BME280_T, last_value_BME280_P);
        }
        SENSOR_INFLUX_QUEUE(sensor_influx_line);
        if (data_transmit_laeq_to_sc || data_transmit_laeq_to_madavi) {
          if (last_value_BME280_H >= 0.0f) {
            snprintf(sensor_sc_json, sizeof(sensor_sc_json),
                     "{\"software_version\":\"%s\","
                     "\"sensordatavalues\":["
                     "{\"value_type\":\"BME280_temperature\",\"value\":\"%.2f\"},"
                     "{\"value_type\":\"BME280_pressure\",\"value\":\"%.2f\"},"
                     "{\"value_type\":\"BME280_humidity\",\"value\":\"%.2f\"}"
                     "]}",
                     firmware_version, last_value_BME280_T, last_value_BME280_P, last_value_BME280_H);
          } else {
            snprintf(sensor_sc_json, sizeof(sensor_sc_json),
                     "{\"software_version\":\"%s\","
                     "\"sensordatavalues\":["
                     "{\"value_type\":\"BME280_temperature\",\"value\":\"%.2f\"},"
                     "{\"value_type\":\"BME280_pressure\",\"value\":\"%.2f\"}"
                     "]}",
                     firmware_version, last_value_BME280_T, last_value_BME280_P);
          }
          if (data_transmit_laeq_to_sc)     SENSOR_SC_POST("11", sensor_sc_json);
          if (data_transmit_laeq_to_madavi) SENSOR_MADAVI_POST("11", sensor_sc_json);
        }
      }

      /* SCD30 (SC PIN 17) */
      if (enable_scd30 && last_value_SCD30_CO2 >= 0.0f) {
        snprintf(sensor_influx_line, sizeof(sensor_influx_line),
                 "SCD30_co2=%.0f,SCD30_temperature=%.2f,SCD30_humidity=%.2f",
                 last_value_SCD30_CO2, last_value_SCD30_T, last_value_SCD30_H);
        SENSOR_INFLUX_QUEUE(sensor_influx_line);
        if (data_transmit_laeq_to_sc || data_transmit_laeq_to_madavi) {
          snprintf(sensor_sc_json, sizeof(sensor_sc_json),
                   "{\"software_version\":\"%s\","
                   "\"sensordatavalues\":["
                   "{\"value_type\":\"SCD30_co2\",\"value\":\"%.0f\"},"
                   "{\"value_type\":\"SCD30_temperature\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SCD30_humidity\",\"value\":\"%.2f\"}"
                   "]}",
                   firmware_version,
                   last_value_SCD30_CO2, last_value_SCD30_T, last_value_SCD30_H);
          if (data_transmit_laeq_to_sc)     SENSOR_SC_POST("17", sensor_sc_json);
          if (data_transmit_laeq_to_madavi) SENSOR_MADAVI_POST("17", sensor_sc_json);
        }
      }

      /* SCD4x (SC PIN 17) */
      if (enable_scd4x && last_value_SCD4X_CO2 >= 0.0f) {
        snprintf(sensor_influx_line, sizeof(sensor_influx_line),
                 "SCD4X_co2=%.0f,SCD4X_temperature=%.2f,SCD4X_humidity=%.2f",
                 last_value_SCD4X_CO2, last_value_SCD4X_T, last_value_SCD4X_H);
        SENSOR_INFLUX_QUEUE(sensor_influx_line);
        if (data_transmit_laeq_to_sc || data_transmit_laeq_to_madavi) {
          snprintf(sensor_sc_json, sizeof(sensor_sc_json),
                   "{\"software_version\":\"%s\","
                   "\"sensordatavalues\":["
                   "{\"value_type\":\"SCD4X_co2\",\"value\":\"%.0f\"},"
                   "{\"value_type\":\"SCD4X_temperature\",\"value\":\"%.2f\"},"
                   "{\"value_type\":\"SCD4X_humidity\",\"value\":\"%.2f\"}"
                   "]}",
                   firmware_version,
                   last_value_SCD4X_CO2, last_value_SCD4X_T, last_value_SCD4X_H);
          if (data_transmit_laeq_to_sc)     SENSOR_SC_POST("17", sensor_sc_json);
          if (data_transmit_laeq_to_madavi) SENSOR_MADAVI_POST("17", sensor_sc_json);
        }
      }

#undef SENSOR_SC_POST
#undef SENSOR_MADAVI_POST
#undef SENSOR_INFLUX_QUEUE
    } /* end sensor transmission block */

    /* push JSON to custom API endpoint (if enabled) */
    if (custom_api_send_1st) send_to_custom_api(custom_api_spectrum_1st, 1, 0);

    if (data_laeq_1st_output_on_terminal || data_lzeq_1st_output_on_terminal || data_la_spec_1st_output_on_terminal || data_lz_spec_1st_output_on_terminal) {
      pthread_mutex_lock(&print_mutex);

      bzero((char *)&print_string_1st, sizeof(print_string_1st));

      sprintf((char *)&print_string_1st, "%s", timestamp_4_log_A_1st);

      if (data_laeq_1st_output_on_terminal || data_la_spec_1st_output_on_terminal) {  // LA values
        if (data_laeq_1st_output_on_terminal) {
          sprintf(part_string_1st, "1,A,%.2f,%.2f,%.2f",
                  last_value_dnms_laeq, last_value_dnms_lamin,
                  last_value_dnms_lamax);
          strcat(print_string_1st, part_string_1st);
        } else {
          strcat(print_string_1st, "1,A, , , ");
        }

        if (data_la_spec_1st_output_on_terminal) {
          for (unsigned i = 0; i < 31; i++) {
            sprintf(part_string_1st, ",%.2f", last_value_dnms_spectrum[i]);
            strcat(print_string_1st, part_string_1st);
          }
        }
        strcat(print_string_1st, "\n");  // new line

        // output on terminal configured ??
        if (data_on_terminal) {
          printf("%s", print_string_1st);
        }

        // write to named pipe configured ??
        if (data_transmit_via_pipe) {
          write(dnms_pipe_fd, print_string_1st, strlen(print_string_1st));
        }

        // data logging configured ??
        if (data_logging) {
          logging_write(print_string_1st, strlen(print_string_1st));
        }
      }

      bzero((char *)&print_string_1st, sizeof(print_string_1st));

      sprintf((char *)&print_string_1st, "%s", timestamp_4_log_Z_1st);

      if (data_lzeq_1st_output_on_terminal || data_lz_spec_1st_output_on_terminal) {  // LZ values
        if (data_lzeq_1st_output_on_terminal) {
          sprintf(part_string_1st, "1,Z,%.2f,%.2f,%.2f",
                  last_value_dnms_lzeq, last_value_dnms_lzmin,
                  last_value_dnms_lzmax);
          strcat(print_string_1st, part_string_1st);
        } else {
          strcat(print_string_1st, "1,Z, , , ");
        }

        if (data_lz_spec_1st_output_on_terminal) {
          for (unsigned i = 0; i < 31; i++) {
            sprintf(part_string_1st, ",%.2f", last_value_dnms_z_spectrum[i]);
            strcat(print_string_1st, part_string_1st);
          }
        }
        strcat(print_string_1st, "\n");  // new line

        // output on terminal configured ??
        if (data_on_terminal) {
          printf("%s", print_string_1st);
        }

        // write to named pipe configured ??
        if (data_transmit_via_pipe) {
          write(dnms_pipe_fd, print_string_1st, strlen(print_string_1st));
        }

        // data logging configured ??
        if (data_logging) {
          logging_write(print_string_1st, strlen(print_string_1st));
        }
      }

      bzero((char *)&print_string_1st, sizeof(print_string_1st));

      sprintf((char *)&print_string_1st, "%s", timestamp_4_log_C_1st);

      if (data_lceq_1st_output_on_terminal || data_lc_spec_1st_output_on_terminal) {  // LC values
        if (data_lceq_1st_output_on_terminal) {
          sprintf(part_string_1st, "1,C,%.2f,%.2f,%.2f",
                  last_value_dnms_lceq, last_value_dnms_lcmin,
                  last_value_dnms_lcmax);
          strcat(print_string_1st, part_string_1st);
        } else {
          strcat(print_string_1st, "1,C, , , ");
        }

        if (data_lc_spec_1st_output_on_terminal) {
          for (unsigned i = 0; i < 31; i++) {
            sprintf(part_string_1st, ",%.2f", last_value_dnms_c_spectrum[i]);
            strcat(print_string_1st, part_string_1st);
          }
        }
        strcat(print_string_1st, "\n");  // new line

        // output on terminal configured ??
        if (data_on_terminal) {
          printf("%s", print_string_1st);
        }

        // write to named pipe configured ??
        if (data_transmit_via_pipe) {
          write(dnms_pipe_fd, print_string_1st, strlen(print_string_1st));
        }

        // data logging configured ??
        if (data_logging) {
          logging_write(print_string_1st, strlen(print_string_1st));
        }
      }


      fflush(stdout);
      pthread_mutex_unlock(&print_mutex);
    }
  }
}


/* ******************************************************************************************************************** */
void *print_ipc_and_log_2nd_function(void *ptr) {
  setprio(pthread_self(), SCHED_RR, prio_print_ipc_and_log_2nd);
  if (output_thread_info) {
    getprio(pthread_self(), "print_ipc_and_log_2nd_thread");
  }

  for (;;) {
    /* wait until a 2nd interval measurement is done and a signal/broadcast for new data arrives */
    pthread_mutex_lock(&new_data_2nd_mutex);
    pthread_cond_wait(&new_data_2nd_cond, &new_data_2nd_mutex);
    pthread_mutex_unlock(&new_data_2nd_mutex);

    if (data_laeq_2nd_output_on_terminal || data_lzeq_2nd_output_on_terminal || data_la_spec_2nd_output_on_terminal || data_lz_spec_2nd_output_on_terminal && (counter_threshold_2nd > 0)) {
      pthread_mutex_lock(&print_mutex);

      bzero((char *)&print_string_2nd, sizeof(print_string_2nd));

      sprintf((char *)&print_string_2nd, "%s", timestamp_4_log_A_2nd);

      if (data_laeq_2nd_output_on_terminal || data_la_spec_2nd_output_on_terminal) {  // LA values
        if (data_laeq_2nd_output_on_terminal) {
          sprintf(part_string_2nd, "2,A,%.2f,%.2f,%.2f",
                  last_value_dnms_laeq_2nd, last_value_dnms_lamin_2nd,
                  last_value_dnms_lamax_2nd);
          strcat(print_string_2nd, part_string_2nd);
        } else {
          strcat(print_string_2nd, "2,A, , , ");
        }

        if (data_la_spec_2nd_output_on_terminal) {
          for (unsigned i = 0; i < 31; i++) {
            sprintf(part_string_2nd, ",%.2f", last_value_dnms_spectrum_2nd[i]);
            strcat(print_string_2nd, part_string_2nd);
          }
        }
        strcat(print_string_2nd, "\n");  // new line

        // output on terminal configured ??
        if (data_on_terminal) {
          printf("%s", print_string_2nd);
        }

        // write to named pipe configured ??
        if (data_transmit_via_pipe) {
          write(dnms_pipe_fd, print_string_2nd, strlen(print_string_2nd));
        }

        // data logging configured ??
        if (data_logging) {
          logging_write(print_string_2nd, strlen(print_string_2nd));
        }
      }

      bzero((char *)&print_string_2nd, sizeof(print_string_2nd));

      sprintf((char *)&print_string_2nd, "%s", timestamp_4_log_Z_2nd);

      if (data_lzeq_2nd_output_on_terminal || data_lz_spec_2nd_output_on_terminal) {  // LZ values
        if (data_lzeq_2nd_output_on_terminal) {
          sprintf(part_string_2nd, "2,Z,%.2f,%.2f,%.2f",
                  last_value_dnms_lzeq_2nd, last_value_dnms_lzmin_2nd,
                  last_value_dnms_lzmax_2nd);
          strcat(print_string_2nd, part_string_2nd);
        } else {
          strcat(print_string_2nd, "2,Z, , , ");
        }

        if (data_lz_spec_2nd_output_on_terminal) {
          for (unsigned i = 0; i < 31; i++) {
            sprintf(part_string_2nd, ",%.2f", last_value_dnms_z_spectrum_2nd[i]);
            strcat(print_string_2nd, part_string_2nd);
          }
        }
        strcat(print_string_2nd, "\n");  // new line

        // output on terminal configured ??
        if (data_on_terminal) {
          printf("%s", print_string_2nd);
        }

        // write to named pipe configured ??
        if (data_transmit_via_pipe) {
          write(dnms_pipe_fd, print_string_2nd, strlen(print_string_2nd));
        }

        // data logging configured ??
        if (data_logging) {
          logging_write(print_string_2nd, strlen(print_string_2nd));
        }
      }


      bzero((char *)&print_string_2nd, sizeof(print_string_2nd));

      sprintf((char *)&print_string_2nd, "%s", timestamp_4_log_C_2nd);

      if (data_lceq_2nd_output_on_terminal || data_lc_spec_2nd_output_on_terminal) {  // LC values
        if (data_lceq_2nd_output_on_terminal) {
          sprintf(part_string_2nd, "2,C,%.2f,%.2f,%.2f",
                  last_value_dnms_lceq_2nd, last_value_dnms_lcmin_2nd,
                  last_value_dnms_lcmax_2nd);
          strcat(print_string_2nd, part_string_2nd);
        } else {
          strcat(print_string_2nd, "2,C, , , ");
        }

        if (data_lc_spec_2nd_output_on_terminal) {
          for (unsigned i = 0; i < 31; i++) {
            sprintf(part_string_2nd, ",%.2f", last_value_dnms_c_spectrum_2nd[i]);
            strcat(print_string_2nd, part_string_2nd);
          }
        }
        strcat(print_string_2nd, "\n");  // new line

        // output on terminal configured ??
        if (data_on_terminal) {
          printf("%s", print_string_2nd);
        }

        // write to named pipe configured ??
        if (data_transmit_via_pipe) {
          write(dnms_pipe_fd, print_string_2nd, strlen(print_string_2nd));
        }

        // data logging configured ??
        if (data_logging) {
          logging_write(print_string_2nd, strlen(print_string_2nd));
        }
      }

      fflush(stdout);
      pthread_mutex_unlock(&print_mutex);
    }

    /* push JSON to custom API endpoint at 2nd interval (if enabled) */
    if (custom_api_send_2nd) send_to_custom_api(custom_api_spectrum_2nd, 0, 1);
  }
}



/* ******************************************************************************************************************** */
/* if external start stop is configured this function will become active */
void *start_stop_function(void *ptr) {

  int loop_number = 0;

  setprio(pthread_self(), SCHED_RR, prio_start_stop);
  if (output_thread_info) {
    getprio(pthread_self(), "start_stop_thread");
  }

  while (1) {  // wait for message via named pipe in endless loop
    usleep(20000);
    if (read(start_stop_pipe_fd, start_stop_buffer, start_stop_buffer_size) == -1) {
      if (errno == EAGAIN) {
        // nothing to do
      } else {
        perror("read pipe");
        exit(EXIT_FAILURE);
      }
    } else {

      if (start_stop_buffer[0] == '0') {

        transmission_enabled = false;
      } else {
        if (start_stop_buffer[0] == '1') {

          transmission_enabled = true;
        }
      }
    }
  }
}


/* ******************************************************************************************************************** */
void *wlan_function(void *ptr) {
  int res;
  bool something_to_transmit;
  uint16_t transmit_again;

  setprio(pthread_self(), SCHED_RR, prio_wlan);
  if (output_thread_info) {
    getprio(pthread_self(), "wlan_thread");
  }

  for (;;) {
    something_to_transmit = false;
    transmit_again = 0;
    usleep(20000);


    /* check buffers, if there is something to transmit ? */
    pthread_mutex_lock(&buffer_influx_mutex);
    if (buffer_out_influx((char *)send_data, (char *)send_timestamp, &send_last_tt, &destination) == buffer_success) {
      something_to_transmit = true;
    } else {
      pthread_mutex_lock(&buffer_sc_mutex);
      if (buffer_out_sc((char *)send_data, (char *)send_timestamp, &send_last_tt, &destination) == buffer_success) {
        something_to_transmit = true;
      }
      pthread_mutex_unlock(&buffer_sc_mutex);
    }
    pthread_mutex_unlock(&buffer_influx_mutex);

    if (something_to_transmit) {
	  if (enable_wifi_signal_strength_influxdb || enable_wifi_signal_strength_mqtt || data_transmit_laeq_to_madavi) {
		  wifi_signal_strength = get_signal_strength();
		  sprintf(string_wifi_signal_strength, "%3d", wifi_signal_strength);
	  }
      /* ***** transmission to Sensor.Community ?? ***** */
      if (destination == dest_sc) {
        /* starttime of actual WLAN transmission to SC*/
        gettimeofday(&t1, NULL);
        /* https or http transmision ?? */
        if (sc_transmit_https) {
          if (!secure_connect(sc_hostname)) {
            sc_error_count++;
            printf("SC https: connection failed, skipping\n");
            goto sc_skip;
          }

          for (unsigned i = 0; i < 100; i++) {
            if ((res = BIO_puts(bio, send_data)) > 0) {
              break;
            }
            usleep(2000);
          }
          if (res <= 0) {
            BIO_free_all(bio); SSL_CTX_free(ctx); bio = NULL;
            sc_error_count++;
            printf("SC https: send failed, skipping\n");
            goto sc_skip;
          }
          /* drain HTTP response so server can flush before TLS close_notify */
          { char _drain[256]; while (BIO_read(bio, _drain, sizeof(_drain)) > 0) {} }

          BIO_free_all(bio);
          SSL_CTX_free(ctx);
        } else {
          /* http Socket connection to Sensor.Community server  */
sc_again_http:
          transmit_again++;
          for (unsigned i = 0; i < 100; i++) {
            if ((bio = BIO_new(BIO_s_connect())) != NULL) {
              break;
            }
            usleep(2000);
          }
          if (bio == NULL) {
            if (transmit_again <= 3) {
              goto sc_again_http;
            } else {
              sc_error_count++;
              printf("SC http: BIO_new failed, skipping\n");
              goto sc_skip;
            }
          }

          BIO_set_conn_hostname(bio, sc_hostname);

          res = bio_do_connect_timed(bio, 5);
          if (res <= 0) {
            BIO_free_all(bio); bio = NULL;
            if (transmit_again <= 3) {
              goto sc_again_http;
            } else {
              sc_error_count++;
              printf("SC http: connection to %s failed, skipping\n", sc_hostname);
              goto sc_skip;
            }
          }
          for (unsigned i = 0; i < 100; i++) {
            if ((res = BIO_puts(bio, send_data)) > 0) {
//			 printf("SC data: %s\n\n", send_data);
              break;
            }
            usleep(2000);
          }
          if (res <= 0) {
            BIO_free_all(bio); bio = NULL;
            sc_error_count++;
            printf("SC http: send failed, skipping\n");
            goto sc_skip;
          }

          usleep(50000);

          /* close socket connection  */
          BIO_free_all(bio);
        }

        /* endtime of WLAN transmission to SC */
        gettimeofday(&t2, NULL);
        /* Berechne die verbrauchte Zeit in Microsekunden */
        elapsedTime = ((t2.tv_sec * 1000000) + t2.tv_usec) - ((t1.tv_sec * 1000000) + t1.tv_usec);
        last_sc_transmission_time = elapsedTime / 1000;
sc_skip: ;
      }

      if (destination == dest_madavi) { /* transmission to Madavi */
        gettimeofday(&t1, NULL);
        for (unsigned i = 0; i < 100; i++) {
          if ((bio = BIO_new(BIO_s_connect())) != NULL) break;
          usleep(2000);
        }
        if (bio != NULL) {
          BIO_set_conn_hostname(bio, madavi_hostname);
          res = bio_do_connect_timed(bio, 5);
          if (res > 0) {
            for (unsigned i = 0; i < 100; i++) {
              if ((res = BIO_puts(bio, send_data)) > 0) break;
              usleep(2000);
            }
            if (res <= 0) {
              madavi_error_count++;
              printf("Madavi: error transmitting data\n");
            }
          } else {
            madavi_error_count++;
            printf("Madavi: connection to %s failed\n", madavi_hostname);
          }
          usleep(50000);
          BIO_free_all(bio);
        }
        gettimeofday(&t2, NULL);
        elapsedTime = ((t2.tv_sec * 1000000) + t2.tv_usec) - ((t1.tv_sec * 1000000) + t1.tv_usec);
      }

      if (destination == dest_mqtt) { /* transmission to MQTT Broker */
        if (enable_wifi_signal_strength_mqtt) {
          strcat(send_data, ",signal=");
          strcat(send_data, string_wifi_signal_strength);
        }
        if (last_mqtt_transmission_time_to_payload) {
          sprintf(value_2_string_wlan, "%ld ", last_mqtt_transmission_time);
          strcat(send_data, ",last_transmission_time=");
          strcat(send_data, value_2_string_wlan);
        }
        strcat(send_data, send_timestamp);

        gettimeofday(&t0, NULL);
        rc = mosquitto_publish(mosq, NULL, mqtt_topic, strlen(send_data), send_data, mqtt_qos, false);
        if (rc != MOSQ_ERR_SUCCESS) {
          mqtt_error_count++;
          bzero((char *)&mqtt_error_timestamp, sizeof(mqtt_error_timestamp));
          bzero((char *)&zeit_string_mqtt, sizeof(zeit_string_mqtt));
          gettimeofday(&mqtt_error_tv, NULL);
          ns_mqtt = (uint64_t)mqtt_error_tv.tv_sec * 1000000000 + (uint64_t)mqtt_error_tv.tv_usec * 1000;
          rawtime_mqtt = ns_mqtt / 1000000000;
          ts_mqtt_1 = localtime(&rawtime_mqtt);
          ts_mqtt_2 = localtime(&mqtt_error_tv.tv_sec);
          sprintf(mqtt_error_timestamp, "%llu", ns_mqtt);
          strftime(zeit_string_mqtt, 80, fmt_d, ts_mqtt_1);
          sprintf(value_2_string, " %02d:%02d:%02d.%03d ", ts_mqtt_2->tm_hour, ts_mqtt_2->tm_min, ts_mqtt_2->tm_sec, (int)(mqtt_error_tv.tv_usec / 1000));
          strcat(zeit_string_mqtt, value_2_string);
          strcat(zeit_string_mqtt, "MQTT error publishing: ");
          strcat(zeit_string_mqtt, mosquitto_strerror(rc));
          fprintf(stderr, "%s\n", zeit_string_mqtt);
        }
        gettimeofday(&t2, NULL);
        elapsedTime = ((t2.tv_sec * 1000000) + t2.tv_usec) - ((t0.tv_sec * 1000000) + t0.tv_usec);
        last_mqtt_transmission_time = elapsedTime / 1000;
        if (last_mqtt_transmission_time == 0) last_mqtt_transmission_time = 1;
      }

      if (destination == dest_influxdb) { /* transmission to InfluxDB */
        /* starttime of actual WLAN transmission to InfluxDB*/
        gettimeofday(&t0, NULL);
        if (enable_wifi_signal_strength_influxdb) {
          strcat(send_data, ",signal=");
          strcat(send_data, string_wifi_signal_strength);
        }
        if (last_influxdb_transmission_time_to_payload) {
          sprintf(value_2_string_wlan, "%ld ", send_last_tt);
          strcat(send_data, ",last_transmission_time=");
          strcat(send_data, value_2_string_wlan);
        }
        strcat(send_data, send_timestamp);

        length_data_str = strlen(send_data);
        sprintf(str_of_length_data_str, "%d", length_data_str);
        bzero((char *)&msg, sizeof(msg));
        strcat(msg, msg_header1);
        encoded_user_passwd = b64_encode((const unsigned char *)influxdb_user_passwd, strlen(influxdb_user_passwd));
        strcat(msg, encoded_user_passwd);
        strcat(msg, msg_header2);
        strcat(msg, str_of_length_data_str);
        strcat(msg, msg_header3);
        strcat(msg, send_data);

        /*   ******** transmission to InfluxDB  with https ??  ************** */
        if (influxdb_transmit_https) {
influx_again_https:
          transmit_again++;
          if (!secure_connect(influxdb_hostname)) {
            if (transmit_again <= 3) goto influx_again_https;
            influxdb_error_count++;
            printf("InfluxDB https: connection failed after %d retries, skipping\n", transmit_again);
            goto influx_skip;
          }

          for (unsigned i = 0; i < 100; i++) {
            if ((res = BIO_puts(bio, msg)) > 0) {
              break;
            }
            usleep(2000);
          }
          if (res <= 0) {
            BIO_free_all(bio); SSL_CTX_free(ctx); bio = NULL;
            if (transmit_again <= 3) {
              goto influx_again_https;
            } else {
              influxdb_error_count++;
              printf("InfluxDB https: send failed after %d retries, skipping\n", transmit_again);
              goto influx_skip;
            }
          }
          /* drain HTTP response so server can flush before TLS close_notify */
          { char _drain[256]; while (BIO_read(bio, _drain, sizeof(_drain)) > 0) {} }

          BIO_free_all(bio);
          SSL_CTX_free(ctx);

          /* endtime of WLAN transmission to influxdb */
          gettimeofday(&t2, NULL);
          /* Berechne die verbrauchte Zeit in Microsekunden */
          elapsedTime = ((t2.tv_sec * 1000000) + t2.tv_usec) - ((t0.tv_sec * 1000000) + t0.tv_usec);
          last_influxdb_transmission_time = elapsedTime / 1000;
          if (last_influxdb_transmission_time == 0) {
            last_influxdb_transmission_time = 1;
          }
          pthread_mutex_lock(&buffer_influx_mutex);
          buffer_in_influx_last_tt(last_influxdb_transmission_time);
          pthread_mutex_unlock(&buffer_influx_mutex);
        }

        /*   ******** transmission to InfluxDB  with http ?? ************** */
        if (influxdb_transmit_http) {
influx_again_http:
          transmit_again++;
          /* Socket connection to InflluxDB server  */
          for (unsigned i = 0; i < 100; i++) {
            if ((bio = BIO_new(BIO_s_connect())) != NULL) {
              break;
            }
            usleep(2000);
          }
          if (bio == NULL) {
            if (transmit_again <= 3) {
              goto influx_again_http;
            } else {
              influxdb_error_count++;
              printf("InfluxDB http: BIO_new failed, skipping\n");
              goto influx_skip;
            }
          }

          BIO_set_conn_hostname(bio, influxdb_hostname);

          res = bio_do_connect_timed(bio, 5);
          if (res <= 0) {
            BIO_free_all(bio); bio = NULL;
            if (transmit_again <= 3) {
              goto influx_again_http;
            } else {
              influxdb_error_count++;
              printf("InfluxDB http: connection to %s failed, skipping\n", influxdb_hostname);
              goto influx_skip;
            }
          }

          for (unsigned i = 0; i < 100; i++) {
            if ((res = BIO_puts(bio, msg)) > 0) {
//				printf("\n%s\n", msg);  // only for test purposes
              break;
            }
            usleep(2000);
          }

          if (res <= 0) {
            BIO_free_all(bio); bio = NULL;
            if (transmit_again <= 3) {
              goto influx_again_http;
            } else {
              influxdb_error_count++;
              printf("InfluxDB http: send failed, skipping\n");
              goto influx_skip;
            }
          }

          usleep(50000);

          /* close socket connection  */
          BIO_free_all(bio);

          /* endtime of WLAN transmission to influxdb */
          gettimeofday(&t2, NULL);
          /* calculate used time for transmission */
          elapsedTime = ((t2.tv_sec * 1000000) + t2.tv_usec) - ((t0.tv_sec * 1000000) + t0.tv_usec);
          last_influxdb_transmission_time = elapsedTime / 1000;
          if (last_influxdb_transmission_time == 0) {
            last_influxdb_transmission_time = 1;
          }
          pthread_mutex_lock(&buffer_influx_mutex);
          buffer_in_influx_last_tt(last_influxdb_transmission_time);
          pthread_mutex_unlock(&buffer_influx_mutex);
        }
influx_skip: ;
      }

      time(&time_wlan);
      ts_wlan = localtime(&time_wlan);
      strftime(zeit_string_wlan, 80, fmt_dt, ts_wlan);
      if ((last_influxdb_transmission_time_to_terminal && (destination == dest_influxdb)) && (last_influxdb_transmission_time >= threshold_output_influxdb_ltt_to_terminal)) {
        printf("%s  ", zeit_string_wlan);
        printf("last transmission time to InfluxDB %s: %ld ms\n",
               influxdb_transmit_https ? "via https" : "via http",
               last_influxdb_transmission_time);
      }

      if ((last_mqtt_transmission_time_to_terminal && (destination == dest_mqtt)) && (last_mqtt_transmission_time >= threshold_output_mqtt_ltt_to_terminal)) {
        printf("%s  ", zeit_string_wlan);
        printf("last transmission time to MQTT Broker %s: %ld ms\n",
               mqtt_use_tls ? "via MQTT/TLS" : "via MQTT",
               last_mqtt_transmission_time);
      }

      if ((last_sc_transmission_time_to_terminal && (destination == dest_sc)) && (last_sc_transmission_time >= threshold_output_sc_ltt_to_terminal)) {
        printf("%s  ", zeit_string_wlan);
        printf("last transmission time to Sensor.Community %s: %ld ms\n",
               sc_transmit_https ? "via https" : "via http",
               last_sc_transmission_time);
      }
    } else {
      // nothing to do
    }
    fflush(stdout);
  }
}


/* ******************************************************************************************************************** */
void *measurement_1st_interval_timer_function(void *ptr) {

  setprio(pthread_self(), SCHED_RR, prio_1st_timer);
  if (output_thread_info) {
    getprio(pthread_self(), "measurement_1st_interval_timer_thread");
  }
  // wait for start
  pthread_mutex_lock(&start_1st_measurement_mutex);
  pthread_cond_wait(&start_1st_measurement_cond, &start_1st_measurement_mutex);
  pthread_mutex_unlock(&start_1st_measurement_mutex);

  for (;;) {
    /* Re-anchor to CLOCK_REALTIME each cycle so NTP corrections converge
       inter-device offset over time (see 2nd timer for full rationale). */
    struct timespec now_rt;
    clock_gettime(CLOCK_REALTIME, &now_rt);
    int64_t now_ms    = (int64_t)now_rt.tv_sec * 1000 + now_rt.tv_nsec / 1000000;
    int64_t start_ms  = (int64_t)measurement_start_time * 1000;
    int64_t n         = (now_ms - start_ms) / (int64_t)measurement_1st_interval_ms;
    int64_t target_ms = start_ms + (n + 1) * (int64_t)measurement_1st_interval_ms;
    struct timespec target_rt;
    target_rt.tv_sec  = (time_t)(target_ms / 1000);
    target_rt.tv_nsec = (long)(target_ms % 1000) * 1000000L;
    while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &target_rt, NULL) == EINTR)
      ;
    {
      struct timespec actual_rt;
      clock_gettime(CLOCK_REALTIME, &actual_rt);
      g_timer_fire_1st.tv_sec  = actual_rt.tv_sec;
      g_timer_fire_1st.tv_usec = (suseconds_t)(actual_rt.tv_nsec / 1000);
    }
    pthread_mutex_lock(&new_1st_measurement_mutex);
    pthread_cond_signal(&new_1st_measurement_cond);
    pthread_mutex_unlock(&new_1st_measurement_mutex);
  }
}


/* ******************************************************************************************************************** */
void *measurement_1st_interval_function(void *ptr) {
  int ret;

  setprio(pthread_self(), SCHED_RR, prio_1st_measurement);
  if (output_thread_info) {
    getprio(pthread_self(), "measurement_1st_interval_thread");
  }

  for (;;) {
    pthread_mutex_lock(&new_1st_measurement_mutex);
    pthread_cond_wait(&new_1st_measurement_cond, &new_1st_measurement_mutex);
    pthread_mutex_unlock(&new_1st_measurement_mutex);

    /* ── Phase 1: send trigger (brief mutex hold) ───────────────────────
       The DNMS snapshots the 1st interval the instant it receives cmd 3.
       tv_1st is recorded here so the timestamp reflects the true end of the
       measurement window, not the (later) time when reads complete. */
    pthread_mutex_lock(&dnms_mutex);
    dnms_error = false;
    if (dnms_calculate_leq() != 0) {
      dnms_error_count++;
      dnms_error = true;
      print_time();
      printf("Error DNMS_CALCULATE_LEQ\n\n");
      close_dnms();
      open_dnms();
      print_time();
      printf("try to reset DNMS there was an Error\n\n");
      sleep(1);
      pthread_mutex_unlock(&dnms_mutex);
      fflush(stdout);
      pthread_mutex_lock(&new_data_1st_mutex);
      if (transmission_enabled) {
        pthread_cond_broadcast(&new_data_1st_cond);
      }
      pthread_mutex_unlock(&new_data_1st_mutex);
      continue;
    }
    tv_1st = g_timer_fire_1st;             /* use timer-fire time, not post-I2C time */
    pthread_mutex_unlock(&dnms_mutex);     /* release immediately after trigger */

    /* Yield between Phase 1 and Phase 2 so the lower-priority 2nd-interval
       thread (SCHED_RR prio 5) can run.  How long depends on which path the
       2nd thread uses:
       ─ 2nd < 1st  (e.g. 1st=500ms, 2nd=125ms):
           The 2nd timer signals directly (not via interval_2nd_due) so the
           2nd thread may be competing for the mutex RIGHT NOW.  Sleep 50ms —
           enough for it to complete Phase 1 + Phase 2 (all reads) before we
           re-acquire the mutex.  Skipping this sleep causes a ~200ms delivery
           gap at every coincidence boundary.
       ─ 2nd >= 1st  (e.g. 1st=125ms, 2nd=500ms):
           The 2nd thread NEVER runs concurrently here; it is only signalled
           at the END of this function via interval_2nd_due.  A 50ms sleep on
           every 125ms cycle would waste 40% of the period and introduce
           scheduler jitter.  Use the minimal 2ms yield instead. */
    if (interval_2nd_active &&
        measurement_2nd_interval_ms < measurement_1st_interval_ms) {
      usleep(50000);
    } else {
      usleep(2000);
    }

    /* ── Phase 2: read results (separate mutex hold) ────────────────────
       The DNMS snapshot is already taken; reads just fetch buffered data. */
    pthread_mutex_lock(&dnms_mutex);
    /* is data ready? */
    for (unsigned i = 0; i < 1000; i++) {
      ret = dnms_read_data_ready(&data_ready);
      if ((ret == 0) && (data_ready != 0)) {
        break;
      }
      usleep(20);
    }

    if (!dnms_error) {
      if (dnms_read_leq(DNMS_CMD_READ_LEQ, &dnms_values) == 0) {
        if (dnms_values.leq_x == 0) {
          dnms_error = true;
          print_time();
          printf("LAeq 1st = 0!\n");
        }
        last_value_dnms_laeq = dnms_values.leq_x + dnms_correction;
        last_value_dnms_lamin = dnms_values.leq_x_min + dnms_correction;
        last_value_dnms_lamax = dnms_values.leq_x_max + dnms_correction;

        if (read_laeq_spec_1st) {
          if (dnms_read_freq_spec(DNMS_CMD_READ_FFT_PART1, &dnms_values) == 0) {
            for (unsigned i = 0; i < 31; i++) {
              last_value_dnms_spectrum[i] = dnms_values.leq_freq_spec_x[i];
            }
          } else {
            dnms_error = true;
            print_time();
            printf("Error DNMS_CMD_READ_FFT\n\n");
          }
        }
      } else {
        dnms_error = true;
        print_time();
        printf("Error DNMS_CMD_READ_LEQ\n\n");
      }

      if (read_lzeq_1st) {
        if (dnms_read_leq(DNMS_CMD_READ_LEQ_Z, &dnms_values) == 0) {
          last_value_dnms_lzeq = dnms_values.leq_x + dnms_correction;
          last_value_dnms_lzmin = dnms_values.leq_x_min + dnms_correction;
          last_value_dnms_lzmax = dnms_values.leq_x_max + dnms_correction;
        } else {
          dnms_error = true;
          print_time();
          printf("Error DNMS_CMD_READ_LEQ_Z\n\n");
        }
      }
      
      if (read_lzeq_spec_1st) {
	if (dnms_read_freq_spec(DNMS_CMD_READ_FFT_Z_PART1, &dnms_values) == 0) {
	  for (unsigned i = 0; i < 31; i++) {
	    last_value_dnms_z_spectrum[i] = dnms_values.leq_freq_spec_x[i];
	  }
	} else {
	  dnms_error = true;
	  print_time();
	  printf("Error DNMS_CMD_READ_FFT_Z\n\n");
	}
      }
      
      if (read_lceq_1st) {
        if (dnms_read_leq(DNMS_CMD_READ_LEQ_C, &dnms_values) == 0) {
          last_value_dnms_lceq = dnms_values.leq_x + dnms_correction;
          last_value_dnms_lcmin = dnms_values.leq_x_min + dnms_correction;
          last_value_dnms_lcmax = dnms_values.leq_x_max + dnms_correction;
        } else {
          dnms_error = true;
          print_time();
          printf("Error DNMS_CMD_READ_LEQ_C\n\n");
        }
      }
      
      if (read_lceq_spec_1st) {
	if (dnms_read_freq_spec(DNMS_CMD_READ_FFT_C_PART1, &dnms_values) == 0) {
	  for (unsigned i = 0; i < 31; i++) {
	    last_value_dnms_c_spectrum[i] = dnms_values.leq_freq_spec_x[i];
	  }
	} else {
	  dnms_error = true;
	  print_time();
	  printf("Error DNMS_CMD_READ_FFT_C\n\n");
	}
      }      
           
    }

    if (dnms_error) {
      dnms_error_count++;
      close_dnms();
      open_dnms();
      print_time();
      printf("try to reset DNMS there was an Error\n\n");
      sleep(1);
      /* unlock DNMS access (Phase 2 mutex) */
      pthread_mutex_unlock(&dnms_mutex);
    } else {
      /* read environment sensors while I2C bus (dnms_mutex) is still held */
      if (measurement_1st_interval_ms > 29000) {
        sensors_rpi_read();
      }
      /* unlock DNMS access (Phase 2 mutex) */
      pthread_mutex_unlock(&dnms_mutex);

      counter_measurements_1st++;

      /* tv_1st was set in Phase 1 right after cmd 3 — no new gettimeofday.
         Timestamp formatting is fast (~1ms); all heavy work (InfluxDB/SC
         string building, buffering) is done in the print thread so this
         high-priority thread releases the CPU as quickly as possible. */
      bzero((char *)&timestamp_1st, sizeof(timestamp_1st));
      bzero((char *)&timestamp_4_log_A_1st, sizeof(timestamp_4_log_A_1st));

      ns_1st = (uint64_t)tv_1st.tv_sec * 1000000000 + (uint64_t)tv_1st.tv_usec * 1000;
      rawtime_1st = ns_1st / 1000000000;
      struct tm ts_local_1_1st, ts_local_2_1st;
      localtime_r(&rawtime_1st, &ts_local_1_1st);
      localtime_r(&tv_1st.tv_sec, &ts_local_2_1st);
      sprintf(timestamp_1st, "%llu", ns_1st);

      strftime(timestamp_4_log_A_1st, 80, fmt_dl, &ts_local_1_1st);
      sprintf(value_2_string, " %02d:%02d:%02d.%03d, ", ts_local_2_1st.tm_hour, ts_local_2_1st.tm_min, ts_local_2_1st.tm_sec, (int)(tv_1st.tv_usec / 1000));
      strcat(timestamp_4_log_A_1st, value_2_string);
      strcpy(timestamp_4_log_Z_1st, timestamp_4_log_A_1st);
      strcpy(timestamp_4_log_C_1st, timestamp_4_log_A_1st);

      /* 1st measurement done — now trigger 2nd interval if it became due while we
         were measuring, so CALCULATE_LEQ_2nd always follows CALCULATE_LEQ. */
      if (interval_2nd_active && interval_2nd_due) {
        interval_2nd_due = false;
        pthread_mutex_lock(&new_2nd_measurement_mutex);
        pthread_cond_signal(&new_2nd_measurement_cond);
        pthread_mutex_unlock(&new_2nd_measurement_mutex);
      }
    }
    /* Signal the print thread — InfluxDB/SC formatting and buffering
       happen there at low priority (prio_print_ipc_and_log_1st). */
    fflush(stdout);
    pthread_mutex_lock(&new_data_1st_mutex);
    if (transmission_enabled) {
      pthread_cond_broadcast(&new_data_1st_cond);
    }
    pthread_mutex_unlock(&new_data_1st_mutex);
  }
}


/* ******************************************************************************************************************** */
int get_config(void)
{
    config_init(&cfg);

    /* Read config file */
    if (!config_read_file(&cfg, "dnms.conf")) {
        fprintf(stderr, "%s:%d - %s\n",
                config_error_file(&cfg),
                config_error_line(&cfg),
                config_error_text(&cfg));
        config_destroy(&cfg);
        return EXIT_FAILURE;
    }

    /* --- integers ---------------------------------------------------- */
    config_lookup_int(&cfg, "waiting_time_after_start", &waiting_time_after_start);
    config_lookup_int(&cfg, "prio_1st_timer", &prio_1st_timer);
    config_lookup_int(&cfg, "prio_1st_measurement", &prio_1st_measurement);
    config_lookup_int(&cfg, "prio_2nd_timer", &prio_2nd_timer);
    config_lookup_int(&cfg, "prio_2nd_measurement", &prio_2nd_measurement);
    config_lookup_int(&cfg, "prio_wlan", &prio_wlan);
    config_lookup_int(&cfg, "prio_print_ipc_and_log_1st", &prio_print_ipc_and_log_1st);
    config_lookup_int(&cfg, "prio_print_ipc_and_log_2nd", &prio_print_ipc_and_log_2nd);
    config_lookup_int(&cfg, "prio_start_stop", &prio_start_stop);
    
     config_lookup_int(&cfg, "dnms_microphone", &dnms_microphone);
    config_lookup_int(&cfg, "dnms_blink_period_1", &dnms_blink_period_1);
    config_lookup_int(&cfg, "dnms_blink_period_2", &dnms_blink_period_2);
    config_lookup_int(&cfg, "dnms_blink_period_3", &dnms_blink_period_3);
    config_lookup_int(&cfg, "dnms_blink_period_4", &dnms_blink_period_4);
    config_lookup_int(&cfg, "dnms_blink_period_5", &dnms_blink_period_5);
    config_lookup_int(&cfg, "dnms_blink_period_6", &dnms_blink_period_6);
    config_lookup_int(&cfg, "dnms_blink_period_7", &dnms_blink_period_7);
    config_lookup_float(&cfg, "dnms_micro_const_1", &dnms_micro_const_1);
    config_lookup_float(&cfg, "dnms_micro_const_2", &dnms_micro_const_2);
    config_lookup_float(&cfg, "dnms_micro_const_3", &dnms_micro_const_3);
    config_lookup_float(&cfg, "dnms_micro_const_4", &dnms_micro_const_4);
    config_lookup_float(&cfg, "dnms_micro_const_5", &dnms_micro_const_5);
    config_lookup_float(&cfg, "dnms_micro_const_6", &dnms_micro_const_6);
    config_lookup_float(&cfg, "dnms_micro_const_7", &dnms_micro_const_7);

    config_lookup_int(&cfg, "measurement_1st_interval_ms", &measurement_1st_interval_ms);
    config_lookup_int(&cfg, "measurement_2nd_interval_ms", &measurement_2nd_interval_ms);
    config_lookup_int(&cfg, "threshold_2nd_interval_laeq_transmit", &threshold_2nd_interval_laeq_transmit);
    config_lookup_int(&cfg, "number_transmissions_after_exceeding", &number_transmissions_after_exceeding);
    config_lookup_int(&cfg, "gpio_output_pin", &gpio_output_pin);

    config_lookup_int(&cfg, "mqtt_port", &mqtt_port);
    config_lookup_int(&cfg, "mqtt_keepalive", &mqtt_keepalive);
    config_lookup_int(&cfg, "mqtt_qos", &mqtt_qos);
    config_lookup_bool(&cfg, "mqtt_use_tls", &mqtt_use_tls);
    config_lookup_string(&cfg, "mqtt_tls_cafile", &mqtt_tls_cafile);
    config_lookup_bool(&cfg, "mqtt_tls_insecure", &mqtt_tls_insecure);

    config_lookup_int(&cfg, "threshold_output_influxdb_ltt_to_terminal",
                      &threshold_output_influxdb_ltt_to_terminal);
    config_lookup_int(&cfg, "threshold_output_mqtt_ltt_to_terminal",
                      &threshold_output_mqtt_ltt_to_terminal);
    config_lookup_int(&cfg, "threshold_output_sc_ltt_to_terminal",
                      &threshold_output_sc_ltt_to_terminal);

    /* --- booleans ---------------------------------------------------- */
    config_lookup_bool(&cfg, "long_id", &long_id);
    config_lookup_bool(&cfg, "enable_wlan_or_lan", &enable_wlan_or_lan);
    config_lookup_bool(&cfg, "enable_wifi_signal_strength_influxdb", &enable_wifi_signal_strength_influxdb);
    config_lookup_bool(&cfg, "enable_wifi_signal_strength_mqtt",     &enable_wifi_signal_strength_mqtt);
    config_lookup_bool(&cfg, "last_influxdb_transmission_time_to_payload", &last_influxdb_transmission_time_to_payload);
    config_lookup_bool(&cfg, "last_mqtt_transmission_time_to_payload",     &last_mqtt_transmission_time_to_payload);

    config_lookup_bool(&cfg, "enable_1st_interval", &enable_1st_interval);
    config_lookup_bool(&cfg, "enable_2nd_interval", &enable_2nd_interval);
    config_lookup_bool(&cfg, "switch_output_pin", &switch_output_pin);
    config_lookup_bool(&cfg, "start_on_full_minute", &start_on_full_minute);
    config_lookup_bool(&cfg, "start_on_full_hour", &start_on_full_hour);

    config_lookup_bool(&cfg, "data_transmit_laeq_1st_to_influxdb", &data_transmit_laeq_1st_to_influxdb);
    config_lookup_bool(&cfg, "data_transmit_lzeq_1st_to_influxdb", &data_transmit_lzeq_1st_to_influxdb);
    config_lookup_bool(&cfg, "data_transmit_lceq_1st_to_influxdb", &data_transmit_lceq_1st_to_influxdb);    
    config_lookup_bool(&cfg, "data_transmit_laeq_1st_spectrum_to_influxdb", &data_transmit_laeq_1st_spectrum_to_influxdb);
    config_lookup_bool(&cfg, "data_transmit_lzeq_1st_spectrum_to_influxdb", &data_transmit_lzeq_1st_spectrum_to_influxdb);
    config_lookup_bool(&cfg, "data_transmit_lceq_1st_spectrum_to_influxdb", &data_transmit_lceq_1st_spectrum_to_influxdb);    
    config_lookup_bool(&cfg, "data_transmit_laeq_2nd_to_influxdb", &data_transmit_laeq_2nd_to_influxdb);
    config_lookup_bool(&cfg, "data_transmit_lzeq_2nd_to_influxdb", &data_transmit_lzeq_2nd_to_influxdb);
    config_lookup_bool(&cfg, "data_transmit_lceq_2nd_to_influxdb", &data_transmit_lceq_2nd_to_influxdb);    
    config_lookup_bool(&cfg, "data_transmit_laeq_2nd_spectrum_to_influxdb", &data_transmit_laeq_2nd_spectrum_to_influxdb);
    config_lookup_bool(&cfg, "data_transmit_lzeq_2nd_spectrum_to_influxdb", &data_transmit_lzeq_2nd_spectrum_to_influxdb);
    config_lookup_bool(&cfg, "data_transmit_lceq_2nd_spectrum_to_influxdb", &data_transmit_lceq_2nd_spectrum_to_influxdb);
    config_lookup_bool(&cfg, "data_transmit_laeq_1st_to_mqtt",          &data_transmit_laeq_1st_to_mqtt);
    config_lookup_bool(&cfg, "data_transmit_lzeq_1st_to_mqtt",          &data_transmit_lzeq_1st_to_mqtt);
    config_lookup_bool(&cfg, "data_transmit_lceq_1st_to_mqtt",          &data_transmit_lceq_1st_to_mqtt);
    config_lookup_bool(&cfg, "data_transmit_laeq_1st_spectrum_to_mqtt", &data_transmit_laeq_1st_spectrum_to_mqtt);
    config_lookup_bool(&cfg, "data_transmit_lzeq_1st_spectrum_to_mqtt", &data_transmit_lzeq_1st_spectrum_to_mqtt);
    config_lookup_bool(&cfg, "data_transmit_lceq_1st_spectrum_to_mqtt", &data_transmit_lceq_1st_spectrum_to_mqtt);
    config_lookup_bool(&cfg, "data_transmit_laeq_2nd_to_mqtt",          &data_transmit_laeq_2nd_to_mqtt);
    config_lookup_bool(&cfg, "data_transmit_lzeq_2nd_to_mqtt",          &data_transmit_lzeq_2nd_to_mqtt);
    config_lookup_bool(&cfg, "data_transmit_lceq_2nd_to_mqtt",          &data_transmit_lceq_2nd_to_mqtt);
    config_lookup_bool(&cfg, "data_transmit_laeq_2nd_spectrum_to_mqtt", &data_transmit_laeq_2nd_spectrum_to_mqtt);
    config_lookup_bool(&cfg, "data_transmit_lzeq_2nd_spectrum_to_mqtt", &data_transmit_lzeq_2nd_spectrum_to_mqtt);
    config_lookup_bool(&cfg, "data_transmit_lceq_2nd_spectrum_to_mqtt", &data_transmit_lceq_2nd_spectrum_to_mqtt);

    config_lookup_bool(&cfg, "timestamp_to_influxdb", &timestamp_to_influxdb);
    config_lookup_bool(&cfg, "influxdb_transmit_http", &influxdb_transmit_http);
    config_lookup_bool(&cfg, "influxdb_transmit_https", &influxdb_transmit_https);

    config_lookup_bool(&cfg, "mqtt_transmit", &mqtt_transmit);
    config_lookup_bool(&cfg, "mqtt_use_id_as_sub_topic", &mqtt_use_id_as_sub_topic);

    config_lookup_bool(&cfg, "data_transmit_laeq_to_sc", &data_transmit_laeq_to_sc);
    config_lookup_bool(&cfg, "sc_transmit_https", &sc_transmit_https);
    config_lookup_bool(&cfg, "data_transmit_laeq_to_madavi", &data_transmit_laeq_to_madavi);

    config_lookup_bool(&cfg, "data_transmit_via_pipe", &data_transmit_via_pipe);
    config_lookup_bool(&cfg, "start_stop_extern", &start_stop_extern);

    config_lookup_bool(&cfg, "data_on_terminal", &data_on_terminal);

    config_lookup_bool(&cfg, "data_laeq_1st_output_on_terminal", &data_laeq_1st_output_on_terminal);
    config_lookup_bool(&cfg, "data_la_spec_1st_output_on_terminal", &data_la_spec_1st_output_on_terminal);
    config_lookup_bool(&cfg, "data_lzeq_1st_output_on_terminal", &data_lzeq_1st_output_on_terminal);
    config_lookup_bool(&cfg, "data_lz_spec_1st_output_on_terminal", &data_lz_spec_1st_output_on_terminal);
    config_lookup_bool(&cfg, "data_lceq_1st_output_on_terminal", &data_lceq_1st_output_on_terminal);
    config_lookup_bool(&cfg, "data_lc_spec_1st_output_on_terminal", &data_lc_spec_1st_output_on_terminal);   
    
    config_lookup_bool(&cfg, "data_laeq_2nd_output_on_terminal", &data_laeq_2nd_output_on_terminal);
    config_lookup_bool(&cfg, "data_la_spec_2nd_output_on_terminal", &data_la_spec_2nd_output_on_terminal);
    config_lookup_bool(&cfg, "data_lzeq_2nd_output_on_terminal", &data_lzeq_2nd_output_on_terminal);
    config_lookup_bool(&cfg, "data_lz_spec_2nd_output_on_terminal", &data_lz_spec_2nd_output_on_terminal);    
    config_lookup_bool(&cfg, "data_lceq_2nd_output_on_terminal", &data_lceq_2nd_output_on_terminal);
    config_lookup_bool(&cfg, "data_lc_spec_2nd_output_on_terminal", &data_lc_spec_2nd_output_on_terminal);    

    config_lookup_bool(&cfg, "last_influxdb_transmission_time_to_terminal",
                       &last_influxdb_transmission_time_to_terminal);
    config_lookup_bool(&cfg, "last_mqtt_transmission_time_to_terminal",
                       &last_mqtt_transmission_time_to_terminal);
    config_lookup_bool(&cfg, "last_sc_transmission_time_to_terminal",
                       &last_sc_transmission_time_to_terminal);
    config_lookup_bool(&cfg, "output_thread_info", &output_thread_info);

    config_lookup_bool(&cfg, "data_logging", &data_logging);

    config_lookup_bool(&cfg, "enable_webserver", &enable_webserver);
    config_lookup_int(&cfg, "webserver_port", &webserver_port);

    /* --- floats (libconfig float = double) --------------------------- */
    config_lookup_float(&cfg, "dnms_correction", &dnms_correction);

    /* --- strings ----------------------------------------------------- */
    config_lookup_string(&cfg, "dev_name_dnms", &dev_name_dnms);
    config_lookup_string(&cfg, "interface_name", &interface_name);

    config_lookup_string(&cfg, "influxdb_server", &influxdb_server);
    config_lookup_string(&cfg, "influxdb_port", &influxdb_port);
    config_lookup_string(&cfg, "influxdb_pfad", &influxdb_pfad);
    config_lookup_string(&cfg, "influxdb_user", &influxdb_user);
    config_lookup_string(&cfg, "influxdb_passwort", &influxdb_passwort);
    config_lookup_string(&cfg, "influxdb_messung", &influxdb_messung);

    config_lookup_string(&cfg, "mqtt_messung", &mqtt_messung);
    config_lookup_string(&cfg, "mqtt_user", &mqtt_user);
    config_lookup_string(&cfg, "mqtt_passwort", &mqtt_passwort);
    config_lookup_string(&cfg, "mqtt_broker", &mqtt_broker);
    config_lookup_string(&cfg, "mqtt_main_topic", &mqtt_main_topic);

    config_lookup_string(&cfg, "host_sc", &host_sc);
    config_lookup_string(&cfg, "port_sc", &port_sc);
    config_lookup_string(&cfg, "url_sc", &url_sc);
    config_lookup_string(&cfg, "DNMS_API_PIN", &DNMS_API_PIN);

    config_lookup_string(&cfg, "name_of_pipe", &name_of_pipe);
    config_lookup_string(&cfg, "start_stop_name_of_pipe", &start_stop_name_of_pipe);
    config_lookup_string(&cfg, "data_logging_directory", &data_logging_directory);

    /* environment sensors */
    config_lookup_bool(&cfg, "enable_sds011", &enable_sds011);
    config_lookup_string(&cfg, "sds011_uart_port", &sds011_uart_port);
    config_lookup_bool(&cfg, "enable_sps30",  &enable_sps30);
    config_lookup_bool(&cfg, "enable_sht3x",  &enable_sht3x);
    config_lookup_bool(&cfg, "enable_sen5x",  &enable_sen5x);
    config_lookup_bool(&cfg,  "enable_bme280",         &enable_bme280);
    config_lookup_int(&cfg,   "sht3x_i2c_addr",        &sht3x_i2c_addr);
    config_lookup_int(&cfg,   "bme280_i2c_addr",       &bme280_i2c_addr);
    config_lookup_float(&cfg, "temp_correction_sht3x",  &temp_correction_sht3x);
    config_lookup_float(&cfg, "temp_correction_sen5x",  &temp_correction_sen5x);
    config_lookup_float(&cfg, "temp_correction_bme280", &temp_correction_bme280);
    config_lookup_bool (&cfg, "enable_scd30",            &enable_scd30);
    config_lookup_float(&cfg, "temp_correction_scd30",    &temp_correction_scd30);
    config_lookup_bool (&cfg, "enable_scd4x",            &enable_scd4x);
    config_lookup_float(&cfg, "temp_correction_scd4x",    &temp_correction_scd4x);

    config_lookup_bool(&cfg, "date_format_iso", &date_format_iso);

    /* Custom API */
    config_lookup_bool(&cfg, "custom_api_enable",       &custom_api_enable);
    config_lookup_bool(&cfg, "custom_api_send_1st",     &custom_api_send_1st);
    config_lookup_bool(&cfg, "custom_api_send_2nd",     &custom_api_send_2nd);
    config_lookup_bool(&cfg, "custom_api_spectrum_1st", &custom_api_spectrum_1st);
    config_lookup_bool(&cfg, "custom_api_spectrum_2nd", &custom_api_spectrum_2nd);
    config_lookup_bool(&cfg, "custom_api_https",        &custom_api_https);
    {
        const char *tmp = NULL;
        if (config_lookup_string(&cfg, "custom_api_server", &tmp) == CONFIG_TRUE && tmp)
            strncpy(custom_api_server, tmp, sizeof(custom_api_server) - 1);
        tmp = NULL;
        if (config_lookup_string(&cfg, "custom_api_port", &tmp) == CONFIG_TRUE && tmp)
            strncpy(custom_api_port, tmp, sizeof(custom_api_port) - 1);
        tmp = NULL;
        if (config_lookup_string(&cfg, "custom_api_path", &tmp) == CONFIG_TRUE && tmp)
            strncpy(custom_api_path, tmp, sizeof(custom_api_path) - 1);
    }

    return EXIT_SUCCESS;
}




/* ******************************************************************************************************************** */
int main(void) {
  int ret;
  int i;

  setvbuf(stdout, NULL, _IONBF, 0);  /* unbuffered: output visible even on crash */

  process_start_time = time(NULL);
  clock_gettime(CLOCK_MONOTONIC, &process_start_mono);

  if (!(get_config() == EXIT_SUCCESS)) {
    /* Output Error and stop */
    report_and_exit("error reading configuration file: dnms.conf");
  }
  fmt_dt = date_format_iso ? "%Y-%m-%d %H:%M:%S" : "%d.%m.%Y %H:%M:%S";
  fmt_d  = date_format_iso ? "%Y-%m-%d"           : "%d.%m.%Y";
  fmt_dl = date_format_iso ? "%Y-%m-%d,"     : "%d.%m.%Y,";
  printf("\n******************************************************************************************\n");
  printf("Program for Raspberry Pi to transmit noise data from a connected DNMS sensor\n ");
  printf("to Sensor.Community and/or to an influxDB database via HTTP or MQTT\n\n");

  printf("Wait %d seconds for everything to be ready!\n\n", waiting_time_after_start);
  sleep(waiting_time_after_start);  // wait some time, that erverything is settled in OS

  printf("Firmware version Raspberry Pi: %s\n\n", firmware_version);

  /* read mac address from system */
  for (i = 0; i < number_of_mac_loops; i++) {
    if (get_mac((char *)interface_name, (char *)mac_adr) == 0) {
      break;
    } else {
      sleep(1);  // wait some time
      if (i == (number_of_mac_loops - 1)) {
        printf("no Mac address found for given device name!\n");
        exit(0);
      }
    }
  }

  printf("MAC of selected Ethernet device %s is: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", interface_name, mac_adr[0], mac_adr[1], mac_adr[2], mac_adr[3], mac_adr[4], mac_adr[5]);

  /* get and print IP address */
  char ip_address[INET_ADDRSTRLEN] = {0};
  if (get_ip((char *)interface_name, ip_address) == 0) {
    printf("IP address of %s: %s\n", interface_name, ip_address);
  } else {
    printf("No IP address found for %s\n", interface_name);
  }

  bzero((char *)&mac_strg, sizeof(mac_strg));
  for (i = 0; i < 6; i++) {
    sprintf(value_2_string, "%.2x", mac_adr[i]);
    strcat(mac_strg, value_2_string);
  }
  mac_strg[12] = '\0';
 
  /* long or short ID ?  */ 
  if(long_id) {  
	  /* long ID is generated from all six bytes of MAC as integer */
	  printf("long raspi_id is configured\n");
	  zw = (mac_adr[0] << 16) + (mac_adr[1] << 8);
	  raspi_id = (zw) << 24;
	  zw = (mac_adr[2] << 24) + (mac_adr[3] << 16) + (mac_adr[4] << 8) + mac_adr[5];
	  raspi_id = raspi_id + zw;
  } else {
	  raspi_id = (mac_adr[3] << 16) + (mac_adr[4] << 8) + mac_adr[5];
  }
    sprintbig(raspi_id_strg, raspi_id);
    /* Prepare message header for influxDB transmission */
    prep_msg_header_influxdb();

  /* Prepare message header and fixed part of data for Sensor.community transmission */
  prep_msg_header_sc();
  prep_data_sc();

  /* check if to start 1st measurement interval */
  /* it will be set active if transmission to Sensor.Community is set or 
		 * Transmission of 1st measurement to influxdb is set */
  if ((data_transmit_laeq_to_sc || data_transmit_laeq_1st_to_influxdb || data_transmit_lzeq_1st_to_influxdb || data_transmit_laeq_1st_spectrum_to_influxdb
       || data_transmit_lzeq_1st_spectrum_to_influxdb || data_laeq_1st_output_on_terminal || data_la_spec_1st_output_on_terminal || data_lzeq_1st_output_on_terminal || data_lz_spec_1st_output_on_terminal || data_transmit_lceq_1st_to_influxdb || data_transmit_lceq_1st_spectrum_to_influxdb || data_lceq_1st_output_on_terminal || data_lc_spec_1st_output_on_terminal
       || data_transmit_laeq_1st_to_mqtt || data_transmit_lzeq_1st_to_mqtt || data_transmit_lceq_1st_to_mqtt || data_transmit_laeq_1st_spectrum_to_mqtt || data_transmit_lzeq_1st_spectrum_to_mqtt || data_transmit_lceq_1st_spectrum_to_mqtt) && enable_1st_interval) {
    interval_1st_active = true;
  }

  /* check if to start 2nd measurement interval */
  if ((data_transmit_laeq_2nd_to_influxdb || data_transmit_lzeq_2nd_to_influxdb || data_transmit_laeq_2nd_spectrum_to_influxdb
       || data_transmit_lzeq_2nd_spectrum_to_influxdb || data_laeq_2nd_output_on_terminal || data_la_spec_2nd_output_on_terminal
       || data_lzeq_2nd_output_on_terminal || data_lz_spec_2nd_output_on_terminal || data_transmit_lceq_2nd_to_influxdb || data_transmit_lceq_2nd_spectrum_to_influxdb || data_lceq_2nd_output_on_terminal || data_lc_spec_2nd_output_on_terminal
       || data_transmit_laeq_2nd_to_mqtt || data_transmit_lzeq_2nd_to_mqtt || data_transmit_lceq_2nd_to_mqtt || data_transmit_laeq_2nd_spectrum_to_mqtt || data_transmit_lzeq_2nd_spectrum_to_mqtt || data_transmit_lceq_2nd_spectrum_to_mqtt) && enable_2nd_interval) {
    interval_2nd_active = true;
  }

   /* shall WiFi signal strength be transmitted to SC/InfluxDB/MQTT ?? */
   if (enable_wifi_signal_strength_influxdb || enable_wifi_signal_strength_mqtt || data_transmit_laeq_to_madavi) {
   		printf("Transmission of WiFi signal strength is configured.\n");
		if (open_socket_to_get_signal_strength() < 0) {
			perror("Error open socket to get WiFi signal");
		}
		for (i = 0; i  < 20; i++) {
			usleep(50000);
			wifi_signal_strength=get_signal_strength();
			if (wifi_signal_strength == -299) {
				perror("Error open socket to get WiFi signal");	
			}
			if (wifi_signal_strength > -80) {
				printf("WiFi signal stgrength: %d\n\n", wifi_signal_strength);
				break;
			}
		}
   }

  /*  setup DNMS   */
  sleep(1);
  open_dnms();

  sleep(1);

  /*  Firmware Version des DNMS auslesen */
  printf("DNMS firmware version: ");
  if (dnms_read_version(dnms_version) != 0) {
    printf("Error reading DNMS version!\n");
    printf("Check DNMS connection!\n");
    exit(-1);
  } else {
    dnms_version[DNMS_MAX_VERSION_LEN] = 0;
    printf("%s\n", dnms_version);
    if (dnms_version[13] == '3') {
      printf("Firmware includes FFT for spectrum output but  2nd measurement interval is not supported.\n");
      data_transmit_laeq_2nd_spectrum_to_influxdb = false;
      data_transmit_lzeq_2nd_spectrum_to_influxdb = false;
      data_la_spec_2nd_output_on_terminal = false;
      data_lz_spec_2nd_output_on_terminal = false;
      interval_2nd_active = false;
    } else {
      if (dnms_version[13] >= '4') {
        printf("Firmware includes FFT for spectrum output.\n");
        printf("Firmware includes 2nd measurement interval for transmission to influxDB.\n");
        if (dnms_version[13] >= '5') {
          printf("Firmware includes calculation of Z-values and A-values.");
        } else {
          /* V4.x: no Z-values or C-values — disable to avoid I2C errors */
          printf("Firmware does not include Z-values or C-values — disabling LZeq and LCeq output.\n");
          data_transmit_lzeq_1st_to_influxdb = false;
          data_transmit_lzeq_1st_spectrum_to_influxdb = false;
          data_transmit_lzeq_2nd_to_influxdb = false;
          data_transmit_lzeq_2nd_spectrum_to_influxdb = false;
          data_transmit_lzeq_1st_to_mqtt = false;
          data_transmit_lzeq_1st_spectrum_to_mqtt = false;
          data_transmit_lzeq_2nd_to_mqtt = false;
          data_transmit_lzeq_2nd_spectrum_to_mqtt = false;
          data_lzeq_1st_output_on_terminal = false;
          data_lz_spec_1st_output_on_terminal = false;
          data_lzeq_2nd_output_on_terminal = false;
          data_lz_spec_2nd_output_on_terminal = false;
          data_transmit_lceq_1st_to_influxdb = false;
          data_transmit_lceq_1st_spectrum_to_influxdb = false;
          data_transmit_lceq_2nd_to_influxdb = false;
          data_transmit_lceq_2nd_spectrum_to_influxdb = false;
          data_transmit_lceq_1st_to_mqtt = false;
          data_transmit_lceq_1st_spectrum_to_mqtt = false;
          data_transmit_lceq_2nd_to_mqtt = false;
          data_transmit_lceq_2nd_spectrum_to_mqtt = false;
          data_lceq_1st_output_on_terminal = false;
          data_lc_spec_1st_output_on_terminal = false;
          data_lceq_2nd_output_on_terminal = false;
          data_lc_spec_2nd_output_on_terminal = false;
        }
        if (((dnms_version[13] == '5') && (dnms_version[15] == '9')) || (dnms_version[13] == '6') ) {
          printf("\nFirmware includes in addition calculation of C-values.");
        } else if (dnms_version[13] >= '5') {
          /* V5.0.x–V5.8.x: Z-values yes, C-values no — disable to avoid I2C errors */
          printf("\nFirmware does not include C-values — disabling LCeq output.\n");
          data_transmit_lceq_1st_to_influxdb = false;
          data_transmit_lceq_1st_spectrum_to_influxdb = false;
          data_transmit_lceq_2nd_to_influxdb = false;
          data_transmit_lceq_2nd_spectrum_to_influxdb = false;
          data_transmit_lceq_1st_to_mqtt = false;
          data_transmit_lceq_1st_spectrum_to_mqtt = false;
          data_transmit_lceq_2nd_to_mqtt = false;
          data_transmit_lceq_2nd_spectrum_to_mqtt = false;
          data_lceq_1st_output_on_terminal = false;
          data_lc_spec_1st_output_on_terminal = false;
          data_lceq_2nd_output_on_terminal = false;
          data_lc_spec_2nd_output_on_terminal = false;
        }
        printf("\n");
      } else {
        printf("No support for spectrum output and/or 2nd measurement interval in DNMS firmware, \nfor Teensy4  you can upgrade to version 3 or 4!\n");
        data_transmit_laeq_2nd_spectrum_to_influxdb = false;
        data_transmit_laeq_1st_spectrum_to_influxdb = false;
        data_transmit_lzeq_2nd_spectrum_to_influxdb = false;
        data_transmit_lzeq_1st_spectrum_to_influxdb = false;
        data_transmit_lceq_2nd_spectrum_to_influxdb = false;
        data_transmit_lceq_1st_spectrum_to_influxdb = false;	
        data_la_spec_1st_output_on_terminal = false;
        data_lz_spec_1st_output_on_terminal = false;
        data_lc_spec_1st_output_on_terminal = false;		
        data_la_spec_2nd_output_on_terminal = false;
        data_lz_spec_2nd_output_on_terminal = false;
        data_lc_spec_2nd_output_on_terminal = false;	
        interval_2nd_active = false;
      }
    }
  }

  /* set microphone depending on dnms.conf file */
  if (dnms_version[13] >= '6') {
    /* V6.0.x and above: transmit full calibration table to Teensy */
    const struct dnms_calib_point *pts;
    uint8_t  n;
    uint8_t  mtype;
    uint32_t blink;
    float    mc;
    switch (dnms_microphone) {
      case 2:  /* IM72D128 */
        pts = calib_IM72D128;     n = DNMS_CALIB_IM72D128_COUNT;
        mtype = 2; blink = (uint32_t)dnms_blink_period_2; mc = (float)dnms_micro_const_2;
        break;
      case 3:  /* IM72D128 with DLR housing */
        pts = calib_IM72D128_DLR; n = DNMS_CALIB_IM72D128_DLR_COUNT;
        mtype = 2; blink = (uint32_t)dnms_blink_period_3; mc = (float)dnms_micro_const_3;
        break;
      case 4:  /* ICS-43434 no frequency correction */
        pts = calib_no_corr;      n = DNMS_CALIB_NO_CORR_COUNT;
        mtype = 1; blink = (uint32_t)dnms_blink_period_4; mc = (float)dnms_micro_const_4;
        break;
      case 5:  /* IM72D128 no frequency correction */
        pts = calib_no_corr;      n = DNMS_CALIB_NO_CORR_COUNT;
        mtype = 2; blink = (uint32_t)dnms_blink_period_5; mc = (float)dnms_micro_const_5;
        break;
      case 6:  /* micro_3 (custom — fill in calib_micro3 values) */
        pts = calib_micro3;       n = DNMS_CALIB_MICRO3_COUNT;
        mtype = 3; blink = (uint32_t)dnms_blink_period_6; mc = (float)dnms_micro_const_6;
        break;
      case 7:  /* micro_4 (custom — fill in calib_micro4 values) */
        pts = calib_micro4;       n = DNMS_CALIB_MICRO4_COUNT;
        mtype = 4; blink = (uint32_t)dnms_blink_period_7; mc = (float)dnms_micro_const_7;
        break;
      default: /* case 1: ICS-43434 */
        pts = calib_ICS43434;     n = DNMS_CALIB_ICS43434_COUNT;
        mtype = 1; blink = (uint32_t)dnms_blink_period_1; mc = (float)dnms_micro_const_1;
        break;
    }
    if (dnms_send_calibration(pts, n, mtype, blink, mc) == 0)
      printf("Calibration table sent to Teensy: ok\n");
    else
      printf("Error sending calibration table to Teensy!\n");
  } else if ((dnms_version[13] == '5') && (dnms_version[15] >= '3')) {
    /* V5.3.x to V5.9.x: use simple set-microphone commands */
    switch (dnms_microphone) {
      case 1:
        if (dnms_set_ICS43434() == 0)
          printf("Set ICS-43434 microphone: ok\n");
        else
          printf("Error setting ICS-43434 microphone, check it!\n");
        break;
      case 2:
        if (dnms_set_IM72D128() == 0)
          printf("Set IM72D128 microphone: ok\n");
        else
          printf("Error setting IM72D128 microphone, check it!\n");
        break;
      case 3:
        if (dnms_version[15] >= '7') {
          if (dnms_set_IM72D128_DLR_case() == 0)
            printf("Set IM72D128 microphone with DLR case: ok\n");
          else
            printf("Error setting IM72D128 microphone with DLR case, check it!\n");
        } else {
          printf("Setting IM72D128 microphone with DLR case not possible based on Teensy version, check it!\n");
        }
        break;
      case 4:
        if (dnms_version[15] >= '7') {
          if (dnms_set_ICS43434_no_correction() == 0)
            printf("Set ICS-43434 with no frequency correction: ok\n");
          else
            printf("Error setting ICS-43434 with no frequency correction, check it!\n");
        } else {
          printf("Setting ICS-43434 with no frequency correction not possible based on Teensy version, check it!\n");
        }
        break;
      case 5:
        if (dnms_version[15] >= '7') {
          if (dnms_set_IM72D128_no_correction() == 0)
            printf("Set IM72D128 microphone with no frequency correction: ok\n");
          else
            printf("Error setting IM72D128 microphone with no frequency correction, check it!\n");
        } else {
          printf("Setting IM72D128 microphone with no frequency correction not possible based on Teensy version, check it!\n");
        }
        break;
      default:
        if (dnms_set_ICS43434() == 0)
          printf("Set ICS-43434 microphone as default: ok\n");
        else
          printf("Error setting ICS-43434 microphone as default, check it!\n");
        break;
    }
  }

  /* print out dnms_correction value */
  printf("\ndnms_correction:%.2f\n\n", dnms_correction);


#ifdef gpio_switch
  /* switching of GPIO PIN if 2nd measurement threshold is exceeded  */
  if (switch_output_pin) {
    gpioSetMode(gpio_output_pin, PI_OUTPUT);
    gpioWrite(gpio_output_pin, 0);
  }
#endif

  if (data_transmit_laeq_to_sc && enable_1st_interval) {
    measurement_1st_interval_ms = 150000; /*    *****   set the interval time for transmission to Sensor.Community to 150 seconds !! */
  }

  if (measurement_1st_interval_ms < 125) {
    measurement_1st_interval_ms = 125;
  }

  if (measurement_2nd_interval_ms < 125) {
    measurement_2nd_interval_ms = 125;
  }

  /* 1st interval active ? 	*/
  printf("1st interval enable is set to: ");
  printf("%s\n", interval_1st_active ? "true" : "false");
  if (interval_1st_active) {
    printf("1st interval measurement time is set to: %d ms\n", measurement_1st_interval_ms);
  }

  /* 2nd interval active ? 	*/
  printf("2nd interval enable is set to: ");
  printf("%s\n", interval_2nd_active ? "true" : "false");
  if (interval_2nd_active) {
    printf("2nd interval measurement time is set to: %d ms\n", measurement_2nd_interval_ms);
  }

  printf("\n");

  if (data_transmit_laeq_to_sc) {
    /* is transmission to Sensor.Community configured?  Output configuration on terminal */
    printf("Transmission to Sensor.Community of 1st interval values is set to: ");
    printf("%s\n", data_transmit_laeq_to_sc ? "true" : "false");

    /* output last Sensor.Community transmissons time on terminal */
    printf("Output last transmission time to Sensor.Community on terminal is set to: ");
    printf("%s\n", last_sc_transmission_time_to_terminal ? "true" : "false");

    if (last_sc_transmission_time_to_terminal) {
      printf("Threshold time for output last transmission time to Sensor.Community is set to: %d ms\n", threshold_output_sc_ltt_to_terminal);
    }

    {
      int _res;
      printf("%s transmission to Sensor.Community is configured\n", sc_transmit_https ? "https" : "http");
      bio = BIO_new(BIO_s_connect());
      if (bio != NULL) {
        BIO_set_conn_hostname(bio, sc_hostname);
        _res = bio_do_connect_timed(bio, 5);
        if (_res > 0)
          printf("connection to Sensor.Community successful\n");
        else
          printf("connection to Sensor.Community failed! %s\n",
                 sc_transmit_https ? "Check port (use 443 for HTTPS)" : "");
        BIO_free_all(bio);
      }
    }
  }

  if (data_transmit_laeq_to_madavi) {
    int _res;
    printf("http transmission to Madavi is configured\n");
    bio = BIO_new(BIO_s_connect());
    if (bio != NULL) {
      BIO_set_conn_hostname(bio, madavi_hostname);
      _res = bio_do_connect_timed(bio, 5);
      printf(_res > 0 ? "connection to Madavi successful\n"
                      : "connection to Madavi failed!\n");
      BIO_free_all(bio);
    }
  }

  /* is transmission to influxdb configured? Output configuration on terminal */
  data_to_influxdb = (data_transmit_laeq_1st_to_influxdb || data_transmit_laeq_1st_spectrum_to_influxdb || data_transmit_laeq_2nd_to_influxdb ||
    data_transmit_laeq_2nd_spectrum_to_influxdb || data_transmit_lzeq_1st_to_influxdb || data_transmit_lzeq_1st_spectrum_to_influxdb ||
    data_transmit_lzeq_2nd_to_influxdb || data_transmit_lzeq_2nd_spectrum_to_influxdb || data_transmit_lceq_2nd_to_influxdb ||
    data_transmit_lceq_2nd_spectrum_to_influxdb) && (interval_1st_active || interval_2nd_active);

  data_1st_to_influxdb = (data_transmit_laeq_1st_to_influxdb || data_transmit_laeq_1st_spectrum_to_influxdb ||
    data_transmit_lzeq_1st_to_influxdb || data_transmit_lzeq_1st_spectrum_to_influxdb ||
    data_transmit_lceq_1st_to_influxdb || data_transmit_lceq_1st_spectrum_to_influxdb) && interval_1st_active;

  data_2nd_to_influxdb = (data_transmit_laeq_2nd_to_influxdb || data_transmit_laeq_2nd_spectrum_to_influxdb ||
    data_transmit_lzeq_2nd_to_influxdb || data_transmit_lzeq_2nd_spectrum_to_influxdb ||
    data_transmit_lceq_2nd_to_influxdb || data_transmit_lceq_2nd_spectrum_to_influxdb) && interval_2nd_active;

  data_1st_to_mqtt = (data_transmit_laeq_1st_to_mqtt || data_transmit_laeq_1st_spectrum_to_mqtt ||
    data_transmit_lzeq_1st_to_mqtt || data_transmit_lzeq_1st_spectrum_to_mqtt ||
    data_transmit_lceq_1st_to_mqtt || data_transmit_lceq_1st_spectrum_to_mqtt) && interval_1st_active;

  data_2nd_to_mqtt = (data_transmit_laeq_2nd_to_mqtt || data_transmit_laeq_2nd_spectrum_to_mqtt ||
    data_transmit_lzeq_2nd_to_mqtt || data_transmit_lzeq_2nd_spectrum_to_mqtt ||
    data_transmit_lceq_2nd_to_mqtt || data_transmit_lceq_2nd_spectrum_to_mqtt) && interval_2nd_active;

  data_to_mqtt = (data_1st_to_mqtt || data_2nd_to_mqtt) && mqtt_transmit;
  
  if (data_to_influxdb) {
    printf("Transmission to InfluxDB is configured as follows:\n");

    printf("LAeq 1st is set to: ");
    printf("%s\n", data_transmit_laeq_1st_to_influxdb ? "true" : "false");
    printf("LZeq 1st is set to: ");
    printf("%s\n", data_transmit_lzeq_1st_to_influxdb ? "true" : "false");
    printf("LCeq 1st is set to: ");
    printf("%s\n", data_transmit_lceq_1st_to_influxdb ? "true" : "false");    
    printf("LAeq spectrum 1st is set to: ");
    printf("%s\n", data_transmit_laeq_1st_spectrum_to_influxdb ? "true" : "false");
    printf("LZeq spectrum 1st is set to: ");
    printf("%s\n", data_transmit_lzeq_1st_spectrum_to_influxdb ? "true" : "false");
    printf("LCeq spectrum 1st is set to: ");
    printf("%s\n", data_transmit_lceq_1st_spectrum_to_influxdb ? "true" : "false");    

    if (interval_2nd_active) {
      printf("LAeq 2nd is set to: ");
      printf("%s\n", data_transmit_laeq_2nd_to_influxdb ? "true" : "false");
      printf("LZeq 2nd is set to: ");
      printf("%s\n", data_transmit_lzeq_2nd_to_influxdb ? "true" : "false");
      printf("LCeq 2nd is set to: ");
      printf("%s\n", data_transmit_lceq_2nd_to_influxdb ? "true" : "false");
      printf("LAeq spectrum 2nd is set to: ");
      printf("%s\n", data_transmit_laeq_2nd_spectrum_to_influxdb ? "true" : "false");
      printf("LZeq spectrum 2nd is set to: ");
      printf("%s\n", data_transmit_lzeq_2nd_spectrum_to_influxdb ? "true" : "false");
      printf("LCeq spectrum 2nd is set to: ");
      printf("%s\n", data_transmit_lceq_2nd_spectrum_to_influxdb ? "true" : "false");      
      printf("\n");
      if (data_transmit_laeq_2nd_to_influxdb) {
        /* output LAeq threshold for tansmission of 2nd measurement interval to influxDB and number of transmissions */
        printf("LAeq threshold for transmisson of 2nd measurement to InfluxDB is set to: %d LAeq\n", threshold_2nd_interval_laeq_transmit);
        printf("Number of transmissons of 2nd measurement interval after exceeding LAeq threshold is set to: %d\n", number_transmissions_after_exceeding);
      }
    }

    /* output last influxdb transmissons time on terminal */
    printf("Output last http/https transmisson time to InfluxDB on terminal is set to: ");
    printf("%s\n", last_influxdb_transmission_time_to_terminal ? "true" : "false");

    if (last_influxdb_transmission_time_to_terminal) {
      printf("Threshold time for last InfluxDB http/https transmission time is set to: %d ms\n", threshold_output_influxdb_ltt_to_terminal);
    }
    printf("\n");
    if (influxdb_transmit_https) {
      influxdb_transmit_http = false;  // if https is configured, no http!
    }
    if (influxdb_transmit_https || influxdb_transmit_http) {
      int _res;
      printf("%s transmission to InfluxDB is configured\n", influxdb_transmit_https ? "https" : "http");
      bio = BIO_new(BIO_s_connect());
      if (bio != NULL) {
        BIO_set_conn_hostname(bio, influxdb_hostname);
        _res = bio_do_connect_timed(bio, 5);
        if (_res > 0)
          printf("connection to InfluxDB server: %s successful\n", influxdb_server);
        else
          printf("connection to InfluxDB server: %s failed! %s\n",
                 influxdb_server,
                 influxdb_transmit_https ? "Check port (use 443 for HTTPS)" : "");
        BIO_free_all(bio);
      }
    }
    printf("\n");
  }

  if (data_to_mqtt) {
    printf("Transmission to MQTT Broker %s is configured as follows:\n",
           mqtt_use_tls ? (mqtt_tls_insecure ? "via MQTT/TLS (insecure)" : "via MQTT/TLS") : "via MQTT");
    printf("LAeq 1st is set to: %s\n", data_transmit_laeq_1st_to_mqtt ? "true" : "false");
    printf("LZeq 1st is set to: %s\n", data_transmit_lzeq_1st_to_mqtt ? "true" : "false");
    printf("LCeq 1st is set to: %s\n", data_transmit_lceq_1st_to_mqtt ? "true" : "false");
    printf("LAeq spectrum 1st is set to: %s\n", data_transmit_laeq_1st_spectrum_to_mqtt ? "true" : "false");
    printf("LZeq spectrum 1st is set to: %s\n", data_transmit_lzeq_1st_spectrum_to_mqtt ? "true" : "false");
    printf("LCeq spectrum 1st is set to: %s\n", data_transmit_lceq_1st_spectrum_to_mqtt ? "true" : "false");
    if (interval_2nd_active) {
      printf("LAeq 2nd is set to: %s\n", data_transmit_laeq_2nd_to_mqtt ? "true" : "false");
      printf("LZeq 2nd is set to: %s\n", data_transmit_lzeq_2nd_to_mqtt ? "true" : "false");
      printf("LCeq 2nd is set to: %s\n", data_transmit_lceq_2nd_to_mqtt ? "true" : "false");
      printf("LAeq spectrum 2nd is set to: %s\n", data_transmit_laeq_2nd_spectrum_to_mqtt ? "true" : "false");
      printf("LZeq spectrum 2nd is set to: %s\n", data_transmit_lzeq_2nd_spectrum_to_mqtt ? "true" : "false");
      printf("LCeq spectrum 2nd is set to: %s\n", data_transmit_lceq_2nd_spectrum_to_mqtt ? "true" : "false");
    }

    printf("Output last transmission time to MQTT Broker on terminal is set to: ");
    printf("%s\n", last_mqtt_transmission_time_to_terminal ? "true" : "false");
    if (last_mqtt_transmission_time_to_terminal) {
      printf("Threshold time for last MQTT Broker transmission time is set to: %d ms\n", threshold_output_mqtt_ltt_to_terminal);
    }
    printf("\n");
  }

  /* output measurement values on terminal */
  if (data_transmit_via_pipe || data_on_terminal || data_logging) {
    if (interval_1st_active) {
      printf("Output LAeq 1st values on terminal, named pipe and/or data logging is set to: ");
      printf("%s\n", data_laeq_1st_output_on_terminal ? "true" : "false");
      printf("Output LZeq 1st values on terminal, named pipe and/or data logging is set to: ");
      printf("%s\n", data_lzeq_1st_output_on_terminal ? "true" : "false");      
      printf("Output LCeq 1st values on terminal, named pipe and/or data logging is set to: ");
      printf("%s\n", data_lceq_1st_output_on_terminal ? "true" : "false");      
      printf("Output LAeq 1st spectrum values on terminal, named pipe and/or data logging is set to: ");
      printf("%s\n", data_la_spec_1st_output_on_terminal ? "true" : "false");
      printf("Output LZeq 1st spectrum values on terminal, named pipe and/or data logging is set to: ");
      printf("%s\n", data_lz_spec_1st_output_on_terminal ? "true" : "false");      
      printf("Output LCeq 1st spectrum values on terminal, named pipe and/or data logging is set to: ");
      printf("%s\n", data_lc_spec_1st_output_on_terminal ? "true" : "false");      
      printf("\n");
    }
    if (interval_2nd_active) {
      printf("Output LAeq 2nd values on terminal, named pipe and/or data logging is set to: ");
      printf("%s\n", data_laeq_2nd_output_on_terminal ? "true" : "false");
      printf("Output LZeq 2nd values on terminal, named pipe and/or data logging is set to: ");
      printf("%s\n", data_lzeq_2nd_output_on_terminal ? "true" : "false");
      printf("Output LCeq 2nd values on terminal, named pipe and/or data logging is set to: ");
      printf("%s\n", data_lceq_2nd_output_on_terminal ? "true" : "false");      
      printf("Output LAeq 2nd spectrum values on terminal, named pipe and/or data logging is set to: ");
      printf("%s\n", data_la_spec_2nd_output_on_terminal ? "true" : "false");
      printf("Output LZeq 2nd spectrum values on terminal, named pipe and/or data logging is set to: ");
      printf("%s\n", data_lz_spec_2nd_output_on_terminal ? "true" : "false");
      printf("Output LCeq 2nd spectrum values on terminal, named pipe and/or data logging is set to: ");
      printf("%s\n", data_lc_spec_2nd_output_on_terminal ? "true" : "false");      
      printf("\n");
    }
  }


  /* MQTT transmission configured ?? */
  if (mqtt_transmit) {
    /* Required before calling other mosquitto functions */
    mosquitto_lib_init();
    /* Create a new client instance.
			 * id = NULL -> ask the broker to generate a client id for us
			 * clean session = true -> the broker should remove old sessions when we connect
			 * obj = NULL -> we aren't passing any of our private data for callbacks
			 */
    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
      fprintf(stderr, "MQTT error: Out of memory.\n");
      exit(-1);
    }
    /* Configure user and passwd for broker */
    rc = mosquitto_username_pw_set(mosq, mqtt_user, mqtt_passwort);
    if (rc != MOSQ_ERR_SUCCESS) {
      mosquitto_destroy(mosq);
      fprintf(stderr, "MQTT error: %s\n", mosquitto_strerror(rc));
      exit(-1);
    }
    /* Configure TLS if enabled */
    if (mqtt_use_tls) {
      const char *cafile = (mqtt_tls_cafile && mqtt_tls_cafile[0]) ? mqtt_tls_cafile : NULL;
      const char *capath = cafile ? NULL : "/etc/ssl/certs";
      rc = mosquitto_tls_set(mosq, cafile, capath, NULL, NULL, NULL);
      if (rc != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(mosq);
        fprintf(stderr, "MQTT TLS error: %s\n", mosquitto_strerror(rc));
        exit(-1);
      }
      if (mqtt_tls_insecure) {
        mosquitto_tls_insecure_set(mosq, true);
      }
    }
    /* Configure callbacks. This should be done before connecting ideally. */
    mosquitto_connect_callback_set(mosq, on_mqtt_connect);
    mosquitto_publish_callback_set(mosq, on_mqtt_publish);
    /* Connect to MQTT Broker with MQTT Port and keepalive time.
			 * This call makes the socket connection only, it does not complete the MQTT
			 * CONNECT/CONNACK flow, you should use mosquitto_loop_start() or
			 * mosquitto_loop_forever() for processing net traffic. */
    rc = mosquitto_connect(mosq, mqtt_broker, mqtt_port, mqtt_keepalive);
    if (rc != MOSQ_ERR_SUCCESS) {
      mosquitto_destroy(mosq);
      fprintf(stderr, "MQTT error: %s\n", mosquitto_strerror(rc));
      exit(-1);
    }
    /* Run the network loop in a background thread, this call returns quickly. */
    rc = mosquitto_loop_start(mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
      mosquitto_destroy(mosq);
      fprintf(stderr, "MQTT error: %s\n", mosquitto_strerror(rc));
      exit(-1);
    }
    strcpy(mqtt_topic, mqtt_main_topic);
    if (mqtt_use_id_as_sub_topic) {
      strcat(mqtt_topic, "/raspi-");
      strcat(mqtt_topic, raspi_id_strg);
    }
  }

  /*  transmission via named pipe (FIFO) to other process ?? */
  if (data_transmit_via_pipe) {
    printf("named pipe transmission  is configured\n");
    umask(0);
    if (mkfifo(name_of_pipe, 0666) < 0) {
      if (errno != EEXIST) {  // does fifo exist??
        perror("mkfifo()");
        exit(EXIT_FAILURE);
      }
    }
    printf("named pipe is created or does exist with name: %s\n", name_of_pipe);
    dnms_pipe_fd = open(name_of_pipe, O_RDWR);
    if (dnms_pipe_fd == -1) {
      perror("open()");
      exit(EXIT_FAILURE);
    }
    printf("named pipe is opened with name: %s\n", name_of_pipe);
  }


  /*  transmission stoppable and startable based on external signal  via named pipe ?? */
  if (start_stop_extern) {
    transmission_enabled = false;
    umask(0);
    printf("\nexternal starting and stopping of transmission is configured\n");
    if (mkfifo(start_stop_name_of_pipe, O_RDWR | 0666) < 0) {
      if (errno != EEXIST) {  // does fifo exist??
        perror("mkfifo()");
        exit(EXIT_FAILURE);
      }
    }
    printf("named pipe for starting and stopping is created or does exist with name: %s\n", start_stop_name_of_pipe);
    start_stop_pipe_fd = open(start_stop_name_of_pipe, O_RDWR);
    if (start_stop_pipe_fd == -1) {
      perror("open()");
      exit(EXIT_FAILURE);
    }
    printf("named pipe for starting and stopping is opened with name: %s\n", start_stop_name_of_pipe);
  }

  /* data logging configured ?? */
  if (data_logging) {
    if (logging_init(data_logging_directory) == 0) {
      printf("\ndata logging  is configured\n");
      if (logging_file(data_logging_file_name) == 0) {
        printf("actual file for data logging is opened: %s\n", data_logging_file_name);
      } else {
        data_logging = false;
      }
    } else {
      data_logging = false;
    }
    if (!data_logging) {
      printf("\ndata logging could not be opened: error!\n");
    }
  }

  printf("\n");

  fflush(stdout);
  sleep(1);

  
 /* Set boolean read values for 1st and 2nd measurement interval */
  read_laeq_spec_1st = (data_transmit_laeq_1st_spectrum_to_influxdb || data_la_spec_1st_output_on_terminal);
  read_lzeq_1st = (data_transmit_lzeq_1st_to_influxdb || data_lzeq_1st_output_on_terminal || data_lz_spec_1st_output_on_terminal);
  read_lzeq_spec_1st = (data_transmit_lzeq_1st_spectrum_to_influxdb || data_lz_spec_1st_output_on_terminal);
  read_lceq_1st = (data_transmit_lceq_1st_to_influxdb || data_lceq_1st_output_on_terminal || data_lc_spec_1st_output_on_terminal);
  read_lceq_spec_1st = (data_transmit_lceq_1st_spectrum_to_influxdb || data_lc_spec_1st_output_on_terminal);  
  
  read_laeq_spec_2nd = (data_transmit_laeq_2nd_spectrum_to_influxdb || data_la_spec_2nd_output_on_terminal);
  read_lzeq_2nd = (data_transmit_lzeq_2nd_to_influxdb || data_lzeq_2nd_output_on_terminal || data_lz_spec_2nd_output_on_terminal);
  read_lzeq_spec_2nd = (data_transmit_lzeq_2nd_spectrum_to_influxdb || data_lz_spec_2nd_output_on_terminal);
  read_lceq_2nd = (data_transmit_lceq_2nd_to_influxdb || data_lceq_2nd_output_on_terminal || data_lc_spec_2nd_output_on_terminal);
  read_lceq_spec_2nd = (data_transmit_lceq_2nd_spectrum_to_influxdb || data_lc_spec_2nd_output_on_terminal);

  /*  disable reading and transmission of spectrum values if measurement interval time is below 500ms */
  if (measurement_1st_interval_ms < 500) {
    data_transmit_laeq_1st_spectrum_to_influxdb = false;
    data_transmit_lzeq_1st_spectrum_to_influxdb = false;
    data_transmit_lceq_1st_spectrum_to_influxdb = false;    
    data_la_spec_1st_output_on_terminal = false;
    data_lz_spec_1st_output_on_terminal = false;
    data_lc_spec_1st_output_on_terminal = false;    
    read_laeq_spec_1st = false;
    read_lzeq_spec_1st = false;
    read_lceq_spec_1st = false;    
    if (measurement_1st_interval_ms < 125) {
      measurement_1st_interval_ms = 125;
    }
  }

  if (measurement_2nd_interval_ms < 500) {
    data_transmit_laeq_2nd_spectrum_to_influxdb = false;
    data_transmit_lzeq_2nd_spectrum_to_influxdb = false;
    data_transmit_lceq_2nd_spectrum_to_influxdb = false;    
    data_la_spec_2nd_output_on_terminal = false;
    data_lz_spec_2nd_output_on_terminal = false;
    data_lc_spec_2nd_output_on_terminal = false;    
    read_laeq_spec_2nd = false;
    read_lzeq_spec_2nd = false;
    read_lceq_spec_2nd = false;          
    if (measurement_2nd_interval_ms < 125) {
      measurement_2nd_interval_ms = 125;
    }
  }  
  

  /* Initialize environment sensors (pigpio i2c, placed here so init messages
     appear in the console just before the webserver start) */
  gpioInitialise();
  sensors_rpi_init();

  /* Start embedded webserver (if configured) */
  if (enable_webserver) {
    webserver_init();
  }

  /* Create independent threads for dnms query, print and wlan */

  if ((data_laeq_1st_output_on_terminal || data_lzeq_1st_output_on_terminal || data_la_spec_1st_output_on_terminal
       || data_lz_spec_1st_output_on_terminal)
      && interval_1st_active) {
    i_print_ipc_and_log_1st_thread = pthread_create(&print_ipc_and_log_1st_thread, NULL, print_ipc_and_log_1st_function, (void *)print_ipc_and_log_1st_message);
  }

  if ((data_laeq_2nd_output_on_terminal || data_lzeq_2nd_output_on_terminal || data_la_spec_2nd_output_on_terminal
       || data_lz_spec_2nd_output_on_terminal)
      && interval_2nd_active) {
    i_print_ipc_and_log_2nd_thread = pthread_create(&print_ipc_and_log_2nd_thread, NULL, print_ipc_and_log_2nd_function, (void *)print_ipc_and_log_2nd_message);
  }

  if (enable_wlan_or_lan) {
    i_wlan_thread = pthread_create(&wlan_thread, NULL, wlan_function, (void *)wlan_message);
  }

  if (interval_1st_active) {
    i_measurement_1st_interval_timer_thread = pthread_create(&measurement_1st_interval_timer_thread, NULL, measurement_1st_interval_timer_function, (void *)measurement_1st_timer_message);
    i_measurement_1st_interval_thread = pthread_create(&measurement_1st_interval_thread, NULL, measurement_1st_interval_function, (void *)measurement_1st_message);
  }

  if (interval_2nd_active) {
    i_measurement_2nd_interval_timer_thread = pthread_create(&measurement_2nd_interval_timer_thread, NULL, measurement_2nd_interval_timer_function, (void *)measurement_2nd_timer_message);
    i_measurement_2nd_interval_thread = pthread_create(&measurement_2nd_interval_thread, NULL, measurement_2nd_interval_function, (void *)measurement_2nd_message);
  }

  if (start_stop_extern) {
    i_start_stop_thread = pthread_create(&start_stop_thread, NULL, start_stop_function, (void *)start_stop_message);
  }

  {
    time_t _now = time(NULL);
    char _tbuf[32];
    strftime(_tbuf, sizeof(_tbuf), fmt_dt, localtime(&_now));
    printf("System up at: %s\n", _tbuf);
  }
  if (start_on_full_minute || start_on_full_hour) {
    printf("Waiting for configured start on full minute or full hour - be patient :-)\n");
  }

  if (start_on_full_hour) {
    /* Use clock_nanosleep for the bulk of the wait (energy-efficient), then
       poll at 1 ms for the exact boundary.  The poll guards against CLOCK_REALTIME
       being stepped by NTP during the sleep, which can cause a 1-second early
       wake-up.  1 ms poll resolution gives ~1 ms inter-device jitter — well
       within the NTP accuracy floor of ~1-3 ms on a local network. */
    struct timespec ts_align;
    clock_gettime(CLOCK_REALTIME, &ts_align);
    ts_align.tv_sec = ((ts_align.tv_sec / 3600) + 1) * 3600;
    ts_align.tv_nsec = 0;
    while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts_align, NULL) == EINTR)
      ;
    for (;;) {
      time_t t; time(&t);
      struct tm *tc = localtime(&t);
      if (tc->tm_min == 0 && tc->tm_sec == 0) break;
      usleep(1000);
    }
  } else {
    if (start_on_full_minute) {
      struct timespec ts_align;
      clock_gettime(CLOCK_REALTIME, &ts_align);
      ts_align.tv_sec = ((ts_align.tv_sec / 60) + 1) * 60;
      ts_align.tv_nsec = 0;
      while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts_align, NULL) == EINTR)
        ;
      for (;;) {
        time_t t; time(&t);
        if (localtime(&t)->tm_sec == 0) break;
        usleep(1000);
      }
    }
  }

  time(&sekunden);
  measurement_start_time = sekunden;
  clock_gettime(CLOCK_MONOTONIC, &measurement_start_mono);
  ts_print_1 = localtime(&sekunden);
  strftime(zeit_string_print, 80, fmt_dt, ts_print_1);
  printf("\nStarting measurements at: %s\n\n", zeit_string_print);
  fflush(stdout);

  pthread_mutex_lock(&start_1st_measurement_mutex);
  pthread_cond_signal(&start_1st_measurement_cond);
  pthread_mutex_unlock(&start_1st_measurement_mutex);
  pthread_mutex_lock(&start_2nd_measurement_mutex);
  pthread_cond_signal(&start_2nd_measurement_cond);
  pthread_mutex_unlock(&start_2nd_measurement_mutex);

  // should never arrive here **** this is an error condition *****
  pthread_join(measurement_2nd_interval_timer_thread, NULL);
  pthread_join(measurement_2nd_interval_thread, NULL);
  pthread_join(print_ipc_and_log_1st_thread, NULL);
  pthread_join(print_ipc_and_log_2nd_thread, NULL);
  pthread_join(wlan_thread, NULL);
  pthread_join(measurement_1st_interval_timer_thread, NULL);
  pthread_join(measurement_1st_interval_thread, NULL);
  pthread_join(start_stop_thread, NULL);

  printf("measurement_2nd_interval_timer_thread returns: %d\n", i_measurement_2nd_interval_timer_thread);
  printf("measurement_2nd_interval_thread returns: %d\n", i_measurement_2nd_interval_thread);
  printf("print_thread  returns: %d\n", i_print_ipc_and_log_1st_thread);
  printf("print_and_log_2nd_thread  returns: %d\n", i_print_ipc_and_log_2nd_thread);
  printf("wlan_thread  returns: %d\n", i_wlan_thread);
  printf("measurement_1st_interval_timer_thread returns: %d\n", i_measurement_1st_interval_timer_thread);
  printf("measurement_1st_interval_thread  returns: %d\n", i_measurement_1st_interval_thread);
  printf("start_stop_thread returns: %d\n", i_start_stop_thread);

  if (mqtt_transmit) {
    mosquitto_lib_cleanup();
  }

  exit(0);
}
