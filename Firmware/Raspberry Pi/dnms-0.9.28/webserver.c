/*
 * webserver.c — embedded HTTP webserver for dnms-0.9.27
 * Uses libmicrohttpd + PAM authentication
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <iwlib.h>
#include <microhttpd.h>
#include <security/pam_appl.h>
#include <libconfig.h>

#include "webserver.h"
#include "sensors_rpi.h"

/* ── Logo PNG (blue #2c7be5 background, 100×100) ─────────────────────────── */
static const unsigned int LOGO_PNG_SIZE = 1397;
static const unsigned char LOGO_PNG[] = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x64, 0x04, 0x03, 0x00, 0x00, 0x00, 0x82, 0xcc, 0x88,
  0x67, 0x00, 0x00, 0x00, 0x30, 0x50, 0x4c, 0x54, 0x45, 0x6b, 0xa3, 0xed, 0xb1, 0xce, 0xf5, 0xd4,
  0xe2, 0xf9, 0x93, 0xb6, 0xef, 0x14, 0x6c, 0xe2, 0x2b, 0x7a, 0xe4, 0xfd, 0xfd, 0xfe, 0x1d, 0x71,
  0xe3, 0x31, 0x7d, 0xe5, 0x58, 0x96, 0xea, 0xd8, 0xe7, 0xfa, 0x38, 0x83, 0xe6, 0xea, 0xf3, 0xfc,
  0x46, 0x8a, 0xe8, 0x88, 0xb5, 0xf0, 0x9a, 0xc3, 0xf3, 0xbc, 0x1d, 0x74, 0x98, 0x00, 0x00, 0x05,
  0x00, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0xed, 0x97, 0x6f, 0x68, 0x1b, 0x65, 0x1c, 0xc7, 0x53,
  0x69, 0x2e, 0x0c, 0x99, 0x3a, 0xb0, 0x37, 0x98, 0xa2, 0x5b, 0x47, 0x63, 0xc5, 0x31, 0xa6, 0x6e,
  0x9d, 0xf8, 0xc6, 0xf9, 0x62, 0x2f, 0x7c, 0x21, 0x24, 0xc1, 0xc7, 0xbb, 0x24, 0xb6, 0xcb, 0x25,
  0xf0, 0xc5, 0x9c, 0x69, 0x96, 0x82, 0x47, 0x22, 0xb2, 0x54, 0x06, 0x47, 0x0a, 0xbe, 0xb0, 0x05,
  0x13, 0x0d, 0x3a, 0xfc, 0x47, 0xf3, 0x4e, 0xd0, 0x41, 0x0a, 0x83, 0x6e, 0xa0, 0xb0, 0x43, 0xa8,
  0xca, 0x44, 0x9a, 0x22, 0x83, 0xe1, 0x40, 0x33, 0x7d, 0x53, 0x06, 0xc5, 0xf8, 0xb2, 0xfe, 0x9e,
  0xe7, 0x2e, 0xed, 0x5d, 0x92, 0x8e, 0xde, 0x1b, 0x51, 0xe8, 0x41, 0x2f, 0xe9, 0x73, 0xf7, 0xb9,
  0xdf, 0x3d, 0xdf, 0xdf, 0xdf, 0x04, 0x98, 0xef, 0x23, 0xb0, 0x87, 0xec, 0x21, 0xff, 0x57, 0x44,
  0x29, 0x1a, 0x3e, 0x91, 0x77, 0x02, 0xeb, 0xfb, 0x9a, 0x86, 0x1f, 0x24, 0xde, 0x00, 0xf0, 0x4d,
  0xd0, 0x07, 0xa2, 0xce, 0x80, 0x1f, 0x7a, 0xd0, 0x8f, 0x95, 0x63, 0x47, 0x0e, 0x8c, 0x02, 0x9a,
  0x9f, 0xbd, 0xbc, 0xcb, 0x8a, 0xec, 0x13, 0x60, 0xcc, 0xa7, 0xc8, 0xca, 0xd7, 0x5d, 0x33, 0xbb,
  0x46, 0x54, 0xd6, 0xc1, 0x1d, 0xbf, 0xae, 0x9c, 0xc1, 0xb4, 0x1f, 0xa4, 0xc8, 0x4f, 0x0d, 0xcd,
  0xd8, 0x11, 0x31, 0xe9, 0x2f, 0xe9, 0xd1, 0xec, 0xb1, 0xaa, 0xc1, 0x2a, 0x12, 0x56, 0x77, 0x42,
  0x22, 0xa3, 0x4d, 0xc6, 0x1e, 0x3d, 0xe5, 0x5a, 0x59, 0x86, 0xfe, 0x07, 0x63, 0x09, 0x7b, 0x33,
  0x83, 0x90, 0x65, 0x1c, 0x64, 0x05, 0x64, 0x5d, 0x2b, 0x17, 0xc9, 0x93, 0x1f, 0xb0, 0xb2, 0x2d,
  0xf3, 0x20, 0x44, 0xc2, 0x38, 0x8b, 0x22, 0xe3, 0x8a, 0xc4, 0xc9, 0xdb, 0x16, 0x52, 0x6a, 0x1c,
  0x2b, 0xfd, 0x88, 0xf8, 0x4f, 0x99, 0x20, 0x0b, 0x17, 0x91, 0x76, 0x5f, 0x30, 0xae, 0x01, 0x41,
  0x35, 0x9c, 0xed, 0x43, 0x2e, 0xd3, 0xe3, 0xe9, 0x90, 0x91, 0x62, 0x2d, 0xa4, 0x4d, 0x8f, 0x27,
  0x3f, 0xa6, 0xb7, 0x0a, 0xa7, 0x8c, 0x5e, 0x24, 0x6c, 0x3b, 0xb8, 0x81, 0x0c, 0xbd, 0x1c, 0x97,
  0xe7, 0xea, 0xb6, 0x27, 0xdf, 0xa2, 0xe7, 0xc8, 0x99, 0x2d, 0xa4, 0xe8, 0x28, 0x85, 0x63, 0xa8,
  0x92, 0xbc, 0x96, 0xa5, 0xb1, 0x76, 0x8e, 0xbe, 0x46, 0xac, 0x4f, 0xb7, 0x98, 0x8a, 0xac, 0x31,
  0x39, 0xdf, 0x45, 0x2e, 0x04, 0xd6, 0xc4, 0x6a, 0x14, 0xf3, 0xa4, 0x15, 0x91, 0x87, 0xd2, 0x4a,
  0x47, 0x43, 0xc8, 0xac, 0x21, 0xd7, 0xdc, 0x62, 0x6a, 0xe9, 0xca, 0x36, 0x22, 0xe3, 0xbc, 0x58,
  0x6c, 0xe9, 0xaa, 0xb5, 0xc2, 0x91, 0x67, 0xf4, 0x8a, 0xfc, 0x23, 0x4e, 0x9b, 0x8d, 0x1c, 0x7f,
  0x84, 0x73, 0x2c, 0x61, 0x4d, 0xee, 0xee, 0x25, 0x06, 0x3b, 0xae, 0x95, 0x89, 0x0c, 0xe3, 0x8b,
  0x05, 0x0c, 0xe5, 0x9a, 0xe1, 0xc3, 0x18, 0x89, 0xe0, 0x29, 0x6b, 0xda, 0xe5, 0x9b, 0xd5, 0x46,
  0xca, 0x51, 0x4c, 0xc6, 0x3a, 0x44, 0xf4, 0xc8, 0x59, 0x26, 0xd1, 0x97, 0x32, 0x16, 0xe8, 0xea,
  0x97, 0x18, 0x89, 0x61, 0x78, 0x22, 0x6f, 0x6c, 0x3b, 0xf8, 0xa6, 0x1d, 0x97, 0x01, 0xee, 0xe7,
  0x37, 0x6b, 0x08, 0xf1, 0x6d, 0x2f, 0xb2, 0x65, 0x9d, 0x47, 0xec, 0xcf, 0xa8, 0x5a, 0xe3, 0xd6,
  0x58, 0x4b, 0xa7, 0x77, 0xdd, 0x76, 0x70, 0x2e, 0x82, 0x45, 0x1b, 0xb9, 0x8c, 0xd0, 0x5c, 0x81,
  0x1c, 0xa2, 0x46, 0xe8, 0xc5, 0xa3, 0x24, 0xed, 0x0c, 0xae, 0xe1, 0x7e, 0x8c, 0x34, 0x9e, 0x90,
  0x32, 0xa5, 0x18, 0x97, 0xd0, 0x76, 0x4c, 0x3b, 0xfd, 0xa2, 0xbd, 0xb3, 0x00, 0x5b, 0xd6, 0x4a,
  0x5c, 0xc1, 0x92, 0x99, 0x20, 0x53, 0x65, 0xd2, 0x29, 0x41, 0x8f, 0xdb, 0x8f, 0x87, 0xc3, 0xd7,
  0xe5, 0xac, 0x5a, 0x70, 0x21, 0xf9, 0x57, 0x11, 0xb4, 0x91, 0xf8, 0x73, 0x5c, 0x2c, 0x0c, 0x2b,
  0x32, 0x85, 0x48, 0x1c, 0x07, 0xcd, 0x56, 0xfa, 0x0c, 0xfe, 0xc4, 0x49, 0xf9, 0x0d, 0x79, 0xda,
  0xed, 0xea, 0xf7, 0xfe, 0x6a, 0xa7, 0xbb, 0x51, 0xc5, 0x37, 0x58, 0x40, 0x7e, 0x81, 0x6f, 0x4e,
  0x0d, 0x8f, 0x2b, 0x52, 0xfe, 0x25, 0x9c, 0xc0, 0x6f, 0x72, 0x36, 0xbc, 0x62, 0x5f, 0x74, 0xcc,
  0xbc, 0x6d, 0xe5, 0x3d, 0x29, 0x26, 0x93, 0xd2, 0x21, 0x93, 0x29, 0xa4, 0x9a, 0x9c, 0xbd, 0x80,
  0x87, 0x30, 0xdc, 0x4e, 0x35, 0x16, 0x3d, 0xc9, 0x1f, 0xc5, 0x77, 0x9e, 0x44, 0x9e, 0x02, 0x0e,
  0xd3, 0x33, 0x14, 0x49, 0x7b, 0x19, 0x8b, 0x49, 0xeb, 0x71, 0xbc, 0x2f, 0x69, 0xd6, 0x88, 0x1b,
  0xa1, 0xa4, 0x0c, 0x79, 0x10, 0xe5, 0xd6, 0xdd, 0xa6, 0xad, 0xfe, 0x14, 0x4e, 0xb3, 0xc6, 0x67,
  0x58, 0x6b, 0xe5, 0xf0, 0xb7, 0x1b, 0x49, 0x5a, 0xe9, 0x9e, 0xa2, 0xa4, 0xd8, 0xb1, 0x39, 0x4b,
  0xa1, 0xb9, 0xca, 0xc2, 0xdf, 0xe7, 0x9a, 0x94, 0x89, 0x21, 0x37, 0x12, 0xc3, 0x0f, 0x83, 0xeb,
  0x58, 0x12, 0x38, 0x6f, 0x28, 0x32, 0x74, 0x1e, 0x47, 0x41, 0x77, 0xc2, 0xc8, 0xdd, 0x27, 0x04,
  0xfa, 0xb3, 0x78, 0x8c, 0x7c, 0x00, 0xad, 0x54, 0xf6, 0xe4, 0x25, 0x79, 0x28, 0x53, 0xda, 0x01,
  0x89, 0x9c, 0x10, 0x5c, 0xa6, 0x34, 0xe9, 0xc9, 0x7e, 0x56, 0xc3, 0xc8, 0x8e, 0x65, 0xbc, 0x28,
  0x02, 0x9d, 0x62, 0xf6, 0xbe, 0x47, 0xdc, 0xcb, 0x8d, 0x74, 0xf3, 0x9e, 0x95, 0x3f, 0xc6, 0x6b,
  0x89, 0xea, 0x5e, 0x99, 0xdd, 0x2e, 0x52, 0x83, 0x91, 0xb8, 0x15, 0xec, 0x59, 0x69, 0xe1, 0x8e,
  0x79, 0xef, 0xfe, 0x72, 0xb5, 0xb7, 0x55, 0xc8, 0xba, 0xdf, 0x26, 0x3e, 0x89, 0x94, 0xe1, 0x13,
  0x29, 0x63, 0x71, 0xf7, 0x56, 0xf6, 0x89, 0xb3, 0x53, 0xc1, 0x77, 0x85, 0x9c, 0xb3, 0x1b, 0x44,
  0xcd, 0xfe, 0xd8, 0x15, 0x12, 0x13, 0x61, 0xa2, 0xc8, 0x9a, 0xb1, 0x6b, 0x64, 0x96, 0xd2, 0x94,
  0x51, 0xcd, 0xcf, 0xee, 0x7e, 0x20, 0xa1, 0x9b, 0xcf, 0xf2, 0xad, 0x8c, 0xf9, 0x98, 0x61, 0xc2,
  0xb9, 0x60, 0x91, 0x26, 0x92, 0xaa, 0x0f, 0x44, 0x82, 0x7e, 0xa0, 0x81, 0xbc, 0x9f, 0x49, 0x29,
  0x22, 0x06, 0x98, 0xdf, 0x7d, 0xcd, 0x63, 0x3f, 0x11, 0xf1, 0xa4, 0xe1, 0x6f, 0x84, 0xbb, 0xb1,
  0xbe, 0xdf, 0xf7, 0xd4, 0x57, 0xfc, 0x4f, 0x4f, 0xb0, 0xa6, 0x6f, 0xc4, 0x35, 0xc3, 0x3a, 0x48,
  0xbc, 0x5e, 0xaf, 0xdf, 0x54, 0xf8, 0xb7, 0x5f, 0xea, 0xe4, 0xe8, 0x2b, 0x75, 0x0a, 0xdc, 0x7a,
  0xfd, 0x43, 0x1e, 0xfc, 0xf5, 0x4b, 0x3c, 0xc5, 0x1e, 0xa0, 0x19, 0xd6, 0x8b, 0x24, 0x48, 0xfc,
  0xdc, 0x51, 0x32, 0xae, 0x74, 0x78, 0xfe, 0x49, 0x54, 0x64, 0xa9, 0x08, 0x52, 0x55, 0x4a, 0x5a,
  0xa2, 0x36, 0x75, 0xf8, 0x0c, 0xdb, 0x8f, 0x90, 0xc7, 0x04, 0x42, 0x63, 0x85, 0x40, 0x2c, 0x5e,
  0xfb, 0xca, 0x02, 0x8c, 0xf2, 0xab, 0xe9, 0x5e, 0x24, 0xfd, 0x39, 0xad, 0x56, 0x39, 0x42, 0xdd,
  0xdb, 0x41, 0x28, 0x57, 0x12, 0x02, 0x91, 0xa0, 0x3d, 0x7b, 0x48, 0xef, 0x45, 0xb2, 0xea, 0x2d,
  0xde, 0xcb, 0x39, 0x32, 0xde, 0xb5, 0x82, 0x31, 0x82, 0x09, 0xa1, 0xc5, 0x61, 0xc5, 0x98, 0xef,
  0x43, 0xd8, 0x2b, 0x12, 0x6d, 0x83, 0x23, 0x19, 0xc3, 0x46, 0xd2, 0x16, 0x8d, 0x2d, 0xba, 0x83,
  0xb0, 0x39, 0xd6, 0x8f, 0xd0, 0xd4, 0xad, 0x71, 0x2b, 0x32, 0xd6, 0x1c, 0xa4, 0xa3, 0x9d, 0xc1,
  0x71, 0x8e, 0xb4, 0x71, 0xfc, 0x23, 0x45, 0xed, 0x47, 0xa8, 0x61, 0xea, 0xac, 0xd2, 0xc1, 0x10,
  0x4e, 0x3a, 0x48, 0x0b, 0xe7, 0xa8, 0x6b, 0x8a, 0xbd, 0x00, 0x5f, 0xac, 0x0e, 0x40, 0x22, 0x24,
  0x16, 0x21, 0x01, 0xbc, 0xee, 0x20, 0x33, 0x78, 0x1e, 0x37, 0x38, 0x22, 0x92, 0x26, 0x33, 0x10,
  0x11, 0x56, 0x86, 0xe5, 0x8c, 0x8d, 0xe8, 0x74, 0x27, 0x35, 0x74, 0xee, 0x97, 0x05, 0xd2, 0xa2,
  0xdb, 0xa1, 0xdc, 0x7b, 0x99, 0x25, 0xe9, 0xf9, 0x4e, 0x6b, 0x68, 0xdb, 0x08, 0x0d, 0x73, 0x29,
  0x81, 0xa8, 0xca, 0x95, 0x21, 0x74, 0x1b, 0x8c, 0x4b, 0x64, 0x73, 0xd9, 0x16, 0x27, 0x18, 0x85,
  0x65, 0x23, 0x0a, 0x0d, 0xa6, 0x05, 0x61, 0x65, 0x8e, 0x7a, 0x75, 0xcf, 0xcf, 0x84, 0x04, 0xa6,
  0xcf, 0x46, 0x1a, 0x34, 0x2c, 0x70, 0x84, 0xbf, 0xb9, 0x6d, 0x65, 0x09, 0xa7, 0x22, 0x1c, 0x99,
  0x57, 0x98, 0xb2, 0xd4, 0x87, 0x68, 0x4f, 0x87, 0xf9, 0x8d, 0x84, 0x84, 0x2a, 0x72, 0x17, 0x89,
  0x6c, 0x34, 0x0b, 0x42, 0xe4, 0xa3, 0x97, 0xbe, 0x92, 0xbb, 0xe3, 0x9c, 0x27, 0xc6, 0x32, 0x22,
  0xc6, 0x42, 0x66, 0xab, 0x8b, 0xb0, 0xa2, 0x6a, 0x23, 0xe2, 0x72, 0xd5, 0x83, 0xbc, 0x26, 0xe2,
  0x2e, 0x68, 0x23, 0xa4, 0x83, 0x88, 0x64, 0x11, 0x87, 0x02, 0x91, 0x06, 0x88, 0x3c, 0xb5, 0xb9,
  0xb9, 0xf1, 0xab, 0xf0, 0xd5, 0x83, 0x9b, 0x41, 0x96, 0xdc, 0xe4, 0xa7, 0x17, 0x36, 0x44, 0x26,
  0x6d, 0xde, 0x25, 0xf9, 0x6f, 0x8f, 0x7e, 0x7b, 0xa4, 0xea, 0x45, 0x94, 0x62, 0xd1, 0x49, 0x3b,
  0x31, 0x65, 0x14, 0xb7, 0x4e, 0xf4, 0x61, 0x88, 0x22, 0x33, 0xa7, 0xf4, 0x26, 0xb2, 0x39, 0xe7,
  0x24, 0xb7, 0xaa, 0x6e, 0x9d, 0xec, 0x15, 0xd3, 0xdc, 0xfb, 0xf1, 0xbe, 0x87, 0xfc, 0xcb, 0xc8,
  0x3f, 0x28, 0x20, 0xe5, 0x77, 0x14, 0xc0, 0x0a, 0xd4, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e,
  0x44, 0xae, 0x42, 0x60, 0x82
};

