#include "globals.h"

struct fluxmap* create_fluxmap(void)
{
    struct fluxmap* fluxmap = calloc(1, sizeof(*fluxmap));
    return fluxmap;
}

void free_fluxmap(struct fluxmap* fluxmap)
{
    free(fluxmap->intervals);
    free(fluxmap);
}

void fluxmap_clear(struct fluxmap* fluxmap)
{
    fluxmap->bytes = fluxmap->length_ticks = fluxmap->length_us = 0;
}

void fluxmap_append_intervals(struct fluxmap* fluxmap, const uint8_t* intervals, int count)
{
    int newsize = fluxmap->bytes + count;
    if (newsize > fluxmap->buffersize)
    {
        fluxmap->buffersize *= 2;
        if (newsize > fluxmap->buffersize)
            fluxmap->buffersize = newsize;
        fluxmap->intervals = realloc(fluxmap->intervals, fluxmap->buffersize);
    }

    for (int i=0; i<count; i++)
    {
        uint8_t interval = *intervals++;
        fluxmap->length_ticks += interval;
        fluxmap->intervals[fluxmap->bytes++] = interval;
    }

    fluxmap->length_us = fluxmap->length_ticks / (TICK_FREQUENCY / 1000000);
}

struct encoding_buffer* fluxmap_decode(const struct fluxmap* fluxmap)
{
    struct encoding_buffer* buffer = create_encoding_buffer(fluxmap->length_us);

    int total_us = 0;
    for (int cursor = 0; cursor < fluxmap->bytes; cursor++)
    {
        uint8_t interval = fluxmap->intervals[cursor];
        int us = (interval + TICKS_PER_US/2) / TICKS_PER_US;
        total_us += us;
        encoding_buffer_pulse(buffer, total_us);
    }

    return buffer;
}
