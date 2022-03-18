#include "globals.h"
#include "utils.h"
#include "snowhouse/snowhouse.h"

using namespace snowhouse;
 
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

int main(void)
{
	testLeftTrim();
	testRightTrim();
	testTrim();
	testLeafname();
	return 0;
}

