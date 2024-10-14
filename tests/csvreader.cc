#include "lib/core/globals.h"
#include "lib/external/csvreader.h"
#include <assert.h>

typedef std::vector<std::string> strings;

static void test_csvreader()
{
    std::string data(
        "header1,header2,header3\n"
        "1,2,3\n"
        "foo bar\n"
        "1,\"2,3\",4\n");
    std::istringstream istream(data);
    CsvReader reader(istream);

    assert((reader.readLine() == strings{"header1", "header2", "header3"}));
    assert((reader.readLine() == strings{"1", "2", "3"}));
    assert((reader.readLine() == strings{"foo bar"}));
    assert((reader.readLine() == strings{"1", "2,3", "4"}));
}

int main(int argc, const char* argv[])
{
    test_csvreader();
    return 0;
}
