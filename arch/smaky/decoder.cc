#include "globals.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "protocol.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "smaky.h"
#include "bytes.h"
#include "crc.h"
#include "fmt/format.h"
#include "lib/decoders/decoders.pb.h"
#include <string.h>
#include <algorithm>

static const FluxPattern SECTOR_PATTERN(20, 0x92aaa);

class SmakyDecoder : public Decoder
{
public:
	SmakyDecoder(const DecoderProto& config):
		Decoder(config),
		_config(config.smaky())
	{}

	void beginTrack() override
	{
		/* Find the start-of-track index marks, which will be an interval
		 * of about 6ms. */

		seekToIndexMark();
		for (;;)
		{
			auto previous = tell();
			seekToIndexMark();
			auto now = tell();
			if (eof())
				return;

			if ((now.ns() - previous.ns()) < 8e6)
			{
				seekToIndexMark();
				auto next = tell();
				if ((next.ns() - now.ns()) < 8e6)
				{
					/* We have seen two short gaps in a row, so the index
					 * mark must be now. */

					seek(previous);
					break;
				}
				else
				{
					/* We have seen one short gap and one long gap. This
					 * means the index mark must be off the beginning of
					 * the data. Seek to the start to simulate this. */

					rewind();
					break;
				}
			}
		}

		/* Now we know where to start counting, start finding sectors. */

		int sectorId = 0;
		_sectorStarts.clear();
		for (;;)
		{
			auto previous = tell();
			seekToIndexMark();
			auto now = tell();
			if (eof())
			{
				if (_sectorStarts.empty())
					return;

				_sectorIndex = 0;
				return;
			}

			if ((now.ns() - previous.ns()) < 8e6)
			{
				/* This is an index mark! */
				/* Advance to the start of the first sector and record
				 * the time. */

				seekToIndexMark();
				sectorId = 0;
			}

			_sectorStarts.push_back(std::make_pair(sectorId, now));
			sectorId++;
		}
	}

    nanoseconds_t advanceToNextRecord() override
	{
		if (_sectorIndex == _sectorStarts.size())
		{
			seekToIndexMark();
			return 0;
		}

		const auto& p = _sectorStarts[_sectorIndex++];
		_sectorId = p.first;
		seek(p.second);

		nanoseconds_t clock = seekToPattern(SECTOR_PATTERN);
		_sector->headerStartTime = tell().ns();

		return clock;
	}

    void decodeSectorRecord() override
	{
		readRawBits(21);
		const auto& rawbits = readRawBits(SMAKY_RECORD_SIZE*16);
		if (rawbits.size() < SMAKY_SECTOR_SIZE)
			return;
		const auto& rawbytes = toBytes(rawbits).slice(0, SMAKY_RECORD_SIZE*16);

		/* The Smaky bytes are stored backwards! Backwards! */

		const auto& bytes = decodeFmMfm(rawbits).slice(0, SMAKY_RECORD_SIZE).reverseBits();
		ByteReader br(bytes);

		uint8_t track = br.read_8();
		Bytes data = br.read(SMAKY_SECTOR_SIZE);
		uint8_t wantedChecksum = br.read_8();
		uint8_t gotChecksum = sumBytes(data) & 0xff;

		if (track != _sector->physicalTrack)
			return;

		_sector->logicalTrack = _sector->physicalTrack;
		_sector->logicalSide = _sector->physicalSide;
		_sector->logicalSector = _sectorId;

		_sector->data = data;
		_sector->status = (wantedChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
	}

private:
	const SmakyDecoderProto& _config;
	nanoseconds_t _startOfTrack;
	std::vector<std::pair<int, Fluxmap::Position>> _sectorStarts;
	int _sectorId;
	int _sectorIndex;
};

std::unique_ptr<Decoder> createSmakyDecoder(const DecoderProto& config)
{
	return std::unique_ptr<Decoder>(new SmakyDecoder(config));
}


