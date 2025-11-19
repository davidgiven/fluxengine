#include "lib/core/globals.h"
#include "lib/core/utils.h"
#include "snowhouse/snowhouse.h"

using namespace snowhouse;

template <typename F, typename S>
struct snowhouse::Stringizer<std::pair<F, S>>
{
    static std::string ToString(const std::pair<F, S>& a)
    {
        std::stringstream stream;
        stream << "pair(" << Stringizer<F>::ToString(a.first) << ", "
               << Stringizer<S>::ToString(a.second) << ')';
        return stream.str();
    }
};

static void testJoin()
{
    AssertThat(join({}, "/"), Equals(""));
    AssertThat(join({"one"}, "/"), Equals("one"));
    AssertThat(join({"one", "two"}, "/"), Equals("one/two"));
}

static void testLeftTrim()
{
    AssertThat(leftTrimWhitespace("string"), Equals("string"));
    AssertThat(leftTrimWhitespace(" string"), Equals("string"));
    AssertThat(leftTrimWhitespace(" string "), Equals("string "));
    AssertThat(leftTrimWhitespace("string "), Equals("string "));
}

static void testRightTrim()
{
    AssertThat(rightTrimWhitespace("string"), Equals("string"));
    AssertThat(rightTrimWhitespace(" string"), Equals(" string"));
    AssertThat(rightTrimWhitespace(" string "), Equals(" string"));
    AssertThat(rightTrimWhitespace("string "), Equals("string"));
}

static void testTrim()
{
    AssertThat(trimWhitespace("string"), Equals("string"));
    AssertThat(trimWhitespace(" string"), Equals("string"));
    AssertThat(trimWhitespace(" string "), Equals("string"));
    AssertThat(trimWhitespace("string "), Equals("string"));
}

static void testLeafname()
{
    AssertThat(getLeafname(""), Equals(""));
    AssertThat(getLeafname("filename"), Equals("filename"));
    AssertThat(getLeafname("path/filename"), Equals("filename"));
    AssertThat(getLeafname("/path/path/filename"), Equals("filename"));
}

static void testUnhex()
{
    AssertThat(unhex(""), Equals(""));
    AssertThat(unhex("foo"), Equals("foo"));
    AssertThat(unhex("f%20o"), Equals("f o"));
}

static void testUnbcd()
{
    AssertThat(unbcd(0x1234), Equals(1234));
    AssertThat(unbcd(0x87654321), Equals(87654321));
}

static void testMultimapToMap()
{
    std::multimap<int, std::string> input = {
        {0, "zero" },
        {0, "nil"  },
        {1, "one"  },
        {3, "two"  },
        {3, "drei" },
        {3, "trois"}
    };
    std::map<int, std::vector<std::string>> wanted = {
        {0, {"zero", "nil"}         },
        {1, {"one"}                 },
        {3, {"two", "drei", "trois"}}
    };

    auto output = multimapToMapOfVectors(input);
    AssertThat(output, Equals(wanted));
}

int main(void)
{
    testJoin();
    testLeftTrim();
    testRightTrim();
    testTrim();
    testLeafname();
    testUnhex();
    testUnbcd();
    testMultimapToMap();
    return 0;
}
