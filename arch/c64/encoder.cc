#include "globals.h"
#include "record.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "c64.h"
#include "crc.h"
#include "sectorset.h"
#include "sector.h"
#include "writer.h"
#include "fmt/format.h"
#include <ctype.h>
#include "bytes.h"

uint8_t formatByte1;
uint8_t formatByte2;

FlagGroup Commodore64EncoderFlags;

static DoubleFlag postIndexGapUs(
	{ "--post-index-gap-us" },
	"Post-index gap before first sector header (microseconds).",
	0);

static DoubleFlag clockCompensation(
	{ "--clock-compensation-factor" },
	"Scale the output clock by this much.",
	1.0);

static bool lastBit;

static double clockRateUsForTrack(unsigned track)
{	//what are the clockrateUsfortrack for a c64 disk... ????
/*
		Track 	# Sectors/Track	Speed Zone 	bits/rotation		
		1 – 17 			21 			3 		61,538.4
		18 – 24 		19 			2 		57,142.8
		25 – 30 		18 			1 		53,333.4
		31 – 35 		17 			0 		50,000.0
*/
	if (track < 17)
		return 3.25;		//200000.0/61538.4
	if (track < 24)
		return 3.50;		//200000.0/57142.8
	if (track < 30)
		return 3.750;		//200000.0/53,333.4
	return 4.00;			//200000.0/50.000.0

}

static unsigned sectorsForTrack(unsigned track)
/*
 		Track   Sectors/track   # Sectors   Storage in Bytes
        -----   -------------   ---------   ----------------
         1-17        21            357           7820
        18-24        19            133           7170
        25-30        18            108           6300
        31-40(*)     17             85           6020
                                   ---
                                   683 (for a 35 track image)
*/
{
	if (track < 17)
		return 21;
	if (track < 24)
		return 19;
	if (track < 30)
		return 18;
	return 17;
}

static int encode_data_gcr(uint8_t data)
{
    switch (data)
    {
		#define GCR_ENTRY(gcr, data) \
			case data: return gcr;
		#include "data_gcr.h"
		#undef GCR_ENTRY
    }
    return -1;
};

static void write_bits(std::vector<bool>& bits, unsigned& cursor, const std::vector<bool>& src)
{
	for (bool bit : src)  //Range-based for loop
	{
		if (cursor < bits.size())
			bits[cursor++] = bit;
	}
}

static void write_bits(std::vector<bool>& bits, unsigned& cursor, uint64_t data, int width)
{
	cursor += width;
	for (int i=0; i<width; i++)
	{
		unsigned pos = cursor - i - 1;				
		if (pos < bits.size())
			bits[pos] = data & 1;
		data >>= 1;
	}
}

void bindump(std::ostream& stream, std::vector<bool>& buffer)
{
	size_t pos = 0;

	while ((pos < buffer.size()) and (pos <520))
	{
		stream << fmt::format("{:5d} : ", pos);
		for (int i=0; i<40; i++)
		{
			if ((pos+i) < buffer.size())
				stream << fmt::format("{:01b}", (buffer[pos+i]));
			else
				stream << "-- ";
			if ((((pos + i + 1) % 8) == 0) and i != 0)
				stream << "  ";

		}
		stream << std::endl;
		pos += 40;
	}
}
static std::vector<bool> encode_data(uint8_t input)
/*
Four 8-bit data bytes are converted to four 10-bit GCR bytes at a time by the 1541 DOS.
RAM is only an 8-bit storage device though. This hardware limitation prevents a 10-bit
GCR byte from being stored in a single memory location. Four 10-bit GCR bytes total
40 bits - a number evenly divisible by our overriding 8-bit constraint. Commodore sub-
divides the 40 GCR bits into five 8-bit bytes to solve this dilemma. This explains why
four 8-bit data bytes are converted to GCR form at a time. The following step by step
example demonstrates how this bit manipulation is performed by the DOS.
STEP 1. Four 8-bit Data Bytes
$08 $10 $00 $12
STEP 2. Hexadecimal to Binary Conversion
1. Binary Equivalents
$08 		$10 		$00 		$12
00001000	00010000	00000000 	00010010
STEP 3. Binary to GCR Conversion
1. Four 8-bit Data Bytes
00001000 00010000 00000000 00010010
2. High and Low Nybbles
0000 1000 0001 0000 0000 0000 0001 0010
3. High and Low Nybble GCR Equivalents
01010 01001 01011 01010 01010 01010 01011 10010
4. Four 10-bit GCR Bytes
0101001001 0101101010 0101001010 0101110010
STEP 4. 10-bit GCR to 8-bit GCR Conversion
	1. Concatenate Four 10-bit GCR Bytes
	0101001001010110101001010010100101110010
	2. Five 8-bit Subdivisions
	01010010 01010110 10100101 00101001 01110010
STEP 5. Binary to Hexadecimal Conversion
1. Hexadecimal Equivalents
	01010010	01010110 	10100101 	00101001 	01110010
	$52			$56			$A5			$29			$72
STEP 6. Four 8-bit Data Bytes are Recorded as Five 8-bit GCR Bytes
	$08	$10	$00	$12
are recorded as
	$52	$56	$A5	$29	$72
*/