/* ── Favicon PNG (32×32, blue #2c7be5 background) ────────────────────────── */
static const unsigned int FAVICON_PNG_SIZE = 484;
static const unsigned char FAVICON_PNG[] = {
  0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x20, 0x08, 0x03, 0x00, 0x00, 0x00, 0x44, 0xa4, 0x8a,
  0xc6, 0x00, 0x00, 0x00, 0x60, 0x50, 0x4c, 0x54, 0x45, 0x2b, 0x7a, 0xe4, 0x1c, 0x71, 0xe3, 0xc6,
  0xdb, 0xf7, 0x77, 0xa9, 0xee, 0xd9, 0xe7, 0xfa, 0xfa, 0xfb, 0xfe, 0x3a, 0x83, 0xe6, 0xa8, 0xc8,
  0xf4, 0x30, 0x7e, 0xe5, 0x59, 0x97, 0xea, 0xe9, 0xf1, 0xfc, 0x87, 0xb5, 0xf0, 0xb9, 0xd3, 0xf6,
  0x46, 0x8b, 0xe8, 0x63, 0x9d, 0xeb, 0x95, 0xbc, 0xf1, 0x14, 0x6c, 0xe2, 0x69, 0xa2, 0xec, 0x9b,
  0xc1, 0xf2, 0xb0, 0xce, 0xf5, 0x08, 0x65, 0xe0, 0xe3, 0xee, 0xfb, 0xcd, 0xe0, 0xf9, 0x7d, 0xb1,
  0xf0, 0x82, 0xb0, 0xef, 0x00, 0x5b, 0xdf, 0x4e, 0x90, 0xe9, 0x78, 0xad, 0xf0, 0x80, 0xaf, 0xef,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd5, 0xb5, 0xf4, 0x9d, 0x00, 0x00, 0x01,
  0x3f, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda, 0xcd, 0x53, 0xd9, 0x72, 0xc3, 0x20, 0x0c, 0x94, 0x84,
  0x0e, 0x6e, 0xdf, 0x49, 0xaf, 0xff, 0xff, 0xce, 0x12, 0xdb, 0xd3, 0xc4, 0x99, 0xc6, 0x8f, 0x9d,
  0xf2, 0x04, 0xd2, 0xb2, 0xd2, 0xb2, 0x02, 0xe0, 0xff, 0x2e, 0x5f, 0x14, 0xcf, 0xf2, 0xca, 0xcc,
  0x59, 0x4f, 0x09, 0x7c, 0xb1, 0x78, 0x86, 0x80, 0xe0, 0x67, 0xa3, 0x63, 0x15, 0x8f, 0xdd, 0xd4,
  0x93, 0xdb, 0x82, 0x9e, 0xa2, 0x83, 0x58, 0xd6, 0x3d, 0xde, 0x42, 0xe5, 0x32, 0x00, 0x70, 0x54,
  0xc9, 0xbd, 0x6e, 0xcc, 0x41, 0x0a, 0x0d, 0x2b, 0x7a, 0x4c, 0xed, 0xb2, 0x11, 0x81, 0x67, 0xae,
  0x91, 0x80, 0x79, 0xa5, 0xe8, 0x6d, 0xcc, 0x2b, 0x1d, 0x19, 0x75, 0x3d, 0xb0, 0x52, 0x50, 0xa2,
  0x8e, 0xa6, 0xce, 0xac, 0xbb, 0xc5, 0xd1, 0x26, 0x0a, 0x37, 0x41, 0x09, 0x7d, 0x0c, 0x1d, 0xcd,
  0x96, 0x4a, 0xc3, 0xda, 0x74, 0x19, 0x8a, 0x5b, 0x6b, 0x38, 0xe3, 0xad, 0x05, 0x0f, 0x81, 0x49,
  0x71, 0xc8, 0x63, 0x97, 0x24, 0x4d, 0x12, 0xf6, 0xde, 0x91, 0xdd, 0x51, 0x05, 0x89, 0x7f, 0x93,
  0x60, 0xe2, 0xf7, 0x7c, 0x7e, 0x52, 0x09, 0x5a, 0xb0, 0x8a, 0x86, 0xb4, 0xe7, 0x5d, 0xfc, 0xed,
  0x01, 0xeb, 0x7d, 0x1b, 0xc3, 0x6b, 0x33, 0x9c, 0x36, 0x02, 0x7e, 0x9d, 0xc7, 0xcb, 0x38, 0x43,
  0x74, 0x27, 0x6e, 0x7e, 0x2c, 0xb4, 0xd8, 0xe9, 0x34, 0xa8, 0x0b, 0x4f, 0xa4, 0xab, 0x1f, 0xf8,
  0x70, 0x3e, 0x1c, 0x61, 0xcc, 0xf9, 0x0d, 0x20, 0x65, 0x08, 0x13, 0x84, 0x5c, 0x7b, 0x97, 0x0b,
  0xa4, 0x0b, 0xdd, 0x25, 0x45, 0x72, 0x62, 0x7d, 0x16, 0x37, 0x08, 0x7e, 0x0a, 0x41, 0x94, 0x50,
  0xc4, 0xd5, 0x7b, 0x19, 0xee, 0xde, 0x93, 0xa0, 0x31, 0xa5, 0x88, 0xd7, 0x85, 0x32, 0xc5, 0xaa,
  0x91, 0x47, 0x7d, 0x00, 0xcc, 0x26, 0x7d, 0xb6, 0x14, 0x19, 0xae, 0xec, 0x64, 0x88, 0x15, 0xb5,
  0x6e, 0xb6, 0xaf, 0x6b, 0x59, 0x1a, 0xfd, 0xdc, 0x26, 0x42, 0x04, 0x93, 0x78, 0xa7, 0xad, 0x04,
  0x25, 0xce, 0x3f, 0x6d, 0x0e, 0xe9, 0xab, 0x0d, 0x57, 0xad, 0x58, 0x1d, 0x34, 0xaf, 0xb1, 0x41,
  0x9a, 0xd9, 0x49, 0xe1, 0x28, 0x0b, 0x36, 0x69, 0xbb, 0x42, 0xec, 0xf1, 0xaf, 0xfe, 0xe0, 0x37,
  0x7c, 0x54, 0x0b, 0x30, 0xff, 0x3b, 0x42, 0x15, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44,
  0xae, 0x42, 0x60, 0x82
};

/* ─── config file path ──────────────────────────────────────────────────── */
#define CONF_FILE "dnms.conf"

/* ─── extern globals from dnms.c ────────────────────────────────────────── */

/* identity */
extern const char firmware_version[];
extern char dnms_version[];
extern char raspi_id_strg[];
extern int wifi_signal_strength;
extern int enable_wifi_signal_strength_influxdb;
extern int enable_wifi_signal_strength_mqtt;
extern int last_influxdb_transmission_time_to_payload;
extern int last_mqtt_transmission_time_to_payload;
extern long int last_sc_transmission_time;
extern long int last_influxdb_transmission_time;
extern long int last_mqtt_transmission_time;
extern const char *interface_name;

/* interval state */
extern bool interval_1st_active;
extern bool interval_2nd_active;

/* 1st interval */
extern float last_value_dnms_laeq;
extern float last_value_dnms_lamin;
extern float last_value_dnms_lamax;
extern float last_value_dnms_spectrum[31];
extern float last_value_dnms_lzeq;
extern float last_value_dnms_lzmin;
extern float last_value_dnms_lzmax;
extern float last_value_dnms_z_spectrum[31];
extern float last_value_dnms_lceq;
extern float last_value_dnms_lcmin;
extern float last_value_dnms_lcmax;
extern float last_value_dnms_c_spectrum[31];

/* 2nd interval */
extern float last_value_dnms_laeq_2nd;
extern float last_value_dnms_lamin_2nd;
extern float last_value_dnms_lamax_2nd;
extern float last_value_dnms_spectrum_2nd[31];
extern float last_value_dnms_lzeq_2nd;
extern float last_value_dnms_lzmin_2nd;
extern float last_value_dnms_lzmax_2nd;
extern float last_value_dnms_z_spectrum_2nd[31];
extern float last_value_dnms_lceq_2nd;
extern float last_value_dnms_lcmin_2nd;
extern float last_value_dnms_lcmax_2nd;
extern float last_value_dnms_c_spectrum_2nd[31];

/* active read flags */
extern bool read_lzeq_1st;
extern bool read_lceq_1st;
extern bool read_lzeq_2nd;
extern bool read_lceq_2nd;

/* config variables */
extern int waiting_time_after_start;
extern int prio_1st_timer, prio_1st_measurement;
extern int prio_2nd_timer, prio_2nd_measurement;
extern int prio_wlan, prio_print_ipc_and_log_1st, prio_print_ipc_and_log_2nd, prio_start_stop;
extern const char *dev_name_dnms;
extern int enable_wlan_or_lan;
extern int long_id;
extern int dnms_microphone;
extern int dnms_blink_period_1, dnms_blink_period_2, dnms_blink_period_3;
extern int dnms_blink_period_4, dnms_blink_period_5, dnms_blink_period_6, dnms_blink_period_7;
extern double dnms_micro_const_1, dnms_micro_const_2, dnms_micro_const_3;
extern double dnms_micro_const_4, dnms_micro_const_5, dnms_micro_const_6, dnms_micro_const_7;
extern double dnms_correction;
extern int enable_1st_interval, measurement_1st_interval_ms;
extern int enable_2nd_interval, measurement_2nd_interval_ms;
extern int threshold_2nd_interval_laeq_transmit, number_transmissions_after_exceeding;
extern int switch_output_pin, gpio_output_pin;
extern int start_on_full_minute, start_on_full_hour;
extern int data_transmit_laeq_1st_to_influxdb, data_transmit_lzeq_1st_to_influxdb, data_transmit_lceq_1st_to_influxdb;
extern int data_transmit_laeq_1st_spectrum_to_influxdb, data_transmit_lzeq_1st_spectrum_to_influxdb, data_transmit_lceq_1st_spectrum_to_influxdb;
extern int data_transmit_laeq_2nd_to_influxdb, data_transmit_lzeq_2nd_to_influxdb, data_transmit_lceq_2nd_to_influxdb;
extern int data_transmit_laeq_2nd_spectrum_to_influxdb, data_transmit_lzeq_2nd_spectrum_to_influxdb, data_transmit_lceq_2nd_spectrum_to_influxdb;
extern int data_transmit_laeq_1st_to_mqtt, data_transmit_lzeq_1st_to_mqtt, data_transmit_lceq_1st_to_mqtt;
extern int data_transmit_laeq_1st_spectrum_to_mqtt, data_transmit_lzeq_1st_spectrum_to_mqtt, data_transmit_lceq_1st_spectrum_to_mqtt;
extern int data_transmit_laeq_2nd_to_mqtt, data_transmit_lzeq_2nd_to_mqtt, data_transmit_lceq_2nd_to_mqtt;
extern int data_transmit_laeq_2nd_spectrum_to_mqtt, data_transmit_lzeq_2nd_spectrum_to_mqtt, data_transmit_lceq_2nd_spectrum_to_mqtt;
extern int timestamp_to_influxdb;
extern int influxdb_transmit_http, influxdb_transmit_https;
extern const char *influxdb_server, *influxdb_port, *influxdb_pfad, *influxdb_user, *influxdb_passwort, *influxdb_messung;
extern int mqtt_transmit, mqtt_port, mqtt_keepalive, mqtt_qos, mqtt_use_id_as_sub_topic;
extern const char *mqtt_user, *mqtt_passwort, *mqtt_broker, *mqtt_main_topic, *mqtt_messung;
extern int mqtt_use_tls, mqtt_tls_insecure;
extern const char *mqtt_tls_cafile;
extern int data_transmit_laeq_to_sc, sc_transmit_https, data_transmit_laeq_to_madavi;
extern const char *host_sc, *port_sc, *url_sc, *DNMS_API_PIN;
extern int  custom_api_enable;
extern char custom_api_server[128];
extern char custom_api_port[8];
extern char custom_api_path[128];
extern int  custom_api_send_1st;
extern int  custom_api_send_2nd;
extern int  custom_api_spectrum_1st;
extern int  custom_api_spectrum_2nd;
extern int  custom_api_https;
extern int data_transmit_via_pipe, start_stop_extern;
extern const char *name_of_pipe, *start_stop_name_of_pipe;
extern int data_on_terminal;
extern int data_laeq_1st_output_on_terminal, data_la_spec_1st_output_on_terminal;
extern int data_lzeq_1st_output_on_terminal, data_lz_spec_1st_output_on_terminal;
extern int data_lceq_1st_output_on_terminal, data_lc_spec_1st_output_on_terminal;
extern int data_laeq_2nd_output_on_terminal, data_la_spec_2nd_output_on_terminal;
extern int data_lzeq_2nd_output_on_terminal, data_lz_spec_2nd_output_on_terminal;
extern int data_lceq_2nd_output_on_terminal, data_lc_spec_2nd_output_on_terminal;
extern int last_influxdb_transmission_time_to_terminal, threshold_output_influxdb_ltt_to_terminal;
extern int last_mqtt_transmission_time_to_terminal, threshold_output_mqtt_ltt_to_terminal;
extern int last_sc_transmission_time_to_terminal, threshold_output_sc_ltt_to_terminal;
extern int output_thread_info;
extern int data_logging;
extern const char *data_logging_directory;
extern uint32_t counter_measurements_1st;
extern uint32_t counter_measurements_2nd;
extern time_t measurement_start_time;
extern time_t process_start_time;
extern struct timespec process_start_mono;
extern struct timespec measurement_start_mono;
extern const char *fmt_dt;
extern const char *fmt_d;
extern const char *fmt_dl;
extern int date_format_iso;

extern uint32_t dnms_error_count;
extern uint32_t influxdb_error_count;
extern uint32_t sc_error_count;
extern uint32_t madavi_error_count;
extern uint32_t mqtt_error_count;
extern uint32_t buffer_overflow_count;

/* sensor enable flags, addresses, corrections (defined in dnms.c) */
extern int         enable_sds011;
extern const char *sds011_uart_port;
extern int         enable_sps30;
extern int         enable_sht3x;
extern int         sht3x_i2c_addr;
extern int         enable_sen5x;
extern int         enable_bme280;
extern int         bme280_i2c_addr;
extern double      temp_correction_sht3x;
extern double      temp_correction_sen5x;
extern double      temp_correction_bme280;
extern int         enable_scd30;
extern double      temp_correction_scd30;
extern int         enable_scd4x;
extern double      temp_correction_scd4x;
/* last_value_*, init status, error counters — from sensors_rpi.h (included above) */

/* ═══════════════════════════════════════════════════════════════════════════
 * STRING BUFFER (SB)
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    char  *buf;
    size_t len;
    size_t cap;
} SB;

static void sb_init(SB *s) {
    s->cap = 8192;
    s->buf = malloc(s->cap);
    if (s->buf) s->buf[0] = '\0';
    s->len = 0;
}

static void sb_free(SB *s) {
    free(s->buf);
    s->buf = NULL;
    s->len = s->cap = 0;
}

static void sb_printf(SB *s, const char *fmt, ...) {
    if (!s->buf) return;
    va_list ap;
    while (1) {
        size_t avail = s->cap - s->len;
        va_start(ap, fmt);
        int need = vsnprintf(s->buf + s->len, avail, fmt, ap);
        va_end(ap);
        if (need < 0) return;
        if ((size_t)need < avail) {
            s->len += (size_t)need;
            return;
        }
        /* grow */
        size_t newcap = s->cap;
        while (newcap - s->len <= (size_t)need) newcap *= 2;
        char *tmp = realloc(s->buf, newcap);
        if (!tmp) return;
        s->buf = tmp;
        s->cap = newcap;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * URL DECODE
 * ═══════════════════════════════════════════════════════════════════════════ */

