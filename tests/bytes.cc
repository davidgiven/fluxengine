#include "globals.h"
#include "bytes.h"

static void check_oob(Bytes& b, unsigned pos)
{
    try
    {
        b[pos];
    }
    catch (const std::out_of_range& e)
    {
        return;
    }

    assert(false);
}

static void test_bounds()
{
    Bytes b1 = {1, 2, 3, 4};
    assert(b1.size() == 4);
    assert(b1[0] == 1);
    assert(b1[3] == 4);
    check_oob(b1, 4);

    Bytes b2 = b1.slice(1, 2);
    assert(b2.size() == 2);
    assert(b2[0] == 2);
    assert(b2[1] == 3);
    check_oob(b2, 2);
}

static void test_loop()
{
    Bytes b = {1, 2, 3, 4};

    std::vector<uint8_t> v(b.begin(), b.end());
    assert((v == std::vector<uint8_t>{ 1, 2, 3, 4 }));

    unsigned sum = 0;
    for (uint8_t i : b)
        sum += i;
    assert(sum == 10);
}

static void test_equality()
{
    Bytes b1 = {1, 2, 1, 2};
    Bytes b2 = b1.slice(0, 2);
    Bytes b3 = b1.slice(2, 2);
    assert(b2 == b3);

    b2 = b1.slice(1, 2);
    assert(b2 != b3);
}

static void test_reads()
{
    Bytes b = {1, 2, 3, 4};

    b.seek(0);
    assert(b.read_be16() == 0x0102);
    assert(b.read_le16() == 0x0403);

    b.seek(0);
    assert(b.read_8() == 0x01);
    assert(b.read_8() == 0x02);

    b.seek(0); assert(b.read_be24() == 0x010203);
    b.seek(0); assert(b.read_le24() == 0x030201);
    b.seek(0); assert(b.read_be32() == 0x01020304);
    b.seek(0); assert(b.read_le32() == 0x04030201);
}

static void test_writes()
{
    Bytes b;

    b.seek(0);
    b.write_8(1);
    assert((b == Bytes{ 1 }));
    b.write_be32(0x02020202);
    assert((b == Bytes{ 1, 2, 2, 2, 2 }));

    auto reset = [&]() { b.resize(0); b.seek(0); };

    reset(); b.write_le16(0x0102); assert((b == Bytes{ 2, 1 }));
    reset(); b.write_be16(0x0102); assert((b == Bytes{ 1, 2 }));
    reset(); b.write_le24(0x010203); assert((b == Bytes{ 3, 2, 1 }));
    reset(); b.write_be24(0x010203); assert((b == Bytes{ 1, 2, 3 }));
    reset(); b.write_le32(0x01020304); assert((b == Bytes{ 4, 3, 2, 1 }));
    reset(); b.write_be32(0x01020304); assert((b == Bytes{ 1, 2, 3, 4 }));

    reset();
    b.write({ 1, 2, 3, 4, 5 });
    assert((b == Bytes{ 1, 2, 3, 4, 5 }));
}

int main(int argc, const char* argv[])
{
    test_bounds();
    test_loop();
    test_equality();
    test_reads();
    test_writes();
    return 0;
}
