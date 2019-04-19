#ifndef F85_H
#define F85_H

#define F85_SECTOR_RECORD 0xffffce /* 1111 1111 1111 1111 1100 1110 */
#define F85_DATA_RECORD 0xffffcb /* 1111 1111 1111 1111 1100 1101 */
#define F85_SECTOR_LENGTH    512

class Sector;
class Fluxmap;

class DurangoF85Decoder : public AbstractSplitDecoder
{
public:
    virtual ~DurangoF85Decoder() {}

    nanoseconds_t findSector(FluxmapReader& fmr, Track& track) override;
    nanoseconds_t findData(FluxmapReader& fmr, Track& track) override;
    void decodeHeader(FluxmapReader& fmr, Track& track, Sector& sector) override;
    void decodeData(FluxmapReader& fmr, Track& track, Sector& sector) override;
};

#endif
