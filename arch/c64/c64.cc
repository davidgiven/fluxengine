#include "globals.h"
#include "c64.h"

/*
 *   Track   Sectors/track   # Sectors   Storage in Bytes   Clock rate
 *   -----   -------------   ---------   ----------------   ----------
 *    1-17        21            357           7820             3.25
 *   18-24        19            133           7170             3.5
 *   25-30        18            108           6300             3.75
 *   31-40(*)     17             85           6020             4
 *                              ---
 *                              683 (for a 35 track image)
 *
 * The clock rate is normalised for a 200ms drive.
 */

nanoseconds_t clockPeriodForC64Track(unsigned track)
{
    constexpr double BYTE_SIZE = 8.0;

    if (track < 17)
        return 26.0 / BYTE_SIZE;
    if (track < 24)
        return 28.0 / BYTE_SIZE;
    if (track < 30)
        return 30.0 / BYTE_SIZE;
    return 32.0 / BYTE_SIZE;
}
