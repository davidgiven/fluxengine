#include "lib/core/globals.h"
#include "lib/core/bytes.h"

static void test_roundtrip()
{
    Bytes source = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    auto compressed = source.compress();
    auto decompressed = compressed.decompress();

    assert(decompressed == source);
}

int main(int argc, const char* argv[])
{
    test_roundtrip();
    return 0;
}
