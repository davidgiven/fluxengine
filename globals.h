#ifndef GLOBALS_H
#define GLOBALS_H

#if defined _WIN32 || defined __CYGWIN__
    #define WINDOWS
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "protocol.h"

extern void error(const char* message, ...);

extern void usb_init(void);
extern void usb_cmd_send(void* ptr, int len);
extern void usb_cmd_recv(void* ptr, int len);

extern int usb_get_version(void);
extern void usb_seek(int track);
extern int usb_measure_speed(void);

#endif
