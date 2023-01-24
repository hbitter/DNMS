/************************************************************************
 *                                                                      *
    DNMS - Digital Noise Measurement Sensor
 *                                                                      *
    This source code is for the board Teensy 4.0
    I2S Input from IvenSense Microphone ICS-43434
    Output as I2C Slave to other Microcontroller (e.g. NodeMCU)
 *                                                                      *
 ************************************************************************
 *                                                                      *
      DNMS - Digital Noise Measurement Sensor
      Copyright (C) 2021  Helmut Bitter
      Copyright (C) 2021  Burkhard Volkemer for FFT software parts
 *                                                                      *
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
 *                                                                      *
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 *                                                                      *
   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
 *                                                                      *
 ************************************************************************
*/
#include <Arduino.h>
#include <math.h>
#include <Audio.h>
#include <dnms_audio_lib.h>
#include <i2c_driver.h>
#include <i2c_driver_wire.h>


#define SOFTWARE_VERSION "DNMS Version 3.4.4"  // Version 3.4.4 for Teensy4 mit FFT

/*

   Attention: in Wire.h set BUFFER_LENGTH to 64 !!

   #define BUFFER_LENGTH 64

*/
#define NUMBER_OF_LOOPS 43         // number of 2,9ms loops (128 PCM values in a block if sample rate is 44,1 kHz)
                                   // it gives about 125 ms for the fast time weighting
#define LIMIT_OF_RMS_CALCULATIONS_IN_START_PHASE 200 // define number of first RMS calculations to throw away
#define BLK_LEN 128               // block length of audio blocks in Audio Lib
#define DECI_FACTOR 32

#define DNMS_I2C_ADDRESS                0x55
#define DNMS_MAX_VERSION_LEN            32
#define DNMS_WORD_SIZE                  2
#define DNMS_NUM_WORDS(x)               (sizeof(x) / DNMS_WORD_SIZE)
#define STATUS_OK                       0
#define STATUS_FAIL                     (-1)
#define DNMS_COMMAND_SIZE               2
#define CRC8_POLYNOMIAL                 0x31
#define CRC8_INIT                       0xFF
#define CRC8_LEN                        1
#define DNMS_MAX_BUFFER_WORDS           32
#define DNMS_IS_ERR_STATE(err_code)     (((err_code) | 0xff) == 0x1ff)

#define DNMS_CMD_RESET                  0x0001
#define DNMS_CMD_READ_VERSION           0x0002
#define DNMS_CMD_CALCULATE_LEQ          0x0003
#define DNMS_CMD_READ_DATA_READY        0x0004
#define DNMS_CMD_READ_LEQ               0x0005


#define be16_to_cpu(s) (((uint16_t)(s) << 8) | (0xff & ((uint16_t)(s)) >> 8))
#define be32_to_cpu(s) (((uint32_t)be16_to_cpu(s) << 16) | \
                        (0xffff & (be16_to_cpu((s) >> 16))))
#define be64_to_cpu(s) (((uint64_t)be32_to_cpu(s) << 32) | \
                        (0xffffffff & ((uint64_t)be32_to_cpu((s) >> 32))))
/**
   Convert a word-array to a bytes-array, effectively reverting the
   host-endianness to big-endian
   @a:  word array to change (must be (u16 *) castable)
   @w:  number of word-sized elements in the array (DNMS_NUM_WORDS(a)).
*/
#define DNMS_WORDS_TO_BYTES(a, w) \
  for (uint16_t *__a = (uint16_t *)(a), __e = (w), __w = 0; __w < __e; ++__w) { \
    __a[__w] = be16_to_cpu(__a[__w]); \
  }

// Function prototypes
void i2c_receive_from_master(int num_bytes);
void i2c_request_from_master(void);


// GUItool: begin automatically generated code
AudioInputI2S_F32         i2s_in;
//AudioOutputUSB            usb_out;
AudioConvert_F32toI16     convert_fp_to_int;
AudioFilterBiquad_F32     filter_LP;
AudioFilterBiquad_F32     filter_A;
AudioAnalyzeRMS_F32       aa_rms_A;
AudioAnalyzeFFT1024_F32   dnms_fft1024;
AudioAnalyzeFFT4096_F32   dnms_fft;
AudioDecimationFilter_F32 dnms_decimation;
AudioConnection_F32       patchCord1(i2s_in, 0, filter_LP, 0);
AudioConnection_F32       patchCord2(filter_LP, 0, filter_A, 0);
AudioConnection_F32       patchCord3(filter_A, 0, aa_rms_A, 0);
AudioConnection_F32       patchCord4(filter_A, 0, dnms_fft, 0);
AudioConnection_F32       patchCord5(filter_A, 0, dnms_decimation, 0);
AudioConnection_F32       patchCord6(dnms_decimation, 0, dnms_fft1024, 0);
//AudioConnection           patchCord8(convert_fp_to_int, 0, usb_out, 0);
//AudioConnection           patchCord9(convert_fp_to_int, 0, usb_out, 1);
// GUItool: end automatically generated code


