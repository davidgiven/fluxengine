#ifndef READIBM_H
#define READIBM_H

#include "lib/config/flags.h"
#include "dataspec.h"

extern IntFlag sectorIdBase;
extern BoolFlag ignoreSideByte;
extern RangeFlag requiredSectors;

extern int mainReadIBM(int argc, const char* argv[]);

#endif
