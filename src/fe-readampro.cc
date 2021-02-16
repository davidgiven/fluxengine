#include "globals.h"
#include "reader.h"
#include "ibm/ibm.h"
#include "fmt/format.h"
#include "readibm.h"

int mainReadAmpro(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0");
	setReaderDefaultOutput("ampro.adf");
    setReaderRevolutions(2);
	sectorIdBase.setDefaultValue(17);
	return mainReadIBM(argc, argv);
}

