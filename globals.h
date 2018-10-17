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

typedef int nanoseconds_t; /* maximum value is over 2s, which is fine */

struct encoding_buffer
{
    int pulselength_ns;
    int length_pulses;
    bool* bitmap;
};

extern struct encoding_buffer* create_encoding_buffer(int pulselength_ns, int length_pulses);
extern void free_encoding_buffer(struct encoding_buffer* buffer);

extern void encoding_buffer_pulse(struct encoding_buffer* buffer, int timestamp_ns);
extern struct fluxmap* encoding_buffer_encode(const struct encoding_buffer* buffer);

struct fluxmap
{
    int length_ticks;
    int length_us;
    int bytes;
    int buffersize;
    uint8_t* intervals;
};

extern struct fluxmap* create_fluxmap(void);
extern void free_fluxmap(struct fluxmap* fluxmap);
extern struct fluxmap* copy_fluxmap(const struct fluxmap* fluxmap);
extern void fluxmap_clear(struct fluxmap* fluxmap);
extern void fluxmap_append_intervals(struct fluxmap* fluxmap, const uint8_t* intervals, int count);
extern void fluxmap_append_interval(struct fluxmap* fluxmap, uint8_t interval);
extern int fluxmap_seek_clock(const struct fluxmap* fluxmap, int* cursor, int pulses);
extern nanoseconds_t fluxmap_guess_clock(const struct fluxmap* fluxmap);
extern void fluxmap_precompensate(struct fluxmap* fluxmap, int threshold_ticks, int amount_ticks);
extern struct encoding_buffer* fluxmap_decode(const struct fluxmap* fluxmap, int clock_ns);

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
extern struct fluxmap* usb_read(int side);
extern void usb_write(int side, struct fluxmap* fluxmap);

extern void cmd_rpm(char* const* argv);
extern void cmd_usbbench(char* const* argv);
extern void cmd_read(char* const* argv);
extern void cmd_write(char* const* argv);
extern void cmd_mfmdecode(char* const* argv);
extern void cmd_testpattern(char* const* argv);
extern void cmd_fluxdump(char* const* argv);
extern void cmd_calibrate(char* const* argv);
extern void cmd_getclock(char* const* argv);

#endif
