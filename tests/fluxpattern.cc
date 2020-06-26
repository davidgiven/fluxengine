#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "betterassert.h"
#include <sstream>

FlagGroup flags { &fluxmapReaderFlags };

typedef std::vector<unsigned> ivector;

void test_patternconstruction()
{
    {
        FluxPattern fp(16, 0x0003);
        assertEquals(fp._bits, 16U);
        assertEquals(fp._intervals, ivector{ 1 });
    }

    {
        FluxPattern fp(16, 0xc000);
        assertEquals(fp._bits, 16U);
        assertEquals(fp._intervals, (ivector{ 1, 15 }));
    }

    {
        FluxPattern fp(16, 0x0050);
        assertEquals(fp._bits, 16U);
        assertEquals(fp._intervals, (ivector{ 2, 5 }));
    }

    {
        FluxPattern fp(16, 0x0070);
        assertEquals(fp._bits, 16U);
        assertEquals(fp._intervals, (ivector{ 1, 1, 5 }));
    }

    {
        FluxPattern fp(16, 0x0070);
        assertEquals(fp._bits, 16U);
        assertEquals(fp._intervals, (ivector{ 1, 1, 5 }));
    }

    {
        FluxPattern fp(16, 0x0110);
        assertEquals(fp._bits, 16U);
        assertEquals(fp._intervals, (ivector{ 4, 5 }));
    }
}

void test_patternmatchingwithouttrailingzeros()
{
    FluxPattern fp(16, 0x000b);
    const unsigned matching[] = { 100, 100, 200, 100 };
    const unsigned notmatching[] = { 100, 200, 100, 100 };
    const unsigned closematch1[] = { 90, 90, 180, 90 };
    const unsigned closematch2[] = { 110, 110, 220, 110 };
    
    FluxMatch match;
    assertEquals(fp.matches(&matching[4], match), true);
    assertEquals(match.intervals, 2U);

    assertEquals(fp.matches(&notmatching[4], match), false);
    
    assertEquals(fp.matches(&closematch1[4], match), true);
    assertEquals(match.intervals, 2U);

    assertEquals(fp.matches(&closematch2[4], match), true);
    assertEquals(match.intervals, 2U);
}

void test_patternmatchingwithtrailingzeros()
{
    FluxPattern fp(16, 0x0016);
    const unsigned matching[] = { 100, 100, 200, 100, 200 };
    const unsigned notmatching[] = { 100, 200, 100, 100, 100 };
    const unsigned closematch1[] = { 90, 90, 180, 90, 300 };
    const unsigned closematch2[] = { 110, 110, 220, 110, 220 };
    
    FluxMatch match;
    assertEquals(fp.matches(&matching[5], match), true);
    assertEquals(match.intervals, 3U);

    assertEquals(fp.matches(&notmatching[5], match), false);

    assertEquals(fp.matches(&closematch1[5], match), true);
    assertEquals(match.intervals, 3U);

    assertEquals(fp.matches(&closematch2[5], match), true);
    assertEquals(match.intervals, 3U);
}

int main(int argc, const char* argv[])
{
    flags.parseFlags(0, NULL);

    test_patternconstruction();
    test_patternmatchingwithouttrailingzeros();
    test_patternmatchingwithtrailingzeros();
    return 0;
}
