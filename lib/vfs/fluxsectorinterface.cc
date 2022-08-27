#include "lib/globals.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/image.h"
#include "lib/readerwriter.h"
#include "lib/decoders/decoders.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/mapper.h"

class FluxSectorInterface : public SectorInterface
{
public:
    FluxSectorInterface(std::shared_ptr<FluxSource> fluxSource,
        std::shared_ptr<AbstractDecoder> decoder):
        _fluxSource(fluxSource),
        _decoder(decoder)
    {
    }

public:
    std::shared_ptr<const Sector> get(
        unsigned track, unsigned side, unsigned sectorId)
    {
        trackid_t trackid(track, side);
        if (_loadedtracks.find(trackid) == _loadedtracks.end())
			populateSectors(track, side);

		return _sectorstore.get(track, side, sectorId);
    }

    std::shared_ptr<Sector> put(
        unsigned track, unsigned side, unsigned sectorId)
    {
        trackid_t trackid(track, side);
        if (_loadedtracks.find(trackid) == _loadedtracks.end())
			populateSectors(track, side);

		_changedtracks.insert(trackid);
		return _sectorstore.put(track, side, sectorId);
    }

private:
    void populateSectors(unsigned track, unsigned side)
    {
		auto location = Mapper::computeLocationFor(track, side);
        auto trackdata =
            readAndDecodeTrack(*_fluxSource, *_decoder, location);

		for (const auto& sector : trackdata->sectors)
			*_sectorstore.put(track, side, sector->logicalSector) = *sector;
		_loadedtracks.insert(trackid_t(track, side));
    }

    std::shared_ptr<FluxSource> _fluxSource;
    std::shared_ptr<AbstractDecoder> _decoder;

    typedef std::pair<unsigned, unsigned> trackid_t;
    Image _sectorstore;
    std::set<trackid_t> _loadedtracks;
    std::set<trackid_t> _changedtracks;
};

std::unique_ptr<SectorInterface> SectorInterface::createFluxSectorInterface(
    std::shared_ptr<FluxSource> fluxSource, std::shared_ptr<AbstractDecoder> decoder)
{
    return std::make_unique<FluxSectorInterface>(fluxSource, decoder);
}