static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static void url_decode_inplace(char *s) {
    char *rd = s, *wr = s;
    while (*rd) {
        if (*rd == '+') {
            *wr++ = ' ';
            rd++;
        } else if (*rd == '%' && rd[1] && rd[2]) {
            int hi = hex_val(rd[1]);
            int lo = hex_val(rd[2]);
            if (hi >= 0 && lo >= 0) {
                *wr++ = (char)(hi * 16 + lo);
                rd += 3;
            } else {
                *wr++ = *rd++;
            }
        } else {
            *wr++ = *rd++;
        }
    }
    *wr = '\0';
}

/* ═══════════════════════════════════════════════════════════════════════════
 * KEY-VALUE STORE (KVS) for POST bodies
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct { char key[128]; char val[1024]; } KV;
typedef struct { KV *items; int n; int cap; } KVS;

static void kvs_init(KVS *k) {
    k->items = NULL;
    k->n = k->cap = 0;
}

static void kvs_free(KVS *k) {
    free(k->items);
    k->items = NULL;
    k->n = k->cap = 0;
}

static void kvs_parse_body(KVS *k, const char *body) {
    if (!body) return;
    char *copy = strdup(body);
    if (!copy) return;
    char *saveptr = NULL;
    char *pair = strtok_r(copy, "&", &saveptr);
    while (pair) {
        char *eq = strchr(pair, '=');
        if (eq) {
            *eq = '\0';
            url_decode_inplace(pair);
            url_decode_inplace(eq + 1);

            if (k->n >= k->cap) {
                int newcap = k->cap ? k->cap * 2 : 16;
                KV *tmp = realloc(k->items, (size_t)newcap * sizeof(KV));
                if (!tmp) break;
                k->items = tmp;
                k->cap = newcap;
            }
            strncpy(k->items[k->n].key, pair,   sizeof(k->items[0].key)  - 1);
            strncpy(k->items[k->n].val, eq + 1, sizeof(k->items[0].val)  - 1);
            k->items[k->n].key[sizeof(k->items[0].key) - 1] = '\0';
            k->items[k->n].val[sizeof(k->items[0].val) - 1] = '\0';
            k->n++;
        }
        pair = strtok_r(NULL, "&", &saveptr);
    }
    free(copy);
}

static const char *kvs_get(const KVS *k, const char *key) {
    for (int i = 0; i < k->n; i++)
        if (strcmp(k->items[i].key, key) == 0)
            return k->items[i].val;
    return NULL;
}

static bool kvs_has(const KVS *k, const char *key) {
    return kvs_get(k, key) != NULL;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PAM AUTHENTICATION
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    const char *password;
} PamCbData;

static int pam_conversation(int num_msg, const struct pam_message **msg,
                             struct pam_response **resp, void *appdata_ptr) {
    PamCbData *data = (PamCbData *)appdata_ptr;
    struct pam_response *r = calloc((size_t)num_msg, sizeof(struct pam_response));
    if (!r) return PAM_CONV_ERR;
    for (int i = 0; i < num_msg; i++) {
        if (msg[i]->msg_style == PAM_PROMPT_ECHO_OFF ||
            msg[i]->msg_style == PAM_PROMPT_ECHO_ON) {
            r[i].resp = strdup(data->password ? data->password : "");
            r[i].resp_retcode = 0;
        }
    }
    *resp = r;
    return PAM_SUCCESS;
}

static bool check_pam_auth(const char *username, const char *password) {
    if (!username || !password) return false;
    PamCbData cb_data = { .password = password };
    struct pam_conv conv = { pam_conversation, &cb_data };
    pam_handle_t *pamh = NULL;
    int ret = pam_start("login", username, &conv, &pamh);
    if (ret != PAM_SUCCESS) return false;
    ret = pam_authenticate(pamh, PAM_SILENT);
    pam_end(pamh, ret);
    return (ret == PAM_SUCCESS);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * HTTP HELPERS
 * ═══════════════════════════════════════════════════════════════════════════ */

static enum MHD_Result send_response(struct MHD_Connection *conn,
                                     unsigned int status,
                                     const char *content_type,
                                     const char *body, size_t body_len) {
    struct MHD_Response *resp = MHD_create_response_from_buffer(
        body_len, (void *)body, MHD_RESPMEM_MUST_COPY);
    if (!resp) return MHD_NO;
    MHD_add_response_header(resp, "Content-Type", content_type);
    enum MHD_Result r = MHD_queue_response(conn, status, resp);
    MHD_destroy_response(resp);
    return r;
}

static enum MHD_Result send_redirect(struct MHD_Connection *conn, const char *location) {
    struct MHD_Response *resp = MHD_create_response_from_buffer(0, (void *)"", MHD_RESPMEM_PERSISTENT);
    if (!resp) return MHD_NO;
    MHD_add_response_header(resp, "Location", location);
    enum MHD_Result r = MHD_queue_response(conn, MHD_HTTP_SEE_OTHER, resp);
    MHD_destroy_response(resp);
    return r;
}

static enum MHD_Result send_auth_challenge(struct MHD_Connection *conn) {
    struct MHD_Response *resp = MHD_create_response_from_buffer(
        12, (void *)"Unauthorized", MHD_RESPMEM_PERSISTENT);
    if (!resp) return MHD_NO;
    enum MHD_Result r = MHD_queue_basic_auth_fail_response(conn, "DNMS", resp);
    MHD_destroy_response(resp);
    return r;
}

