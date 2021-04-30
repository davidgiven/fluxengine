#ifndef IBM_H
#define IBM_H

#include "decoders/decoders.h"
#include "encoders/encoders.h"

/* IBM format (i.e. ordinary PC floppies). */

#define IBM_MFM_SYNC   0xA1   /* sync byte for MFM */
#define IBM_IAM        0xFC   /* start-of-track record */
#define IBM_IAM_LEN    1      /* plus prologue */
#define IBM_IDAM       0xFE   /* sector header */
#define IBM_IDAM_LEN   7      /* plus prologue */
#define IBM_DAM1       0xF8   /* sector data (type 1) */
#define IBM_DAM2       0xFB   /* sector data (type 2) */
#define IBM_TRS80DAM1  0xF9   /* sector data (TRS-80 directory) */
#define IBM_TRS80DAM2  0xFA   /* sector data (TRS-80 directory) */
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

class IbmDecoder : public AbstractDecoder
{
public:
    IbmDecoder(unsigned sectorBase, bool ignoreSideByte=false,
			const std::set<unsigned> requiredSectors=std::set<unsigned>()):
        _sectorBase(sectorBase),
        _ignoreSideByte(ignoreSideByte),
		_requiredSectors(requiredSectors)
    {}

    RecordType advanceToNextRecord();
    void decodeSectorRecord();
    void decodeDataRecord();

	std::set<unsigned> requiredSectors(Track& track) const
	{ return _requiredSectors; }

private:
    unsigned _sectorBase;
    bool _ignoreSideByte;
	std::set<unsigned> _requiredSectors;
    unsigned _currentSectorSize;
    unsigned _currentHeaderLength;
};

class IbmEncoder : public AbstractEncoder
{
public:
	IbmEncoder(int trackLengthMs,int sectorSize,bool emitIam,int startSectorId,int clockRateKhz,bool useFm,uint16_t idamByte,uint16_t damByte,int gap0,int gap1,int gap2,int gap3,std::string sectorSkew,bool swapSides)
	{}

	virtual ~IbmEncoder() 
	{}

public:
    std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide, const SectorSet& allSectors);
	static void setsectorSize(int newsectorSize);
	static void setstartSectorId(int newstartSectorId);
	static void setuseFm(bool newuseFm);
	static void setclockRateKhz(int newclockRateKhz);
	static void setsectorSkew(std::string newsectorSkew);
	static void settrackLengthMs(int newtrackLengthMs);
	static void setemitIam(bool newemitIam);
	static void setidamByte(uint16_t newidamByte);
	static void setdamByte(uint16_t newdamByte);
	static void setgap0(int newgap0);
	static void setgap1(int newgap1);
	static void setgap2(int newgap2);
	static void setgap3(int newgap3);
	static void setswapSides(bool newswapSides);
	static int getsectorSize();
	static int getstartSectorId();
	static bool getuseFm();
	static int getclockRateKhz();
	static std::string getsectorSkew();
	static int gettrackLengthMs();
	static bool getemitIam();
	static uint16_t getidamByte();
	static uint16_t getdamByte();
	static int getgap0();
	static int getgap1();
	static int getgap2();
	static int getgap3();
	static bool getswapSides();

private:
	void writeRawBits(uint32_t data, int width);
	void writeBytes(const Bytes& bytes);
	void writeBytes(int count, uint8_t value);
	void writeSync();
	
private:
	static int _trackLengthMs;
	static int _sectorSize;
	static bool _emitIam;
	static int _startSectorId;
	static int _clockRateKhz;
	static bool _useFm;
	static uint16_t _idamByte;
	static uint16_t _damByte;
	static int _gap0;
	static int _gap1;
	static int _gap2;
	static int _gap3;
	static std::string _sectorSkew;
	static bool _swapSides;
	std::vector
	<bool> _bits;
	unsigned _cursor;
	bool _lastBit;
};

#endif
