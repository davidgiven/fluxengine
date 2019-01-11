#include "globals.h"
#include "flags.h"
#include "writer.h"

int main(int argc, const char* argv[])
{
	setWriterDefaults(0, 81, 0, 1);
    Flag::parseFlags(argc, argv);

	writeTracks(-1, -1, NULL);

    return 0;
}

