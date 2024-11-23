#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "snowhouse/snowhouse.h"

using namespace snowhouse;

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
    assert((v == std::vector<uint8_t>{1, 2, 3, 4}));

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
    ByteReader br(b);

    br.seek(0);
    assert(br.read_be16() == 0x0102);
    assert(br.read_le16() == 0x0403);

    br.seek(0);
    assert(br.read_8() == 0x01);
    assert(br.read_8() == 0x02);

    // clang-format off
    br.seek(0); assert(br.read_be24() == 0x010203);
    br.seek(0); assert(br.read_le24() == 0x030201);
    br.seek(0); assert(br.read_be32() == 0x01020304);
    br.seek(0); assert(br.read_le32() == 0x04030201);
    // clang-format on
}

static void test_writes()
{
    Bytes b;
    ByteWriter bw(b);

    bw.seek(0);
    bw.write_8(1);
    assert((b == Bytes{1}));
    bw.write_be32(0x02020202);
    assert((b == Bytes{1, 2, 2, 2, 2}));

    auto reset = [&]()
    {
        b.resize(0);
        bw.seek(0);
    };

    // clang-format off
    reset(); bw.write_le16(0x0102); assert((b == Bytes{2, 1}));
    reset(); bw.write_be16(0x0102); assert((b == Bytes{1, 2}));
    reset(); bw.write_le24(0x010203); assert((b == Bytes{3, 2, 1}));
    reset(); bw.write_be24(0x010203); assert((b == Bytes{1, 2, 3}));
    reset(); bw.write_le32(0x01020304); assert((b == Bytes{4, 3, 2, 1}));
    reset(); bw.write_be32(0x01020304); assert((b == Bytes{1, 2, 3, 4}));
    // clang-format on

    reset();
    bw += {1, 2, 3, 4, 5};
    assert((b == Bytes{1, 2, 3, 4, 5}));
}

static void test_slice()
{
    Bytes b = {1, 2, 3};

    Bytes bs = b.slice(1, 1);
    assert((bs == Bytes{2}));

    bs = b.slice(1, 2);
    assert((bs == Bytes{2, 3}));

    bs = b.slice(1, 3);
    assert((bs == Bytes{2, 3, 0}));

    bs = b.slice(4, 2);
    assert((bs == Bytes{0, 0}));
}

static void test_split()
{
    AssertThat((Bytes{}).split(0), Equals(std::vector<Bytes>{Bytes{}}));

    AssertThat(
        (Bytes{0}).split(0), Equals(std::vector<Bytes>{Bytes{}, Bytes{}}));

    AssertThat((Bytes{1}).split(0), Equals(std::vector<Bytes>{Bytes{1}}));

    AssertThat(
        (Bytes{1, 0}).split(0), Equals(std::vector<Bytes>{Bytes{1}, Bytes{}}));

    AssertThat(
        (Bytes{0, 1}).split(0), Equals(std::vector<Bytes>{Bytes{}, Bytes{1}}));

    AssertThat((Bytes{1, 0, 1}).split(0),
        Equals(std::vector<Bytes>{Bytes{1}, Bytes{1}}));
}

static void test_tobits()
{
    Bytes b = {1, 2};
    assert(b.toBits() == (std::vector<bool>{false,
                             false,
                             false,
                             false,
                             false,
                             false,
                             false,
                             true,
                             false,
                             false,
                             false,
                             false,
                             false,
                             false,
                             true,
                             false}));
}

static void test_tostring()
{
    std::string s = "Hello, world";
    Bytes b(s);
    assert(b == s);
}

int main(int argc, const char* argv[])
{
    test_bounds();
    test_loop();
    test_equality();
    test_reads();
    test_writes();
    test_slice();
    test_split();
    test_tobits();
    test_tostring();
    return 0;
}
