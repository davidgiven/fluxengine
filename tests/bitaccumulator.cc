#include "globals.h"
#include "bytes.h"
#include <assert.h>

int main(int argc, const char* argv[])
{
    BitAccumulator ba;

    ba.reset();
    ba.push(0x1e, 5);
    assert((std::vector<uint8_t>)ba == std::vector<uint8_t>{ 0x1e });

    return 0;
}

