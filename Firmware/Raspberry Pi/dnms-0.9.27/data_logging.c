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

#include "./data_logging.h"

FILE *data_fd;
int result;

struct tm *tmnow;
time_t tnow;
char datum_file[16];
char datum_file_last[16];
char logging_file_name[128];
char logging_directory[64];

struct stat st = {0};

size_t written;


int logging_init (const char *log_directory) {

	/* get actual date */
	time(&tnow);
	tmnow = localtime (&tnow);
	strftime (datum_file, 16, "%d.%m.%Y", tmnow);
	strcpy (datum_file_last, datum_file);
	
	strcpy (logging_directory, log_directory);
	/* check if "data_logging" directory exists, if not, it is created*/
	if (stat(logging_directory, &st) == -1) {
			if (mkdir(logging_directory, 0777) == -1) {
				return -1;
			}	
	}	

	/* build logging file name */
	strcpy (logging_file_name, logging_directory);
	strcat (logging_file_name, "/data_");
	strcat (logging_file_name, datum_file);
	strcat (logging_file_name, ".csv");
	data_fd = fopen (logging_file_name, "a+");
	if (data_fd == NULL) {
		return -1;
	}

	return 0;
}


int logging_file (char *name) {

	strcpy (name, logging_file_name);
	return 0;
}


int logging_write (const char *data, size_t count) {

	/* check if date has changed and new logging file has to be created ?? */
	/* get actual date */
	time(&tnow);
	tmnow = localtime (&tnow);
	strftime (datum_file, 16, "%d.%m.%Y", tmnow);
	if (strcmp (datum_file_last, datum_file) != 0) {
		// date has changed, close actual file and open a new file with new date in the file name
		fclose (data_fd);
		strcpy (datum_file_last, datum_file);

		/* build logging file name */
		strcpy (logging_file_name, logging_directory);
		strcat (logging_file_name, "/data_");
		strcat (logging_file_name, datum_file);
		strcat (logging_file_name, ".csv");
		data_fd = fopen (logging_file_name, "a+");
		if (data_fd == NULL) {
			return -1;
		}
	}	
	/* write the data  */
	written = fwrite (data, sizeof(char), strlen (data), data_fd);
	if (written != strlen (data)) {
		return -1;
	}	
	return 0;
}


int logging_close (void) {

	fclose (data_fd);
	return 0;
}
