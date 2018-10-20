#include "globals.h"
#include "flags.h"
#include "reader.h"

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);
    allTracks();
    return 0;
}