static bool auth_check(struct MHD_Connection *conn) {
    char *pass = NULL;
    char *user = MHD_basic_auth_get_username_password(conn, &pass);
    if (!user) return false;
    bool ok = check_pam_auth(user, pass ? pass : "");
    free(user);
    free(pass);
    return ok;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * CSS / HTML PAGE STRUCTURE
 * ═══════════════════════════════════════════════════════════════════════════ */

static const char *CSS =
    "body{font:14px/1.5 Arial,sans-serif;margin:0;padding:0;background:#f4f4f4;}"
    "header{background:#2c7be5;color:#fff;padding:16px 18px;display:flex;align-items:center;gap:16px;}"
    "header h1{font-size:2.0em;font-weight:bold;margin:0;}"
    "header .id{font-size:1.35em;font-weight:bold;opacity:.85;}"
    "nav{background:#1a5cb5;padding:6px 20px;}"
    "nav a{color:#cde;text-decoration:none;margin-right:18px;font-size:1.2em;font-weight:bold;}"
    "nav a:hover{color:#fff;text-decoration:underline;}"
    ".content{padding:0 12px;max-width:900px;margin:14px auto;}"
    "table{border-collapse:collapse;width:100%;background:#fff;margin-bottom:16px;}"
    "th{background:#2c7be5;color:#fff;padding:7px 12px;text-align:left;}"
    "th.r{text-align:right;}"
    "td{padding:6px 12px;border-bottom:1px solid #ddd;}"
    ".num{text-align:right;font-family:monospace;}"
    "tr:nth-child(even) td{background:#e9f0fb;}"
    "h2{color:#2c7be5;font-size:1.15em;font-weight:bold;border-bottom:2px solid #2c7be5;padding-bottom:3px;margin:16px 0 8px;}"
    "h3{color:#1a5cb5;margin-top:14px;margin-bottom:4px;}"
    "fieldset{border:1px solid #ccc;padding:10px 14px;margin:0 0 12px;background:#fff;border-radius:3px;}"
    "fieldset table{margin-bottom:0;}"
    "legend{color:#2c7be5;font-weight:bold;padding:0 6px;font-size:1em;}"
    ".sectbar{background:#fff;border:1px solid #ddd;padding:6px 14px;margin-bottom:12px;border-radius:3px;line-height:2;}"
    ".sectbar a{color:#2c7be5;margin-right:12px;text-decoration:none;}"
    ".sectbar a:hover{text-decoration:underline;}"
    ".btn{display:inline-block;padding:10px 22px;margin:6px;background:#2c7be5;"
    "     color:#fff;border:none;border-radius:4px;cursor:pointer;font-size:1em;"
    "     text-decoration:none;}"
    ".btn:hover{background:#1a5cb5;}"
    ".btn-danger{background:#c0392b;}"
    ".btn-danger:hover{background:#922b21;}"
    "input[type=text],input[type=password],input[type=number],select{"
    "  padding:4px 8px;border:1px solid #bbb;border-radius:3px;width:95%;box-sizing:border-box;}"
    "input[type=submit]{padding:9px 22px;background:#2c7be5;color:#fff;border:none;"
    "  border-radius:4px;cursor:pointer;font-size:1em;margin-top:10px;}"
    "input[type=submit]:hover{background:#1a5cb5;}"
    "input[type=submit].btn-danger{background:#c0392b;}"
    "input[type=submit].btn-danger:hover{background:#922b21;}";

static void page_start(SB *s, const char *title, bool refresh) {
    sb_printf(s,
        "<!DOCTYPE html><html><head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>%s</title>", title);
    if (refresh)
        sb_printf(s, "<meta http-equiv=\"refresh\" content=\"10\">");
            sb_printf(s, "<link rel=\"icon\" type=\"image/png\" href=\"data:image/png;base64,"
                 "iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAAflBMVEUse+X2+f5V"
                 "leo4gubX5vpvpe2oyfTG2/hEiuiHtPC50/ZineuUvPFLjumfw/Pk7vvg6/ubwPMA"
                 "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
                 "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAABzK7eTAAAAIHRSTlP+9dbw2MfBzeTAxs6/"
                 "38Dj378AAAAAAAAAAAAAAAAAADHdrzwAAAFSSURBVHjarVPtksMgCGSRiBrT9t7/"
                 "ZW/RtOa+fl0ZJomwI7BLRN5n+VjfR/4FgEsw43u2ZtkuQWyS6yWtzJbEqFQYnxtS"
                 "4Umf+Ujduxhr7PUGlWzS7wF7IppFjM7qNXtAw9urBMtPHzUsWpj+tIKjIO2Y4HvC"
                 "nlAOFFlNKCM6am7gDUq8rhYm4IwkVLkczzHjeBLRGY8GiNFFE8K7jXZ2EesS99Qr"
                 "y97Ec/mYqObhi+/UBo3YCuqYrWKQ2VYTrMxGCl95jq2C/kUs1aDzZSRR9Yfci7gv"
                 "yi46/zz831Ka46T4HK/5SHqO6Rhr1NlldA+PUCwZzr7NNos1oozmsgOqIOHmaZuj"
                 "MppIUGbGA2C3bAQ46kkFAcJ7s9183FBB30UNp1yMKm6SuUloBKiZTqltkvtAA3Wg"
                 "eHkCaFGidcy1zU6hKWGT4lXU+Qsezin80d5C6CeTFwfL8jw7SQAAAABJRU5ErkJg"
                 "gg==\">"
                 "<style>%s</style></head><body>", CSS);
    sb_printf(s,
        "<header><a href=\"/\"><img src=\"/logo.png\" width=\"100\" height=\"100\" alt=\"\"></a>"
        "<div><h1>Noise Sensor</h1>"
        "<div><span class=\"id\">Communication Processor Raspberry Pi</span>"
        "<span class=\"id\">&nbsp;ID: ");
    sb_printf(s, "%s", raspi_id_strg ? raspi_id_strg : "");
    sb_printf(s,
        "</span></div></div></header>"
        "<nav>"
        "<a href=\"/\">Home</a>"
        "<a href=\"/values\">Values</a>"
        "<a href=\"/status\">Status</a>"
        "<a href=\"/config\">Config</a>"
        "<a href=\"/reset\">Restart</a>"
        "</nav>"
        "<div class=\"content\">"
        "<h2>%s</h2>", title);
}

static void page_end(SB *s) {
    sb_printf(s, "</div></body></html>");
}

/* ─── value table ─────────────────────────────────────────────────────────── */

static void tbl_start(SB *s) {
    sb_printf(s,
        "<table><tr>"
        "<th>Category</th><th>Parameter</th><th>Value</th><th>Unit</th>"
        "</tr>");
}

static void tbl_end(SB *s) {
    sb_printf(s, "</table>");
}

static void tbl_row_f(SB *s, const char *cat, const char *label, float val, const char *unit) {
    sb_printf(s,
        "<tr><td>%s</td><td>%s</td><td>%.2f</td><td>%s</td></tr>",
        cat, label, (double)val, unit ? unit : "");
}

static void tbl_row_i(SB *s, const char *cat, const char *label, int val, const char *unit) {
    sb_printf(s,
        "<tr><td>%s</td><td>%s</td><td>%d</td><td>%s</td></tr>",
        cat, label, val, unit ? unit : "");
}

static void tbl_row_s(SB *s, const char *cat, const char *label, const char *val) {
    sb_printf(s,
        "<tr><td>%s</td><td>%s</td><td>%s</td><td></td></tr>",
        cat, label, val ? val : "");
}

/* Like tbl_row_f but shows "—" when val < invalid_below (sentinel). */
static void tbl_row_fopt(SB *s, const char *cat, const char *label,
                         float val, float invalid_below, const char *unit) {
    if (val < invalid_below)
        sb_printf(s, "<tr><td>%s</td><td>%s</td><td>&mdash;</td><td>%s</td></tr>",
                  cat, label, unit ? unit : "");
    else
        sb_printf(s, "<tr><td>%s</td><td>%s</td><td>%.2f</td><td>%s</td></tr>",
                  cat, label, (double)val, unit ? unit : "");
}

/* ─── config form helpers ─────────────────────────────────────────────────── */

static void html_escape(SB *s, const char *str) {
    if (!str) return;
    for (const char *p = str; *p; p++) {
        switch (*p) {
        case '<': sb_printf(s, "&lt;");  break;
        case '>': sb_printf(s, "&gt;");  break;
        case '&': sb_printf(s, "&amp;"); break;
        case '"': sb_printf(s, "&quot;"); break;
        default:  sb_printf(s, "%c", *p); break;
        }
    }
}

static void form_section(SB *s, const char *id, const char *title) {
    sb_printf(s, "<fieldset id=\"%s\"><legend>%s</legend><table>", id, title);
}

static void form_section_end(SB *s) {
    sb_printf(s, "</table></fieldset>");
}

static void form_bool(SB *s, const char *label, const char *name, int val) {
    sb_printf(s,
        "<tr><td>%s</td><td>"
        "<input type=\"checkbox\" name=\"%s\" value=\"1\"%s>"
        "</td></tr>",
        label, name, val ? " checked" : "");
}

static void form_int(SB *s, const char *label, const char *name, int val) {
    sb_printf(s,
        "<tr><td>%s</td><td>"
        "<input type=\"number\" name=\"%s\" value=\"%d\">"
        "</td></tr>",
        label, name, val);
}

static void form_float(SB *s, const char *label, const char *name, double val, int prec) {
    sb_printf(s,
        "<tr><td>%s</td><td>"
        "<input type=\"text\" name=\"%s\" value=\"%.*f\">"
        "</td></tr>",
        label, name, prec, val);
}

static void form_i2c_addr(SB *s, const char *label, const char *name, int val,
                           int addr_a, const char *label_a,
                           int addr_b, const char *label_b) {
    sb_printf(s,
        "<tr><td>%s</td><td>"
        "<select name=\"%s\">"
        "<option value=\"%d\"%s>%s</option>"
        "<option value=\"%d\"%s>%s</option>"
        "</select></td></tr>",
        label, name,
        addr_a, val == addr_a ? " selected" : "", label_a,
        addr_b, val == addr_b ? " selected" : "", label_b);
}

static void form_str(SB *s, const char *label, const char *name, const char *val) {
    sb_printf(s,
        "<tr><td>%s</td><td>"
        "<input type=\"text\" name=\"%s\" value=\"",
        label, name);
    html_escape(s, val ? val : "");
    sb_printf(s, "\"></td></tr>");
}

static void form_password(SB *s, const char *label, const char *name, const char *val) {
    int len = val ? (int)strlen(val) : 0;
    sb_printf(s,
        "<tr><td>%s <small>(current: %d chars)</small></td><td>"
        "<input type=\"password\" name=\"%s\" value=\"",
        label, len, name);
    html_escape(s, val ? val : "");
    sb_printf(s, "\"></td></tr>");
}

static void form_select(SB *s, const char *label, const char *name, int val,
                        const char **opts, int n_opts) {
    sb_printf(s,
        "<tr><td>%s</td><td>"
        "<select name=\"%s\">",
        label, name);
    for (int i = 0; i < n_opts; i++) {
        sb_printf(s, "<option value=\"%d\"%s>%s</option>",
            i + 1, (val == i + 1) ? " selected" : "", opts[i]);
    }
    sb_printf(s, "</select></td></tr>");
}

/* ═══════════════════════════════════════════════════════════════════════════
 * POST BODY ACCUMULATION
 * ═══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    char  *body;
    size_t len;
    size_t cap;
} PostState;

static PostState *post_state_new(void) {
    PostState *ps = calloc(1, sizeof(PostState));
    if (!ps) return NULL;
    ps->cap = 4096;
    ps->body = malloc(ps->cap);
    if (!ps->body) { free(ps); return NULL; }
    ps->body[0] = '\0';
    return ps;
}

static void post_state_append(PostState *ps, const char *data, size_t size) {
    if (!ps || !ps->body) return;
    while (ps->len + size + 1 > ps->cap) {
        size_t newcap = ps->cap * 2;
        char *tmp = realloc(ps->body, newcap);
        if (!tmp) return;
        ps->body = tmp;
        ps->cap = newcap;
    }
    memcpy(ps->body + ps->len, data, size);
    ps->len += size;
    ps->body[ps->len] = '\0';
}

static void post_state_free(PostState *ps) {
    if (!ps) return;
    free(ps->body);
    free(ps);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SCHEDULE RESTART
 * ═══════════════════════════════════════════════════════════════════════════ */

static void schedule_restart(void) {
    pid_t parent = getpid();
    pid_t pid = fork();
    if (pid == 0) {
        sleep(2);
        kill(parent, SIGTERM);
        _exit(0);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * SPECTRUM TABLE HELPER
 * ═══════════════════════════════════════════════════════════════════════════ */

static const char *FREQ_LABELS[31] = {
    "20 Hz", "25 Hz", "31.5 Hz", "40 Hz", "50 Hz",
    "63 Hz", "80 Hz", "100 Hz", "125 Hz", "160 Hz",
    "200 Hz", "250 Hz", "315 Hz", "400 Hz", "500 Hz",
    "630 Hz", "800 Hz", "1 kHz", "1.25 kHz", "1.6 kHz",
    "2 kHz", "2.5 kHz", "3.15 kHz", "4 kHz", "5 kHz",
    "6.3 kHz", "8 kHz", "10 kHz", "12.5 kHz", "16 kHz", "20 kHz"
};

static void spectrum_table(SB *s,
                           const float *la_spec,
                           bool has_lz, const float *lz_spec,
                           bool has_lc, const float *lc_spec) {
    sb_printf(s, "<table><tr><th>Freq</th>"
                 "<th class=\"r\">LA 1/3-oct (dB(A))</th>");
    if (has_lz) sb_printf(s, "<th class=\"r\">LZ 1/3-oct (dB)</th>");
    if (has_lc) sb_printf(s, "<th class=\"r\">LC 1/3-oct (dB(C))</th>");
    sb_printf(s, "</tr>");
    for (int i = 0; i < 31; i++) {
        sb_printf(s, "<tr><td>%s</td><td class=\"num\">%.1f</td>",
                  FREQ_LABELS[i], (double)la_spec[i]);
        if (has_lz) sb_printf(s, "<td class=\"num\">%.1f</td>", (double)lz_spec[i]);
        if (has_lc) sb_printf(s, "<td class=\"num\">%.1f</td>", (double)lc_spec[i]);
        sb_printf(s, "</tr>");
    }
    sb_printf(s, "</table>");
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PAGE HANDLERS
 * ═══════════════════════════════════════════════════════════════════════════ */

static enum MHD_Result handle_root(struct MHD_Connection *conn) {
    SB s;
    sb_init(&s);
    page_start(&s, "DNMS Raspi", false);
    sb_printf(&s, "<p>Firmware: <strong>%s</strong></p><p>",
        firmware_version ? firmware_version : "");
    sb_printf(&s, "<a class=\"btn\" href=\"/values\">Values</a>");
    sb_printf(&s, "<a class=\"btn\" href=\"/status\">Status</a>");
    sb_printf(&s, "<a class=\"btn\" href=\"/config\">Config</a>");
    sb_printf(&s, "<a class=\"btn btn-danger\" href=\"/reset\">Restart</a>");
    sb_printf(&s, "</p>");
    page_end(&s);
    enum MHD_Result r = send_response(conn, MHD_HTTP_OK, "text/html", s.buf, s.len);
    sb_free(&s);
    return r;
}

static enum MHD_Result handle_values(struct MHD_Connection *conn) {
    SB s;
    sb_init(&s);
    page_start(&s, "Current Values", true);

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), fmt_dt, tm_info);
    sb_printf(&s, "<p>Last update: %s</p>", timebuf);

    if (enable_wifi_signal_strength_influxdb || enable_wifi_signal_strength_mqtt || data_transmit_laeq_to_madavi) {
        tbl_start(&s);
        tbl_row_i(&s, "WiFi", "Signal Strength", wifi_signal_strength, "dBm");
        tbl_end(&s);
    }

    /* ── 1st interval ─────────────────────────────── */
    if (interval_1st_active) {
        sb_printf(&s, "<h3>1st Measurement Interval (%d ms)</h3>", measurement_1st_interval_ms);
        tbl_start(&s);
        tbl_row_f(&s, "DNMS 1st", "LAeq", last_value_dnms_laeq, "dB(A)");
        tbl_row_f(&s, "DNMS 1st", "LAmin", last_value_dnms_lamin, "dB(A)");
        tbl_row_f(&s, "DNMS 1st", "LAmax", last_value_dnms_lamax, "dB(A)");
        if (read_lzeq_1st) {
            tbl_row_f(&s, "DNMS 1st", "LZeq", last_value_dnms_lzeq, "dB");
            tbl_row_f(&s, "DNMS 1st", "LZmin", last_value_dnms_lzmin, "dB");
            tbl_row_f(&s, "DNMS 1st", "LZmax", last_value_dnms_lzmax, "dB");
        }
        if (read_lceq_1st) {
            tbl_row_f(&s, "DNMS 1st", "LCeq", last_value_dnms_lceq, "dB(C)");
            tbl_row_f(&s, "DNMS 1st", "LCmin", last_value_dnms_lcmin, "dB(C)");
            tbl_row_f(&s, "DNMS 1st", "LCmax", last_value_dnms_lcmax, "dB(C)");
        }
        tbl_row_i(&s, "Measurements", "", (int)counter_measurements_1st, "");
        tbl_end(&s);

        spectrum_table(&s,
                       last_value_dnms_spectrum,
                       read_lzeq_1st, last_value_dnms_z_spectrum,
                       read_lceq_1st, last_value_dnms_c_spectrum);
    }

    /* ── Environmental Sensors ────────────────────────────────────────── */
    if (enable_sds011 || enable_sps30 || enable_sht3x || enable_sen5x || enable_bme280 || enable_scd30 || enable_scd4x) {
        sb_printf(&s, "<h3>Environmental Sensors</h3>");
        if (measurement_1st_interval_ms <= 29000) {
            sb_printf(&s, "<p><em>Sensor reads require 1st interval &gt; 29 s.</em></p>");
        } else {
            tbl_start(&s);
            if (enable_sds011) {
                if (!sds011_ok)
                    sb_printf(&s, "<tr><td>SDS011</td><td colspan='3'><em>not initialised</em></td></tr>");
                else {
                    tbl_row_fopt(&s, "SDS011", "PM10",  last_value_SDS_P1, 0.0f, "\xc2\xb5g/m\xc2\xb3");
                    tbl_row_fopt(&s, "SDS011", "PM2.5", last_value_SDS_P2, 0.0f, "\xc2\xb5g/m\xc2\xb3");
                }
            }
            if (enable_sps30) {
                if (!sps30_ok)
                    sb_printf(&s, "<tr><td>SPS30</td><td colspan='3'><em>not initialised</em></td></tr>");
                else {
                    tbl_row_fopt(&s, "SPS30", "PM1.0",    last_value_SPS30_P0,  0.0f, "\xc2\xb5g/m\xc2\xb3");
                    tbl_row_fopt(&s, "SPS30", "PM2.5",    last_value_SPS30_P2,  0.0f, "\xc2\xb5g/m\xc2\xb3");
                    tbl_row_fopt(&s, "SPS30", "PM4.0",    last_value_SPS30_P4,  0.0f, "\xc2\xb5g/m\xc2\xb3");
                    tbl_row_fopt(&s, "SPS30", "PM10",     last_value_SPS30_P1,  0.0f, "\xc2\xb5g/m\xc2\xb3");
                    tbl_row_fopt(&s, "SPS30", "NC0.5",    last_value_SPS30_N05, 0.0f, "#/cm\xc2\xb3");
                    tbl_row_fopt(&s, "SPS30", "NC1.0",    last_value_SPS30_N1,  0.0f, "#/cm\xc2\xb3");
                    tbl_row_fopt(&s, "SPS30", "NC2.5",    last_value_SPS30_N25, 0.0f, "#/cm\xc2\xb3");
                    tbl_row_fopt(&s, "SPS30", "NC4.0",    last_value_SPS30_N4,  0.0f, "#/cm\xc2\xb3");
                    tbl_row_fopt(&s, "SPS30", "NC10",     last_value_SPS30_N10, 0.0f, "#/cm\xc2\xb3");
                    tbl_row_fopt(&s, "SPS30", "Typ.size", last_value_SPS30_TS,  0.0f, "\xc2\xb5m");
                }
            }
            if (enable_sht3x) {
                if (!sht3x_ok)
                    sb_printf(&s, "<tr><td>SHT3x</td><td colspan='3'><em>not initialised</em></td></tr>");
                else {
                    tbl_row_fopt(&s, "SHT3x", "Temperature", last_value_SHT3X_T, -100.0f, "\xc2\xb0""C");
                    tbl_row_fopt(&s, "SHT3x", "Humidity",    last_value_SHT3X_H,    0.0f, "%RH");
                }
            }
            if (enable_sen5x) {
                if (!sen5x_ok)
                    sb_printf(&s, "<tr><td>SEN5x</td><td colspan='3'><em>not initialised</em></td></tr>");
                else {
                    tbl_row_fopt(&s, "SEN5x", "PM1.0",       last_value_SEN5X_P0,  0.0f, "\xc2\xb5g/m\xc2\xb3");
                    tbl_row_fopt(&s, "SEN5x", "PM2.5",       last_value_SEN5X_P2,  0.0f, "\xc2\xb5g/m\xc2\xb3");
                    tbl_row_fopt(&s, "SEN5x", "PM4.0",       last_value_SEN5X_P4,  0.0f, "\xc2\xb5g/m\xc2\xb3");
                    tbl_row_fopt(&s, "SEN5x", "PM10",        last_value_SEN5X_P1,  0.0f, "\xc2\xb5g/m\xc2\xb3");
                    tbl_row_fopt(&s, "SEN5x", "Temperature", last_value_SEN5X_T, -100.0f, "\xc2\xb0""C");
                    tbl_row_fopt(&s, "SEN5x", "Humidity",    last_value_SEN5X_H,    0.0f, "%RH");
                    tbl_row_fopt(&s, "SEN5x", "VOC index",   last_value_SEN5X_VOC,  0.0f, "");
                    tbl_row_fopt(&s, "SEN5x", "NOx index",   last_value_SEN5X_NOX,  0.0f, "");
                }
            }
            if (enable_bme280) {
                if (!bme280_ok)
                    sb_printf(&s, "<tr><td>BME280</td><td colspan='3'><em>not initialised</em></td></tr>");
                else {
                    const char *bme_name = bme280_has_humidity ? "BME280" : "BMP280";
                    tbl_row_fopt(&s, bme_name, "Temperature", last_value_BME280_T, -100.0f, "\xc2\xb0""C");
                    tbl_row_fopt(&s, bme_name, "Pressure",    last_value_BME280_P,    0.0f, "hPa");
                    if (last_value_BME280_H >= 0.0f)
                        tbl_row_fopt(&s, bme_name, "Humidity", last_value_BME280_H,  0.0f, "%RH");
                }
            }
            if (enable_scd30) {
                if (!scd30_ok)
                    sb_printf(&s, "<tr><td>SCD30</td><td colspan='3'><em>not initialised</em></td></tr>");
                else {
                    tbl_row_fopt(&s, "SCD30", "CO\xc2\xb2",     last_value_SCD30_CO2,  0.0f, "ppm");
                    tbl_row_fopt(&s, "SCD30", "Temperature", last_value_SCD30_T, -100.0f, "\xc2\xb0""C");
                    tbl_row_fopt(&s, "SCD30", "Humidity",    last_value_SCD30_H,    0.0f, "%RH");
                }
            }
            if (enable_scd4x) {
                if (!scd4x_ok)
                    sb_printf(&s, "<tr><td>SCD4x</td><td colspan='3'><em>not initialised</em></td></tr>");
                else {
                    tbl_row_fopt(&s, "SCD4x", "CO\xc2\xb2",     last_value_SCD4X_CO2,  0.0f, "ppm");
                    tbl_row_fopt(&s, "SCD4x", "Temperature", last_value_SCD4X_T, -100.0f, "\xc2\xb0""C");
                    tbl_row_fopt(&s, "SCD4x", "Humidity",    last_value_SCD4X_H,    0.0f, "%RH");
                }
            }
            tbl_end(&s);
        }
    }

    /* ── 2nd interval ─────────────────────────────── */
    if (interval_2nd_active) {
        sb_printf(&s, "<h3>2nd Measurement Interval (%d ms)</h3>", measurement_2nd_interval_ms);
        tbl_start(&s);
        tbl_row_f(&s, "DNMS 2nd", "LAeq", last_value_dnms_laeq_2nd, "dB(A)");
        tbl_row_f(&s, "DNMS 2nd", "LAmin", last_value_dnms_lamin_2nd, "dB(A)");
        tbl_row_f(&s, "DNMS 2nd", "LAmax", last_value_dnms_lamax_2nd, "dB(A)");
        if (read_lzeq_2nd) {
            tbl_row_f(&s, "DNMS 2nd", "LZeq", last_value_dnms_lzeq_2nd, "dB");
            tbl_row_f(&s, "DNMS 2nd", "LZmin", last_value_dnms_lzmin_2nd, "dB");
            tbl_row_f(&s, "DNMS 2nd", "LZmax", last_value_dnms_lzmax_2nd, "dB");
        }
        if (read_lceq_2nd) {
            tbl_row_f(&s, "DNMS 2nd", "LCeq", last_value_dnms_lceq_2nd, "dB(C)");
            tbl_row_f(&s, "DNMS 2nd", "LCmin", last_value_dnms_lcmin_2nd, "dB(C)");
            tbl_row_f(&s, "DNMS 2nd", "LCmax", last_value_dnms_lcmax_2nd, "dB(C)");
        }
        tbl_row_i(&s, "Measurements", "", (int)counter_measurements_2nd, "");
        tbl_end(&s);

        spectrum_table(&s,
                       last_value_dnms_spectrum_2nd,
                       read_lzeq_2nd, last_value_dnms_z_spectrum_2nd,
                       read_lceq_2nd, last_value_dnms_c_spectrum_2nd);
    }

    if (!interval_1st_active && !interval_2nd_active)
        sb_printf(&s, "<p><em>No measurement intervals active.</em></p>");

    page_end(&s);
    enum MHD_Result r = send_response(conn, MHD_HTTP_OK, "text/html", s.buf, s.len);
    sb_free(&s);
    return r;
}

static void iface_mac_str(const char *iface, char *out, size_t outlen) {
    if (!iface || !iface[0]) { snprintf(out, outlen, "N/A"); return; }
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { snprintf(out, outlen, "N/A"); return; }
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
        unsigned char *m = (unsigned char *)ifr.ifr_hwaddr.sa_data;
        snprintf(out, outlen, "%02X:%02X:%02X:%02X:%02X:%02X",
                 m[0], m[1], m[2], m[3], m[4], m[5]);
    } else {
        snprintf(out, outlen, "N/A");
    }
    close(fd);
}

static bool iface_connected(const char *iface) {
    if (!iface || !iface[0]) return false;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return false;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    bool up = false;
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) == 0)
        up = (ifr.ifr_flags & IFF_UP) && (ifr.ifr_flags & IFF_RUNNING);
    close(fd);
    return up;
}

static void iface_ssid(const char *iface, char *out, size_t outlen) {
    if (!iface || !iface[0]) { snprintf(out, outlen, "N/A"); return; }
    int skfd = iw_sockets_open();
    if (skfd < 0) { snprintf(out, outlen, "N/A"); return; }
    wireless_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (iw_get_basic_config(skfd, iface, &cfg) >= 0 && cfg.has_essid && cfg.essid[0])
        snprintf(out, outlen, "%s", cfg.essid);
    else
        snprintf(out, outlen, "N/A");
    iw_sockets_close(skfd);
}

static enum MHD_Result handle_status(struct MHD_Connection *conn) {
    SB s;
    sb_init(&s);
    page_start(&s, "Status", false);

    /* Device */
    sb_printf(&s,
        "<div class=\"sectbar\">"
        "<a href=\"#st-device\">Device</a>"
        "<a href=\"#st-meas\">Measurements</a>"
        "<a href=\"#st-tx\">Tx&nbsp;Durations</a>"
        "<a href=\"#st-mem\">Memory</a>"
        "<a href=\"#st-sensors\">Sensor&nbsp;Status</a>"
        "<a href=\"#st-errors\">Errors</a>"
        "</div>");

