#include "globals.h"
#include "fluxmap.h"
#include "kryoflux.h"
#include "lib/fluxsource/fluxsource.pb.h"
#include "lib/utils.h"
#include "fluxsource/fluxsource.h"
#include "scp.h"
#include "proto.h"
#include "fmt/format.h"
#include <fstream>

static int trackno(int strack)
{
    return strack >> 1;
}

static int headno(int strack)
{
    return strack & 1;
}

static int strackno(int track, int side)
{
    return (track << 1) | side;
}

class ScpFluxSource : public TrivialFluxSource
{
public:
    ScpFluxSource(const ScpFluxSourceProto& config): _config(config)
    {
        _if.open(_config.filename(), std::ios::in | std::ios::binary);
        if (!_if.is_open())
            Error() << fmt::format("cannot open input file '{}': {}",
                _config.filename(),
                strerror(errno));

        _if.read((char*)&_header, sizeof(_header));
        check_for_error();

        if ((_header.file_id[0] != 'S') || (_header.file_id[1] != 'C') ||
            (_header.file_id[2] != 'P'))
            Error() << "input not a SCP file";

		int tpi = (_header.flags & SCP_FLAG_96TPI) ? 96 : 48;
        ::config.set_tpi(tpi);
		::config.mutable_drive()->set_tpi(tpi);

        _resolution = 25 * (_header.resolution + 1);
        int startSide = (_header.heads == 2) ? 1 : 0;
        int endSide = (_header.heads == 1) ? 0 : 1;

        if ((_header.cell_width != 0) && (_header.cell_width != 16))
            Error() << "currently only 16-bit cells in SCP files are supported";

        std::cout << fmt::format("SCP tracks {}-{}, heads {}-{}\n",
            trackno(_header.start_track),
            trackno(_header.end_track),
            startSide,
            endSide);
        std::cout << fmt::format("SCP sample resolution: {} ns\n", _resolution);
    }

public:
    std::unique_ptr<const Fluxmap> readSingleFlux(int track, int side) override
    {
        int strack = strackno(track, side);
        if (strack >= ARRAY_SIZE(_header.track))
            return std::make_unique<Fluxmap>();
        uint32_t offset = Bytes(_header.track[strack], 4).reader().read_le32();
        if (offset == 0)
            return std::make_unique<Fluxmap>();

        ScpTrackHeader trackheader;
        _if.seekg(offset, std::ios::beg);
        _if.read((char*)&trackheader, sizeof(trackheader));
        check_for_error();

        if ((trackheader.track_id[0] != 'T') ||
            (trackheader.track_id[1] != 'R') ||
            (trackheader.track_id[2] != 'K'))
            Error() << "corrupt SCP file";

        std::vector<ScpTrackRevolution> revs(_header.revolutions);
        for (int revolution = 0; revolution < _header.revolutions; revolution++)
        {
            ScpTrackRevolution trackrev;
            _if.read((char*)&trackrev, sizeof(trackrev));
            check_for_error();
            revs[revolution] = trackrev;
        }

        auto fluxmap = std::make_unique<Fluxmap>();
        nanoseconds_t pending = 0;
        unsigned inputBytes = 0;
        for (int revolution = 0; revolution < _header.revolutions; revolution++)
        {
            if (revolution != 0)
                fluxmap->appendIndex();

            uint32_t datalength =
                Bytes(revs[revolution].length, 4).reader().read_le32();
            uint32_t dataoffset =
                Bytes(revs[revolution].offset, 4).reader().read_le32();

            Bytes data(datalength * 2);
            _if.seekg(dataoffset + offset, std::ios::beg);
            _if.read((char*)data.begin(), data.size());
            check_for_error();

            ByteReader br(data);
            for (int cell = 0; cell < datalength; cell++)
            {
                uint16_t interval = br.read_be16();
                if (interval)
                {
                    fluxmap->appendInterval(
                        (interval + pending) * _resolution / NS_PER_TICK);
                    fluxmap->appendPulse();
                    pending = 0;
                }
                else
                    pending += 0x10000;
            }

            inputBytes += datalength * 2;
        }

        return fluxmap;
    }

    void recalibrate() {}

private:
    void check_for_error()
    {
        if (_if.fail())
            Error() << fmt::format("SCP read I/O error: {}", strerror(errno));
    }

private:
    const ScpFluxSourceProto& _config;
    std::ifstream _if;
    ScpHeader _header;
    nanoseconds_t _resolution;
};

std::unique_ptr<FluxSource> FluxSource::createScpFluxSource(
    const ScpFluxSourceProto& config)
{
    return std::unique_ptr<FluxSource>(new ScpFluxSource(config));
}
