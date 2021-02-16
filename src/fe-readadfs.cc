#include "globals.h"
#include "reader.h"
#include "fmt/format.h"
#include "readibm.h"

int mainReadADFS(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0-1");
	setReaderDefaultOutput("adfs.img");
	sectorIdBase.setDefaultValue(0);
	return mainReadIBM(argc, argv);
}

