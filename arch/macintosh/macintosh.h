#ifndef MACINTOSH_H
#define MACINTOSH_H

#include "decoders/decoders.h"
#include "encoders/encoders.h"

#define MAC_SECTOR_RECORD   0xd5aa96 /* 1101 0101 1010 1010 1001 0110 */
#define MAC_DATA_RECORD     0xd5aaad /* 1101 0101 1010 1010 1010 1101 */

#define MAC_SECTOR_LENGTH   524 /* yes, really */
#define MAC_ENCODED_SECTOR_LENGTH 703
#define MAC_FORMAT_BYTE     0x22

#define MAC_TRACKS_PER_DISK 80

class Sector;
class Fluxmap;
class MacintoshDecoderProto;
class MacintoshEncoderProto;

class MacintoshDecoder : public AbstractDecoder
{
public:
	MacintoshDecoder(const MacintoshDecoderProto&) {}
    virtual ~MacintoshDecoder() {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
    void decodeDataRecord();

	std::set<unsigned> requiredSectors(Track& track) const;
};

class MacintoshEncoder : public AbstractEncoder
{
public:
	MacintoshEncoder(const MacintoshEncoderProto&) {}
	virtual ~MacintoshEncoder() {}

public:
    std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide, const SectorSet& allSectors);
};

extern FlagGroup macintoshEncoderFlags;


#endif