int led = LED_BUILTIN;
bool led_on;
uint16_t led_count = 0;

uint8_t i2c_state = 0;
uint8_t buf[64];
uint16_t data_ready = 0;
bool calculate_leq = false;
uint16_t command_received;

boolean start_phase = true;
uint16_t number_of_start_phase_rms_calculations = 0;

double z;
uint16_t i;

double rms_A;
double rms_mean_A = 0;
uint32_t mean_count_A = 0;
uint32_t measure_loop_count_A = 0;
double leq_A;
float leq_A_max = 0;
float leq_A_min = 0;
double leq_g_A = 0;
double sum_leq_g_A = 0;
double delog_leq_A;
float last_leq_g_A;
float last_leq_A_min;
float last_leq_A_max;

uint32_t mean_count_fft = 0;
uint32_t measure_loop_count_fft = 0;
float freq_band[32];
float mean_freq_band[32] = {0};
float leq_freq_band[32];
float sum_leq_freq_band[32] = {0};
float leq_freq_band_g_A[32] = {0};
float last_leq_freq_band_g_A[32] = {0};
static double window_norm4096;
uint16_t fft_average;

uint32_t mean_count_fft1024 = 0;
uint32_t measure_loop_count_fft1024 = 0;
float freq_band1024[18];
float mean_freq_band1024[18] = {0};
float leq_freq_band1024[18];
float sum_leq_freq_band1024[18] = {0};
float leq_freq_band1024_g_A[18] = {0};
float last_leq_freq_band1024_g_A[18] = {0};
static double window_norm1024;
uint16_t fft1024_average;

/*
  FIR filter designed with
  http://t-filter.appspot.com

  sampling frequency: 44100 Hz

  0 Hz - 400 Hz
  gain = 1
  desired ripple = 1 dB
  actual ripple = 0.5951469174068507 dB

  680 Hz - 22050 Hz
  gain = 0
  desired attenuation = -80 dB
  actual attenuation = -82.03021272680374 dB

*/

