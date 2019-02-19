#include "globals.h"
#include "decoders.h"
#include <assert.h>

int main(int argc, const char* argv[])
{
    assert(decodeFmMfm(
        std::vector<bool>{
            true, false,
            true, false,
            true, false,
            true, false,
            true, false,
            true, false,
            true, false,
            true, false
        }
    ) == std::vector<uint8_t>{ 0x00 });

    assert(decodeFmMfm(
        std::vector<bool>{
            true, true,
            true, false,
            true, false,
            true, false,
            true, false,
            true, false,
            true, false,
            true, true
        }
    ) == std::vector<uint8_t>{ 0x81 });

    assert(decodeFmMfm(
        std::vector<bool>{
            true, true,
            true, false,
        }
    ) == std::vector<uint8_t>{ 0x80 });

    return 0;
}
