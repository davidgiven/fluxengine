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

static void test_dataspec(void)
{
    DataSpec spec("foo:t=0-2:s=0-1:d=0");
    assert(spec.filename == "foo");
    assert((spec.locations
        == std::vector<DataSpec::Location>
        {{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1, 1}, {0, 2, 0}, {0, 2, 1}}));
    assert((std::string)spec == "foo:d=0:s=0-1:t=0-2");

    spec.set("bar");
    assert(spec.filename == "bar");
    assert((spec.locations
        == std::vector<DataSpec::Location>
        {{0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1, 1}, {0, 2, 0}, {0, 2, 1}}));
    assert((std::string)spec == "bar:d=0:s=0-1:t=0-2");

    spec.set(":t=0");
    assert(spec.filename.empty());
    assert((spec.locations
        == std::vector<DataSpec::Location>
        {{0, 0, 0}, {0, 0, 1}}));
    assert((std::string)spec == ":d=0:s=0-1:t=0");

    spec.set(":s=1");
    assert(spec.filename.empty());
    assert((spec.locations
        == std::vector<DataSpec::Location>
        {{0, 0, 1}}));
    assert((std::string)spec == ":d=0:s=1:t=0");

    spec.set(":t=9:d=1");
    assert(spec.filename.empty());
    assert((spec.locations
        == std::vector<DataSpec::Location>
        {{1, 9, 1}}));
    assert((std::string)spec == ":d=1:s=1:t=9");
}

int main(int argc, const char* argv[])
{
    test_split();
    test_parsemod();
    test_dataspec();
    return 0;
}