    sb_printf(&s, "<h2 id=\"st-device\">Device</h2>"
              "<table><tr><th>Parameter</th><th>Value</th></tr>");
    sb_printf(&s, "<tr><td>DNMS firmware Teensy4.0</td><td>%s</td></tr>",
              dnms_version ? dnms_version : "");
    sb_printf(&s, "<tr><td>Raspi firmware version</td><td>%s</td></tr>",
              firmware_version ? firmware_version : "");
    sb_printf(&s, "<tr><td>Device ID</td><td>%s</td></tr>",
              raspi_id_strg ? raspi_id_strg : "");
    {
        char mac[32];
        iface_mac_str(interface_name ? interface_name : "", mac, sizeof(mac));
        sb_printf(&s, "<tr><td>MAC address</td><td>%s</td></tr>", mac);
    }
    sb_printf(&s, "<tr><td>Interface</td><td>%s</td></tr>",
              interface_name ? interface_name : "");
    sb_printf(&s, "<tr><td>WiFi connected</td><td>%s</td></tr>",
              iface_connected(interface_name ? interface_name : "") ? "yes" : "no");
    if (enable_wifi_signal_strength_influxdb || enable_wifi_signal_strength_mqtt || data_transmit_laeq_to_madavi) {
        char ssid[IW_ESSID_MAX_SIZE + 2];
        iface_ssid(interface_name ? interface_name : "", ssid, sizeof(ssid));
        sb_printf(&s, "<tr><td>WiFi SSID</td><td>%s</td></tr>", ssid);
        int q = 2 * (wifi_signal_strength + 100);
        if (q < 0) q = 0; if (q > 100) q = 100;
        sb_printf(&s, "<tr><td>WiFi RSSI</td><td>%d dBm</td></tr>", wifi_signal_strength);
        sb_printf(&s, "<tr><td>WiFi signal quality</td><td>%d %%</td></tr>", q);
    }
    {
        struct timespec now_mono;
        clock_gettime(CLOCK_MONOTONIC, &now_mono);
        long up = process_start_mono.tv_sec ? (long)(now_mono.tv_sec - process_start_mono.tv_sec) : 0;
        char buf[48];
        snprintf(buf, sizeof(buf), "%ldd %02ld:%02ld:%02ld",
                 up / 86400, (up % 86400) / 3600, (up % 3600) / 60, up % 60);
        sb_printf(&s, "<tr><td>System uptime</td><td>%s</td></tr>", buf);
    }
    {
        struct timespec now_mono;
        clock_gettime(CLOCK_MONOTONIC, &now_mono);
        long up = measurement_start_mono.tv_sec ? (long)(now_mono.tv_sec - measurement_start_mono.tv_sec) : 0;
        char buf[48];
        snprintf(buf, sizeof(buf), "%ldd %02ld:%02ld:%02ld",
                 up / 86400, (up % 86400) / 3600, (up % 3600) / 60, up % 60);
        sb_printf(&s, "<tr><td>Measurement uptime</td><td>%s</td></tr>", buf);
    }
    {
        time_t now = time(NULL);
        char utc[40], loc[40];
        strftime(utc, sizeof(utc), fmt_dt, gmtime(&now));
        strncat(utc, " UTC", sizeof(utc) - strlen(utc) - 1);
        strftime(loc, sizeof(loc), fmt_dt, localtime(&now));
        sb_printf(&s, "<tr><td>UTC time</td><td>%s</td></tr>", utc);
        sb_printf(&s, "<tr><td>Local time</td><td>%s</td></tr>", loc);
    }
    sb_printf(&s, "</table>");

    /* Measurements */
    sb_printf(&s, "<h2 id=\"st-meas\">Measurements</h2>"
              "<table><tr><th>Parameter</th><th>Value</th></tr>");
    sb_printf(&s, "<tr><td>1st interval (%d ms)</td><td>%s &mdash; %u completed</td></tr>",
              measurement_1st_interval_ms, interval_1st_active ? "active" : "inactive",
              counter_measurements_1st);
    sb_printf(&s, "<tr><td>2nd interval (%d ms)</td><td>%s &mdash; %u completed</td></tr>",
              measurement_2nd_interval_ms, interval_2nd_active ? "active" : "inactive",
              counter_measurements_2nd);
    {
        char startbuf[64];
        if (measurement_start_time)
            strftime(startbuf, sizeof(startbuf), fmt_dt,
                     localtime(&measurement_start_time));
        else
            strcpy(startbuf, "not yet");
        sb_printf(&s, "<tr><td>Measurements started</td><td>%s</td></tr>", startbuf);
    }
    sb_printf(&s, "</table>");

    /* Transmission durations */
    sb_printf(&s, "<h2 id=\"st-tx\">Last Transmission Duration</h2>"
              "<table><tr><th>Channel</th><th>Duration</th></tr>");
    {
        char _tb[24];
        if (data_transmit_laeq_to_sc || data_transmit_laeq_to_madavi) {
            if (last_sc_transmission_time) snprintf(_tb, sizeof(_tb), "%ld ms", last_sc_transmission_time);
            else strcpy(_tb, "pending");
            sb_printf(&s, "<tr><td>Sensor.Community / Madavi</td><td>%s</td></tr>", _tb);
        }
        if (influxdb_transmit_http || influxdb_transmit_https) {
            if (last_influxdb_transmission_time) snprintf(_tb, sizeof(_tb), "%ld ms", last_influxdb_transmission_time);
            else strcpy(_tb, "pending");
            sb_printf(&s, "<tr><td>InfluxDB</td><td>%s</td></tr>", _tb);
        }
        if (mqtt_transmit) {
            if (last_mqtt_transmission_time) snprintf(_tb, sizeof(_tb), "%ld ms", last_mqtt_transmission_time);
            else strcpy(_tb, "pending");
            sb_printf(&s, "<tr><td>MQTT</td><td>%s</td></tr>", _tb);
        }
    }
    sb_printf(&s, "</table>");

    /* Memory */
    sb_printf(&s, "<h2 id=\"st-mem\">Memory</h2>"
              "<table><tr><th>Parameter</th><th>Value</th></tr>");
    {
        struct sysinfo si;
        if (sysinfo(&si) == 0) {
            unsigned long unit = si.mem_unit ? (unsigned long)si.mem_unit : 1UL;
            unsigned long total_mb = (si.totalram * unit) / (1024UL * 1024UL);
            unsigned long free_mb  = (si.freeram  * unit) / (1024UL * 1024UL);
            unsigned long buf_mb   = ((si.freeram + si.bufferram) * unit) / (1024UL * 1024UL);
            sb_printf(&s, "<tr><td>Total RAM</td><td>%lu MB</td></tr>", total_mb);
            sb_printf(&s, "<tr><td>Free RAM</td><td>%lu MB</td></tr>", free_mb);
            sb_printf(&s, "<tr><td>Free + buffers</td><td>%lu MB</td></tr>", buf_mb);
        }
    }
    sb_printf(&s, "</table>");

    /* Sensor Status */
    sb_printf(&s, "<h2 id=\"st-sensors\">Sensor Status</h2>"
              "<table><tr><th>Sensor</th><th>Status</th><th>Read errors</th></tr>");
    /* DNMS (Teensy 4.0) — always shown first */
    sb_printf(&s, "<tr><td>DNMS (Teensy 4.0)</td><td>%s</td><td>%u</td></tr>",
              (dnms_version && dnms_version[0]) ? dnms_version : "<em>not initialised</em>",
              dnms_error_count);
    if (enable_sds011) sb_printf(&s, "<tr><td>SDS011</td><td>%s</td><td>%u</td></tr>",
                                 sds011_ok ? "ok" : "<em>not initialised</em>", sensor_err_sds011);
    if (enable_sps30)  sb_printf(&s, "<tr><td>SPS30</td><td>%s</td><td>%u</td></tr>",
                                 sps30_ok  ? "ok" : "<em>not initialised</em>", sensor_err_sps30);
    if (enable_sht3x)  sb_printf(&s, "<tr><td>SHT3x</td><td>%s</td><td>%u</td></tr>",
                                 sht3x_ok  ? "ok" : "<em>not initialised</em>", sensor_err_sht3x);
    if (enable_sen5x)  sb_printf(&s, "<tr><td>SEN5x</td><td>%s</td><td>%u</td></tr>",
                                 sen5x_ok  ? "ok" : "<em>not initialised</em>", sensor_err_sen5x);
    if (enable_bme280) sb_printf(&s, "<tr><td>BME280</td><td>%s</td><td>%u</td></tr>",
                                 !bme280_ok ? "<em>not initialised</em>" :
                                 bme280_has_humidity ? "ok" : "BMP280 (no humidity)",
                                 sensor_err_bme280);
    if (enable_scd30)  sb_printf(&s, "<tr><td>SCD30</td><td>%s</td><td>%u</td></tr>",
                                 scd30_ok  ? "ok" : "<em>not initialised</em>", sensor_err_scd30);
    if (enable_scd4x)  sb_printf(&s, "<tr><td>SCD4x</td><td>%s</td><td>%u</td></tr>",
                                 scd4x_ok  ? "ok" : "<em>not initialised</em>", sensor_err_scd4x);
    sb_printf(&s, "</table>");

    /* Errors */
    sb_printf(&s, "<h2 id=\"st-errors\">Errors</h2>"
              "<table><tr><th>Parameter</th><th>Value</th></tr>");
    sb_printf(&s, "<tr><td>DNMS read errors</td><td>%u</td></tr>",           dnms_error_count);
    if (influxdb_transmit_http || influxdb_transmit_https)
        sb_printf(&s, "<tr><td>InfluxDB errors</td><td>%u</td></tr>",        influxdb_error_count);
    if (data_transmit_laeq_to_sc)
        sb_printf(&s, "<tr><td>Sensor.Community errors</td><td>%u</td></tr>", sc_error_count);
    if (data_transmit_laeq_to_madavi)
        sb_printf(&s, "<tr><td>Madavi errors</td><td>%u</td></tr>",           madavi_error_count);
    if (mqtt_transmit)
        sb_printf(&s, "<tr><td>MQTT errors</td><td>%u</td></tr>",             mqtt_error_count);
    sb_printf(&s, "<tr><td>Buffer overflows (dropped)</td><td>%u</td></tr>",  buffer_overflow_count);
    sb_printf(&s, "</table>");

    page_end(&s);
    enum MHD_Result r = send_response(conn, MHD_HTTP_OK, "text/html", s.buf, s.len);
    sb_free(&s);
    return r;
}

static enum MHD_Result handle_config_get(struct MHD_Connection *conn) {
    if (!auth_check(conn)) return send_auth_challenge(conn);

    SB s;
    sb_init(&s);
    page_start(&s, "Configuration", false);

    sb_printf(&s,
        "<div class=\"sectbar\">"
        "<a href=\"#system\">System</a>"
        "<a href=\"#hardware\">Hardware</a>"
        "<a href=\"#microphone\">Microphone</a>"
        "<a href=\"#intervals\">Intervals</a>"
        "<a href=\"#influx\">InfluxDB&nbsp;Connection</a>"
        "<a href=\"#txdata_influxdb\">InfluxDB&nbsp;-&nbsp;Transmit&nbsp;Data</a>"
        "<a href=\"#mqtt\">MQTT&nbsp;Connection</a>"
        "<a href=\"#txdata_mqtt\">MQTT&nbsp;-&nbsp;Transmit&nbsp;Data</a>"
        "<a href=\"#sc\">Sensor.Community</a>"
        "<a href=\"#custom_api\">Custom&nbsp;API</a>"
        "<a href=\"#ipc\">IPC</a>"
        "<a href=\"#terminal\">Terminal</a>"
        "<a href=\"#logging\">Data&nbsp;Logging</a>"
        "<a href=\"#webserver\">Webserver</a>"
        "<a href=\"#sensors\">Environmental&nbsp;Sensors</a>"
        "</div>"
        "<form method=\"post\" action=\"/config\">");

    /* ── Section 1: System ─────────────────────────── */
    form_section(&s, "system", "System");
    form_bool(&s, "Date format ISO (YYYY-MM-DD)", "date_format_iso", date_format_iso);
    form_int(&s, "Waiting time after start (s)", "waiting_time_after_start", waiting_time_after_start);
    form_int(&s, "Priority: 1st timer",           "prio_1st_timer",           prio_1st_timer);
    form_int(&s, "Priority: 1st measurement",     "prio_1st_measurement",     prio_1st_measurement);
    form_int(&s, "Priority: 2nd timer",           "prio_2nd_timer",           prio_2nd_timer);
    form_int(&s, "Priority: 2nd measurement",     "prio_2nd_measurement",     prio_2nd_measurement);
    form_int(&s, "Priority: WLAN",                "prio_wlan",                prio_wlan);
    form_int(&s, "Priority: print IPC+log 1st",  "prio_print_ipc_and_log_1st", prio_print_ipc_and_log_1st);
    form_int(&s, "Priority: print IPC+log 2nd",  "prio_print_ipc_and_log_2nd", prio_print_ipc_and_log_2nd);
    form_int(&s, "Priority: start/stop",          "prio_start_stop",          prio_start_stop);
    form_section_end(&s);

    /* ── Section 2: Hardware ───────────────────────── */
    form_section(&s, "hardware", "Hardware");
    form_str( &s, "DNMS device name",         "dev_name_dnms",             dev_name_dnms);
    form_str( &s, "Network interface",         "interface_name",            interface_name);
    form_bool(&s, "Enable WLAN/LAN",           "enable_wlan_or_lan",        enable_wlan_or_lan);
    form_bool(&s, "WiFi RSSI → InfluxDB payload",   "enable_wifi_signal_strength_influxdb", enable_wifi_signal_strength_influxdb);
    form_bool(&s, "WiFi RSSI → MQTT payload",        "enable_wifi_signal_strength_mqtt",     enable_wifi_signal_strength_mqtt);
    form_bool(&s, "Tx duration (ms) → InfluxDB payload", "last_influxdb_transmission_time_to_payload", last_influxdb_transmission_time_to_payload);
    form_bool(&s, "Tx duration (ms) → MQTT payload",     "last_mqtt_transmission_time_to_payload",     last_mqtt_transmission_time_to_payload);
    form_bool(&s, "Long ID",                   "long_id",                   long_id);
    form_section_end(&s);

    /* ── Section 3: Microphone ─────────────────────── */
    {
        static const char *mic_opts[] = {
            "ICS-43434",
            "IM72D128",
            "IM72D128 with DLR case",
            "ICS-43434 no correction",
            "IM72D128 no correction",
            "micro_3 (custom)",
            "micro_4 (custom)"
        };
        form_section(&s, "microphone", "Microphone");
        form_select(&s, "Microphone type", "dnms_microphone", dnms_microphone,
                    mic_opts, 7);
        form_float(&s, "Correction (dB)",      "dnms_correction",    dnms_correction,    4);
        form_int(  &s, "Blink period 1 (ms)",  "dnms_blink_period_1", dnms_blink_period_1);
        form_int(  &s, "Blink period 2 (ms)",  "dnms_blink_period_2", dnms_blink_period_2);
        form_int(  &s, "Blink period 3 (ms)",  "dnms_blink_period_3", dnms_blink_period_3);
        form_int(  &s, "Blink period 4 (ms)",  "dnms_blink_period_4", dnms_blink_period_4);
        form_int(  &s, "Blink period 5 (ms)",  "dnms_blink_period_5", dnms_blink_period_5);
        form_int(  &s, "Blink period 6 (ms)",  "dnms_blink_period_6", dnms_blink_period_6);
        form_int(  &s, "Blink period 7 (ms)",  "dnms_blink_period_7", dnms_blink_period_7);
        form_float(&s, "Micro constant 1",     "dnms_micro_const_1",  dnms_micro_const_1, 10);
        form_float(&s, "Micro constant 2",     "dnms_micro_const_2",  dnms_micro_const_2, 10);
        form_float(&s, "Micro constant 3",     "dnms_micro_const_3",  dnms_micro_const_3, 10);
        form_float(&s, "Micro constant 4",     "dnms_micro_const_4",  dnms_micro_const_4, 10);
        form_float(&s, "Micro constant 5",     "dnms_micro_const_5",  dnms_micro_const_5, 10);
        form_float(&s, "Micro constant 6",     "dnms_micro_const_6",  dnms_micro_const_6, 10);
        form_float(&s, "Micro constant 7",     "dnms_micro_const_7",  dnms_micro_const_7, 10);
        form_section_end(&s);
    }

    /* ── Section 4: Measurement Intervals ─────────── */
    form_section(&s, "intervals", "Measurement Intervals");
    form_bool(&s, "Enable 1st interval",                    "enable_1st_interval",                  enable_1st_interval);
    form_int( &s, "1st interval duration (ms)",             "measurement_1st_interval_ms",           measurement_1st_interval_ms);
    form_bool(&s, "Enable 2nd interval",                    "enable_2nd_interval",                  enable_2nd_interval);
    form_int( &s, "2nd interval duration (ms)",             "measurement_2nd_interval_ms",           measurement_2nd_interval_ms);
    form_int( &s, "2nd interval LAeq transmit threshold (dB(A))", "threshold_2nd_interval_laeq_transmit", threshold_2nd_interval_laeq_transmit);
    form_int( &s, "Transmissions after exceeding threshold","number_transmissions_after_exceeding",  number_transmissions_after_exceeding);
    form_bool(&s, "Switch output pin",                      "switch_output_pin",                    switch_output_pin);
    form_int( &s, "GPIO output pin",                        "gpio_output_pin",                      gpio_output_pin);
    form_bool(&s, "Start on full minute",                   "start_on_full_minute",                 start_on_full_minute);
    form_bool(&s, "Start on full hour",                     "start_on_full_hour",                   start_on_full_hour);
    form_section_end(&s);

    /* ── InfluxDB Connection ───────────── */
    form_section(&s, "influx", "InfluxDB Connection");
    form_bool(    &s, "Transmit via HTTP",   "influxdb_transmit_http",   influxdb_transmit_http);
    form_bool(    &s, "Transmit via HTTPS",  "influxdb_transmit_https",  influxdb_transmit_https);
    form_str(     &s, "Server",              "influxdb_server",           influxdb_server);
    form_str(     &s, "Port",                "influxdb_port",             influxdb_port);
    form_str(     &s, "Path",                "influxdb_pfad",             influxdb_pfad);
    form_str(     &s, "User",                "influxdb_user",             influxdb_user);
    form_password(&s, "Password",            "influxdb_passwort",         influxdb_passwort);
    form_str(     &s, "InfluxDB measurement name", "influxdb_messung",     influxdb_messung);
    form_section_end(&s);

