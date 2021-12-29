#include "globals.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "ibm.h"
#include "crc.h"
#include "writer.h"
#include "image.h"
#include "arch/ibm/ibm.pb.h"
#include "lib/encoders/encoders.pb.h"
#include "fmt/format.h"
#include <ctype.h>

/* IAM record separator:
 * 0xC2 is:
 * data:    1  1  0  0  0  0  1  0  = 0xc2
 * mfm:     01 01 00 10 10 10 01 00 = 0x5254
 * special: 01 01 00 10 00 10 01 00 = 0x5224
 */
#define MFM_IAM_SEPARATOR 0x5224

/* FM IAM record:
 * flux:   XXXX-XXX-XXXX-X- = 0xf77a
 * clock:  X X - X - X X X  = 0xd7
 * data:    X X X X X X - - = 0xfc
 */
#define FM_IAM_RECORD 0xf77a

/* MFM IAM record:
 * data:   1  1  1  1  1  1  0  0  = 0xfc
 * flux:   01 01 01 01 01 01 00 10 = 0x5552
 */
#define MFM_IAM_RECORD 0x5552

/* MFM record separator:
 * 0xA1 is:
 * data:    1  0  1  0  0  0  0  1  = 0xa1
 * mfm:     01 00 01 00 10 10 10 01 = 0x44a9
 * special: 01 00 01 00 10 00 10 01 = 0x4489
 *                       ^^^^^
 * When shifted out of phase, the special 0xa1 byte becomes an illegal
 * encoding (you can't do 10 00). So this can't be spoofed by user data.
 * 
 * shifted: 10 00 10 01 00 01 00 1
 * 
 * It's repeated three times.
 */
#define MFM_RECORD_SEPARATOR 0x4489
#define MFM_RECORD_SEPARATOR_BYTE 0xa1

/* MFM IDAM byte:
 * data:    1  1  1  1  1  1  1  0  = 0xfe
 * mfm:     01 01 01 01 01 01 01 00 = 0x5554
 */

/* MFM DAM byte:
 * data:    1  1  1  1  1  0  1  1  = 0xfb
 * mfm:     01 01 01 01 01 00 01 01 = 0x5545
 */

static uint8_t decodeUint16(uint16_t raw)
{
	Bytes b;
	ByteWriter bw(b);
	bw.write_be16(raw);
	return decodeFmMfm(b.toBits())[0];
}

class IbmEncoder : public AbstractEncoder
{
public:
	IbmEncoder(const EncoderProto& config):
		AbstractEncoder(config),
		_config(config.ibm())
	{}

private:
	void writeRawBits(uint32_t data, int width)
	{
		_cursor += width;
		_lastBit = data & 1;
		for (int i=0; i<width; i++)
		{
			unsigned pos = _cursor - i - 1;
			if (pos < _bits.size())
				_bits[pos] = data & 1;
			data >>= 1;
		}
	}

	void getTrackFormat(IbmEncoderProto::TrackdataProto& trackdata, unsigned cylinder, unsigned head)
	{
		trackdata.Clear();
		for (const auto& f : _config.trackdata())
		{
			if (f.has_cylinder() && (f.cylinder() != cylinder))
				continue;
			if (f.has_head() && (f.head() != head))
				continue;

			trackdata.MergeFrom(f);
		}
	}

private:
	static std::set<unsigned> getSectorIds(const IbmEncoderProto::TrackdataProto& trackdata)
	{
		std::set<unsigned> s;
		if (trackdata.has_sectors())
		{
			for (int sectorId : trackdata.sectors().sector())
				s.insert(sectorId);
		}
		else if (trackdata.has_sector_range())
		{
			int sectorId = trackdata.sector_range().min_sector();
			while (sectorId <= trackdata.sector_range().max_sector())
			{
				s.insert(sectorId);
				sectorId++;
			}
		}
		return s;
	}

public:
	std::vector<std::shared_ptr<Sector>> collectSectors(int physicalTrack, int physicalSide, const Image& image) override
	{
		std::vector<std::shared_ptr<Sector>> sectors;
		IbmEncoderProto::TrackdataProto trackdata;
		getTrackFormat(trackdata, physicalTrack, physicalSide);

		int logicalSide = physicalSide ^ trackdata.swap_sides();
		for (int sectorId : getSectorIds(trackdata))
        {
			const auto& sector = image.get(physicalTrack, logicalSide, sectorId);
			if (sector)
				sectors.push_back(sector);
        }

		return sectors;
	}

