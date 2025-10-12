#include "lib/core/globals.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/data/image.h"
#include "lib/data/layout.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/decoders/decoders.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/data/layout.h"
#include "lib/config/proto.h"

class FluxSectorInterface : public SectorInterface
{
public:
    FluxSectorInterface(const std::shared_ptr<const DiskLayout>& diskLayout,
        std::shared_ptr<FluxSource> fluxSource,
        std::shared_ptr<FluxSink> fluxSink,
        std::shared_ptr<Encoder> encoder,
        std::shared_ptr<Decoder> decoder):
        _diskLayout(diskLayout),
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

        CylinderHead trackid(track, side);
        if (_loadedTracks.find(trackid) == _loadedTracks.end())
            populateSectors(track, side);

        return _loadedSectors.get(track, side, sectorId);
    }

    std::shared_ptr<Sector> put(
        unsigned track, unsigned side, unsigned sectorId) override
    {
        CylinderHead trackid(track, side);
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
        std::vector<CylinderHead> locations;

        for (const auto& trackid : _changedTracks)
        {
            auto& ltl = _diskLayout->layoutByLogicalLocation.at(trackid);
            locations.push_back(trackid);

            /* If we don't have all the sectors of this track, we may need to
             * populate any non-changed sectors as we can only write a track at
             * a time. */

            if (!imageContainsAllSectorsOf(_changedSectors,
                    trackid.cylinder,
                    trackid.head,
                    ltl->filesystemSectorOrder))
            {
                /* If we don't have any loaded sectors for this track, pre-read
                 * it. */

                if (_loadedTracks.find(trackid) == _loadedTracks.end())
                    populateSectors(trackid.cylinder, trackid.head);

                /* Now merge the loaded track with the changed one, and write
                 * the result back. */

                for (unsigned sectorId : ltl->naturalSectorOrder)
                {
                    if (!_changedSectors.contains(trackid, sectorId))
                        _changedSectors.put(trackid, sectorId)->data =
                            _loadedSectors.get(trackid, sectorId)->data;
                }
            }
        }

        /* We now have complete tracks which can be written. */

        writeDiskCommand(*_diskLayout,
            _changedSectors,
            *_encoder,
            *_fluxSink,
            _decoder.get(),
            _fluxSource.get(),
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

    void populateSectors(unsigned logicalCylinder, unsigned logicalSide)
    {
        CylinderHead logicalLocation = {logicalCylinder, logicalSide};
        auto& ltl = _diskLayout->layoutByLogicalLocation.at(logicalLocation);
        auto trackdata =
            readAndDecodeTrack(*_diskLayout, *_fluxSource, *_decoder, ltl);

        for (const auto& sector : trackdata.sectors)
            *_loadedSectors.put(logicalLocation, sector->logicalSector) =
                *sector;
        _loadedTracks.insert(logicalLocation);
    }

    std::shared_ptr<const DiskLayout> _diskLayout;
    std::shared_ptr<FluxSource> _fluxSource;
    std::shared_ptr<FluxSink> _fluxSink;
    std::shared_ptr<Encoder> _encoder;
    std::shared_ptr<Decoder> _decoder;

    Image _loadedSectors;
    Image _changedSectors;

    std::set<CylinderHead> _loadedTracks;
    std::set<CylinderHead> _changedTracks;
};

std::unique_ptr<SectorInterface> SectorInterface::createFluxSectorInterface(
    const std::shared_ptr<const DiskLayout>& diskLayout,
    std::shared_ptr<FluxSource> fluxSource,
    std::shared_ptr<FluxSink> fluxSink,
    std::shared_ptr<Encoder> encoder,
    std::shared_ptr<Decoder> decoder)
{
    return std::make_unique<FluxSectorInterface>(
        diskLayout, fluxSource, fluxSink, encoder, decoder);
}
