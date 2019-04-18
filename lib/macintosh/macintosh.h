#ifndef MACINTOSH_H
#define MACINTOSH_H

#define MAC_SECTOR_RECORD   0xd5aa96 /* 1101 0101 1010 1010 1001 0110 */
#define MAC_DATA_RECORD     0xd5aaad /* 1101 0101 1010 1010 1010 1101 */

#define MAC_SECTOR_LENGTH   524 /* yes, really */
#define MAC_ENCODED_SECTOR_LENGTH 703

class Sector;
class Fluxmap;

class MacintoshDecoder : public AbstractSplitDecoder
{
public:
    virtual ~MacintoshDecoder() {}

    nanoseconds_t findSector(FluxmapReader& fmr, Track& track) override;
    nanoseconds_t findData(FluxmapReader& fmr, Track& track) override;
    void decodeHeader(FluxmapReader& fmr, Track& track, Sector& sector) override;
    void decodeData(FluxmapReader& fmr, Track& track, Sector& sector) override;
};

#endif