#define number_FIR_taps_decimation 481
float32_t dnms_decimate_coeffs[number_FIR_taps_decimation] = {
  0.0000471383600320714,
  0.000016301596047241903,
  0.00001901838533262954,
  0.00002195710225532371,
  0.00002511706807628949,
  0.000028496145590866775,
  0.000032089069638904686,
  0.00003588984044233524,
  0.0000398882586848162,
  0.00004407332532949502,
  0.00004842867721961784,
  0.00005293673671602723,
  0.000057577387528762004,
  0.00006232694780664914,
  0.00006716033814356466,
  0.00007204765851524332,
  0.00007695557781724213,
  0.0000818481786403771,
  0.00008668708148137359,
  0.00009143119912596985,
  0.00009603631472288667,
  0.00010045458610489161,
  0.00010463449417502303,
  0.00010852368172993478,
  0.00011206795752171156,
  0.00011520968699385207,
  0.00011788810760819046,
  0.00012004137187003245,
  0.00012160777965420912,
  0.0001225244743510312,
  0.00012272659372754044,
  0.0001221468297613478,
  0.00012072094909544358,
  0.00011838675572903929,
  0.00011507880693041189,
  0.00011073331414765251,
  0.00010529235450621233,
  0.0000986995126784784,
  0.0000908943451829125,
  0.00008182837111951404,
  0.0000714570692209576,
  0.00005972722836360007,
  0.000046611225436764645,
  0.000032066904381567764,
  0.000016072750828967696,
  -0.0000013940225652626149,
  -0.000020346467916179732,
  -0.000040790163037848685,
  -0.00006272508012467977,
  -0.00008613796001944554,
  -0.00011101273959281197,
  -0.00013731807236869398,
  -0.00016501736507133438,
  -0.00019406293577373633,
  -0.0002243941302601069,
  -0.00025594360357272583,
  -0.0002886319682221024,
  -0.0003223687790341295,
  -0.0003570539998984142,
  -0.0003925771918210422,
  -0.0004288162267256607,
  -0.00046563921874648816,
  -0.0005029051233585627,
  -0.0005404613457133949,
  -0.0005781462426394746,
  -0.0006157896038453638,
  -0.0006532120668778607,
  -0.0006902261039838089,
  -0.0007266375141342037,
  -0.0007622452941411238,
  -0.000796841841174669,
  -0.0008302157529142783,
  -0.0008621511285813636,
  -0.0008924291338706002,
  -0.0009208300598690523,
  -0.0009471332242556754,
  -0.0009711176504682137,
  -0.0009925648339082448,
  -0.001011260947128469,
  -0.0010269932539606794,
  -0.0010395561978589656,
  -0.0010487513910028602,
  -0.0010543861479193435,
  -0.0010562820619548525,
  -0.0010542671085905357,
  -0.0010481853628674528,
  -0.0010378932200028534,
  -0.0010232628002609895,
  -0.0010041874466998816,
  -0.0009805662990069583,
  -0.0009523423095557878,
  -0.0009194414471633933,
  -0.0008818512418639634,
  -0.0008395558478016096,
  -0.0007925633470570893,
  -0.0007409205705879145,
  -0.0006846900819976415,
  -0.0006239560160256803,
  -0.0005588354264142083,
  -0.0004894768920879604,
  -0.0004160506030563989,
  -0.00033875215171672577,
  -0.0002578078558236505,
  -0.00017347208052214923,
  -0.00008602478050109694,
  0.000004231572154418817,
  0.00009696947339799503,
  0.00019183383272933886,
  0.0002884430071948419,
  0.0003863920520159775,
  0.00048525472188900806,
  0.0005845839036383074,
  0.0006839144749567095,
  0.0007827649796909485,
  0.0008806407516676106,
  0.0009770360771514139,
  0.0010714339675038983,
  0.001163305056448213,
  0.001252108993529765,
  0.0013373004772826735,
  0.0014183349759653993,
  0.0014946786106369266,
  0.0015658084815352604,
  0.0016312049210033218,
  0.0016903459484639562,
  0.0017427074164863305,
  0.0017877920321473966,
  0.0018251573787866912,
  0.001854399517528259,
  0.0018750611401687398,
  0.0018866479461201967,
  0.001889018703844741,
  0.0018816557670701,
  0.0018643691228286707,
  0.0018368793009498673,
  0.0017989849834250935,
  0.0017505253301582687,
  0.0016913860575621664,
  0.001621504080137037,
  0.0015408643395107974,
  0.0014495075277926029,
  0.0013475267884776282,
  0.0012350725537571754,
  0.0011123535639001648,
  0.000979634363685438,
  0.0008372394477434903,
  0.0006855518586738752,
  0.0005250137998768134,
  0.0003561267613578626,
  0.00017944998864581255,
  -0.000004400943514743695,
  -0.00019475360512155352,
  -0.00039087852726912186,
  -0.0005919935425821721,
  -0.0007972656476525225,
  -0.001005812738219166,
  -0.0012167055115603264,
  -0.0014289702661287903,
  -0.001641593117980651,
  -0.0018535238760436967,
  -0.002063678611613515,
  -0.0022709410375054136,
  -0.0024741709124270986,
  -0.002672207949737096,
  -0.002863872191715818,
  -0.003047971710202813,
  -0.003223310046784998,
  -0.003388687829028146,
  -0.0035429037949452342,
  -0.0036847711998833433,
  -0.003813114934466011,
  -0.003926772213821544,
  -0.0040246168355100795,
  -0.004105538217546315,
  -0.004168471765787286,
  -0.004212386863138347,
  -0.004236301654015142,
  -0.0042392850095192254,
  -0.004220458678811098,
  -0.004179010803211966,
  -0.004114186927264874,
  -0.004025311933180579,
  -0.003911777142700405,
  -0.0037730549359714267,
  -0.0036087025471155157,
  -0.0034183583140872565,
  -0.0032017526114933356,
  -0.002958708164990921,
  -0.0026891402803372265,
  -0.0023930607108727155,
  -0.002070581458518304,
  -0.0017219131243490326,
  -0.0013473654206805994,
  -0.0009473511215214237,
  -0.0005223837402567792,
  -0.00007307703518576995,
  0.00039985441229130716,
  0.0008955961683397391,
  0.001413236499092882,
  0.0019517682768953367,
  0.0025100909134907704,
  0.0030870141544513956,
  0.0036812597295153987,
  0.004291466632764309,
  0.004916196188570418,
  0.005553934828535905,
  0.006203097953230668,
  0.006862036553100437,
  0.007529043812200826,
  0.008202357443914782,
  0.008880169611825346,
  0.009560630702627004,
  0.010241855286570773,
  0.010921934425602707,
  0.011598935302887041,
  0.012270914879455759,
  0.012935921446527705,
  0.013592004259606948,
  0.014237225803734588,
  0.014869645588453741,
  0.015487392964906043,
  0.016088545188732668,
  0.016671317891367683,
  0.017233898809926745,
  0.017774539253017255,
  0.018291578568387264,
  0.018783405115034377,
  0.019248465429425914,
  0.019685290507102115,
  0.020092506400795223,
  0.020468824721053668,
  0.02081304528329175,
  0.02112406593322869,
  0.02140089081770444,
  0.021642635877469618,
  0.02184852601935613,
  0.022017896674842857,
  0.02215019880136337,
  0.022245004187789873,
  0.02230200814821385,
  0.022321029355359717,
  0.02230200814821385,
  0.022245004187789873,
  0.02215019880136337,
  0.022017896674842857,
  0.02184852601935613,
  0.021642635877469618,
  0.02140089081770444,
  0.02112406593322869,
  0.02081304528329175,
  0.020468824721053668,
  0.020092506400795223,
  0.019685290507102115,
  0.019248465429425914,
  0.018783405115034377,
  0.018291578568387264,
  0.017774539253017255,
  0.017233898809926745,
  0.016671317891367683,
  0.016088545188732668,
  0.015487392964906043,
  0.014869645588453741,
  0.014237225803734588,
  0.013592004259606948,
  0.012935921446527705,
  0.012270914879455759,
  0.011598935302887041,
  0.010921934425602707,
  0.010241855286570773,
  0.009560630702627004,
  0.008880169611825346,
  0.008202357443914782,
  0.007529043812200826,
  0.006862036553100437,
  0.006203097953230668,
  0.005553934828535905,
  0.004916196188570418,
  0.004291466632764309,
  0.0036812597295153987,
  0.0030870141544513956,
  0.0025100909134907704,
  0.0019517682768953367,
  0.001413236499092882,
  0.0008955961683397391,
  0.00039985441229130716,
  -0.00007307703518576995,
  -0.0005223837402567792,
  -0.0009473511215214237,
  -0.0013473654206805994,
  -0.0017219131243490326,
  -0.002070581458518304,
  -0.0023930607108727155,
  -0.0026891402803372265,
  -0.002958708164990921,
  -0.0032017526114933356,
  -0.0034183583140872565,
  -0.0036087025471155157,
  -0.0037730549359714267,
  -0.003911777142700405,
  -0.004025311933180579,
  -0.004114186927264874,
  -0.004179010803211966,
  -0.004220458678811098,
  -0.0042392850095192254,
  -0.004236301654015142,
  -0.004212386863138347,
  -0.004168471765787286,
  -0.004105538217546315,
  -0.0040246168355100795,
  -0.003926772213821544,
  -0.003813114934466011,
  -0.0036847711998833433,
  -0.0035429037949452342,
  -0.003388687829028146,
  -0.003223310046784998,
  -0.003047971710202813,
  -0.002863872191715818,
  -0.002672207949737096,
  -0.0024741709124270986,
  -0.0022709410375054136,
  -0.002063678611613515,
  -0.0018535238760436967,
  -0.001641593117980651,
  -0.0014289702661287903,
  -0.0012167055115603264,
  -0.001005812738219166,
  -0.0007972656476525225,
  -0.0005919935425821721,
  -0.00039087852726912186,
  -0.00019475360512155352,
  -0.000004400943514743695,
  0.00017944998864581255,
  0.0003561267613578626,
  0.0005250137998768134,
  0.0006855518586738752,
  0.0008372394477434903,
  0.000979634363685438,
  0.0011123535639001648,
  0.0012350725537571754,
  0.0013475267884776282,
  0.0014495075277926029,
  0.0015408643395107974,
  0.001621504080137037,
  0.0016913860575621664,
  0.0017505253301582687,
  0.0017989849834250935,
  0.0018368793009498673,
  0.0018643691228286707,
  0.0018816557670701,
  0.001889018703844741,
  0.0018866479461201967,
  0.0018750611401687398,
  0.001854399517528259,
  0.0018251573787866912,
  0.0017877920321473966,
  0.0017427074164863305,
  0.0016903459484639562,
  0.0016312049210033218,
  0.0015658084815352604,
  0.0014946786106369266,
  0.0014183349759653993,
  0.0013373004772826735,
  0.001252108993529765,
  0.001163305056448213,
  0.0010714339675038983,
  0.0009770360771514139,
  0.0008806407516676106,
  0.0007827649796909485,
  0.0006839144749567095,
  0.0005845839036383074,
  0.00048525472188900806,
  0.0003863920520159775,
  0.0002884430071948419,
  0.00019183383272933886,
  0.00009696947339799503,
  0.000004231572154418817,
  -0.00008602478050109694,
  -0.00017347208052214923,
  -0.0002578078558236505,
  -0.00033875215171672577,
  -0.0004160506030563989,
  -0.0004894768920879604,
  -0.0005588354264142083,
  -0.0006239560160256803,
  -0.0006846900819976415,
  -0.0007409205705879145,
  -0.0007925633470570893,
  -0.0008395558478016096,
  -0.0008818512418639634,
  -0.0009194414471633933,
  -0.0009523423095557878,
  -0.0009805662990069583,
  -0.0010041874466998816,
  -0.0010232628002609895,
  -0.0010378932200028534,
  -0.0010481853628674528,
  -0.0010542671085905357,
  -0.0010562820619548525,
  -0.0010543861479193435,
  -0.0010487513910028602,
  -0.0010395561978589656,
  -0.0010269932539606794,
  -0.001011260947128469,
  -0.0009925648339082448,
  -0.0009711176504682137,
  -0.0009471332242556754,
  -0.0009208300598690523,
  -0.0008924291338706002,
  -0.0008621511285813636,
  -0.0008302157529142783,
  -0.000796841841174669,
  -0.0007622452941411238,
  -0.0007266375141342037,
  -0.0006902261039838089,
  -0.0006532120668778607,
  -0.0006157896038453638,
  -0.0005781462426394746,
  -0.0005404613457133949,
  -0.0005029051233585627,
  -0.00046563921874648816,
  -0.0004288162267256607,
  -0.0003925771918210422,
  -0.0003570539998984142,
  -0.0003223687790341295,
  -0.0002886319682221024,
  -0.00025594360357272583,
  -0.0002243941302601069,
  -0.00019406293577373633,
  -0.00016501736507133438,
  -0.00013731807236869398,
  -0.00011101273959281197,
  -0.00008613796001944554,
  -0.00006272508012467977,
  -0.000040790163037848685,
  -0.000020346467916179732,
  -0.0000013940225652626149,
  0.000016072750828967696,
  0.000032066904381567764,
  0.000046611225436764645,
  0.00005972722836360007,
  0.0000714570692209576,
  0.00008182837111951404,
  0.0000908943451829125,
  0.0000986995126784784,
  0.00010529235450621233,
  0.00011073331414765251,
  0.00011507880693041189,
  0.00011838675572903929,
  0.00012072094909544358,
  0.0001221468297613478,
  0.00012272659372754044,
  0.0001225244743510312,
  0.00012160777965420912,
  0.00012004137187003245,
  0.00011788810760819046,
  0.00011520968699385207,
  0.00011206795752171156,
  0.00010852368172993478,
  0.00010463449417502303,
  0.00010045458610489161,
  0.00009603631472288667,
  0.00009143119912596985,
  0.00008668708148137359,
  0.0000818481786403771,
  0.00007695557781724213,
  0.00007204765851524332,
  0.00006716033814356466,
  0.00006232694780664914,
  0.000057577387528762004,
  0.00005293673671602723,
  0.00004842867721961784,
  0.00004407332532949502,
  0.0000398882586848162,
  0.00003588984044233524,
  0.000032089069638904686,
  0.000028496145590866775,
  0.00002511706807628949,
  0.00002195710225532371,
  0.00001901838533262954,
  0.000016301596047241903,
  0.0000471383600320714
};


