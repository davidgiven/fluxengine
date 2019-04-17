#include "globals.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
#include <assert.h>

typedef std::vector<unsigned> ivector;

static void test_patternconstruction()
{
    {
        FluxPattern fp(16, 0x0003);
        assert(fp.bits == 16);
        assert(fp.intervals == ivector{ 1 });
    }

    {
        FluxPattern fp(16, 0xc000);
        assert(fp.bits == 16);
        assert(fp.intervals == ivector{ 1 });
    }

    {
        FluxPattern fp(16, 0x0050);
        assert(fp.bits == 16);
        assert(fp.intervals == ivector{ 2 });
    }

    {
        FluxPattern fp(16, 0x0070);
        assert(fp.bits == 16);
        assert((fp.intervals == ivector{ 1, 1 }));
    }

    {
        FluxPattern fp(16, 0x0070);
        assert(fp.bits == 16);
        assert((fp.intervals == ivector{ 1, 1 }));
    }

    {
        FluxPattern fp(16, 0x0110);
        assert(fp.bits == 16);
        assert((fp.intervals == ivector{ 4 }));
    }
}

static void test_patternmatching()
{
    FluxPattern fp(16, 0x000b);
    const unsigned matching[] = { 100, 100, 200, 100 };
    const unsigned notmatching[] = { 100, 200, 100, 100 };
    const unsigned closematch1[] = { 90, 90, 180, 90 };
    const unsigned closematch2[] = { 110, 110, 220, 110 };
    
    double clock;
    assert(fp.matches(&matching[4], clock));
    assert(!fp.matches(&notmatching[4], clock));
    assert(fp.matches(&closematch1[4], clock));
    assert(fp.matches(&closematch2[4], clock));
}

int main(int argc, const char* argv[])
{
    test_patternconstruction();
    test_patternmatching();
    return 0;
}


