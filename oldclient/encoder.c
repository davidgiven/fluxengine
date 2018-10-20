#include "globals.h"

#define MAX_INTERVAL_US (128/(TICK_FREQUENCY/1000000))

struct encoding_buffer* create_encoding_buffer(int pulselength_ns, int length_pulses)
{
    struct encoding_buffer* buffer = calloc(1, sizeof(*buffer));
    buffer->pulselength_ns = pulselength_ns;
    buffer->length_pulses = length_pulses;
    buffer->bitmap = calloc(1, length_pulses);
    return buffer;
}

void free_encoding_buffer(struct encoding_buffer* buffer)
{
    free(buffer->bitmap);
    free(buffer);
}

void encoding_buffer_pulse(struct encoding_buffer* buffer, int timestamp_ns)
{
    int pulse = timestamp_ns / buffer->pulselength_ns;
    if (pulse < buffer->length_pulses)
        buffer->bitmap[pulse] = 1;
}

struct fluxmap* encoding_buffer_encode(const struct encoding_buffer* buffer)
{
    struct fluxmap* fluxmap = create_fluxmap();

    int lastpulse = 0;
    int cursor = 1;
    while (cursor < buffer->length_pulses)
    {
        if ((buffer->bitmap[cursor]) || (cursor-lastpulse == MAX_INTERVAL_US))
        {
            int delta_ticks = (cursor - lastpulse) / TICKS_PER_NS;
            while (delta_ticks >= 0xfe)
            {
                fluxmap_append_interval(fluxmap, 0xfe);
                delta_ticks -= 0xfe;
            }

            fluxmap_append_interval(fluxmap, delta_ticks);
            lastpulse = cursor;
        }
        cursor++;
    }

    return fluxmap;
}