    /* ── InfluxDB - Transmit Data ── */
    form_section(&s, "txdata_influxdb", "InfluxDB - Transmit Data");
    form_bool(&s, "LAeq 1st interval",        "data_transmit_laeq_1st_to_influxdb",          data_transmit_laeq_1st_to_influxdb);
    form_bool(&s, "LZeq 1st interval",        "data_transmit_lzeq_1st_to_influxdb",          data_transmit_lzeq_1st_to_influxdb);
    form_bool(&s, "LCeq 1st interval",        "data_transmit_lceq_1st_to_influxdb",          data_transmit_lceq_1st_to_influxdb);
    form_bool(&s, "LA spectrum 1st interval", "data_transmit_laeq_1st_spectrum_to_influxdb",  data_transmit_laeq_1st_spectrum_to_influxdb);
    form_bool(&s, "LZ spectrum 1st interval", "data_transmit_lzeq_1st_spectrum_to_influxdb",  data_transmit_lzeq_1st_spectrum_to_influxdb);
    form_bool(&s, "LC spectrum 1st interval", "data_transmit_lceq_1st_spectrum_to_influxdb",  data_transmit_lceq_1st_spectrum_to_influxdb);
    form_bool(&s, "LAeq 2nd interval",        "data_transmit_laeq_2nd_to_influxdb",          data_transmit_laeq_2nd_to_influxdb);
    form_bool(&s, "LZeq 2nd interval",        "data_transmit_lzeq_2nd_to_influxdb",          data_transmit_lzeq_2nd_to_influxdb);
    form_bool(&s, "LCeq 2nd interval",        "data_transmit_lceq_2nd_to_influxdb",          data_transmit_lceq_2nd_to_influxdb);
    form_bool(&s, "LA spectrum 2nd interval", "data_transmit_laeq_2nd_spectrum_to_influxdb",  data_transmit_laeq_2nd_spectrum_to_influxdb);
    form_bool(&s, "LZ spectrum 2nd interval", "data_transmit_lzeq_2nd_spectrum_to_influxdb",  data_transmit_lzeq_2nd_spectrum_to_influxdb);
    form_bool(&s, "LC spectrum 2nd interval", "data_transmit_lceq_2nd_spectrum_to_influxdb",  data_transmit_lceq_2nd_spectrum_to_influxdb);
    form_bool(&s, "Timestamp to InfluxDB",    "timestamp_to_influxdb",                        timestamp_to_influxdb);
    form_section_end(&s);

    /* ── MQTT Connection ──────────────────────────── */
    form_section(&s, "mqtt", "MQTT Connection");
    form_bool(    &s, "Enable MQTT",         "mqtt_transmit",             mqtt_transmit);
    form_str(     &s, "Broker",              "mqtt_broker",               mqtt_broker);
    form_int(     &s, "Port",                "mqtt_port",                 mqtt_port);
    form_str(     &s, "User",                "mqtt_user",                 mqtt_user);
    form_password(&s, "Password",            "mqtt_passwort",             mqtt_passwort);
    form_int(     &s, "Keepalive (s)",       "mqtt_keepalive",            mqtt_keepalive);
    form_int(     &s, "QoS",                 "mqtt_qos",                  mqtt_qos);
    form_str(     &s, "Main topic",          "mqtt_main_topic",           mqtt_main_topic);
    form_bool(    &s, "Use ID as sub-topic", "mqtt_use_id_as_sub_topic",  mqtt_use_id_as_sub_topic);
    form_str(     &s, "MQTT measurement name", "mqtt_messung",            mqtt_messung);
    form_bool(    &s, "Use TLS/SSL",         "mqtt_use_tls",              mqtt_use_tls);
    form_str(     &s, "CA certificate file", "mqtt_tls_cafile",           mqtt_tls_cafile ? mqtt_tls_cafile : "");
    form_bool(    &s, "Skip hostname verification (insecure)", "mqtt_tls_insecure", mqtt_tls_insecure);
    form_section_end(&s);

    /* ── MQTT - Transmit Data ── */
    form_section(&s, "txdata_mqtt", "MQTT - Transmit Data");
    form_bool(&s, "LAeq 1st interval",        "data_transmit_laeq_1st_to_mqtt",          data_transmit_laeq_1st_to_mqtt);
    form_bool(&s, "LZeq 1st interval",        "data_transmit_lzeq_1st_to_mqtt",          data_transmit_lzeq_1st_to_mqtt);
    form_bool(&s, "LCeq 1st interval",        "data_transmit_lceq_1st_to_mqtt",          data_transmit_lceq_1st_to_mqtt);
    form_bool(&s, "LA spectrum 1st interval", "data_transmit_laeq_1st_spectrum_to_mqtt",  data_transmit_laeq_1st_spectrum_to_mqtt);
    form_bool(&s, "LZ spectrum 1st interval", "data_transmit_lzeq_1st_spectrum_to_mqtt",  data_transmit_lzeq_1st_spectrum_to_mqtt);
    form_bool(&s, "LC spectrum 1st interval", "data_transmit_lceq_1st_spectrum_to_mqtt",  data_transmit_lceq_1st_spectrum_to_mqtt);
    form_bool(&s, "LAeq 2nd interval",        "data_transmit_laeq_2nd_to_mqtt",          data_transmit_laeq_2nd_to_mqtt);
    form_bool(&s, "LZeq 2nd interval",        "data_transmit_lzeq_2nd_to_mqtt",          data_transmit_lzeq_2nd_to_mqtt);
    form_bool(&s, "LCeq 2nd interval",        "data_transmit_lceq_2nd_to_mqtt",          data_transmit_lceq_2nd_to_mqtt);
    form_bool(&s, "LA spectrum 2nd interval", "data_transmit_laeq_2nd_spectrum_to_mqtt",  data_transmit_laeq_2nd_spectrum_to_mqtt);
    form_bool(&s, "LZ spectrum 2nd interval", "data_transmit_lzeq_2nd_spectrum_to_mqtt",  data_transmit_lzeq_2nd_spectrum_to_mqtt);
    form_bool(&s, "LC spectrum 2nd interval", "data_transmit_lceq_2nd_spectrum_to_mqtt",  data_transmit_lceq_2nd_spectrum_to_mqtt);
    form_section_end(&s);

    /* ── Section 8: Sensor.Community ─────────────── */
    form_section(&s, "sc", "Sensor.Community");
    form_bool(&s, "Transmit LAeq to SC",          "data_transmit_laeq_to_sc",      data_transmit_laeq_to_sc);
    form_str( &s, "Host",                         "host_sc",                        host_sc);
    form_str( &s, "URL",                          "url_sc",                         url_sc);
    form_str( &s, "API PIN",                      "DNMS_API_PIN",                   DNMS_API_PIN);
    form_bool(&s, "Transmit via HTTPS",           "sc_transmit_https",              sc_transmit_https);
    form_bool(&s, "Enable Madavi (api-rrd.madavi.de)", "data_transmit_laeq_to_madavi", data_transmit_laeq_to_madavi);
    form_section_end(&s);

    /* ── Section 9: Custom API ──────────────────────── */
    form_section(&s, "custom_api", "Custom API");
    form_bool(&s, "Enable Custom API push",                    "custom_api_enable",       custom_api_enable);
    form_str( &s, "Server (IP or hostname)",                   "custom_api_server",       custom_api_server);
    form_str( &s, "Port",                                      "custom_api_port",         custom_api_port);
    form_str( &s, "Path",                                      "custom_api_path",         custom_api_path);
    form_bool(&s, "Use HTTPS",                                 "custom_api_https",        custom_api_https);
    form_bool(&s, "Enable 1st interval for transmission",      "custom_api_send_1st",     custom_api_send_1st);
    form_bool(&s, "Enable 1/3-octave values of 1st interval",  "custom_api_spectrum_1st", custom_api_spectrum_1st);
    form_bool(&s, "Enable 2nd interval for transmission",      "custom_api_send_2nd",     custom_api_send_2nd);
    form_bool(&s, "Enable 1/3-octave values of 2nd interval",  "custom_api_spectrum_2nd", custom_api_spectrum_2nd);
    form_section_end(&s);

    /* ── Section 10: IPC ──────────────────────────── */
    form_section(&s, "ipc", "IPC (Named Pipes)");
    form_bool(&s, "Transmit via pipe",        "data_transmit_via_pipe",      data_transmit_via_pipe);
    form_str( &s, "Pipe name",                "name_of_pipe",                name_of_pipe);
    form_bool(&s, "Start/stop via extern",    "start_stop_extern",           start_stop_extern);
    form_str( &s, "Start/stop pipe name",     "start_stop_name_of_pipe",     start_stop_name_of_pipe);
    form_section_end(&s);

    /* ── Section 10: Terminal / Pipe / Log Output ── */
    form_section(&s, "terminal", "Terminal / Pipe / Log Output");
    form_bool(&s, "Data on terminal",                     "data_on_terminal",                     data_on_terminal);
    form_bool(&s, "LAeq 1st on terminal",                 "data_laeq_1st_output_on_terminal",     data_laeq_1st_output_on_terminal);
    form_bool(&s, "LA spectrum 1st on terminal",          "data_la_spec_1st_output_on_terminal",  data_la_spec_1st_output_on_terminal);
    form_bool(&s, "LZeq 1st on terminal",                 "data_lzeq_1st_output_on_terminal",     data_lzeq_1st_output_on_terminal);
    form_bool(&s, "LZ spectrum 1st on terminal",          "data_lz_spec_1st_output_on_terminal",  data_lz_spec_1st_output_on_terminal);
    form_bool(&s, "LCeq 1st on terminal",                 "data_lceq_1st_output_on_terminal",     data_lceq_1st_output_on_terminal);
    form_bool(&s, "LC spectrum 1st on terminal",          "data_lc_spec_1st_output_on_terminal",  data_lc_spec_1st_output_on_terminal);
    form_bool(&s, "LAeq 2nd on terminal",                 "data_laeq_2nd_output_on_terminal",     data_laeq_2nd_output_on_terminal);
    form_bool(&s, "LA spectrum 2nd on terminal",          "data_la_spec_2nd_output_on_terminal",  data_la_spec_2nd_output_on_terminal);
    form_bool(&s, "LZeq 2nd on terminal",                 "data_lzeq_2nd_output_on_terminal",     data_lzeq_2nd_output_on_terminal);
    form_bool(&s, "LZ spectrum 2nd on terminal",          "data_lz_spec_2nd_output_on_terminal",  data_lz_spec_2nd_output_on_terminal);
    form_bool(&s, "LCeq 2nd on terminal",                 "data_lceq_2nd_output_on_terminal",     data_lceq_2nd_output_on_terminal);
    form_bool(&s, "LC spectrum 2nd on terminal",          "data_lc_spec_2nd_output_on_terminal",  data_lc_spec_2nd_output_on_terminal);
    form_bool(&s, "Print InfluxDB tx time",        "last_influxdb_transmission_time_to_terminal", last_influxdb_transmission_time_to_terminal);
    form_int( &s, "InfluxDB tx time threshold (ms)", "threshold_output_influxdb_ltt_to_terminal",   threshold_output_influxdb_ltt_to_terminal);
    form_bool(&s, "Print MQTT tx time",            "last_mqtt_transmission_time_to_terminal",     last_mqtt_transmission_time_to_terminal);
    form_int( &s, "MQTT tx time threshold (ms)",   "threshold_output_mqtt_ltt_to_terminal",       threshold_output_mqtt_ltt_to_terminal);
    form_bool(&s, "Print Sensor.Community tx time","last_sc_transmission_time_to_terminal",        last_sc_transmission_time_to_terminal);
    form_int( &s, "SC tx time threshold (ms)",     "threshold_output_sc_ltt_to_terminal",          threshold_output_sc_ltt_to_terminal);
    form_bool(&s, "Output thread info",                   "output_thread_info",                   output_thread_info);
    form_section_end(&s);

    /* ── Section 11: Data Logging ─────────────────── */
    form_section(&s, "logging", "Data Logging");
    form_bool(&s, "Enable data logging",   "data_logging",           data_logging);
    form_str( &s, "Logging directory",     "data_logging_directory", data_logging_directory);
    form_section_end(&s);

    /* ── Section 12: Webserver ────────────────────── */
    form_section(&s, "webserver", "Webserver");
    form_bool(&s, "Enable webserver (restart required)", "enable_webserver", enable_webserver);
    form_int( &s, "Webserver port",                       "webserver_port",  webserver_port);
    form_section_end(&s);

    /* ── Section 13: Environmental Sensors ──────── */
    form_section(&s, "sensors", "Environmental Sensors");
    form_bool(     &s, "Enable SDS011 (UART PM)",                    "enable_sds011",          enable_sds011);
    form_str(      &s, "SDS011 UART port",                           "sds011_uart_port",       sds011_uart_port ? sds011_uart_port : "/dev/ttyUSB0");
    form_bool(     &s, "Enable SPS30 (I\xc2\xb2""C PM)",            "enable_sps30",           enable_sps30);
    form_bool(     &s, "Enable SHT3x (I\xc2\xb2""C T/RH)",          "enable_sht3x",           enable_sht3x);
    form_i2c_addr( &s, "SHT3x I\xc2\xb2""C address",               "sht3x_i2c_addr",         sht3x_i2c_addr,
                        0x44, "0x44 (default, ADDR=GND)", 0x45, "0x45 (ADDR=VDD)");
    form_float(    &s, "SHT3x temperature correction (\xc2\xb0""C)", "temp_correction_sht3x",  temp_correction_sht3x,  4);
    form_bool(     &s, "Enable SEN5x (I\xc2\xb2""C PM+T+RH+VOC+NOx)", "enable_sen5x",         enable_sen5x);
    form_float(    &s, "SEN5x temperature correction (\xc2\xb0""C)", "temp_correction_sen5x",  temp_correction_sen5x,  4);
    form_bool(     &s, "Enable BME280 (I\xc2\xb2""C T/P/RH)",       "enable_bme280",          enable_bme280);
    form_i2c_addr( &s, "BME280 I\xc2\xb2""C address",              "bme280_i2c_addr",         bme280_i2c_addr,
                        0x76, "0x76 (default, SDO=GND)", 0x77, "0x77 (SDO=VDD)");
    form_float(    &s, "BME280 temperature correction (\xc2\xb0""C)", "temp_correction_bme280", temp_correction_bme280, 4);
    form_bool( &s, "Enable SCD30 (I\xc2\xb2""C CO\xc2\xb2"")", "enable_scd30",    enable_scd30);
    form_float(&s, "SCD30 temperature correction (\xc2\xb0""C)", "temp_correction_scd30", temp_correction_scd30, 4);
    form_bool( &s, "Enable SCD4x (I\xc2\xb2""C CO\xc2\xb2"")", "enable_scd4x",    enable_scd4x);
    form_float(&s, "SCD4x temperature correction (\xc2\xb0""C)", "temp_correction_scd4x", temp_correction_scd4x, 4);
    sb_printf(&s,
        "<tr><td colspan=\"2\"><small style=\"color:#888\">"
        "SPS30 and SEN5x share I\xc2\xb2""C address 0x69 &mdash; enable only one at a time. "
        "Sensors are read only when 1st interval &gt; 29&nbsp;s."
        "</small></td></tr>\n");
    form_section_end(&s);

    sb_printf(&s,
        "<p>"
        "<input type=\"submit\" class=\"btn-danger\" value=\"Save and restart\">"
        "</p>"
        "<p><small>After saving, the program will restart automatically (systemd).</small></p>"
        "</form>");

    page_end(&s);
    enum MHD_Result r = send_response(conn, MHD_HTTP_OK, "text/html", s.buf, s.len);
    sb_free(&s);
    return r;
}

/* Helper macro: set int config setting */
#define CFG_SET_INT(cfg, path, val) do { \
    config_setting_t *_st = config_lookup((cfg), (path)); \
    if (!_st) _st = config_setting_add(config_root_setting(cfg), (path), CONFIG_TYPE_INT); \
    if (_st) config_setting_set_int(_st, (val)); \
} while (0)

#define CFG_SET_BOOL(cfg, path, val) do { \
    config_setting_t *_st = config_lookup((cfg), (path)); \
    if (!_st) _st = config_setting_add(config_root_setting(cfg), (path), CONFIG_TYPE_BOOL); \
    if (_st) config_setting_set_bool(_st, (val)); \
} while (0)

#define CFG_SET_FLOAT(cfg, path, val) do { \
    config_setting_t *_st = config_lookup((cfg), (path)); \
    if (!_st) _st = config_setting_add(config_root_setting(cfg), (path), CONFIG_TYPE_FLOAT); \
    if (_st) config_setting_set_float(_st, (val)); \
} while (0)

#define CFG_SET_STR(cfg, path, val) do { \
    config_setting_t *_st = config_lookup((cfg), (path)); \
    if (!_st) _st = config_setting_add(config_root_setting(cfg), (path), CONFIG_TYPE_STRING); \
    if (_st && (val)) config_setting_set_string(_st, (val)); \
} while (0)

/* kvs_get with fallback empty string for numeric conversions */
static const char *kvs_get_or_empty(const KVS *k, const char *key) {
    const char *v = kvs_get(k, key);
    return v ? v : "";
}

/* Write cfg to path while preserving comments and blank lines from the original file.
 * Reads the original file line-by-line; for each key = value; line it substitutes
 * the current value from cfg.  Inline comments (after the semicolon) are kept.
 * Writes atomically via a .tmp file + rename.  Returns 0 on success, -1 on error. */
