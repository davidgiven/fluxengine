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

static const FluxPattern SECTOR_PATTERN(28, 0x4892aaa);

class SmakyDecoder : public Decoder
{
public:
    SmakyDecoder(const DecoderProto& config):
        Decoder(config),
        _config(config.smaky())
    {
    }

private:
    void adjustForIndex(
        const Fluxmap::Position& previous, const Fluxmap::Position& now)
    {
        if ((now.ns() - previous.ns()) < 8e6)
        {
            seekToIndexMark();
            auto next = tell();
            if ((next.ns() - now.ns()) < 8e6)
            {
                /* We have seen two short gaps in a row, so sector 0
                 * starts here. */
            }
            else
            {
                /* We have seen one short gap and one long gap. This
                 * means the index mark must be at the beginning of
                 * the long gap. */

                seek(now);
            }

            _sectorId = 0;
        }
    }

public:
    void beginTrack() override
    {
        /* Find the start-of-track index marks, which will be an interval
         * of about 6ms. */

        seekToIndexMark();
        _sectorId = -1;
        while (_sectorId == -1)
        {
            auto previous = tell();
            seekToIndexMark();
            auto now = tell();
            if (eof())
                return;

            adjustForIndex(previous, now);
        }

        /* Now we know where to start counting, start finding sectors. */

        _sectorId = 0;
        _sectorStarts.clear();
        for (;;)
        {
            auto now = tell();
            if (eof())
                break;

            if (_sectorId < 16)
                _sectorStarts.push_back(std::make_pair(_sectorId, now));
            _sectorId++;

            seekToIndexMark();
            auto next = tell();

            adjustForIndex(now, next);
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
        readRawBits(29);
        const auto& rawbits = readRawBits(SMAKY_RECORD_SIZE * 16);
        if (rawbits.size() < SMAKY_SECTOR_SIZE)
            return;
        const auto& rawbytes =
            toBytes(rawbits).slice(0, SMAKY_RECORD_SIZE * 16);

        /* The Smaky bytes are stored backwards! Backwards! */

        const auto& bytes =
            decodeFmMfm(rawbits).slice(0, SMAKY_RECORD_SIZE).reverseBits();
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
        _sector->status =
            (wantedChecksum == gotChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
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
