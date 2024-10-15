#include "lib/core/globals.h"
#include "lib/decoders/decoders.h"
#include "arch/mx/mx.h"
#include "lib/core/crc.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "lib/data/sector.h"
#include <string.h>

const int SECTOR_SIZE = 256;

/*
 * MX disks are a bunch of sectors glued together with no gaps or sync markers,
 * following a single beginning-of-track synchronisation and identification
 * sequence.
 */

/* FM beginning of track marker:
 *         0         0         f         3 decoded nibbles
 *  0 0  0 0  0 0  0 0  1 1  1 1  0 0  1 1
 * 1010 1010 1010 1010 1111 1111 1010 1111
 *    a    a    a    a    f    f    a    f encoded nibbles
 */
const FluxPattern ID_PATTERN(32, 0xaaaaffaf);

class MxDecoder : public Decoder
{
public:
    MxDecoder(const DecoderProto& config): Decoder(config) {}

    void beginTrack() override
    {
        _clock = _sector->clock = seekToPattern(ID_PATTERN);
        _currentSector = 0;
    }

    nanoseconds_t advanceToNextRecord() override
    {
        if (_currentSector == 11)
        {
            /* That was the last sector on the disk. */
            return 0;
        }
        else
            return _clock;
    }

    void decodeSectorRecord() override
    {
        /* Skip the ID pattern and track word, which is only present on the
         * first sector. We don't trust the track word because some driver
         * don't write it correctly. */

        if (_currentSector == 0)
            readRawBits(64);

        auto bits = readRawBits((SECTOR_SIZE + 2) * 16);
        auto bytes = decodeFmMfm(bits).slice(0, SECTOR_SIZE + 2);

        uint16_t gotChecksum = 0;
        ByteReader br(bytes);
        for (int i = 0; i < (SECTOR_SIZE / 2); i++)
            gotChecksum += br.read_be16();
        uint16_t wantChecksum = br.read_be16();

        _sector->logicalTrack = _sector->physicalTrack;
        _sector->logicalSide = _sector->physicalSide;
        _sector->logicalSector = _currentSector;
        _sector->data = bytes.slice(0, SECTOR_SIZE).swab();
        _sector->status =
            (gotChecksum == wantChecksum) ? Sector::OK : Sector::BAD_CHECKSUM;
        _currentSector++;
    }

private:
    nanoseconds_t _clock;
    int _currentSector;
};

std::unique_ptr<Decoder> createMxDecoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new MxDecoder(config));
}
