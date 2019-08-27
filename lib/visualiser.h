#ifndef VISUALISER_H
#define VISUALISER_H

#include "flags.h"

class SectorSet;

extern FlagGroup visualiserFlags;

extern void visualiseSectorsToFile(const SectorSet& sectors, const std::string& filename);

#endif
