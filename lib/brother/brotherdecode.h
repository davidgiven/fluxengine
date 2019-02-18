#ifndef BROTHER_H
#define BROTHER_H

/* Brother word processor format (or at least, one of them) */

#define BROTHER_SECTOR_RECORD 0xFFFFFD57
#define BROTHER_DATA_RECORD   0xFFFFFDDB
#define BROTHER_DATA_RECORD_PAYLOAD  256
#define BROTHER_DATA_RECORD_CHECKSUM 3

class Sector;
class Fluxmap;

class BrotherDecoder : public AbstractDecoder
{
public:
    virtual ~BrotherDecoder() {}

    SectorVector decodeToSectors(const RawRecordVector& rawRecords);
    int recordMatcher(uint64_t fifo) const;
};

extern void writeBrotherSectorHeader(std::vector<bool>& bits, unsigned& cursor,
		int track, int sector);
extern void writeBrotherSectorData(std::vector<bool>& bits, unsigned& cursor,
		const std::vector<uint8_t>& data);

#endif
