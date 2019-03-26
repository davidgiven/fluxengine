#include <stdint.h>
#include <stdbool.h>
#include "crunch.h"

void crunch(crunch_state_t* state)
{
    while (state->inputlen && state->outputlen)
    {
        uint8_t data = *state->inputptr++;
        state->inputlen--;

        if (data & 0x80)
        {
            state->fifo = (state->fifo << 2) | 2 | (data & 1);
            state->fifolen += 2;
        }
        else
        {
            state->fifo = (state->fifo << 8) | data;
            state->fifolen += 8;
        }

        if (state->fifolen >= 8)
        {
            data = state->fifo >> (state->fifolen - 8);
            *state->outputptr++ = data;
            state->outputlen--;
            state->fifolen -= 8;
        }
    }
}

void donecrunch(crunch_state_t* state)
{
    if (state->fifolen > 0)
    {
        uint8_t b = 0;
        state->inputptr = &b;
        state->inputlen = 1;
        crunch(state);
    }
}

void uncrunch(crunch_state_t* state)
{
    do
    {
        if (state->fifolen < 8)
        {
            if (state->inputlen)
            {
                state->fifo = (state->fifo << 8) | *state->inputptr++;
                state->inputlen--;
                state->fifolen += 8;
            }
            else
                state->fifo <<= 8;
        }

        uint8_t data = state->fifo >> (state->fifolen - 8);
        if (data & 0x80)
        {
            data = ((data >> 6) & 0x01) | 0x80;
            state->fifolen -= 2;
        }
        else
            state->fifolen -= 8;

        if (data)
        {
            *state->outputptr++ = data;
            state->outputlen--;
        }
    }
    while (state->inputlen && state->outputlen);
}

void doneuncrunch(crunch_state_t* state)
{
    if (state->fifolen > 0)
    {
        uint8_t b = 0;
        state->inputptr = &b;
        state->inputlen = 1;
        uncrunch(state);
    }
}
