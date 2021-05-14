#ifndef C64_H
#define C64_H

#define C64_SECTOR_RECORD    0xffd49
#define C64_DATA_RECORD      0xffd57
#define C64_SECTOR_LENGTH    256

class Sector;
class Fluxmap;
class Commodore64InputProto;

class Commodore64Decoder : public AbstractDecoder
{
public:
	Commodore64Decoder(const Commodore64InputProto&) {}
    virtual ~Commodore64Decoder() {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
    void decodeDataRecord();
};

#endif
