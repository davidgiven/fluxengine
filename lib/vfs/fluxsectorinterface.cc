#include "lib/globals.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/image.h"
#include "lib/readerwriter.h"
#include "lib/decoders/decoders.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/layout.h"
#include "lib/proto.h"
#include "lib/mapper.h"

class FluxSectorInterface : public SectorInterface
{
public:
    FluxSectorInterface(std::shared_ptr<FluxSource> fluxSource,
        std::shared_ptr<FluxSink> fluxSink,
        std::shared_ptr<AbstractEncoder> encoder,
        std::shared_ptr<AbstractDecoder> decoder):
        _fluxSource(fluxSource),
        _fluxSink(fluxSink),
        _encoder(encoder),
        _decoder(decoder)
    {
    }

public:
    std::shared_ptr<const Sector> get(
        unsigned track, unsigned side, unsigned sectorId)
    {
        auto it = _changedSectors.get(track, side, sectorId);
        if (it)
            return it;

        trackid_t trackid(track, side);
        if (_loadedtracks.find(trackid) == _loadedtracks.end())
            populateSectors(track, side);

        return _readSectors.get(track, side, sectorId);
    }

    std::shared_ptr<Sector> put(
        unsigned track, unsigned side, unsigned sectorId)
    {
        trackid_t trackid(track, side);
        _changedtracks.insert(trackid);
        return _changedSectors.put(track, side, sectorId);
    }

    void flush()
    {
        for (const auto& trackid : _changedtracks)
        {
            unsigned track = trackid.first;
            unsigned side = trackid.second;
            auto layoutdata = Layout::getLayoutOfTrack(track, side);
            auto sectors = Layout::getSectorsInTrack(layoutdata);

            config.mutable_tracks()->Clear();
            config.mutable_tracks()->set_start(track);

            config.mutable_heads()->Clear();
            config.mutable_heads()->set_start(side);

            /* Check to see if we have all sectors of this track in the
             * changesectors image. */

            if (imageContainsAllSectorsOf(
                    _changedSectors, track, side, sectors))
            {
                /* Just write directly from the changedsectors image. */

                writeDiskCommand(_changedSectors,
                    *_encoder,
                    *_fluxSink,
                    &*_decoder,
                    &*_fluxSource);
            }
            else
            {
                /* Only a few sectors have changed. Do we need to populate the
                 * track? */

                if (_loadedtracks.find(trackid) == _loadedtracks.end())
                    populateSectors(track, side);

                /* Now merge the loaded track with the changed one, and write
                 * the result back. */

                Image image;
                for (const unsigned sector : sectors)
                {
                    auto s = image.put(track, side, sector);
                    if (_changedSectors.contains(track, side, sector))
                        s->data =
                            _changedSectors.get(track, side, sector)->data;
                    else
                        s->data = _readSectors.get(track, side, sector)->data;
                }

                writeDiskCommand(
                    image, *_encoder, *_fluxSink, &*_decoder, &*_fluxSource);
            }
        }
    }

private:
    bool imageContainsAllSectorsOf(const Image& image,
        unsigned track,
        unsigned side,
        const std::vector<unsigned>& sectors)
    {
        for (unsigned sector : sectors)
        {
            if (!image.contains(track, side, sector))
                return false;
        }
        return true;
    }

    void populateSectors(unsigned track, unsigned side)
    {
        auto location = Mapper::computeLocationFor(track, side);
        auto trackdata = readAndDecodeTrack(*_fluxSource, *_decoder, location);

        for (const auto& sector : trackdata->sectors)
            *_readSectors.put(track, side, sector->logicalSector) = *sector;
        _loadedtracks.insert(trackid_t(track, side));
    }

    std::shared_ptr<FluxSource> _fluxSource;
    std::shared_ptr<FluxSink> _fluxSink;
    std::shared_ptr<AbstractEncoder> _encoder;
    std::shared_ptr<AbstractDecoder> _decoder;

    typedef std::pair<unsigned, unsigned> trackid_t;
    Image _readSectors;
    Image _changedSectors;
    std::set<trackid_t> _loadedtracks;
    std::set<trackid_t> _changedtracks;
};

std::unique_ptr<SectorInterface> SectorInterface::createFluxSectorInterface(
    std::shared_ptr<FluxSource> fluxSource,
    std::shared_ptr<FluxSink> fluxSink,
    std::shared_ptr<AbstractEncoder> encoder,
    std::shared_ptr<AbstractDecoder> decoder)
{
    return std::make_unique<FluxSectorInterface>(
        fluxSource, fluxSink, encoder, decoder);
}
