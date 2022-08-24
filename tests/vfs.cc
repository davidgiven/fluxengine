#include "globals.h"
#include "lib/vfs/vfs.h"
#include "snowhouse/snowhouse.h"

using namespace snowhouse;
 
static void testPathParsing()
{
	AssertThat(parsePath(""), Equals(std::vector<std::string>{}));
	AssertThat(parsePath("/"), Equals(std::vector<std::string>{}));
	AssertThat(parsePath("one"), Equals(std::vector<std::string>{ "one" }));
	AssertThat(parsePath("one/two"), Equals(std::vector<std::string>{ "one", "two" }));
	AssertThat(parsePath("/one/two"), Equals(std::vector<std::string>{ "one", "two" }));
}

int main(void)
{
	testPathParsing();
	return 0;
}


