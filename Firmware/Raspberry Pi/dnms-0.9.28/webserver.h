#pragma once

/* Two new config parameters (declared extern; defined in dnms.c) */
extern int enable_webserver;
extern int webserver_port;

/* Builds and returns a malloc'd airrohr-compatible JSON string of the
   current measurement values.  Caller must free() the returned pointer. */
extern char *build_data_json(int include_spectrum, int include_1st, int include_2nd);

void webserver_init(void);
void webserver_stop(void);