static int config_write_preserving_comments(config_t *cfg, const char *path)
{
    FILE *fin = fopen(path, "r");
    if (!fin) return -1;

    char tmppath[512];
    snprintf(tmppath, sizeof(tmppath), "%s.tmp", path);
    FILE *fout = fopen(tmppath, "w");
    if (!fout) { fclose(fin); return -1; }

    char seen[256][64];
    int  n_seen = 0;

    char line[1024];
    while (fgets(line, sizeof(line), fin)) {
        /* find first non-whitespace character */
        const char *p = line;
        while (*p == ' ' || *p == '\t') p++;

        /* blank or comment line — copy unchanged */
        if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0') {
            fputs(line, fout);
            continue;
        }

        /* must have '=' to be a key = value line */
        char *eq = strchr(line, '=');
        if (!eq) { fputs(line, fout); continue; }

        /* extract key: from first non-space to last non-space before '=' */
        const char *ke = eq;
        while (ke > p && (ke[-1] == ' ' || ke[-1] == '\t')) ke--;
        size_t klen = (size_t)(ke - p);
        char key[256];
        if (klen == 0 || klen >= sizeof(key)) { fputs(line, fout); continue; }
        memcpy(key, p, klen);
        key[klen] = '\0';

        /* look up in the updated config */
        config_setting_t *s = config_lookup(cfg, key);
        if (!s) { fputs(line, fout); continue; }

        /* capture optional inline comment after the ';' (e.g. "   # ICS-43434") */
        char inline_sfx[256] = "";
        char *semi = strchr(eq + 1, ';');
        if (semi) {
            const char *cp = semi + 1;
            size_t n = strlen(cp);
            while (n > 0 && (cp[n-1] == '\n' || cp[n-1] == '\r')) n--;
            if (n > 0 && n < sizeof(inline_sfx)) {
                memcpy(inline_sfx, cp, n);
                inline_sfx[n] = '\0';
            }
        }

        /* write leading whitespace */
        fwrite(line, 1, (size_t)(p - line), fout);

        /* write key = new_value; according to type */
        bool wrote = true;
        switch (config_setting_type(s)) {
            case CONFIG_TYPE_INT:
                fprintf(fout, "%s = %d;", key, config_setting_get_int(s));
                break;
            case CONFIG_TYPE_INT64:
                fprintf(fout, "%s = %lld;", key,
                        (long long)config_setting_get_int64(s));
                break;
            case CONFIG_TYPE_BOOL:
                fprintf(fout, "%s = %s;", key,
                        config_setting_get_bool(s) ? "true" : "false");
                break;
            case CONFIG_TYPE_FLOAT: {
                char fbuf[64];
                snprintf(fbuf, sizeof(fbuf), "%.13g",
                         config_setting_get_float(s));
                /* libconfig requires a decimal point to distinguish float from int */
                if (!strchr(fbuf, '.') && !strchr(fbuf, 'e') && !strchr(fbuf, 'E'))
                    strncat(fbuf, ".0", sizeof(fbuf) - strlen(fbuf) - 1);
                fprintf(fout, "%s = %s;", key, fbuf);
                break;
            }
            case CONFIG_TYPE_STRING: {
                const char *sv = config_setting_get_string(s);
                if (!sv) sv = "";
                fprintf(fout, "%s = \"", key);
                for (const char *c = sv; *c; c++) {
                    if (*c == '"' || *c == '\\') fputc('\\', fout);
                    else fputc(*c, fout);
                }
                fputs("\";", fout);
                break;
            }
            default:
                wrote = false;
                break;
        }

        if (wrote) {
            if (n_seen < (int)(sizeof(seen)/sizeof(seen[0]))) {
                strncpy(seen[n_seen], key, 63);
                seen[n_seen++][63] = '\0';
            }
            if (inline_sfx[0]) fputs(inline_sfx, fout);
            fputc('\n', fout);
        } else {
            /* unhandled type — reconstruct original line from what we have */
            fputs(p, fout);
        }
    }

    fclose(fin);

    /* Append any keys present in cfg but not found in the original file */
    config_setting_t *root = config_root_setting(cfg);
    int n_root = config_setting_length(root);
    for (int ni = 0; ni < n_root; ni++) {
        config_setting_t *ns = config_setting_get_elem(root, ni);
        if (!ns) continue;
        const char *nname = config_setting_name(ns);
        if (!nname) continue;
        bool already = false;
        for (int j = 0; j < n_seen; j++) {
            if (strcmp(seen[j], nname) == 0) { already = true; break; }
        }
        if (already) continue;
        switch (config_setting_type(ns)) {
            case CONFIG_TYPE_INT:
                fprintf(fout, "%s = %d;\n", nname, config_setting_get_int(ns));
                break;
            case CONFIG_TYPE_INT64:
                fprintf(fout, "%s = %lld;\n", nname,
                        (long long)config_setting_get_int64(ns));
                break;
            case CONFIG_TYPE_BOOL:
                fprintf(fout, "%s = %s;\n", nname,
                        config_setting_get_bool(ns) ? "true" : "false");
                break;
            case CONFIG_TYPE_FLOAT: {
                char fbuf[64];
                snprintf(fbuf, sizeof(fbuf), "%.13g",
                         config_setting_get_float(ns));
                if (!strchr(fbuf, '.') && !strchr(fbuf, 'e') && !strchr(fbuf, 'E'))
                    strncat(fbuf, ".0", sizeof(fbuf) - strlen(fbuf) - 1);
                fprintf(fout, "%s = %s;\n", nname, fbuf);
                break;
            }
            case CONFIG_TYPE_STRING: {
                const char *sv = config_setting_get_string(ns);
                if (!sv) sv = "";
                fprintf(fout, "%s = \"", nname);
                for (const char *c = sv; *c; c++) {
                    if (*c == '"' || *c == '\\') fputc('\\', fout);
                    else fputc(*c, fout);
                }
                fputs("\";\n", fout);
                break;
            }
            default: break;
        }
    }

    fclose(fout);

    if (rename(tmppath, path) != 0) {
        unlink(tmppath);
        return -1;
    }
    return 0;
}

static enum MHD_Result handle_config_post(struct MHD_Connection *conn, const char *body) {
    if (!auth_check(conn)) return send_auth_challenge(conn);

    KVS kv;
    kvs_init(&kv);
    kvs_parse_body(&kv, body);

    config_t tmpcfg;
    config_init(&tmpcfg);
    if (config_read_file(&tmpcfg, CONF_FILE) != CONFIG_TRUE) {
        fprintf(stderr, "webserver: failed to read %s: %s\n",
                CONF_FILE, config_error_text(&tmpcfg));
        config_destroy(&tmpcfg);
        kvs_free(&kv);
        SB s; sb_init(&s);
        page_start(&s, "Error", false);
        sb_printf(&s, "<p>Failed to read configuration file: %s</p>", CONF_FILE);
        page_end(&s);
        enum MHD_Result r = send_response(conn, MHD_HTTP_INTERNAL_SERVER_ERROR,
                                          "text/html", s.buf, s.len);
        sb_free(&s);
        return r;
    }

    /* ── System ─────────────────────────────────── */
    CFG_SET_BOOL(&tmpcfg, "date_format_iso",             kvs_has(&kv, "date_format_iso") ? 1 : 0);
    CFG_SET_INT(&tmpcfg, "waiting_time_after_start",    atoi(kvs_get_or_empty(&kv, "waiting_time_after_start")));
    CFG_SET_INT(&tmpcfg, "prio_1st_timer",               atoi(kvs_get_or_empty(&kv, "prio_1st_timer")));
    CFG_SET_INT(&tmpcfg, "prio_1st_measurement",         atoi(kvs_get_or_empty(&kv, "prio_1st_measurement")));
    CFG_SET_INT(&tmpcfg, "prio_2nd_timer",               atoi(kvs_get_or_empty(&kv, "prio_2nd_timer")));
    CFG_SET_INT(&tmpcfg, "prio_2nd_measurement",         atoi(kvs_get_or_empty(&kv, "prio_2nd_measurement")));
    CFG_SET_INT(&tmpcfg, "prio_wlan",                    atoi(kvs_get_or_empty(&kv, "prio_wlan")));
    CFG_SET_INT(&tmpcfg, "prio_print_ipc_and_log_1st",  atoi(kvs_get_or_empty(&kv, "prio_print_ipc_and_log_1st")));
    CFG_SET_INT(&tmpcfg, "prio_print_ipc_and_log_2nd",  atoi(kvs_get_or_empty(&kv, "prio_print_ipc_and_log_2nd")));
    CFG_SET_INT(&tmpcfg, "prio_start_stop",              atoi(kvs_get_or_empty(&kv, "prio_start_stop")));

