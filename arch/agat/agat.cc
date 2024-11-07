#include "lib/core/globals.h"
#include "lib/decoders/decoders.h"
#include "agat.h"
#include "lib/core/bytes.h"
#include "fmt/format.h"

uint8_t agatChecksum(const Bytes& bytes)
{
    uint16_t checksum = 0;

    for (uint8_t b : bytes)
    {
        if (checksum > 0xff)
            checksum = (checksum + 1) & 0xff;

        checksum += b;
    }

    return checksum & 0xff;
}
