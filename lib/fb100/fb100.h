#ifndef FB100_H
#define FB100_H

#define FB100_RECORD_SIZE 0x516 /* bytes */
#define FB100_ID_SIZE 17
#define FB100_PAYLOAD_SIZE 0x500

class Sector;
class Fluxmap;
class Track;

class Fb100Decoder : public AbstractDecoder
{
public:
    virtual ~Fb100Decoder() {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
};

#endif

