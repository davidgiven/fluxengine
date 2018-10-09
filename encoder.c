#include "globals.h"

#define MAX_INTERVAL_US (128/(TICK_FREQUENCY/1000000))

struct encoding_buffer* create_encoding_buffer(int length_us)
{
    struct encoding_buffer* buffer = calloc(1, sizeof(*buffer));
    buffer->length_us = length_us;
    buffer->bitmap = calloc(1, length_us);
    return buffer;
}

void free_encoding_buffer(struct encoding_buffer* buffer)
{
    free(buffer->bitmap);
    free(buffer);
}

void encoding_buffer_pulse(struct encoding_buffer* buffer, int timestamp_us)
{
    if (timestamp_us < buffer->length_us)
        buffer->bitmap[timestamp_us] = 1;
}

struct fluxmap* encoding_buffer_encode(const struct encoding_buffer* buffer)
{
    struct fluxmap* fluxmap = create_fluxmap();

    int lastpulse = 0;
    int cursor = 1;
    while (cursor < buffer->length_us)
    {
        if ((buffer->bitmap[cursor]) || (cursor-lastpulse == MAX_INTERVAL_US))
        {
            uint8_t interval = (cursor - lastpulse) * (TICK_FREQUENCY/1000000);
            fluxmap_append_intervals(fluxmap, &interval, 1);
            lastpulse = cursor;
        }
        cursor++;
    }

    return fluxmap;
}
