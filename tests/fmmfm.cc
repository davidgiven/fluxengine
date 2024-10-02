#include "lib/core/globals.h"
#include "lib/decoders/decoders.h"
#include <assert.h>

static void testDecode(void)
{
    assert(decodeFmMfm(std::vector<bool>{true,
               false,
               true,
               false,
               true,
               false,
               true,
               false,
               true,
               false,
               true,
               false,
               true,
               false,
               true,
               false}) == Bytes{0x00});

    assert(decodeFmMfm(std::vector<bool>{true,
               true,
               true,
               false,
               true,
               false,
               true,
               false,
               true,
               false,
               true,
               false,
               true,
               false,
               true,
               true}) == Bytes{0x81});

    assert(decodeFmMfm(std::vector<bool>{
               true,
               true,
               true,
               false,
           }) == Bytes{0x80});
}

static std::vector<bool> wrappedEncodeMfm(const Bytes& bytes)
{
    std::vector<bool> bits(16);
    unsigned cursor = 0;
    bool lastBit = false;
    encodeMfm(bits, cursor, bytes, lastBit);
    return bits;
}

static std::vector<bool> wrappedEncodeFm(const Bytes& bytes)
{
    std::vector<bool> bits(16);
    unsigned cursor = 0;
    encodeFm(bits, cursor, bytes);
    return bits;
}

static void testEncodeMfm(void)
{
    assert(wrappedEncodeMfm(Bytes{0xa1}) == (std::vector<bool>{false,
                                                true,
                                                false,
                                                false,
                                                false,
                                                true,
                                                false,
                                                false,
                                                true,
                                                false,
                                                true,
                                                false,
                                                true,
                                                false,
                                                false,
                                                true}));

    assert(wrappedEncodeMfm(Bytes{0xc2}) == (std::vector<bool>{false,
                                                true,
                                                false,
                                                true,
                                                false,
                                                false,
                                                true,
                                                false,
                                                true,
                                                false,
                                                true,
                                                false,
                                                false,
                                                true,
                                                false,
                                                false}));

    assert(wrappedEncodeMfm(Bytes{0xb0}) == (std::vector<bool>{
                                                false,
                                                true,
                                                false,
                                                false,
                                                false,
                                                true,
                                                false,
                                                true,
                                                false,
                                                false,
                                                true,
                                                false,
                                                true,
                                                false,
                                                true,
                                                false,
                                            }));
}

static void testEncodeFm(void)
{
    assert(wrappedEncodeFm(Bytes{0x00}) == (std::vector<bool>{true,
                                               false,
                                               true,
                                               false,
                                               true,
                                               false,
                                               true,
                                               false,
                                               true,
                                               false,
                                               true,
                                               false,
                                               true,
                                               false,
                                               true,
                                               false}));

    assert(wrappedEncodeFm(Bytes{0x81}) == (std::vector<bool>{true,
                                               true,
                                               true,
                                               false,
                                               true,
                                               false,
                                               true,
                                               false,
                                               true,
                                               false,
                                               true,
                                               false,
                                               true,
                                               false,
                                               true,
                                               true}));
}

int main(int argc, const char* argv[])
{
    testDecode();
    testEncodeMfm();
    testEncodeFm();
    return 0;
}