float32_t decimation_buffer[BLK_LEN * DECI_FACTOR];
float32_t decimation_state_buffer[number_FIR_taps_decimation + (BLK_LEN * DECI_FACTOR) - 1];


void setup() {
  //Serial.begin(115200);
  //delay(500);

  Wire.begin(0x55);
  Wire.onReceive(i2c_receive_from_master);
  Wire.onRequest(i2c_request_from_master);

  AudioMemory(10);
  AudioMemory_F32(64);

  dnms_decimation.end();

  AudioNoInterrupts();

  delay(500);

  fft_average = 1;
  dnms_fft.setOutputType(FFT_POWER);
  //  dnms_fft.windowFunction(AudioWindowNone);
  dnms_fft.windowFunction(AudioWindowHanning4096);
  dnms_fft.setNAverage(fft_average);

  fft1024_average = 1;
  dnms_fft1024.setOutputType(FFT_POWER);
  //  dnms_fft1024.windowFunction(AudioWindowNone);
  dnms_fft1024.windowFunction(AudioWindowHanning1024);
  dnms_fft1024.setNAverage(fft1024_average);

  filter_LP.setLowpass(0, 18000, 0.11, false);
  filter_LP.setLowpass(1, 21800, 0.03, true);

  filter_A.setBandpass(0, 2500, 0.33, false);   // A-Filter Bandpass
  filter_A.setHighpass(1, 8, 0.12, true);       // A-Filter Highpass

  delay(100);

  // pure window function normalization part for non coherent gain
  // source: http://www.gigalogchip.com/uploads/3/4/9/4/34940339/2009fftnoise.pdf page 2, table 1
#define window  0.3750 // Hanning
  // #define window 0.2580 // Blackman-Harris
  // technical normalization:
  // factor 2 due to overlapping windows
  window_norm4096 = 1.0 / (window * 2 * 2048.0 * 2048.0 );
  window_norm1024 = 1.0 / (window * 2 * 512.0 * 512.0 );

  dnms_decimation.begin(&dnms_decimate_coeffs[0], number_FIR_taps_decimation, DECI_FACTOR, BLK_LEN, &decimation_buffer[0], &decimation_state_buffer[0]);

}


