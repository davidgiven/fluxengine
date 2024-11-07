#include "lib/core/globals.h"
#include "lib/core/utils.h"
#include "snowhouse/snowhouse.h"

using namespace snowhouse;

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

int main(void)
{
    testJoin();
    testLeftTrim();
    testRightTrim();
    testTrim();
    testLeafname();
    testUnhex();
    testUnbcd();
    return 0;
}
