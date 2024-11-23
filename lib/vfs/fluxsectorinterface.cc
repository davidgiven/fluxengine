#include "lib/core/globals.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/data/image.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/decoders/decoders.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/data/layout.h"
#include "lib/config/proto.h"

class FluxSectorInterface : public SectorInterface
{
public:
    FluxSectorInterface(std::shared_ptr<FluxSource> fluxSource,
        std::shared_ptr<FluxSink> fluxSink,
        std::shared_ptr<Encoder> encoder,
        std::shared_ptr<Decoder> decoder):
        _fluxSource(fluxSource),
        _fluxSink(fluxSink),
        _encoder(encoder),
        _decoder(decoder)
    {
    }

public:
    std::shared_ptr<const Sector> get(
        unsigned track, unsigned side, unsigned sectorId) override
    {
        auto it = _changedSectors.get(track, side, sectorId);
        if (it)
            return it;

        trackid_t trackid(track, side);
        if (_loadedTracks.find(trackid) == _loadedTracks.end())
            populateSectors(track, side);

        return _loadedSectors.get(track, side, sectorId);
    }

    std::shared_ptr<Sector> put(
        unsigned track, unsigned side, unsigned sectorId) override
    {
        trackid_t trackid(track, side);
        _changedTracks.insert(trackid);
        return _changedSectors.put(track, side, sectorId);
    }

    virtual bool isReadOnly() override
    {
        return (_fluxSink == nullptr);
    }

    bool needsFlushing() override
    {
        return !_changedTracks.empty();
    }

    void flushChanges() override
    {
        std::vector<std::shared_ptr<const TrackInfo>> locations;

        for (const auto& trackid : _changedTracks)
        {
            unsigned track = trackid.first;
            unsigned side = trackid.second;
            auto trackLayout = Layout::getLayoutOfTrack(track, side);
            locations.push_back(trackLayout);

            /* If we don't have all the sectors of this track, we may need to
             * populate any non-changed sectors as we can only write a track at
             * a time. */

            if (!imageContainsAllSectorsOf(_changedSectors,
                    track,
                    side,
                    trackLayout->naturalSectorOrder))
            {
                /* If we don't have any loaded sectors for this track, pre-read
                 * it. */

                if (_loadedTracks.find(trackid) == _loadedTracks.end())
                    populateSectors(track, side);

                /* Now merge the loaded track with the changed one, and write
                 * the result back. */

                for (unsigned sectorId : trackLayout->naturalSectorOrder)
                {
                    if (!_changedSectors.contains(track, side, sectorId))
                        _changedSectors.put(track, side, sectorId)->data =
                            _loadedSectors.get(track, side, sectorId)->data;
                }
            }
        }

        /* We now have complete tracks which can be written. */

        writeDiskCommand(_changedSectors,
            *_encoder,
            *_fluxSink,
            &*_decoder,
            &*_fluxSource,
            locations);

        discardChanges();
    }

    void discardChanges() override
    {
        _loadedTracks.clear();
        _loadedSectors.clear();
        _changedTracks.clear();
        _changedSectors.clear();
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
        auto trackInfo = Layout::getLayoutOfTrack(track, side);
        auto trackdata = readAndDecodeTrack(*_fluxSource, *_decoder, trackInfo);

        for (const auto& sector : trackdata->sectors)
            *_loadedSectors.put(track, side, sector->logicalSector) = *sector;
        _loadedTracks.insert(trackid_t(track, side));
    }

    std::shared_ptr<FluxSource> _fluxSource;
    std::shared_ptr<FluxSink> _fluxSink;
    std::shared_ptr<Encoder> _encoder;
    std::shared_ptr<Decoder> _decoder;

    typedef std::pair<unsigned, unsigned> trackid_t;
    Image _loadedSectors;
    Image _changedSectors;
    std::set<trackid_t> _loadedTracks;
    std::set<trackid_t> _changedTracks;
};

std::unique_ptr<SectorInterface> SectorInterface::createFluxSectorInterface(
    std::shared_ptr<FluxSource> fluxSource,
    std::shared_ptr<FluxSink> fluxSink,
    std::shared_ptr<Encoder> encoder,
    std::shared_ptr<Decoder> decoder)
{
    return std::make_unique<FluxSectorInterface>(
        fluxSource, fluxSink, encoder, decoder);
}
