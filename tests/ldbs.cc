#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/external/ldbs.h"

static Bytes testdata{
    // clang-format off
    'L',  'B',  'S',  0x01,  'D',  'S',  'K',  0x02,
    0x29, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
    0x34, 0x12, 0x00, 0x00,  'L',  'D',  'B',  0x01,
    0x00, 0x00, 0x00, 0x01,  0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
    0x01, 'L',  'D',  'B',   0x01, 0x00, 0x00, 0x00,
    0x02, 0x01, 0x00, 0x00,  0x00, 0x01, 0x00, 0x00,
    0x00, 0x14, 0x00, 0x00,  0x00, 0x02
    // clang-format on
};

static void assertBytes(const Bytes& want, const Bytes& got)
{
    if (want != got)
    {
        std::cout << "Wanted bytes:" << std::endl;
        hexdump(std::cout, want);
        std::cout << std::endl << "Produced bytes:" << std::endl;
        hexdump(std::cout, got);
        abort();
    }
}
static void test_getset()
{
    LDBS ldbs;

    uint32_t block1 = ldbs.put(Bytes{1}, 1);
    uint32_t block2 = ldbs.put(Bytes{2}, 2);
    assert(block1 != block2);

    assert(ldbs.get(block1) == Bytes{1});
    assert(ldbs.get(block2) == Bytes{2});
}

static void test_write()
{
    LDBS ldbs;

    uint32_t block1 = ldbs.put(Bytes{1}, 1);
    uint32_t block2 = ldbs.put(Bytes{2}, 2);
    Bytes data = ldbs.write(0x1234);

    assertBytes(testdata, data);
}

static void test_read()
{
    LDBS ldbs;
    uint32_t trackDirectory = ldbs.read(testdata);

    assert(trackDirectory == 0x1234);
    assert(ldbs.get(0x14) == Bytes{1});
    assert(ldbs.get(0x29) == Bytes{2});
}

int main(int argc, const char* argv[])
{
    test_getset();
    test_write();
    test_read();
    return 0;
}
