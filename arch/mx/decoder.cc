#include "globals.h"
#include "decoders/decoders.h"
#include "mx/mx.h"
#include "crc.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "sector.h"
#include <string.h>

const int SECTOR_SIZE = 256;

/*
 * MX disks are a bunch of sectors glued together with no gaps or sync markers,
 * following a single beginning-of-track synchronisation and identification
 * sequence.
 */

/* FM beginning of track marker:
 * 1010 1010 1010 1010 1111 1111 1010 1111
 *    a    a    a    a    f    f    a    f
 */
const FluxPattern ID_PATTERN(32, 0xaaaaffaf);

class MxDecoder : public AbstractDecoder
{
public:
	MxDecoder(const DecoderProto& config):
		AbstractDecoder(config)
	{}

    void beginTrack()
	{
		_currentSector = -1;
		_clock = 0;
	}

    RecordType advanceToNextRecord()
	{
		if (_currentSector == -1)
		{
			/* First sector in the track: look for the sync marker. */
			const FluxMatcher* matcher = nullptr;
			_sector->clock = _clock = _fmr->seekToPattern(ID_PATTERN, matcher);
			readRawBits(32); /* skip the ID mark */
			//_logicalTrack = decodeFmMfm(readRawBits(32)).slice(0, 32).reader().read_be16();
			_logicalTrack = _sector->physicalCylinder;
		}
		else if (_currentSector == 10)
		{
			/* That was the last sector on the disk. */
			return UNKNOWN_RECORD;
		}
		else
		{
			/* Otherwise we assume the clock from the first sector is still valid.
			 * The decoder framwork will automatically stop when we hit the end of
			 * the track. */
			_sector->clock = _clock;
		}

		_currentSector++;
		return SECTOR_RECORD;
	}

    void decodeSectorRecord()
	{
		if(_currentSector==0) {
		
			bits.clear();
			bytes.clear();
		
			/*  Preread an entire track, which is technically just a single
			 * giant sector. (and avoid syncing problems in the meantime) */
			bits = readRawBits(tracksize_bytes*16);
			bytes = decodeFmMfm(bits).slice(0, tracksize_bytes).swab();
	    }

		unsigned start = 4+(SECTOR_SIZE+2)*_currentSector;
		auto sectorBytes = bytes.slice(start, SECTOR_SIZE+2);

		/* Accumulate checksum */
		uint16_t gotChecksum = 0;
		ByteReader br(sectorBytes);
		for (int i=0; i<(SECTOR_SIZE/2); i++)
			gotChecksum += br.read_le16();
		uint16_t wantChecksum = br.read_le16();

		/* We'll match logical parameters to physical since track number may
		 * not be present in track data depending on MX driver version that 
		 * wrote to that disk originally 
		 * (or worse, many different drivers on same disk) */
		_sector->logicalTrack = _sector->physicalCylinder;
		_sector->logicalSide = _sector->physicalHead;
		_sector->logicalSector = _currentSector;
		_sector->data = bytes.slice(start, SECTOR_SIZE);

		_sector->status = (gotChecksum == wantChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
	}

private:

    std::vector<bool> bits;
    Bytes bytes;

    const int tracksize_bytes = ((SECTOR_SIZE+2)*11)+4;
    nanoseconds_t _clock;
    int _currentSector;
    int _logicalTrack;
};

std::unique_ptr<AbstractDecoder> createMxDecoder(const DecoderProto& config)
{
	return std::unique_ptr<AbstractDecoder>(new MxDecoder(config));
}


