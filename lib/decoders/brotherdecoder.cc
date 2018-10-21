#include "globals.h"
#include "sql.h"
#include "fluxmap.h"
#include "decoders.h"
#include <ctype.h>

static std::vector<uint8_t> outputbuffer;

/*
 * Brother disks have this very very non-IBM system where sector header records
 * and data records use two different kinds of GCR: sector headers are 8-in-16
 * (but the encodable values range from 0 to 77ish only) and data headers are
 * 5-in-8. In addition, there's a non-encoded 10-bit ID word at the beginning
 * of each record, as well as a string of 53 1s introducing them. That does at
 * least make them easy to find.
 *
 * Disk formats vary from machine to machine, but mine uses 78 tracks. Track 0
 * is erased but not formatted.  Track alignment is extremely dubious and
 * Brother track 0 shows up on my machine at track 2.
 */

static int decode_data_gcr(uint8_t gcr)
{
    switch (gcr)
    {
        case 0x55: return 0; // 00000
        case 0x57: return 1; // 00001
        case 0x5b: return 2; // 00010
        case 0x5d: return 3; // 00011
        case 0x5f: return 4; // 00100 
        case 0x6b: return 5; // 00101
        case 0x6d: return 6; // 00110
        case 0x6f: return 7; // 00111
        case 0x75: return 8; // 01000
        case 0x77: return 9; // 01001
        case 0x7b: return 10; // 01010
        case 0x7d: return 11; // 01011
        case 0x7f: return 12; // 01100
        case 0xab: return 13; // 01101
        case 0xad: return 14; // 01110
        case 0xaf: return 15; // 01111
        case 0xb5: return 16; // 10000
        case 0xb7: return 17; // 10001
        case 0xbb: return 18; // 10010
        case 0xbd: return 19; // 10011
        case 0xbf: return 20; // 10100
        case 0xd5: return 21; // 10101
        case 0xd7: return 22; // 10110
        case 0xdb: return 23; // 10111
        case 0xdd: return 24; // 11000
        case 0xdf: return 25; // 11001
        case 0xeb: return 26; // 11010
        case 0xed: return 27; // 11011
        case 0xef: return 28; // 11100
        case 0xf5: return 29; // 11101
        case 0xf7: return 30; // 11110
        case 0xfb: return 31; // 11111
    }
    return -1;
};

static int decode_sector_gcr(uint16_t word)
{
	switch (word)
	{
		case 0xDFB5: return 0;
		case 0x5B6F: return 1;
		case 0x7DF7: return 2;
		case 0xBFD5: return 3;
		case 0xF57F: return 4;
		case 0x6D5D: return 5;
		case 0xAFEB: return 6;
		case 0xDDB7: return 7;
		case 0x5775: return 8;
		case 0x7BFB: return 9;
		case 0xBDD7: return 10;
		case 0xEFAB: return 11;
		case 0x6B5F: return 12;
		case 0xADED: return 13;
		case 0xDBBB: return 14;
		case 0x5577: return 15;
		case 0x77DB: return 16;
		case 0xBBAD: return 17;
		case 0xED6B: return 18;
		case 0x5FEF: return 19;
		case 0xABBD: return 20;
		case 0xD77B: return 21;
		case 0xFB57: return 22;
		case 0x75DD: return 23;
		case 0xB7AF: return 24;
		case 0xEB6D: return 25;
		case 0x5DF5: return 26;
		case 0x7FBF: return 27;
		case 0xD57D: return 28;
		case 0xF75B: return 29;
		case 0x6FDF: return 30;
		case 0xB5B5: return 31;
		case 0xDF6F: return 32;
		case 0x5BF7: return 33;
		case 0x7DD5: return 34;
		case 0xBF7F: return 35;
		case 0xF55D: return 36;
		case 0x6DEB: return 37;
		case 0xAFB7: return 38;
		case 0xDD75: return 39;
		case 0x57FB: return 40;
		case 0x7BD7: return 41;
		case 0xBDAB: return 42;
		case 0xEF5F: return 43;
		case 0x6BED: return 44;
		case 0xADBB: return 45;
		case 0xDB77: return 46;
		case 0xBB55: return 47;
		case 0xEDDB: return 48;
		case 0x5FAD: return 49;
		case 0xAB6B: return 50;
		case 0xD7EF: return 51;
		case 0xFBBD: return 52;
		case 0x757B: return 53;
		case 0xB757: return 54;
		case 0xEBDD: return 55;
		case 0x5DAF: return 56;
		case 0x7F6D: return 57;
		case 0xD5F5: return 58;
		case 0xF7BF: return 59;
		case 0x6F7D: return 60;
		case 0xB55B: return 61;
		case 0xDFDF: return 62;
		case 0x5BB5: return 63;
		case 0x7D6F: return 64;
		case 0xBFF7: return 65;
		case 0xF5D5: return 66;
		case 0x6D7F: return 67;
		case 0xAF5D: return 68;
		case 0xDDEB: return 69;
		case 0x57B7: return 70;
		case 0x7B75: return 71;
		case 0xBDFB: return 72;
		case 0xEFD7: return 73;
		case 0x6BAB: return 74;
		case 0xAD5F: return 75;
		case 0xDBED: return 76;
		case 0x55BB: return 77;
	}                       
	return -1;             
};

std::vector<std::vector<uint8_t>> decodeBitsToRecordsBrother(const std::vector<bool>& bits)
{
    std::vector<std::vector<uint8_t>> records;

	enum
	{
		SEEKING,
		READINGSECTOR,
		READINGDATA
	};

    size_t cursor = 0;
    uint32_t inputfifo = 0;
	int inputcount = 0;
	uint8_t outputfifo = 0;
	int outputcount = 0;
    int state = SEEKING;

    while (cursor < bits.size())
    {
        bool bit = bits[cursor++];
        inputfifo = (inputfifo << 1) | bit;
		inputcount++;

		if (inputfifo == BROTHER_SECTOR_RECORD)
		{
			if (state != SEEKING)
				records.push_back(outputbuffer);
			outputbuffer.resize(1);
			outputbuffer[0] = BROTHER_SECTOR_RECORD & 0xff;
			state = READINGSECTOR;
			inputcount = 0;
		}
		else if (inputfifo == BROTHER_DATA_RECORD)
		{
			if (state != SEEKING)
				records.push_back(outputbuffer);
			outputbuffer.resize(1);
			outputbuffer[0] = BROTHER_DATA_RECORD & 0xff;
			state = READINGDATA;
			outputcount = 0;
			inputcount = 0;
		}
		else
		{
			switch (state)
			{
				case READINGSECTOR:
				{
					if (inputcount != 16)
						break;
					int data = decode_sector_gcr(inputfifo & 0xffff);
					inputcount = 0;

					outputbuffer.push_back(data);
					break;
				}

				case READINGDATA:
				{
					if (inputcount != 8)
						break;
					int data = decode_data_gcr(inputfifo & 0xff);
					inputcount = 0;

					for (int i=0; i<5; i++)
					{
						outputfifo = (outputfifo << 1) | !!(data & 0x10);
						data <<= 1;
						outputcount++;
						if (outputcount == 8)
						{
							outputbuffer.push_back(outputfifo);
							outputcount = 0;
						}
					}
					break;
				}
			}
		}
    }

	if (state != SEEKING)
		records.push_back(outputbuffer);

    return records;
}

