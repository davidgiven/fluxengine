#include "lib/core/globals.h"
#include "lib/decoders/decoders.h"
#include "ibm.h"
#include "lib/core/crc.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "lib/data/sector.h"
#include "arch/ibm/ibm.pb.h"
#include "lib/config/proto.h"
#include "lib/data/layout.h"
#include <string.h>

static_assert(std::is_trivially_copyable<IbmIdam>::value,
    "IbmIdam is not trivially copyable");

/*
 * The markers at the beginning of records are special, and have
 * missing clock pulses, allowing them to be found by the logic.
 *
 * IAM record:
 * flux:   XXXX-XXX-XXXX-X- = 0xf77a
 * clock:  X X - X - X X X  = 0xd7
 * data:    X X X X X X - - = 0xfc
 *
 * (We just ignore this one --- it's useless and optional.)
 */

/*
 * IDAM record:
 * flux:   XXXX-X-X-XXXXXX- = 0xf57e
 * clock:  X X - - - X X X  = 0xc7
 * data:    X X X X X X X - = 0xfe
 */
const FluxPattern FM_IDAM_PATTERN(16, 0xf57e);

/*
 * DAM1 record:
 * flux:   XXXX-X-X-XX-X-X- = 0xf56a
 * clock:  X X - - - X X X  = 0xc7
 * data:    X X X X X - - - = 0xf8
 */
const FluxPattern FM_DAM1_PATTERN(16, 0xf56a);

/*
 * DAM2 record:
 * flux:   XXXX-X-X-XX-XXXX = 0xf56f
 * clock:  X X - - - X X X  = 0xc7
 * data:    X X X X X - X X = 0xfb
 */
const FluxPattern FM_DAM2_PATTERN(16, 0xf56f);

/*
 * TRS80DAM1 record:
 * flux:   XXXX-X-X-XX-X-XX = 0xf56b
 * clock:  X X - - - X X X  = 0xc7
 * data:    X X X X X - - X = 0xf9
 */
const FluxPattern FM_TRS80DAM1_PATTERN(16, 0xf56b);

/*
 * TRS80DAM2 record:
 * flux:   XXXX-X-X-XX-XXX- = 0xf56e
 * clock:  X X - - - X X X  = 0xc7
 * data:    X X X X X - X - = 0xfa
 */
const FluxPattern FM_TRS80DAM2_PATTERN(16, 0xf56e);

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
const FluxPattern MFM_PATTERN(48, 0x448944894489LL);

const FluxMatchers ANY_RECORD_PATTERN({
    &MFM_PATTERN,
    &FM_IDAM_PATTERN,
    &FM_DAM1_PATTERN,
    &FM_DAM2_PATTERN,
    &FM_TRS80DAM1_PATTERN,
    &FM_TRS80DAM2_PATTERN,
});

class IbmDecoder : public Decoder
{
public:
    IbmDecoder(const DecoderProto& config):
        Decoder(config),
        _config(config.ibm())
    {
    }

    nanoseconds_t advanceToNextRecord() override
    {
        return seekToPattern(ANY_RECORD_PATTERN);
    }

    void decodeSectorRecord() override
    {
        /* This is really annoying because the IBM record scheme has a
         * variable-sized header _and_ the checksum covers this header too. So
         * we have to read and decode a byte at a time until we know where the
         * record itself starts, saving the bytes for the checksumming later.
         */

        Bytes bytes;
        ByteWriter bw(bytes);

        auto readByte = [&]()
        {
            auto bits = readRawBits(16);
            auto bytes = decodeFmMfm(bits).slice(0, 1);
            uint8_t byte = bytes[0];
            bw.write_8(byte);
            return byte;
        };

        uint8_t id = readByte();
        if (id == 0xa1)
        {
            readByte();
            readByte();
            id = readByte();
        }
        if (id != IBM_IDAM)
            return;

        ByteReader br(bytes);
        br.seek(bw.pos);

        auto bits = readRawBits(IBM_IDAM_LEN * 16);
        bw += decodeFmMfm(bits).slice(0, IBM_IDAM_LEN);

        IbmDecoderProto::TrackdataProto trackdata;
        getTrackFormat(
            trackdata, _sector->physicalTrack, _sector->physicalSide);

        _sector->logicalTrack = br.read_8();
        _sector->logicalSide = br.read_8();
        _sector->logicalSector = br.read_8();
        _currentSectorSize = 1 << (br.read_8() + 7);

        uint16_t gotCrc = crc16(CCITT_POLY, bytes.slice(0, br.pos));
        uint16_t wantCrc = br.read_be16();
        if (wantCrc == gotCrc)
            _sector->status =
                Sector::DATA_MISSING; /* correct but unintuitive */

        if (trackdata.ignore_side_byte())
            _sector->logicalSide =
                Layout::remapSidePhysicalToLogical(_sector->physicalSide);
        _sector->logicalSide ^= trackdata.invert_side_byte();
        if (trackdata.ignore_track_byte())
            _sector->logicalTrack = _sector->physicalTrack;

        for (int sector : trackdata.ignore_sector())
            if (_sector->logicalSector == sector)
            {
                _sector->status = Sector::MISSING;
                break;
            }
    }

    void decodeDataRecord() override
    {
        /* This is the same deal as the sector record. */

        Bytes bytes;
        ByteWriter bw(bytes);

        auto readByte = [&]()
        {
            auto bits = readRawBits(16);
            auto bytes = decodeFmMfm(bits).slice(0, 1);
            uint8_t byte = bytes[0];
            bw.write_8(byte);
            return byte;
        };

        uint8_t id = readByte();
        if (id == 0xa1)
        {
            readByte();
            readByte();
            id = readByte();
        }
        if ((id != IBM_DAM1) && (id != IBM_DAM2) && (id != IBM_TRS80DAM1) &&
            (id != IBM_TRS80DAM2))
            return;

        ByteReader br(bytes);
        br.seek(bw.pos);

        auto bits = readRawBits((_currentSectorSize + 2) * 16);
        bw += decodeFmMfm(bits).slice(0, _currentSectorSize + 2);

        _sector->data = br.read(_currentSectorSize);
        uint16_t gotCrc = crc16(CCITT_POLY, bytes.slice(0, br.pos));
        uint16_t wantCrc = br.read_be16();
        _sector->status =
            (wantCrc == gotCrc) ? Sector::OK : Sector::BAD_CHECKSUM;

        auto layout = Layout::getLayoutOfTrack(
            _sector->logicalTrack, _sector->logicalSide);
        if (_currentSectorSize != layout->sectorSize)
            std::cerr << fmt::format(
                "Warning: configured sector size for t{}.h{}.s{} is {} bytes "
                "but that seen on disk is {} bytes\n",
                _sector->logicalTrack,
                _sector->logicalSide,
                _sector->logicalSector,
                layout->sectorSize,
                _currentSectorSize);
    }

private:
    void getTrackFormat(IbmDecoderProto::TrackdataProto& trackdata,
        unsigned track,
        unsigned head) const
    {
        trackdata.Clear();
        for (const auto& f : _config.trackdata())
        {
            if (f.has_track() && (f.track() != track))
                continue;
            if (f.has_head() && (f.head() != head))
                continue;

            trackdata.MergeFrom(f);
        }
    }

private:
    const IbmDecoderProto& _config;
    unsigned _currentSectorSize;
    unsigned _currentHeaderLength;
};

std::unique_ptr<Decoder> createIbmDecoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new IbmDecoder(config));
}
