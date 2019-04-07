#ifndef CRUNCH_H
#define CRUNCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* To save bandwidth, we compress the byte stream from the sampler when
 * sending it over USB. The encoding used is:
 * 
 * 0nnn.nnnn: value 0x00..0x7f
 * 1n       : value 0x80|n
 *
 * The end of the buffer is terminated with zeroes, which are ignored
 * (not written to the output).
 * 
 * This saves ~40%, which gets us in under the bandwidth cap.
 */
    
typedef struct crunch_state_t
{
    const uint8_t* inputptr;
    uint32_t inputlen;
    uint8_t* outputptr;
    uint32_t outputlen;
    uint16_t fifo;
    uint8_t fifolen;
}
crunch_state_t;

/* Crunches as much as possible and then stops. */
extern void crunch(crunch_state_t* state);
extern void donecrunch(crunch_state_t* state);

/* Uncrunches as much as possible and then stops. */
extern void uncrunch(crunch_state_t* state);
extern void doneuncrunch(crunch_state_t* state);

#ifdef __cplusplus
}
#endif

#endif
