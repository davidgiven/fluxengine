#include "globals.h"
#include "flags.h"
#include "dataspec.h"
#include <assert.h>

static void test_split(void)
{
    assert((DataSpec::split("1,2,3", ",")
        == std::vector<std::string>{"1", "2", "3"}));
    assert((DataSpec::split(",2,3", ",")
        == std::vector<std::string>{"", "2", "3"}));
    assert((DataSpec::split(",2,", ",")
        == std::vector<std::string>{"", "2", ""}));
    assert((DataSpec::split("2", ",")
        == std::vector<std::string>{"2"}));
    assert((DataSpec::split("", ",")
        == std::vector<std::string>()));
}

static void test_parserange(void)
{
    assert(DataSpec::parseRange("")
        == std::set<unsigned>());
    assert(DataSpec::parseRange("1")
        == std::set<unsigned>({1}));
    assert(DataSpec::parseRange("1,3,5")
        == std::set<unsigned>({1, 3, 5}));
    assert(DataSpec::parseRange("1,1,1")
        == std::set<unsigned>({1}));
    assert(DataSpec::parseRange("2-3")
        == std::set<unsigned>({2, 3}));
    assert(DataSpec::parseRange("2+3")
        == std::set<unsigned>({2, 3, 4}));
    assert(DataSpec::parseRange("2+3x2")
        == std::set<unsigned>({2, 4}));
    assert(DataSpec::parseRange("9,2+3x2")
        == std::set<unsigned>({2, 4, 9}));
}

static void test_parsemod(void)
{
    assert(DataSpec::parseMod("x=1")
        == (DataSpec::Modifier{"x", {1}}));
    assert(DataSpec::parseMod("x=1,3,5")
        == (DataSpec::Modifier{"x", {1, 3, 5}}));
    assert(DataSpec::parseMod("x=1,1,1")
        == (DataSpec::Modifier{"x", {1}}));
    assert(DataSpec::parseMod("x=2-3")
        == (DataSpec::Modifier{"x", {2, 3}}));
    assert(DataSpec::parseMod("x=2+3")
        == (DataSpec::Modifier{"x", {2, 3, 4}}));
    assert(DataSpec::parseMod("x=2+3x2")
        == (DataSpec::Modifier{"x", {2, 4}}));
    assert(DataSpec::parseMod("x=9,2+3x2")
        == (DataSpec::Modifier{"x", {2, 4, 9}}));
}

int main(int argc, const char* argv[])
{
    test_split();
	test_parserange();
    test_parsemod();
    return 0;
}
