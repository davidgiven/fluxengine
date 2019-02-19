#ifndef IBM_H
#define IBM_H

#include "decoders.h"
#include "segmenter.h"

/* IBM format (i.e. ordinary PC floppies). */

#define IBM_IAM        0xFC   /* start-of-track record */
#define IBM_IAM_LEN    1      /* plus prologue */
#define IBM_IDAM       0xFE   /* sector header */
#define IBM_IDAM_LEN   7      /* plus prologue */
#define IBM_DAM1       0xF8   /* sector data (type 1) */
#define IBM_DAM2       0xFB   /* sector data (type 2) */
#define IBM_DAM_LEN    1      /* plus prologue and user data */

/* Length of a DAM record is determined by the previous sector header. */

struct IbmIdam
{
    uint8_t id;
    uint8_t cylinder;
    uint8_t side;
    uint8_t sector;
    uint8_t sectorSize;
    uint8_t crc[2];
};

class AbstractIbmDecoder : public AbstractDecoder
{
public:
    AbstractIbmDecoder(unsigned sectorIdBase):
        _sectorIdBase(sectorIdBase)
    {}
    virtual ~AbstractIbmDecoder() {}

    SectorVector decodeToSectors(const RawRecordVector& rawRecords);

protected:
    virtual int skipHeaderBytes() const = 0;

private:
    unsigned _sectorIdBase;
};

class IbmFmDecoder : public AbstractIbmDecoder
{
public:
    IbmFmDecoder(unsigned sectorIdBase):
        AbstractIbmDecoder(sectorIdBase)
    {}

    int recordMatcher(uint64_t fifo) const;

protected:
    int skipHeaderBytes() const
    { return 0; }
};

class IbmMfmDecoder : public AbstractIbmDecoder
{
public:
    IbmMfmDecoder(unsigned sectorIdBase):
        AbstractIbmDecoder(sectorIdBase)
    {}

    nanoseconds_t guessClock(Fluxmap& fluxmap) const;
    int recordMatcher(uint64_t fifo) const;

protected:
    int skipHeaderBytes() const
    { return 3; }
};

#endif
