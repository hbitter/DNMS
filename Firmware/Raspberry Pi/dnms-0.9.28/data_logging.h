/************************************************************************
 *                                                                      *
 *  Data Logger for DNMS - Digital Noise Measurement Sensor                  *
 *                                                                      *
 *  This source code is for the raspberry pi        *
 *                                                                      *
 *                                                                      *
 ************************************************************************
 *                                                                      *
 *    DNMS - Digital Noise Measurement Sensor                           *
 *    Copyright (C) 2022, 2023, 2024  Helmut Bitter                                 *
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
#include <unistd.h>        
#include <fcntl.h> 
#include <time.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>




/**
   logging_init (char *log_directory) - initialize the logging, opens file etc.
   
   in: log_directory, string with the name/full path of the directory for data logging
   Return:  0 on success, an error code otherwise
*/
int logging_init (const char * log_directory);


/**
   logging_file (char *name) - get the name of the logging file
   
   out: name, string with the name of the actual logging file
   Return:  0 on success, an error code otherwise
   If Return is 0, the name of the logging file is returned as string
*/
int logging_file (char *name);


/**
   logging_write (char *data, size_t count) - write data to log file
   
   in: data, string that will be written to the logging file
   Return:  0 on success, an error code otherwise
*/
int logging_write (const char *data, size_t count);


/**
   logging_close (void) - close the logging file
   

   Return:  0 on success, an error code otherwise
*/
int logging_close (void);



 