void loop() {

  if (start_phase && aa_rms_A.available()) {
    rms_A = aa_rms_A.read();
    number_of_start_phase_rms_calculations++;
    if (number_of_start_phase_rms_calculations >= LIMIT_OF_RMS_CALCULATIONS_IN_START_PHASE) {
      start_phase = false;
    }
  } else {

    if (aa_rms_A.available()) {
      rms_A = aa_rms_A.read();
      rms_mean_A += rms_A * rms_A;
      ++mean_count_A;
    }

    // ***** FFT4096 available?
    if (dnms_fft.available()) {
      freq_band[0] = dnms_fft.read(0);
      freq_band[1] = dnms_fft.read(1);
      freq_band[2] = dnms_fft.read(2);
      freq_band[3] = dnms_fft.read(3);
      freq_band[4] = dnms_fft.read(4);
      freq_band[5] = dnms_fft.read(5);
      freq_band[6] = dnms_fft.read(6);
      freq_band[7] = dnms_fft.read(7);
      freq_band[8] = dnms_fft.read(8, 9);
      freq_band[9] = dnms_fft.read(10, 12);
      freq_band[10] = dnms_fft.read(13, 16);
      freq_band[11] = dnms_fft.read(17, 20);
      freq_band[12] = dnms_fft.read(21, 25);
      freq_band[13] = dnms_fft.read(26, 32);
      // from here on used for 1/3 octave values
      freq_band[14] = dnms_fft.read(33, 41);
      freq_band[15] = dnms_fft.read(42, 51);
      freq_band[16] = dnms_fft.read(52, 65);
      freq_band[17] = dnms_fft.read(66, 82);
      freq_band[18] = dnms_fft.read(83, 103);
      freq_band[19] = dnms_fft.read(104, 130);
      freq_band[20] = dnms_fft.read(131, 164);
      freq_band[21] = dnms_fft.read(165, 207);
      freq_band[22] = dnms_fft.read(208, 261);
      freq_band[23] = dnms_fft.read(262, 328);
      freq_band[24] = dnms_fft.read(329, 414);
      freq_band[25] = dnms_fft.read(415, 521);
      freq_band[26] = dnms_fft.read(522, 656);
      freq_band[27] = dnms_fft.read(657, 826);
      freq_band[28] = dnms_fft.read(827, 1039);
      freq_band[29] = dnms_fft.read(1040, 1311);
      freq_band[30] = dnms_fft.read(1312, 1650);
      freq_band[31] = dnms_fft.read(1651, 2047);

      for (i = 14; i < 32; i++) {
        mean_freq_band[i] += freq_band[i];
      }
      ++mean_count_fft;
    }

    // **** FFT1024 available?
    if (dnms_fft1024.available()) {
      freq_band1024[0] = dnms_fft1024.read(0, 12);
      // from here used for 1/3 octave values
      freq_band1024[1] = dnms_fft1024.read(13, 16);
      freq_band1024[2] = dnms_fft1024.read(17, 20);
      freq_band1024[3] = dnms_fft1024.read(21, 25);
      freq_band1024[4] = dnms_fft1024.read(26, 32);
      freq_band1024[5] = dnms_fft1024.read(33, 41);
      freq_band1024[6] = dnms_fft1024.read(42, 52);
      freq_band1024[7] = dnms_fft1024.read(53, 65);
      freq_band1024[8] = dnms_fft1024.read(66, 82);
      freq_band1024[9] = dnms_fft1024.read(83, 104);
      freq_band1024[10] = dnms_fft1024.read(105, 131);
      freq_band1024[11] = dnms_fft1024.read(132, 166);
      freq_band1024[12] = dnms_fft1024.read(167, 209);
      freq_band1024[13] = dnms_fft1024.read(210, 263);
      // up to here used for 1/3 octave values
      freq_band1024[14] = dnms_fft1024.read(264, 331);
      freq_band1024[15] = dnms_fft1024.read(332, 417);
      freq_band1024[16] = dnms_fft1024.read(0, 12);
      freq_band1024[17] = dnms_fft1024.read(418, 511);

      for (i = 1; i < 14; i++) {                    // only for the used ones
        mean_freq_band1024[i] += freq_band1024[i];
      }
      ++mean_count_fft1024;
    }

    /**** 125ms over? ***/
    if (mean_count_A >= NUMBER_OF_LOOPS) 
    {
        // blinking of internal led ( 2 x 125ms on and 2 x 125ms off)
        led_count++;
        if (led_count == 2) 
        {
            led_count = 0;
            if (led_on == false) 
            {
                led_on = true;
                digitalWrite(led, HIGH);
            } 
            else 
            {
                led_on = false;
                digitalWrite(led, LOW);
            }
        }

      // calculate LAeq over 125ms interval from RMS
      z = sqrt(rms_mean_A / mean_count_A);
      if (z == 0) {
        z = 0.000001;
      }

      leq_A = 125.0 + 20 * log10(z);

      // compare new LAeq to saved LAeq min and save, if it is smaller than old LAeq min
      if (leq_A_min == 0) {
        leq_A_min = leq_A;
      } else {
        if (leq_A < leq_A_min) {
          leq_A_min = leq_A;
        }
      }

      // compare new LAeq to saved LAeq max and save, if it is bigger than old LAeq max
      if (leq_A_max == 0) {
        leq_A_max = leq_A;
      } else {
        if (leq_A > leq_A_max) {
          leq_A_max = leq_A;
        }
      }

      // recalculate the overall LAeq
      delog_leq_A = pow(10, leq_A / 10);
      sum_leq_g_A += delog_leq_A;
      ++measure_loop_count_A;
      leq_g_A = 10 * log10(sum_leq_g_A / measure_loop_count_A); // neuer gesamt Leq
      // set the intermediate rms to zero as well as count
      rms_mean_A = 0;
      mean_count_A = 0;

      // ***** FFT4096
      if (mean_count_fft > 0) {
        // now LAeq calculation for 1/3 octave bands
        ++measure_loop_count_fft;
        for (i = 14; i < 32; i++) {
          z = mean_freq_band[i] * window_norm4096 / mean_count_fft;
          if (z == 0) {
            z = 0.0000001;
          }

          leq_freq_band[i] = 125.0 + 10 * log10(z);

          sum_leq_freq_band[i] += pow(10, leq_freq_band[i] / 10);  // gesamt Leq weiterrechnen
          leq_freq_band_g_A[i] = 10 * log10(sum_leq_freq_band[i] / measure_loop_count_fft); // neuer gesamt Leq
          mean_freq_band[i] = 0;
        }
        mean_count_fft = 0;
      }

      // ***** ZoomFFT1024
      if (mean_count_fft1024 > 0) {
        ++measure_loop_count_fft1024;
        for (i = 1; i < 14; i++) {
          z = mean_freq_band1024[i] * window_norm1024 / mean_count_fft1024;
          if (z == 0) {
            z = 0.0000001;
          }

          leq_freq_band1024[i] = 125.0 + 10 * log10(z);

          sum_leq_freq_band1024[i] += pow(10, leq_freq_band1024[i] / 10);  // gesamt Leq weiterrechnen
          leq_freq_band1024_g_A[i] = 10 * log10(sum_leq_freq_band1024[i] / measure_loop_count_fft1024); // neuer gesamt Leq
          mean_freq_band1024[i] = 0;
        }
        mean_count_fft1024 = 0;
      }
    }

    if (calculate_leq) {
      measure_loop_count_A = 0;
      last_leq_g_A = leq_g_A;
      last_leq_A_min = leq_A_min;
      last_leq_A_max = leq_A_max;
      sum_leq_g_A = 0;
      leq_A_min = 0;
      leq_A_max = 0;

      // ***** FFT4096
      measure_loop_count_fft = 0;
      for (i = 14; i < 32; i++) {
        last_leq_freq_band_g_A[i] = leq_freq_band_g_A[i];
        sum_leq_freq_band[i] = 0;
      }

      // **** FFT1024
      measure_loop_count_fft1024 = 0;
      for (i = 1; i < 14; i++) {
        last_leq_freq_band_g_A[i] = leq_freq_band1024_g_A[i];
        sum_leq_freq_band1024[i] = 0;
      }
    calculate_leq = false;
	  data_ready = 1;
    }
  }
}

