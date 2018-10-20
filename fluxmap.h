#ifndef FLUXMAP_H
#define FLUXMAP_H

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

#endif
