#include "globals.h"
#include "reader.h"
#include "fmt/format.h"
#include "readibm.h"

int mainReadDFS(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0");
	setReaderDefaultOutput("dfs.img");
	sectorIdBase.setDefaultValue(0);
    setReaderRevolutions(2);
	return mainReadIBM(argc, argv);
}

