#include "globals.h"
#include "decoders/decoders.h"
#include <assert.h>

static void testDecode(void)
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
}

static std::vector<bool> wrappedEncodeFm(const Bytes& bytes)
{
	std::vector<bool> bits(16);
	unsigned cursor = 0;
	encodeFm(bits, cursor, bytes);
	return bits;
}

static void testEncodeFm(void)
{
	assert(wrappedEncodeFm(Bytes{ 0x00 })
		== (std::vector<bool>{
            true, false,
            true, false,
            true, false,
            true, false,
            true, false,
            true, false,
            true, false,
            true, false
		})
	);

	assert(wrappedEncodeFm(Bytes{ 0x81 })
		== (std::vector<bool>{
            true, true,
            true, false,
            true, false,
            true, false,
            true, false,
            true, false,
            true, false,
            true, true
		})
	);
}

int main(int argc, const char* argv[])
{
	testDecode();
	testEncodeFm();
    return 0;
}
