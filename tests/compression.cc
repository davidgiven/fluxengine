#include "globals.h"
#include "bytes.h"

static void test_roundtrip()
{
    std::string sourceText = "This is some data.";
    std::vector<uint8_t> source(sourceText.begin(), sourceText.end());

    auto compressed = compress(source);
    auto decompressed = decompress(compressed);

    assert(decompressed == source);
}

int main(int argc, const char* argv[])
{
    test_roundtrip();
    return 0;
}