    std::unique_ptr<Fluxmap> encode(int physicalTrack, int physicalSide,
			const std::vector<std::shared_ptr<Sector>>& sectors, const Image& image) override
	{
		IbmEncoderProto::TrackdataProto trackdata;
		getTrackFormat(trackdata, physicalTrack, physicalSide);

		auto writeBytes = [&](const Bytes& bytes)
		{
			if (trackdata.use_fm())
				encodeFm(_bits, _cursor, bytes);
			else
				encodeMfm(_bits, _cursor, bytes, _lastBit);
		};

		auto writeFillerRawBytes = [&](int count, uint16_t byte)
		{
			for (int i=0; i<count; i++)
				writeRawBits(byte, 16);
		};

		auto writeFillerBytes = [&](int count, uint8_t byte)
		{
			Bytes b { byte };
			for (int i=0; i<count; i++)
				writeBytes(b);
		};

		double clockRateUs = 1e3 / trackdata.clock_rate_khz();
		if (!trackdata.use_fm())
			clockRateUs /= 2.0;
		int bitsPerRevolution = (trackdata.track_length_ms() * 1000.0) / clockRateUs;
		_bits.resize(bitsPerRevolution);
		_cursor = 0;

		uint8_t idamUnencoded = decodeUint16(trackdata.idam_byte());
		uint8_t damUnencoded = decodeUint16(trackdata.dam_byte());

		uint8_t sectorSize = 0;
		{
			int s = trackdata.sector_size() >> 7;
			while (s > 1)
			{
				s >>= 1;
				sectorSize += 1;
			}
		}

		uint16_t gapFill = trackdata.gap_fill_byte();

		writeFillerRawBytes(trackdata.gap0(), gapFill);
		if (trackdata.emit_iam())
		{
			writeFillerBytes(trackdata.use_fm() ? 6 : 12, 0x00);
			if (!trackdata.use_fm())
			{
				for (int i=0; i<3; i++)
					writeRawBits(MFM_IAM_SEPARATOR, 16);
			}
			writeRawBits(trackdata.use_fm() ? FM_IAM_RECORD : MFM_IAM_RECORD, 16);
			writeFillerRawBytes(trackdata.gap1(), gapFill);
		}

		int logicalSide = physicalSide ^ trackdata.swap_sides();
		bool first = true;
		for (int sectorId : trackdata.sectors().sector())
		{
			if (!first)
				writeFillerRawBytes(trackdata.gap3(), gapFill);
			first = false;

			const auto& sectorData = image.get(physicalTrack, logicalSide, sectorId);
			if (!sectorData)
				continue;

			/* Writing the sector and data records are fantastically annoying.
			 * The CRC is calculated from the *very start* of the record, and
			 * include the malformed marker bytes. Our encoder doesn't know
			 * about this, of course, with the result that we have to construct
			 * the unencoded header, calculate the checksum, and then use the
			 * same logic to emit the bytes which require special encoding
			 * before encoding the rest of the header normally. */

			{
				Bytes header;
				ByteWriter bw(header);

				writeFillerBytes(trackdata.use_fm() ? 6 : 12, 0x00);
				if (!trackdata.use_fm())
				{
					for (int i=0; i<3; i++)
						bw.write_8(MFM_RECORD_SEPARATOR_BYTE);
				}
				bw.write_8(idamUnencoded);
				bw.write_8(sectorData->logicalTrack);
				bw.write_8(sectorData->logicalSide);
				bw.write_8(sectorData->logicalSector);
				bw.write_8(sectorSize);
				uint16_t crc = crc16(CCITT_POLY, header);
				bw.write_be16(crc);

				int conventionalHeaderStart = 0;
				if (!trackdata.use_fm())
				{
					for (int i=0; i<3; i++)
						writeRawBits(MFM_RECORD_SEPARATOR, 16);
					conventionalHeaderStart += 3;

				}
				writeRawBits(trackdata.idam_byte(), 16);
				conventionalHeaderStart += 1;

				writeBytes(header.slice(conventionalHeaderStart));
			}

			writeFillerRawBytes(trackdata.gap2(), gapFill);

			{
				Bytes data;
				ByteWriter bw(data);

				writeFillerBytes(trackdata.use_fm() ? 6 : 12, 0x00);
				if (!trackdata.use_fm())
				{
					for (int i=0; i<3; i++)
						bw.write_8(MFM_RECORD_SEPARATOR_BYTE);
				}
				bw.write_8(damUnencoded);

				Bytes truncatedData = sectorData->data.slice(0, trackdata.sector_size());
				bw += truncatedData;
				uint16_t crc = crc16(CCITT_POLY, data);
				bw.write_be16(crc);

				int conventionalHeaderStart = 0;
				if (!trackdata.use_fm())
				{
					for (int i=0; i<3; i++)
						writeRawBits(MFM_RECORD_SEPARATOR, 16);
					conventionalHeaderStart += 3;

				}
				writeRawBits(trackdata.dam_byte(), 16);
				conventionalHeaderStart += 1;

				writeBytes(data.slice(conventionalHeaderStart));
			}
		}

		if (_cursor >= _bits.size())
			Error() << "track data overrun";
		while (_cursor < _bits.size())
			writeFillerRawBytes(1, gapFill);

		std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);
		fluxmap->appendBits(_bits, clockRateUs*1e3);
		return fluxmap;
	}

private:
	const IbmEncoderProto& _config;
	std::vector<bool> _bits;
	unsigned _cursor;
	bool _lastBit;
};

std::unique_ptr<AbstractEncoder> createIbmEncoder(const EncoderProto& config)
{
	return std::unique_ptr<AbstractEncoder>(new IbmEncoder(config));
}


