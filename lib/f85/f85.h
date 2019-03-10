#ifndef F85_H
#define F85_H

#define F85_RECORD_SEPARATOR 0xfff5
#define F85_SECTOR_LENGTH    256

class Sector;
class Fluxmap;

class DurangoF85Decoder : public AbstractSoftSectorDecoder
{
public:
    virtual ~DurangoF85Decoder() {}

    SectorVector decodeToSectors(
        const RawRecordVector& rawRecords, unsigned physicalTrack);
    int recordMatcher(uint64_t fifo) const;
};

#endif
