#ifndef ZILOGMCZ_H
#define ZILOGMCZ_H

class Sector;
class Fluxmap;

class ZilogMczDecoder : public AbstractHardSectorDecoder
{
public:
    virtual ~ZilogMczDecoder() {}

    SectorVector decodeToSectors(const RawRecordVector& rawRecords, unsigned physicalTrack);
};

#endif


