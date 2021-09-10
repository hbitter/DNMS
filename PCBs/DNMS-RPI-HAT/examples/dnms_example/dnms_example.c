/************************************************************************
 *                                                                      *
 *  Testprogram for DNMS - Digital Noise Measurement Sensor             *
 *                                                                      *
 *  for Raspberry Pi                                                    *
 *                                                                      *
 *  This source code is for the raspberry pi                            *
 *                                                                      *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 *    DNMS - Digital Noise Measurement Sensor                           *
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
#include <stdbool.h>
#include <time.h>  

#include "./dnms_rpi_i2c.h"

uint16_t interval_sec;
char input[2];

char dnms_version[DNMS_MAX_VERSION_LEN + 1];
int16_t ret;
int16_t ii;
int16_t data_ready;
bool dnms_error = false;
struct dnms_measurements dnms_values;
float last_value_dnms_laeq;
float last_value_dnms_lamin;
float last_value_dnms_lamax;

struct tm *zeit;
time_t sekunde;
char string[80];
bool time_output = false;


int main(void) {
    printf("\n");
    printf("Testprogram DNMS I²C connection to Raspberry Pi\n\n");

// Set Measurement Interval for DNMS
    printf("Set Measurement Interval in sec (value between 1s and 3600s): ");
    scanf("%d", &interval_sec);
    if (interval_sec < 1) interval_sec = 1;
    if (interval_sec > 3600) interval_sec = 3600;

//Want to output date and time 
    printf("\nWant to output date and time? y or n: ");
    scanf("%s", input); 
    if (input[0] == 'y') time_output = true;

// Open i2c bus
    printf("\nOpen i2c bus\n\n"); 
    if ((ret = open_i2c_dnms()) != 0) {
		printf("Error open I²C return:, %3d\n", ret);
        printf("\n");
    }
// Reset DNMS Sensor
    printf("Reset DNMS\n");
    printf("\n");
    dnms_reset();
    sleep(1);
// Firmware Version des DNMS auslesen
    printf("Firmware: ");
    if (dnms_read_version(dnms_version) !=0 ) {
        printf("Erroe reading DNMS version!\n");
        printf("Check DNMS connection!\n");         
    } else {
        dnms_version[DNMS_MAX_VERSION_LEN] = 0;
        printf("%s\n", dnms_version);
        printf("\n");
    }
    
// DNMS Lärm Werte auslesen
    for (;;) {
        sleep(interval_sec);  // wait measurement interval time
        // start LAeq calculation in DNMS
        dnms_error = true;
        if (dnms_calculate_leq() != 0) {
            dnms_error = true;
            printf("Error dnms_calculate_leq\n");
            printf("\n");            
        }

        // is data ready?
        for (ii = 0; ii < 1000; ii++) {
            ret = dnms_read_data_ready(&data_ready);
            if ((ret == 0) && (data_ready != 0)) {
                dnms_error = false;                
                break;
            }
        }

        if (!dnms_error) {
            if (dnms_read_leq(&dnms_values) == 0) {
                last_value_dnms_laeq = dnms_values.laeq;
                last_value_dnms_lamin = dnms_values.lamin;
                last_value_dnms_lamax = dnms_values.lamax;
            } else {
            dnms_error = true;
            }
        }

        if (dnms_error) {
            dnms_reset();  // try to reset DNMS
            sleep(1);
        } else {
            printf("\n");
            if (time_output) {
                time(&sekunde);
                zeit = localtime(&sekunde);
                strftime(string, 80,"%d.%m.%Y %X ", zeit);
                printf("%s  ", string);
            }
            printf("DNMS LAeq: %7.2f, LAmin: %7.2f, LAmax: %7.2f", last_value_dnms_laeq, last_value_dnms_lamin, last_value_dnms_lamax);
        }
    }

    return 0;
}
