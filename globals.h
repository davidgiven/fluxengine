#ifndef GLOBALS_H
#define GLOBALS_H

#define _BSD_SOURCE

#if defined _WIN32 || defined __CYGWIN__
    #define WINDOWS
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <endian.h>

#include "protocol.h"

struct raw_data_buffer
{
    size_t len;
    uint8_t buffer[200*1024];
};

extern void error(const char* message, ...);
extern double gettime(void);
extern int countargs(char* const* argv);

extern void usb_init(void);
extern void usb_cmd_send(void* ptr, int len);
extern void usb_cmd_recv(void* ptr, int len);

extern int usb_get_version(void);
extern void usb_seek(int track);
extern int usb_measure_speed(void);
extern void usb_bulk_test(void);
extern void usb_read(int side, struct raw_data_buffer* buffer);
extern int usb_write(int side, struct raw_data_buffer* buffer);

extern void cmd_rpm(char* const* argv);
extern void cmd_usbbench(char* const* argv);
extern void cmd_read(char* const* argv);
extern void cmd_write(char* const* argv);
extern void cmd_decode(char* const* argv);

#endif
