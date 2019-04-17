#ifndef MACINTOSH_H
#define MACINTOSH_H

#define MAC_SECTOR_RECORD   0xd5aa96
#define MAC_DATA_RECORD     0xd5aaad

#define MAC_SECTOR_LENGTH   524 /* yes, really */
#define MAC_ENCODED_SECTOR_LENGTH 703

class Sector;
class Fluxmap;

class MacintoshDecoder : public AbstractStatefulDecoder
{
public:
    virtual ~MacintoshDecoder() {}

    void decodeToSectors(Track& track) override;
};

#endif

