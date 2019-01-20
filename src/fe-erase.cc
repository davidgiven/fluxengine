#include "globals.h"
#include "flags.h"
#include "writer.h"

int main(int argc, const char* argv[])
{
	setWriterDefaultDest(":t=0-81:s=0-1");
    Flag::parseFlags(argc, argv);

	writeTracks(NULL);

    return 0;
}

