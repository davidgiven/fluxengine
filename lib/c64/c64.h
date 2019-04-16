#ifndef C64_H
#define C64_H

#define C64_RECORD_SEPARATOR 0xfff5
#define C64_SECTOR_LENGTH    256

class Sector;
class Fluxmap;

class Commodore64Decoder : public AbstractSoftSectorDecoder
{
public:
    virtual ~Commodore64Decoder() {}

    SectorVector decodeToSectors(
        const RawRecordVector& rawRecords, unsigned physicalTrack, unsigned physicalSide);
    int recordMatcher(uint64_t fifo) const;
};

#endif