{
    std::vector<bool> output(10, false);
	uint8_t hi =0;
	uint8_t lo =0;
	uint8_t lo_GCR =0;
	uint8_t hi_GCR =0;

	//Convert the byte in high and low nibble
	lo = input >> 4; //get the lo nibble shift the bits 4 to the right  		
	hi = input & 15; //get the hi nibble bij masking the lo bits (00001111) 	


	lo_GCR = encode_data_gcr(lo); 	//example value: 0000	GCR = 01010
	hi_GCR = encode_data_gcr(hi);	//example value: 1000	GCR = 01001
	//output = [0,1,2,3,4,5,6,7,8,9]
	//value  = [0,1,0,1,0,0,1,0,0,1]
	//			01010 01001

	int b = 4;
	for (int i = 0; i < 10; i++)
	{
		if (i < 5) //01234
		{		//i = 0 op 
			output[4-i] = (lo_GCR & 1); //01010

			//01010 -> & 00001 -> 00000 output[4] = 0
			//00101 -> & 00001 -> 00001 output[3] = 1
			//00010 -> & 00001 -> 00000 output[2] = 0
			//00001 -> & 00001 -> 00001 output[1] = 1
			//00000 -> & 00001 -> 00000 output[0] = 0
			lo_GCR >>= 1;
		} else	
		{
			output[i+b] = (hi_GCR & 1); //01001
			//01001 -> & 00001 -> 00001 output[9] = 1
			//00100 -> & 00001 -> 00000 output[8] = 0
			//00010 -> & 00001 -> 00000 output[7] = 0
			//00001 -> & 00001 -> 00001 output[6] = 1
			//00000 -> & 00001 -> 00000 output[5] = 0
			hi_GCR >>= 1;
			b = b-2;
		}
	}
    return output;
}

