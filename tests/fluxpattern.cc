#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxpattern.h"
#include <sstream>

typedef std::vector<unsigned> ivector;

namespace std
{
    template <class T>
    static std::string to_string(const std::vector<T>& vector)
    {
        std::stringstream s;
        s << "vector{";
        bool first = true;
        for (const T& t : vector)
        {
            if (!first)
                s << ", ";
            first = false;
            s << t;
        }
        s << "}";
        return s.str();
    }
}

#undef assert
#define assert(got, expected) assertImpl(__FILE__, __LINE__, got, expected)

template <class T>
static void assertImpl(const char filename[], int linenumber, T got, T expected)
{
    if (got != expected)
    {
        std::cerr << "assertion failure at " << filename << ":" << linenumber
                  << ": got " << std::to_string(got) << ", expected "
                  << std::to_string(expected) << std::endl;
        abort();
    }
}

void test_patternconstruction()
{
    {
        FluxPattern fp(16, 0x0003);
        assert(fp._bits, 16U);
        assert(fp._intervals, ivector{1});
    }

    {
        FluxPattern fp(16, 0xc000);
        assert(fp._bits, 16U);
        assert(fp._intervals, (ivector{1, 15}));
    }

    {
        FluxPattern fp(16, 0x0050);
        assert(fp._bits, 16U);
        assert(fp._intervals, (ivector{2, 5}));
    }

    {
        FluxPattern fp(16, 0x0070);
        assert(fp._bits, 16U);
        assert(fp._intervals, (ivector{1, 1, 5}));
    }

    {
        FluxPattern fp(16, 0x0070);
        assert(fp._bits, 16U);
        assert(fp._intervals, (ivector{1, 1, 5}));
    }

    {
        FluxPattern fp(16, 0x0110);
        assert(fp._bits, 16U);
        assert(fp._intervals, (ivector{4, 5}));
    }
}

void test_patternmatchingwithouttrailingzeros()
{
    FluxPattern fp(16, 0x000b);
    const unsigned matching[] = {100, 100, 200, 100};
    const unsigned notmatching[] = {100, 200, 100, 100};
    const unsigned closematch1[] = {90, 90, 180, 90};
    const unsigned closematch2[] = {110, 110, 220, 110};

    FluxMatch match;
    assert(fp.matches(&matching[4], match), true);
    assert(match.intervals, 2U);

    assert(fp.matches(&notmatching[4], match), false);

    assert(fp.matches(&closematch1[4], match), true);
    assert(match.intervals, 2U);

    assert(fp.matches(&closematch2[4], match), true);
    assert(match.intervals, 2U);
}

void test_patternmatchingwithtrailingzeros()
{
    FluxPattern fp(16, 0x0016);
    const unsigned matching[] = {100, 100, 200, 100, 200};
    const unsigned notmatching[] = {100, 200, 100, 100, 100};
    const unsigned closematch1[] = {90, 90, 180, 90, 300};
    const unsigned closematch2[] = {110, 110, 220, 110, 220};

    FluxMatch match;
    assert(fp.matches(&matching[5], match), true);
    assert(match.intervals, 3U);

    assert(fp.matches(&notmatching[5], match), false);

    assert(fp.matches(&closematch1[5], match), true);
    assert(match.intervals, 3U);

    assert(fp.matches(&closematch2[5], match), true);
    assert(match.intervals, 3U);
}

int main(int argc, const char* argv[])
{
    test_patternconstruction();
    test_patternmatchingwithouttrailingzeros();
    test_patternmatchingwithtrailingzeros();
    return 0;
}
