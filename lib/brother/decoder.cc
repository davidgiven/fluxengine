#include "globals.h"
#include "sql.h"
#include "fluxmap.h"
#include "decoders.h"
#include "record.h"
#include "brother.h"
#include "sector.h"
#include "bytes.h"
#include "crc.h"
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
		#define GCR_ENTRY(gcr, data) \
			case gcr: return data;
		#include "data_gcr.h"
		#undef GCR_ENTRY
    }
    return -1;
};

static int decode_header_gcr(uint16_t word)
{
	switch (word)
	{
		#define GCR_ENTRY(gcr, data) \
			case gcr: return data;
		#include "header_gcr.h"
		#undef GCR_ENTRY
	}                       
	return -1;             
};

SectorVector BrotherDecoder::decodeToSectors(const RawRecordVector& rawRecords, unsigned)
{
    std::vector<std::unique_ptr<Sector>> sectors;
	bool headerIsValid = false;
	unsigned nextTrack = 0;
	unsigned nextSector = 0;

    for (auto& rawrecord : rawRecords)
    {
		if (rawrecord->data.size() < 64)
		{
			headerIsValid = false;
			continue;
		}

		auto ii = rawrecord->data.cbegin();
		uint32_t signature = toBytes(ii, ii+32).reader().read_be32();
		switch (signature)
		{
			case BROTHER_SECTOR_RECORD:
			{
				headerIsValid = false;
				if (rawrecord->data.size() < (32+32))
					break;

				const auto& by = toBytes(ii+32, ii+64);
				ByteReader br(by);
				nextTrack = decode_header_gcr(br.read_be16());
				nextSector = decode_header_gcr(br.read_be16());

				/* Sanity check the values read; there's no header checksum and
				 * occasionally we get garbage due to bit errors. */
				if (nextSector > 11)
					break;
				if (nextTrack > 79)
					break;

				headerIsValid = true;
				break;
			}

			case BROTHER_DATA_RECORD:
			{
				if (!headerIsValid)
					break;

				Bytes rawbytes = toBytes(rawrecord->data.cbegin()+32, rawrecord->data.cend());
				const int totalsize = BROTHER_DATA_RECORD_PAYLOAD + BROTHER_DATA_RECORD_CHECKSUM;

				Bytes output;
				ByteWriter bw(output);
				BitWriter bitw(bw);
				for (uint8_t b : rawbytes)
				{
					uint32_t nibble = decode_data_gcr(b);
					bitw.push(nibble, 5);
					if (output.size() == totalsize)
						break;
				}
				bitw.flush();
				output.resize(totalsize);

				Bytes payload = output.slice(0, BROTHER_DATA_RECORD_PAYLOAD);
				uint32_t realCrc = crcbrother(payload);
				uint32_t wantCrc = output.reader().seek(BROTHER_DATA_RECORD_PAYLOAD).read_be24();
				int status = (realCrc == wantCrc) ? Sector::OK : Sector::BAD_CHECKSUM;

                auto sector = std::unique_ptr<Sector>(
					new Sector(status, nextTrack, 0, nextSector, payload));
                sectors.push_back(std::move(sector));
                headerIsValid = false;
				break;
			}
		}
	}

	return sectors;
}

int BrotherDecoder::recordMatcher(uint64_t fifo) const
{
    uint32_t masked = fifo & 0xffffffff;
	if ((masked == BROTHER_SECTOR_RECORD) || (masked == BROTHER_DATA_RECORD))
		return 32;
    return 0;
}