void i2c_request_from_master() {
  uint16_t i;
  uint16_t idx;
  uint8_t crc;
  uint8_t b[4];

  switch (i2c_state) {
    case 2: // Read DNMS Version
      idx = 0;
      for (i = 0; i < sizeof(SOFTWARE_VERSION); ++i) {
        buf[idx++] = SOFTWARE_VERSION[i++];
        buf[idx++] = SOFTWARE_VERSION[i];
        crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
        buf[idx++] = crc;
      }
      i = Wire.write(buf, idx - 3);
      i2c_state = 0;
      break;

    case 4: // Read Data Ready
      idx = 0;
      buf[idx++] = uint8_t ((data_ready >> 8) & 0xff);
      buf[idx++] = uint8_t (data_ready & 0xff);
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      Wire.write(buf, 3);
      i2c_state = 0;
      break;

    case 5: // Read Leq
      idx = 0;

      f2b(last_leq_g_A, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;

      f2b(last_leq_A_min, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;

      f2b(last_leq_A_max, &b[0]);
      buf[idx++] = b[3];
      buf[idx++] = b[2];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      buf[idx++] = b[1];
      buf[idx++] = b[0];
      crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
      buf[idx++] = crc;
      i = Wire.write(buf, idx);
      data_ready = 0;
      i2c_state = 0;
      break;

    case 6: // Read 1/3 octav values part 1
      fft_values_i2c_write(1, 8);
      break;

    case 7: // Read 1/3 octav values part 2
      fft_values_i2c_write(9, 16);
      break;

    case 8: // Read 1/3 octav values part 3
      fft_values_i2c_write(17, 24);
      break;

    case 9: // Read 1/3 octav values part 4
      fft_values_i2c_write(25, 31);
      break;

    default: // else
      break;
  }
}


void fft_values_i2c_write(uint32_t start_index, uint32_t end_index) {

  uint16_t i;
  uint16_t idx;
  uint8_t crc;
  uint8_t b[4];

  idx = 0;
  for (i = start_index; i < end_index + 1; i++) {
    f2b(last_leq_freq_band_g_A[i], &b[0]);
    buf[idx++] = b[3];
    buf[idx++] = b[2];
    crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
    buf[idx++] = crc;
    buf[idx++] = b[1];
    buf[idx++] = b[0];
    crc = dnms_common_generate_crc((uint8_t *)&buf[idx - 2], DNMS_WORD_SIZE);
    buf[idx++] = crc;
  }
  i = Wire.write(buf, idx);
  data_ready = 0;
  i2c_state = 0;

}


void i2c_receive_from_master(int num_bytes) {
  uint16_t i;

  i2c_state = 0;
  i = 0;
  while (Wire.available()) {
    buf[i] = Wire.read();
    i++;
  }

  command_received = (buf[0] << 8) | buf[1];
  switch (command_received) {
    case 1: // Reset DNMS
      AudioInterrupts();  // start Audio Interrupts again
      break;

    case 2: // Read DNMS Version
      i2c_state = 2;
      break;

    case 3: // Calculate Leq
      i2c_state = 3;
      data_ready = 0;
      calculate_leq = true;
      break;

    case 4: // Read Data (Leq) Ready
      i2c_state = 4;
      break;

    case 5: // Read Leq
      i2c_state = 5;
      break;

    case 6: // Read 1/3 octav values part 1
      i2c_state = 6;
      break;

    case 7: // Read 1/3 octav values part 2
      i2c_state = 7;
      break;

    case 8: // Read 1/3 octav values part 3
      i2c_state = 8;
      break;

    case 9: // Read 1/3 octav values part 4
      i2c_state = 9;
      break;

    default: // else
      break;
  }
}


void f2b(float val, uint8_t* bytes_array) {
  // Create union of shared memory space
  union {
    float float_variable;      //      leq_HP_min = 0;
    //      leq_HP_max = 0;
    uint8_t temp_array[4];
  } u;
  // Overite bytes of union with float variable
  u.float_variable = val;
  // Assign bytes to input array
  memcpy(bytes_array, u.temp_array, 4);
}


uint8_t dnms_common_generate_crc(uint8_t *data, uint16_t count) {
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
