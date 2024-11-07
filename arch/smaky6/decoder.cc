#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "protocol.h"
#include "lib/decoders/decoders.h"
#include "lib/data/sector.h"
#include "smaky6.h"
#include "lib/core/bytes.h"
#include "lib/core/crc.h"
#include "fmt/format.h"
#include "lib/decoders/decoders.pb.h"
#include <string.h>
#include <algorithm>

static const FluxPattern SECTOR_PATTERN(32, 0x54892aaa);

class Smaky6Decoder : public Decoder
{
public:
    Smaky6Decoder(const DecoderProto& config):
        Decoder(config),
        _config(config.smaky6())
    {
    }

private:
    /* Returns the sector ID of the _current_ sector. */
    int advanceToNextSector()
    {
        auto previous = tell();
        seekToIndexMark();
        auto now = tell();
        if ((now.ns() - previous.ns()) < 9e6)
        {
            seekToIndexMark();
            auto next = tell();
            if ((next.ns() - now.ns()) < 9e6)
            {
                /* We just found sector 0. */

                _sectorId = 0;
            }
            else
            {
                /* Spurious... */

                seek(now);
            }
        }

        return _sectorId++;
    }

public:
    void beginTrack() override
    {
        /* Find the start-of-track index marks, which will be an interval
         * of about 6ms. */

        seekToIndexMark();
        _sectorId = 99;
        for (;;)
        {
            auto pos = tell();
            advanceToNextSector();
            if (_sectorId < 99)
            {
                seek(pos);
                break;
            }

            if (eof())
                return;
        }

        /* Now we know where to start counting, start finding sectors. */

        _sectorStarts.clear();
        for (;;)
        {
            auto now = tell();
            if (eof())
                break;

            int id = advanceToNextSector();
            if (id < 16)
                _sectorStarts.push_back(std::make_pair(id, now));
        }

        _sectorIndex = 0;
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
        readRawBits(33);
        const auto& rawbits = readRawBits(SMAKY6_RECORD_SIZE * 16);
        if (rawbits.size() < SMAKY6_SECTOR_SIZE)
            return;
        const auto& rawbytes =
            toBytes(rawbits).slice(0, SMAKY6_RECORD_SIZE * 16);

        /* The Smaky bytes are stored backwards! Backwards! */

        const auto& bytes =
            decodeFmMfm(rawbits).slice(0, SMAKY6_RECORD_SIZE).reverseBits();
        ByteReader br(bytes);

        uint8_t track = br.read_8();
        Bytes data = br.read(SMAKY6_SECTOR_SIZE);
        uint8_t wantedChecksum = br.read_8();
        uint8_t gotChecksum = sumBytes(data) & 0xff;

        if (track != _sector->physicalTrack)
            return;

        _sector->logicalTrack = _sector->physicalTrack;
        _sector->logicalSide = _sector->physicalSide;
        _sector->logicalSector = _sectorId;

        _sector->data = data;
        _sector->status =
            (wantedChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
    }

private:
    const Smaky6DecoderProto& _config;
    nanoseconds_t _startOfTrack;
    std::vector<std::pair<int, Fluxmap::Position>> _sectorStarts;
    int _sectorId;
    int _sectorIndex;
};

std::unique_ptr<Decoder> createSmaky6Decoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new Smaky6Decoder(config));
}
