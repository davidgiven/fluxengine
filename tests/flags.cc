#include "globals.h"
#include "flags.h"
#include <assert.h>

static IntFlag intFlag(
    { "--intFlag" },
    "a global integer flag",
    1);

static void testDefaultIntValue()
{
    intFlag.value = intFlag.defaultValue = 2;
    const char* argv[] = { "prog", NULL };
    Flag::parseFlags(1, argv);
    assert(intFlag.value == 2);
}

int main(int argc, const char* argv[])
{
    testDefaultIntValue();
    return 0;
}

