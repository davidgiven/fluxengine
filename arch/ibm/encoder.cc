#include "globals.h"
#include "record.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "ibm.h"
#include "crc.h"
#include "sectorset.h"
#include "writer.h"

std::unique_ptr<Fluxmap> IbmEncoder::encode(
	int physicalTrack, int physicalSide, const SectorSet& allSectors)
{
	return std::unique_ptr<Fluxmap>();
}

