#include "globals.h"
#include "bytes.h"
#include "lib/config.pb.h"
#include <assert.h>

static void test_proto(void)
{
	Config config;
	config.set_thing("fnord");
}

int main(int argc, const char* argv[])
{
	test_proto();
    return 0;
}