static void write_sector(std::vector<bool>& bits, unsigned& cursor, const Sector* sector)
{
try
{
/* Source: http://www.unusedino.de/ec64/technical/formats/g64.html 
   1. Header sync       FF FF FF FF FF (40 'on' bits, not GCR)
   2. Header info       52 54 B5 29 4B 7A 5E 95 55 55 (10 GCR bytes)
   3. Header gap        55 55 55 55 55 55 55 55 55 (9 bytes, never read)
   4. Data sync         FF FF FF FF FF (40 'on' bits, not GCR)
   5. Data block        55...4A (325 GCR bytes)
   6. Inter-sector gap  55 55 55 55...55 55 (4 to 12 bytes, never read)
   1. Header sync       (SYNC for the next sector)
*/
	if ((sector->status == Sector::OK) or (sector->status == Sector::BAD_CHECKSUM))
	{	//There is data to encode to disk.
		if ((sector->data.size() != C64_SECTOR_LENGTH))
			Error() << fmt::format("unsupported sector size {} --- you must pick 256", sector->data.size());	

		//1. Write header Sync (not GCR)
		for (int i=0; i<6; i++)
			write_bits(bits, cursor, C64_HEADER_DATA_SYNC, 1*8); /* sync */

		//2. Write Header info 10 GCR bytes
	/*
		The 10 byte header info (#2) is GCR encoded and must be decoded  to  it's
		normal 8 bytes to be understood. Once decoded, its breakdown is as follows:

		Byte    	$00 - header block ID 			($08)
					01 - header block checksum 16	(EOR of $02-$05)
					02 - Sector
					03 - Track
					04 - Format ID byte #2
					05 - Format ID byte #1
					06-07 - $0F ("off" bytes)
	*/
		uint8_t encodedTrack = ((sector->logicalTrack) + 1); // C64 track numbering starts with 1. Fluxengine with 0.
		uint8_t encodedSector = sector->logicalSector;
		// uint8_t formatByte1 = C64_FORMAT_ID_BYTE1;
		// uint8_t formatByte2 = C64_FORMAT_ID_BYTE2;
		uint8_t headerChecksum = (encodedTrack ^ encodedSector ^ formatByte1 ^ formatByte2);
		write_bits(bits, cursor, encode_data(C64_HEADER_BLOCK_ID));
		write_bits(bits, cursor, encode_data(headerChecksum));
		write_bits(bits, cursor, encode_data(encodedSector));
		write_bits(bits, cursor, encode_data(encodedTrack));
		write_bits(bits, cursor, encode_data(formatByte2));
		write_bits(bits, cursor, encode_data(formatByte1));
		write_bits(bits, cursor, encode_data(C64_PADDING));
		write_bits(bits, cursor, encode_data(C64_PADDING));

		//3. Write header GAP not GCR
		for (int i=0; i<9; i++)
			write_bits(bits, cursor, C64_HEADER_GAP, 1*8); /* header gap */

		//4. Write Data sync not GCR
		for (int i=0; i<6; i++)
			write_bits(bits, cursor, C64_HEADER_DATA_SYNC, 1*8); /* sync */

		//5. Write data block 325 GCR bytes
	/*
		The 325 byte data block (#5) is GCR encoded and must be  decoded  to  its
		normal 260 bytes to be understood. The data block is made up of the following:

		Byte    $00 - data block ID ($07)
				01-100 - 256 bytes data
				101 - data block checksum (EOR of $01-100)
				102-103 - $00 ("off" bytes, to make the sector size a multiple of 5)
	*/
		write_bits(bits, cursor, encode_data(C64_DATA_BLOCK_ID));
		uint8_t dataChecksum = xorBytes(sector->data);
		ByteReader br(sector->data);
		int i = 0;
		for (i = 0; i < C64_SECTOR_LENGTH; i++)
		{
			uint8_t val = br.read_8();
			write_bits(bits, cursor, encode_data(val));		
		}
		write_bits(bits, cursor, encode_data(dataChecksum));
		write_bits(bits, cursor, encode_data(C64_PADDING));
		write_bits(bits, cursor, encode_data(C64_PADDING));

		//6. Write inter-sector gap 9 - 12 bytes nor gcr
		for (int i=0; i<9; i++)
			write_bits(bits, cursor, C64_INTER_SECTOR_GAP, 1*8); /* sync */

	}
}
catch(const std::exception& e)
{
	std::cerr << e.what() << '\n';
}
}

std::unique_ptr<Fluxmap> Commodore64Encoder::encode(
	int physicalTrack, int physicalSide, const SectorSet& allSectors)
{ 	//the format ID Character # 1 and # 2 are in the .d64 image only present in track 18 sector zero which contains the BAM info in byte 162 and 163.
	//it is written in every header of every sector and track. headers are not stored in a d64 disk image so we have to get it from track 18 which contains the BAM.
	try
	{
		if ((physicalTrack < 0) || (physicalTrack >= C64_TRACKS_PER_DISK))
			return std::unique_ptr<Fluxmap>();

		const auto& sectorData = allSectors.get(C64_BAM_TRACK, 0, 0); //Read de BAM to get the DISK ID bytes

		ByteReader br(sectorData->data);
		br.seek(162); //goto position of the first Disk ID Byte
		formatByte1 = br.read_8();
		formatByte2 = br.read_8();
		
		double clockRateUs = clockRateUsForTrack(physicalTrack) * clockCompensation;

		int bitsPerRevolution = 200000.0 / clockRateUs;

		std::vector<bool> bits(bitsPerRevolution);
		unsigned cursor = 0;

		fillBitmapTo(bits, cursor, postIndexGapUs / clockRateUs, { true, false });
		lastBit = false;

		unsigned numSectors = sectorsForTrack(physicalTrack);
		for (int sectorId=0; sectorId<numSectors; sectorId++)
		{
			const auto& sectorData = allSectors.get(physicalTrack, physicalSide, sectorId);
			write_sector(bits, cursor, sectorData);
		}

		if (cursor >= bits.size())
			Error() << fmt::format("track data overrun by {} bits", cursor - bits.size());
		fillBitmapTo(bits, cursor, bits.size(), { true, false });

		std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
		fluxmap->appendBits(bits, clockRateUs*1e3);
		return fluxmap;
		}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
}

