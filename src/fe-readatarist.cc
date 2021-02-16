#include "globals.h"
#include "reader.h"
#include "ibm/ibm.h"
#include "fmt/format.h"
#include "readibm.h"

int mainReadAtariST(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0-1");
	setReaderDefaultOutput("atarist.st");
	sectorIdBase.setDefaultValue(1);
	return mainReadIBM(argc, argv);
}

