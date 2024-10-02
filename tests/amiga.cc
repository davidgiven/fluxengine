#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/decoders/decoders.h"
#include "arch/amiga/amiga.h"
#include <assert.h>

static const Bytes testData = {
    0x52, /* 0101 0010 */
    0xff, /* 1111 1111 */
    0x4a, /* 0100 1010 */
    0x22, /* 0010 0010 */
};
static const Bytes testDataInterleaved = {
    0x1f, /* 0001 1111 */
    0x35, /* 0011 0101 */
    0xcf, /* 1100 1111 */
    0x80, /* 1000 0000 */
};

static void testInterleave(void)
{
    Bytes interleaved = amigaInterleave(testData);
    assert(interleaved == testDataInterleaved);
}

static void testDeinterleave(void)
{
    Bytes deinterleaved = amigaDeinterleave(testDataInterleaved);
    assert(deinterleaved == testData);
}

int main(int argc, const char* argv[])
{
    testDeinterleave();
    testInterleave();
    return 0;
}
