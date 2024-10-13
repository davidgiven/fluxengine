#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include <assert.h>

static void testDefaultIntValue()
{
    FlagGroup flags;

    IntFlag intFlag({"--intFlag"}, "a global integer flag", 1);

    intFlag.setDefaultValue(2);
    const char* argv[] = {"prog", NULL};
    flags.parseFlags(1, argv);
    assert(intFlag.get() == 2);
}

static void testOverriddenIntValue()
{
    FlagGroup flags;

    IntFlag intFlag({"--intFlag"}, "a global integer flag", 1);

    intFlag.setDefaultValue(2);
    const char* argv[] = {"prog", "--intFlag=3"};
    flags.parseFlags(2, argv);
    assert(intFlag.get() == 3);
}

int main(int argc, const char* argv[])
{
    testDefaultIntValue();
    testOverriddenIntValue();
    return 0;
}
