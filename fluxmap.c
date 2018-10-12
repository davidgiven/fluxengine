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

int fluxmap_seek_clock(const struct fluxmap* fluxmap, int* cursor, int pulses)
{
    int count = 0;
    int value = 0;

    while (*cursor < fluxmap->bytes)
    {
        uint8_t t = fluxmap->intervals[(*cursor)++];
        if (value == 0)
            value = t;
        else
        {
            count++;
            int delta = abs(value - t) / 3 + 1;
            if (delta > (value / 8))
                count = value = 0;
            else
            {
                if (t < value)
                    value -= delta;
                else
                    value += delta;
            }
        }

        if (count >= pulses)
            return value;
    }

    return 1;
}
