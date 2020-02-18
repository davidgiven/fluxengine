#include <stdint.h>
#include <stdbool.h>
#include "crunch.h"

void crunch(crunch_state_t* state)
{
    while (state->inputlen && state->outputlen)
    {
        uint8_t data = *state->inputptr++;
        state->inputlen--;

		if (data == 0x80)
		{
			/* Multiple 0x80s in a row get swallowed as they're
			 * meaningless. */
			state->haspending = true;
		}
        else if (data & 0x80)
        {
			if (state->haspending)
			{
				state->fifo = (state->fifo << 3) | 4;
				state->fifolen += 3;
				state->haspending = false;
			}

            state->fifo = (state->fifo << 3) | 4 | (data & 1);
            state->fifolen += 3;
        }
        else
        {
			if (state->haspending && (data >= 0x40))
			{
				state->fifo = (state->fifo << 3) | 4;
				state->fifolen += 3;
				state->haspending = false;
			}
				
			state->fifo = (state->fifo << 8) | data;
			if (state->haspending)
				state->fifo |= 0xc0;
			state->haspending = false;
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
    while (state->inputlen && state->outputlen)
    {
		if (state->haspending)
		{
			*state->outputptr++ = state->pendingbyte;
			state->outputlen--;
			state->haspending = false;
			continue;
		}

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
		switch (data & 0xc0)
		{
			case 0x80:
				data = ((data >> 5) & 0x01) | 0x80;
				state->fifolen -= 3;
				break;

			case 0xc0:
				state->haspending = true;
				state->pendingbyte = data & 0x3f;
				data = 0x80;
				/* fall through */
			default:
				state->fifolen -= 8;
		}

        if (data)
        {
            *state->outputptr++ = data;
            state->outputlen--;
        }
    }
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
