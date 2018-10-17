#include "globals.h"

struct fluxmap* create_fluxmap(void)
{
    struct fluxmap* fluxmap = calloc(1, sizeof(*fluxmap));
    return fluxmap;
}

void free_fluxmap(struct fluxmap* fluxmap)
{
    if (fluxmap)
    {
        free(fluxmap->intervals);
        free(fluxmap);
    }
}

struct fluxmap* copy_fluxmap(const struct fluxmap* src)
{
    struct fluxmap* copy = create_fluxmap();
    fluxmap_append_intervals(copy, src->intervals, src->bytes);
    return copy;
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
        fluxmap->length_ticks += interval ? interval : 0x100;
        fluxmap->intervals[fluxmap->bytes++] = interval;
    }

    fluxmap->length_us = fluxmap->length_ticks / (TICK_FREQUENCY / 1000000);
}

void fluxmap_append_interval(struct fluxmap* fluxmap, uint8_t interval)
{
    fluxmap_append_intervals(fluxmap, &interval, 1);
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

/* 
 * Tries to guess the clock by finding the smallest common interval.
 * Returns nanoseconds.
 */
nanoseconds_t fluxmap_guess_clock(const struct fluxmap* fluxmap)
{
    uint32_t buckets[256] = {};
    for (int i=0; i<fluxmap->bytes; i++)
        buckets[fluxmap->intervals[i]]++;
    
    int peaklo = 0;
    while (peaklo < 256)
    {
        if (buckets[peaklo] > 100)
            break;
        peaklo++;
    }

    int peakmaxindex = peaklo;
    int peakmaxvalue = buckets[peakmaxindex];
    int peakhi = peaklo;
    while (peakhi < 256)
    {
        uint32_t v = buckets[peakhi];
        if (buckets[peakhi] < 50)
            break;
        if (v > peakmaxvalue)
        {
            peakmaxindex = peakhi;
            peakmaxvalue = v;
        }
        peakhi++;
    }

    /* 
     * Okay, peakmaxindex should now be a good candidate for the (or a) clock.
     * How this maps onto the actual clock rate depends on the encoding.
     */

    return peakmaxindex * NS_PER_TICK;
}

void fluxmap_precompensate(struct fluxmap* fluxmap, int threshold_ticks, int amount_ticks)
{
    uint8_t junk = 0xff;

    for (int i=0; i<fluxmap->bytes; i++)
    {
        uint8_t* prev = (i == 0) ? &junk : &fluxmap->intervals[i-1];
        uint8_t* curr = &fluxmap->intervals[i];

        if ((*prev <= threshold_ticks) && (*curr > threshold_ticks))
        {
            /* 01001; move the previous bit backwards. */
            *prev -= amount_ticks;
            *curr += amount_ticks;
        }
        else if ((*prev > threshold_ticks) && (*curr <= threshold_ticks))
        {
            /* 00101; move the current bit forwards. */
            *prev += amount_ticks;
            *curr -= amount_ticks;
        }
    }
}

struct encoding_buffer* fluxmap_decode(const struct fluxmap* fluxmap, nanoseconds_t clock_period)
{
    int pulses = (fluxmap->length_us*1000) / clock_period;
    nanoseconds_t lower_threshold = clock_period * 0.75;

    struct encoding_buffer* buffer = create_encoding_buffer(clock_period, pulses);
    int count = 0;
    int cursor = 0;
    nanoseconds_t timestamp = 0;
    for (;;)
    {
        while (timestamp < lower_threshold)
        {
            if (cursor >= fluxmap->bytes)
                goto abort;
            uint8_t interval = fluxmap->intervals[cursor++];
            timestamp += interval * NS_PER_TICK;
        }

        int clocks = (timestamp + clock_period/2) / clock_period;
        count += clocks;
        if (count >= buffer->length_pulses)
            goto abort;
        buffer->bitmap[count] = true;
        timestamp = 0;
    }
abort:

    return buffer;
}