    /* ── Hardware ───────────────────────────────── */
    CFG_SET_STR( &tmpcfg, "dev_name_dnms",              kvs_get(&kv, "dev_name_dnms"));
    CFG_SET_STR( &tmpcfg, "interface_name",             kvs_get(&kv, "interface_name"));
    CFG_SET_BOOL(&tmpcfg, "enable_wlan_or_lan",                    kvs_has(&kv, "enable_wlan_or_lan") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "enable_wifi_signal_strength_influxdb",  kvs_has(&kv, "enable_wifi_signal_strength_influxdb") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "enable_wifi_signal_strength_mqtt",      kvs_has(&kv, "enable_wifi_signal_strength_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "last_influxdb_transmission_time_to_payload", kvs_has(&kv, "last_influxdb_transmission_time_to_payload") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "last_mqtt_transmission_time_to_payload",     kvs_has(&kv, "last_mqtt_transmission_time_to_payload") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "long_id",                               kvs_has(&kv, "long_id") ? 1 : 0);

    /* ── Microphone ─────────────────────────────── */
    CFG_SET_INT(  &tmpcfg, "dnms_microphone",   atoi(kvs_get_or_empty(&kv, "dnms_microphone")));
    CFG_SET_FLOAT(&tmpcfg, "dnms_correction",   atof(kvs_get_or_empty(&kv, "dnms_correction")));
    CFG_SET_INT(  &tmpcfg, "dnms_blink_period_1", atoi(kvs_get_or_empty(&kv, "dnms_blink_period_1")));
    CFG_SET_INT(  &tmpcfg, "dnms_blink_period_2", atoi(kvs_get_or_empty(&kv, "dnms_blink_period_2")));
    CFG_SET_INT(  &tmpcfg, "dnms_blink_period_3", atoi(kvs_get_or_empty(&kv, "dnms_blink_period_3")));
    CFG_SET_INT(  &tmpcfg, "dnms_blink_period_4", atoi(kvs_get_or_empty(&kv, "dnms_blink_period_4")));
    CFG_SET_INT(  &tmpcfg, "dnms_blink_period_5", atoi(kvs_get_or_empty(&kv, "dnms_blink_period_5")));
    CFG_SET_INT(  &tmpcfg, "dnms_blink_period_6", atoi(kvs_get_or_empty(&kv, "dnms_blink_period_6")));
    CFG_SET_INT(  &tmpcfg, "dnms_blink_period_7", atoi(kvs_get_or_empty(&kv, "dnms_blink_period_7")));
    CFG_SET_FLOAT(&tmpcfg, "dnms_micro_const_1",  atof(kvs_get_or_empty(&kv, "dnms_micro_const_1")));
    CFG_SET_FLOAT(&tmpcfg, "dnms_micro_const_2",  atof(kvs_get_or_empty(&kv, "dnms_micro_const_2")));
    CFG_SET_FLOAT(&tmpcfg, "dnms_micro_const_3",  atof(kvs_get_or_empty(&kv, "dnms_micro_const_3")));
    CFG_SET_FLOAT(&tmpcfg, "dnms_micro_const_4",  atof(kvs_get_or_empty(&kv, "dnms_micro_const_4")));
    CFG_SET_FLOAT(&tmpcfg, "dnms_micro_const_5",  atof(kvs_get_or_empty(&kv, "dnms_micro_const_5")));
    CFG_SET_FLOAT(&tmpcfg, "dnms_micro_const_6",  atof(kvs_get_or_empty(&kv, "dnms_micro_const_6")));
    CFG_SET_FLOAT(&tmpcfg, "dnms_micro_const_7",  atof(kvs_get_or_empty(&kv, "dnms_micro_const_7")));

    /* ── Measurement Intervals ──────────────────── */
    CFG_SET_BOOL(&tmpcfg, "enable_1st_interval",                   kvs_has(&kv, "enable_1st_interval")   ? 1 : 0);
    CFG_SET_INT( &tmpcfg, "measurement_1st_interval_ms",           atoi(kvs_get_or_empty(&kv, "measurement_1st_interval_ms")));
    CFG_SET_BOOL(&tmpcfg, "enable_2nd_interval",                   kvs_has(&kv, "enable_2nd_interval")   ? 1 : 0);
    CFG_SET_INT( &tmpcfg, "measurement_2nd_interval_ms",           atoi(kvs_get_or_empty(&kv, "measurement_2nd_interval_ms")));
    CFG_SET_INT( &tmpcfg, "threshold_2nd_interval_laeq_transmit",  atoi(kvs_get_or_empty(&kv, "threshold_2nd_interval_laeq_transmit")));
    CFG_SET_INT( &tmpcfg, "number_transmissions_after_exceeding",   atoi(kvs_get_or_empty(&kv, "number_transmissions_after_exceeding")));
    CFG_SET_BOOL(&tmpcfg, "switch_output_pin",                     kvs_has(&kv, "switch_output_pin")     ? 1 : 0);
    CFG_SET_INT( &tmpcfg, "gpio_output_pin",                       atoi(kvs_get_or_empty(&kv, "gpio_output_pin")));
    CFG_SET_BOOL(&tmpcfg, "start_on_full_minute",                  kvs_has(&kv, "start_on_full_minute")  ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "start_on_full_hour",                    kvs_has(&kv, "start_on_full_hour")    ? 1 : 0);

    /* ── InfluxDB - Transmit Data ─────────────── */
    CFG_SET_BOOL(&tmpcfg, "data_transmit_laeq_1st_to_influxdb",          kvs_has(&kv, "data_transmit_laeq_1st_to_influxdb")          ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lzeq_1st_to_influxdb",          kvs_has(&kv, "data_transmit_lzeq_1st_to_influxdb")          ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lceq_1st_to_influxdb",          kvs_has(&kv, "data_transmit_lceq_1st_to_influxdb")          ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_laeq_1st_spectrum_to_influxdb", kvs_has(&kv, "data_transmit_laeq_1st_spectrum_to_influxdb") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lzeq_1st_spectrum_to_influxdb", kvs_has(&kv, "data_transmit_lzeq_1st_spectrum_to_influxdb") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lceq_1st_spectrum_to_influxdb", kvs_has(&kv, "data_transmit_lceq_1st_spectrum_to_influxdb") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_laeq_2nd_to_influxdb",          kvs_has(&kv, "data_transmit_laeq_2nd_to_influxdb")          ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lzeq_2nd_to_influxdb",          kvs_has(&kv, "data_transmit_lzeq_2nd_to_influxdb")          ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lceq_2nd_to_influxdb",          kvs_has(&kv, "data_transmit_lceq_2nd_to_influxdb")          ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_laeq_2nd_spectrum_to_influxdb", kvs_has(&kv, "data_transmit_laeq_2nd_spectrum_to_influxdb") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lzeq_2nd_spectrum_to_influxdb", kvs_has(&kv, "data_transmit_lzeq_2nd_spectrum_to_influxdb") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lceq_2nd_spectrum_to_influxdb", kvs_has(&kv, "data_transmit_lceq_2nd_spectrum_to_influxdb") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_laeq_1st_to_mqtt",          kvs_has(&kv, "data_transmit_laeq_1st_to_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lzeq_1st_to_mqtt",          kvs_has(&kv, "data_transmit_lzeq_1st_to_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lceq_1st_to_mqtt",          kvs_has(&kv, "data_transmit_lceq_1st_to_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_laeq_1st_spectrum_to_mqtt", kvs_has(&kv, "data_transmit_laeq_1st_spectrum_to_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lzeq_1st_spectrum_to_mqtt", kvs_has(&kv, "data_transmit_lzeq_1st_spectrum_to_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lceq_1st_spectrum_to_mqtt", kvs_has(&kv, "data_transmit_lceq_1st_spectrum_to_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_laeq_2nd_to_mqtt",          kvs_has(&kv, "data_transmit_laeq_2nd_to_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lzeq_2nd_to_mqtt",          kvs_has(&kv, "data_transmit_lzeq_2nd_to_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lceq_2nd_to_mqtt",          kvs_has(&kv, "data_transmit_lceq_2nd_to_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_laeq_2nd_spectrum_to_mqtt", kvs_has(&kv, "data_transmit_laeq_2nd_spectrum_to_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lzeq_2nd_spectrum_to_mqtt", kvs_has(&kv, "data_transmit_lzeq_2nd_spectrum_to_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_lceq_2nd_spectrum_to_mqtt", kvs_has(&kv, "data_transmit_lceq_2nd_spectrum_to_mqtt") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "timestamp_to_influxdb",                        kvs_has(&kv, "timestamp_to_influxdb")                        ? 1 : 0);

    /* ── InfluxDB Connection ─────────────────────── */
    CFG_SET_BOOL(&tmpcfg, "influxdb_transmit_http",  kvs_has(&kv, "influxdb_transmit_http")  ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "influxdb_transmit_https", kvs_has(&kv, "influxdb_transmit_https") ? 1 : 0);
    CFG_SET_STR( &tmpcfg, "influxdb_server",    kvs_get(&kv, "influxdb_server"));
    CFG_SET_STR( &tmpcfg, "influxdb_port",      kvs_get(&kv, "influxdb_port"));
    CFG_SET_STR( &tmpcfg, "influxdb_pfad",      kvs_get(&kv, "influxdb_pfad"));
    CFG_SET_STR( &tmpcfg, "influxdb_user",      kvs_get(&kv, "influxdb_user"));
    CFG_SET_STR( &tmpcfg, "influxdb_passwort",  kvs_get(&kv, "influxdb_passwort"));
    CFG_SET_STR( &tmpcfg, "influxdb_messung",   kvs_get(&kv, "influxdb_messung"));

    /* ── MQTT Connection ────────────────────────── */
    CFG_SET_BOOL(&tmpcfg, "mqtt_transmit",            kvs_has(&kv, "mqtt_transmit")           ? 1 : 0);
    CFG_SET_STR( &tmpcfg, "mqtt_broker",              kvs_get(&kv, "mqtt_broker"));
    CFG_SET_INT( &tmpcfg, "mqtt_port",                atoi(kvs_get_or_empty(&kv, "mqtt_port")));
    CFG_SET_STR( &tmpcfg, "mqtt_user",                kvs_get(&kv, "mqtt_user"));
    CFG_SET_STR( &tmpcfg, "mqtt_passwort",            kvs_get(&kv, "mqtt_passwort"));
    CFG_SET_INT( &tmpcfg, "mqtt_keepalive",           atoi(kvs_get_or_empty(&kv, "mqtt_keepalive")));
    CFG_SET_INT( &tmpcfg, "mqtt_qos",                 atoi(kvs_get_or_empty(&kv, "mqtt_qos")));
    CFG_SET_STR( &tmpcfg, "mqtt_main_topic",          kvs_get(&kv, "mqtt_main_topic"));
    CFG_SET_BOOL(&tmpcfg, "mqtt_use_id_as_sub_topic", kvs_has(&kv, "mqtt_use_id_as_sub_topic") ? 1 : 0);
    CFG_SET_STR( &tmpcfg, "mqtt_messung",             kvs_get(&kv, "mqtt_messung"));
    CFG_SET_BOOL(&tmpcfg, "mqtt_use_tls",             kvs_has(&kv, "mqtt_use_tls")             ? 1 : 0);
    CFG_SET_STR( &tmpcfg, "mqtt_tls_cafile",          kvs_get(&kv, "mqtt_tls_cafile"));
    CFG_SET_BOOL(&tmpcfg, "mqtt_tls_insecure",        kvs_has(&kv, "mqtt_tls_insecure")        ? 1 : 0);

    /* ── Sensor.Community ────────────────────────── */
    CFG_SET_BOOL(&tmpcfg, "data_transmit_laeq_to_sc", kvs_has(&kv, "data_transmit_laeq_to_sc") ? 1 : 0);
    CFG_SET_STR( &tmpcfg, "host_sc",                  kvs_get(&kv, "host_sc"));
    CFG_SET_STR( &tmpcfg, "url_sc",                   kvs_get(&kv, "url_sc"));
    CFG_SET_STR( &tmpcfg, "DNMS_API_PIN",             kvs_get(&kv, "DNMS_API_PIN"));
    CFG_SET_BOOL(&tmpcfg, "sc_transmit_https",              kvs_has(&kv, "sc_transmit_https") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_transmit_laeq_to_madavi",   kvs_has(&kv, "data_transmit_laeq_to_madavi") ? 1 : 0);

    /* ── Custom API ─────────────────────────────────── */
    CFG_SET_BOOL(&tmpcfg, "custom_api_enable",       kvs_has(&kv, "custom_api_enable")       ? 1 : 0);
    CFG_SET_STR( &tmpcfg, "custom_api_server",       kvs_get(&kv, "custom_api_server"));
    CFG_SET_STR( &tmpcfg, "custom_api_port",         kvs_get(&kv, "custom_api_port"));
    CFG_SET_STR( &tmpcfg, "custom_api_path",         kvs_get(&kv, "custom_api_path"));
    CFG_SET_BOOL(&tmpcfg, "custom_api_https",        kvs_has(&kv, "custom_api_https")        ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "custom_api_send_1st",     kvs_has(&kv, "custom_api_send_1st")     ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "custom_api_spectrum_1st", kvs_has(&kv, "custom_api_spectrum_1st") ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "custom_api_send_2nd",     kvs_has(&kv, "custom_api_send_2nd")     ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "custom_api_spectrum_2nd", kvs_has(&kv, "custom_api_spectrum_2nd") ? 1 : 0);

    /* ── IPC ─────────────────────────────────────── */
    CFG_SET_BOOL(&tmpcfg, "data_transmit_via_pipe",  kvs_has(&kv, "data_transmit_via_pipe")  ? 1 : 0);
    CFG_SET_STR( &tmpcfg, "name_of_pipe",            kvs_get(&kv, "name_of_pipe"));
    CFG_SET_BOOL(&tmpcfg, "start_stop_extern",        kvs_has(&kv, "start_stop_extern")       ? 1 : 0);
    CFG_SET_STR( &tmpcfg, "start_stop_name_of_pipe", kvs_get(&kv, "start_stop_name_of_pipe"));

    /* ── Terminal / Log ──────────────────────────── */
    CFG_SET_BOOL(&tmpcfg, "data_on_terminal",                         kvs_has(&kv, "data_on_terminal")                         ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_laeq_1st_output_on_terminal",         kvs_has(&kv, "data_laeq_1st_output_on_terminal")         ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_la_spec_1st_output_on_terminal",      kvs_has(&kv, "data_la_spec_1st_output_on_terminal")      ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_lzeq_1st_output_on_terminal",         kvs_has(&kv, "data_lzeq_1st_output_on_terminal")         ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_lz_spec_1st_output_on_terminal",      kvs_has(&kv, "data_lz_spec_1st_output_on_terminal")      ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_lceq_1st_output_on_terminal",         kvs_has(&kv, "data_lceq_1st_output_on_terminal")         ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_lc_spec_1st_output_on_terminal",      kvs_has(&kv, "data_lc_spec_1st_output_on_terminal")      ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_laeq_2nd_output_on_terminal",         kvs_has(&kv, "data_laeq_2nd_output_on_terminal")         ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_la_spec_2nd_output_on_terminal",      kvs_has(&kv, "data_la_spec_2nd_output_on_terminal")      ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_lzeq_2nd_output_on_terminal",         kvs_has(&kv, "data_lzeq_2nd_output_on_terminal")         ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_lz_spec_2nd_output_on_terminal",      kvs_has(&kv, "data_lz_spec_2nd_output_on_terminal")      ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_lceq_2nd_output_on_terminal",         kvs_has(&kv, "data_lceq_2nd_output_on_terminal")         ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "data_lc_spec_2nd_output_on_terminal",      kvs_has(&kv, "data_lc_spec_2nd_output_on_terminal")      ? 1 : 0);
    CFG_SET_BOOL(&tmpcfg, "last_influxdb_transmission_time_to_terminal", kvs_has(&kv, "last_influxdb_transmission_time_to_terminal") ? 1 : 0);
    CFG_SET_INT( &tmpcfg, "threshold_output_influxdb_ltt_to_terminal",   atoi(kvs_get_or_empty(&kv, "threshold_output_influxdb_ltt_to_terminal")));
    CFG_SET_BOOL(&tmpcfg, "last_mqtt_transmission_time_to_terminal",     kvs_has(&kv, "last_mqtt_transmission_time_to_terminal")     ? 1 : 0);
    CFG_SET_INT( &tmpcfg, "threshold_output_mqtt_ltt_to_terminal",       atoi(kvs_get_or_empty(&kv, "threshold_output_mqtt_ltt_to_terminal")));
    CFG_SET_BOOL(&tmpcfg, "last_sc_transmission_time_to_terminal",       kvs_has(&kv, "last_sc_transmission_time_to_terminal")       ? 1 : 0);
    CFG_SET_INT( &tmpcfg, "threshold_output_sc_ltt_to_terminal",         atoi(kvs_get_or_empty(&kv, "threshold_output_sc_ltt_to_terminal")));
    CFG_SET_BOOL(&tmpcfg, "output_thread_info",                          kvs_has(&kv, "output_thread_info")                          ? 1 : 0);

    /* ── Data Logging ────────────────────────────── */
    CFG_SET_BOOL(&tmpcfg, "data_logging",           kvs_has(&kv, "data_logging") ? 1 : 0);
    CFG_SET_STR( &tmpcfg, "data_logging_directory", kvs_get(&kv, "data_logging_directory"));

    /* ── Webserver ───────────────────────────────── */
    CFG_SET_BOOL(&tmpcfg, "enable_webserver", kvs_has(&kv, "enable_webserver") ? 1 : 0);
    CFG_SET_INT( &tmpcfg, "webserver_port",   atoi(kvs_get_or_empty(&kv, "webserver_port")));

    /* ── Environmental Sensors ───────────────────── */
    CFG_SET_BOOL(&tmpcfg, "enable_sds011",  kvs_has(&kv, "enable_sds011")  ? 1 : 0);
    CFG_SET_STR( &tmpcfg, "sds011_uart_port", kvs_get(&kv, "sds011_uart_port"));
    CFG_SET_BOOL(&tmpcfg, "enable_sps30",   kvs_has(&kv, "enable_sps30")   ? 1 : 0);
    CFG_SET_BOOL( &tmpcfg, "enable_sht3x",         kvs_has(&kv, "enable_sht3x")  ? 1 : 0);
    CFG_SET_INT(  &tmpcfg, "sht3x_i2c_addr",       atoi(kvs_get_or_empty(&kv, "sht3x_i2c_addr")));
    CFG_SET_FLOAT(&tmpcfg, "temp_correction_sht3x", atof(kvs_get_or_empty(&kv, "temp_correction_sht3x")));
    CFG_SET_BOOL( &tmpcfg, "enable_sen5x",         kvs_has(&kv, "enable_sen5x")  ? 1 : 0);
    CFG_SET_FLOAT(&tmpcfg, "temp_correction_sen5x", atof(kvs_get_or_empty(&kv, "temp_correction_sen5x")));
    CFG_SET_BOOL( &tmpcfg, "enable_bme280",         kvs_has(&kv, "enable_bme280") ? 1 : 0);
    CFG_SET_INT(  &tmpcfg, "bme280_i2c_addr",      atoi(kvs_get_or_empty(&kv, "bme280_i2c_addr")));
    CFG_SET_FLOAT(&tmpcfg, "temp_correction_bme280", atof(kvs_get_or_empty(&kv, "temp_correction_bme280")));
    CFG_SET_BOOL( &tmpcfg, "enable_scd30",            kvs_has(&kv, "enable_scd30")  ? 1 : 0);
    CFG_SET_FLOAT(&tmpcfg, "temp_correction_scd30",    atof(kvs_get_or_empty(&kv, "temp_correction_scd30")));
    CFG_SET_BOOL( &tmpcfg, "enable_scd4x",            kvs_has(&kv, "enable_scd4x")  ? 1 : 0);
    CFG_SET_FLOAT(&tmpcfg, "temp_correction_scd4x",    atof(kvs_get_or_empty(&kv, "temp_correction_scd4x")));

    /* write file, preserving comments from the original */
    if (config_write_preserving_comments(&tmpcfg, CONF_FILE) != 0) {
        fprintf(stderr, "webserver: failed to write %s\n", CONF_FILE);
    }
    config_destroy(&tmpcfg);
    kvs_free(&kv);

    SB s;
    sb_init(&s);
    page_start(&s, "Configuration Saved", false);
    sb_printf(&s,
        "<p>Configuration saved. Program is restarting...</p>"
        "<p><a class=\"btn\" href=\"/\">Back to Home</a></p>");
    page_end(&s);
    enum MHD_Result r = send_response(conn, MHD_HTTP_OK, "text/html", s.buf, s.len);
    sb_free(&s);

    schedule_restart();
    return r;
}

static enum MHD_Result handle_reset_get(struct MHD_Connection *conn) {
    if (!auth_check(conn)) return send_auth_challenge(conn);

    SB s;
    sb_init(&s);
    page_start(&s, "Restart", false);
    sb_printf(&s,
        "<p>Are you sure you want to restart the program?</p>"
        "<form method=\"post\" action=\"/reset\">"
        "<input type=\"submit\" class=\"btn btn-danger\" value=\"Yes, restart now\">"
        "</form>"
        "<p><a class=\"btn\" href=\"/\">Cancel</a></p>");
    page_end(&s);
    enum MHD_Result r = send_response(conn, MHD_HTTP_OK, "text/html", s.buf, s.len);
    sb_free(&s);
    return r;
}

static enum MHD_Result handle_reset_post(struct MHD_Connection *conn) {
    if (!auth_check(conn)) return send_auth_challenge(conn);

    SB s;
    sb_init(&s);
    page_start(&s, "Restarting", false);
    sb_printf(&s, "<p>Restarting program...</p>");
    page_end(&s);
    enum MHD_Result r = send_response(conn, MHD_HTTP_OK, "text/html", s.buf, s.len);
    sb_free(&s);

    schedule_restart();
    return r;
}


/* ═══════════════════════════════════════════════════════════════════════════
 * MAIN REQUEST HANDLER
 * ═══════════════════════════════════════════════════════════════════════════ */

static void completed_callback(void *cls,
                               struct MHD_Connection *conn,
                               void **con_cls,
                               enum MHD_RequestTerminationCode toe) {
    (void)cls; (void)conn; (void)toe;
    if (*con_cls) {
        post_state_free((PostState *)*con_cls);
        *con_cls = NULL;
    }
}

/* ── GET /data.json — airrohr-compatible JSON snapshot ───────────────────── */
static enum MHD_Result handle_data_json(struct MHD_Connection *conn) {
    char *json = build_data_json(1, 1, 1);
    if (!json) {
        const char *err = "{\"error\":\"out of memory\"}";
        return send_response(conn, MHD_HTTP_INTERNAL_SERVER_ERROR,
                             "application/json", err, strlen(err));
    }
    struct MHD_Response *resp = MHD_create_response_from_buffer(
        strlen(json), json, MHD_RESPMEM_MUST_FREE);
    if (!resp) { free(json); return MHD_NO; }
    MHD_add_response_header(resp, "Content-Type", "application/json");
    MHD_add_response_header(resp, "Cache-Control", "no-cache");
    enum MHD_Result r = MHD_queue_response(conn, MHD_HTTP_OK, resp);
    MHD_destroy_response(resp);
    return r;
}

static enum MHD_Result request_handler(void *cls,
                                       struct MHD_Connection *conn,
                                       const char *url,
                                       const char *method,
                                       const char *version,
                                       const char *upload_data,
                                       size_t *upload_data_size,
                                       void **con_cls) {
    (void)cls; (void)version;

    bool is_get  = (strcmp(method, "GET")  == 0);
    bool is_post = (strcmp(method, "POST") == 0);

    /* ── POST body accumulation ───────────────────── */
    if (is_post) {
        if (*con_cls == NULL) {
            /* first call: allocate state */
            PostState *ps = post_state_new();
            if (!ps) return MHD_NO;
            *con_cls = ps;
            return MHD_YES;
        }
        PostState *ps = (PostState *)*con_cls;
        if (*upload_data_size > 0) {
            post_state_append(ps, upload_data, *upload_data_size);
            *upload_data_size = 0;
            return MHD_YES;
        }
        /* body complete — dispatch */
        const char *body = ps->body ? ps->body : "";
        if (strcmp(url, "/config") == 0)
            return handle_config_post(conn, body);
        if (strcmp(url, "/reset") == 0)
            return handle_reset_post(conn);
        /* unknown POST */
        return send_response(conn, MHD_HTTP_NOT_FOUND,
                             "text/plain", "Not Found", 9);
    }

    /* ── GET routes ───────────────────────────────── */
    if (is_get) {
        if (strcmp(url, "/favicon.ico") == 0) {
            struct MHD_Response *resp = MHD_create_response_from_buffer(
                FAVICON_PNG_SIZE, (void *)FAVICON_PNG, MHD_RESPMEM_PERSISTENT);
            if (!resp) return MHD_NO;
            MHD_add_response_header(resp, "Content-Type", "image/png");
            MHD_add_response_header(resp, "Cache-Control", "max-age=86400");
            enum MHD_Result r = MHD_queue_response(conn, MHD_HTTP_OK, resp);
            MHD_destroy_response(resp);
            return r;
        }
        if (strcmp(url, "/logo.png") == 0) {
            struct MHD_Response *resp = MHD_create_response_from_buffer(
                LOGO_PNG_SIZE, (void *)LOGO_PNG, MHD_RESPMEM_PERSISTENT);
            if (!resp) return MHD_NO;
            MHD_add_response_header(resp, "Content-Type", "image/png");
            MHD_add_response_header(resp, "Cache-Control", "max-age=86400");
            enum MHD_Result r = MHD_queue_response(conn, MHD_HTTP_OK, resp);
            MHD_destroy_response(resp);
            return r;
        }
        if (strcmp(url, "/") == 0)
            return handle_root(conn);
        if (strcmp(url, "/values") == 0)
            return handle_values(conn);
        if (strcmp(url, "/status") == 0)
            return handle_status(conn);
        if (strcmp(url, "/data.json") == 0)
            return handle_data_json(conn);
        if (strcmp(url, "/config") == 0)
            return handle_config_get(conn);
        if (strcmp(url, "/reset") == 0)
            return handle_reset_get(conn);
    }

    /* 404 */
    SB s; sb_init(&s);
    page_start(&s, "Not Found", false);
    sb_printf(&s, "<p>Page not found: ");
    html_escape(&s, url);
    sb_printf(&s, "</p><p><a class=\"btn\" href=\"/\">Home</a></p>");
    page_end(&s);
    enum MHD_Result r = send_response(conn, MHD_HTTP_NOT_FOUND,
                                      "text/html", s.buf, s.len);
    sb_free(&s);
    return r;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * PUBLIC API
 * ═══════════════════════════════════════════════════════════════════════════ */

static struct MHD_Daemon *mhd_daemon = NULL;

void webserver_init(void) {
    mhd_daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_THREAD_PER_CONNECTION,
        (uint16_t)webserver_port,
        NULL, NULL,
        &request_handler, NULL,
        MHD_OPTION_NOTIFY_COMPLETED, &completed_callback, NULL,
        MHD_OPTION_END);
    if (!mhd_daemon) {
        fprintf(stderr, "webserver: failed to start on port %d\n", webserver_port);
        return;
    }
    printf("webserver: listening on 0.0.0.0:%d\n", webserver_port);
}

void webserver_stop(void) {
    if (mhd_daemon) {
        MHD_stop_daemon(mhd_daemon);
        mhd_daemon = NULL;
    }
}
