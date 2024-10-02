#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include <assert.h>

int main(int argc, const char* argv[])
{
    Bytes bytes;
    ByteWriter bw(bytes);
    BitWriter bitw(bw);

    bitw.push(0x1e, 5);
    bitw.flush();

    assert(bytes == Bytes{0x1e});

    return 0;
}
