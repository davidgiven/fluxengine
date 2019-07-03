#include "globals.h"
#include "decoders/decoders.h"
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
    ) == Bytes{ 0x00 });

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
    ) == Bytes{ 0x81 });

    assert(decodeFmMfm(
        std::vector<bool>{
            true, true,
            true, false,
        }
    ) == Bytes{ 0x80 });

    return 0;
}
