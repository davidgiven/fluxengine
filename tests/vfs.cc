#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/vfs/vfs.h"
#include "snowhouse/snowhouse.h"

using namespace snowhouse;

static void testPathParsing()
{
    AssertThat(Path(""), Equals(std::vector<std::string>{}));
    AssertThat(Path("/"), Equals(std::vector<std::string>{}));
    AssertThat(Path("one"), Equals(std::vector<std::string>{"one"}));
    AssertThat(Path("one/two"), Equals(std::vector<std::string>{"one", "two"}));
    AssertThat(
        Path("/one/two"), Equals(std::vector<std::string>{"one", "two"}));
}

static void testPathParenthood()
{
    AssertThat(Path("").parent(), Equals(std::vector<std::string>{}));
    AssertThat(Path("one").parent(), Equals(std::vector<std::string>{}));
    AssertThat(
        Path("one/two").parent(), Equals(std::vector<std::string>{"one"}));
    AssertThat(Path("one/two/three").parent(),
        Equals(std::vector<std::string>{"one", "two"}));
}

int main(void)
{
    testPathParsing();
    testPathParenthood();
    return 0;
}
