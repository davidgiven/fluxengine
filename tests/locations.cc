#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/data/locations.h"
#include "snowhouse/snowhouse.h"
#include <google/protobuf/text_format.h>
#include <assert.h>
#include <regex>

using namespace snowhouse;

static void test_successful_parsing()
{
    AssertThat(parseCylinderHeadsString("c0h1"),
        Is().EqualToContainer(std::vector<CylinderHead>{
            {0, 1}
    }));
    AssertThat(parseCylinderHeadsString("c0h1 c2h3"),
        Is().EqualToContainer(std::vector<CylinderHead>{
            {0, 1},
            {2, 3}
    }));
    AssertThat(parseCylinderHeadsString("c0,1h1"),
        Is().EqualToContainer(std::vector<CylinderHead>{
            {0, 1},
            {1, 1}
    }));
    AssertThat(parseCylinderHeadsString("c0,1,2-4h1"),
        Is().EqualToContainer(std::vector<CylinderHead>{
            {0, 1},
            {1, 1},
            {2, 1},
            {3, 1},
            {4, 1}
    }));
    AssertThat(parseCylinderHeadsString("c1-4x2h1"),
        Is().EqualToContainer(std::vector<CylinderHead>{
            {1, 1},
            {3, 1},
    }));
    AssertThat(parseCylinderHeadsString("c0,1h1,2"),
        Is().EqualToContainer(std::vector<CylinderHead>{
            {0, 1},
            {0, 2},
            {1, 1},
            {1, 2},
    }));
}

static void test_failed_parsing()
{
    AssertThrows(ErrorException, parseCylinderHeadsString("zzz"));
    AssertThrows(ErrorException, parseCylinderHeadsString("c0h0 q"));
    AssertThrows(ErrorException, parseCylinderHeadsString("c-1h0"));
    AssertThrows(ErrorException, parseCylinderHeadsString("c1-10x0h0"));
    AssertThrows(ErrorException, parseCylinderHeadsString("c10-1x1h0"));
    AssertThrows(ErrorException, parseCylinderHeadsString("c10-1x-1h0"));
}

int main(int argc, const char* argv[])
{
    try
    {
        test_successful_parsing();
        test_failed_parsing();
        return 0;
    }
    catch (const ErrorException& e)
    {
        fmt::print(stderr, "Error: {}\n", e.message);
        return 1;
    }
}
