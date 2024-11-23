#include "lib/core/globals.h"
#include "lib/data/image.h"
#include "lib/data/flux.h"
#include "lib/data/layout.h"
#include "globals.h"
#include "datastore.h"

class DatastoreImpl : public Datastore
{
    W_OBJECT(DatastoreImpl);

public:
    DatastoreImpl(
        std::shared_ptr<Encoder>& encoder, std::shared_ptr<Decoder>& decoder):
        _encoder(encoder),
        _decoder(decoder)
    {
    }

public:
    void setDiskData(std::shared_ptr<const DiskFlux> diskData) override {}

    void setTrackData(std::shared_ptr<const TrackFlux> trackData) override
    {
        key_t key = {trackData->trackInfo->physicalTrack,
            trackData->trackInfo->physicalSide};
        _tracks[key] = trackData;
    }

    void setImageData(std::shared_ptr<const Image> imageData) override
    {
        clear();
        _loadedSectors = *imageData;
    }

    void clear() override
    {
        _loadedSectors.clear();
        _changedSectors.clear();
        _tracks.clear();
    }

public:
    std::shared_ptr<const Sector> get(
        unsigned track, unsigned side, unsigned sectorId) override
    {
        auto it = _changedSectors.get(track, side, sectorId);
        if (it)
            return it;

        return _loadedSectors.get(track, side, sectorId);
    }

    std::shared_ptr<Sector> put(
        unsigned track, unsigned side, unsigned sectorId) override
    {
        return _changedSectors.put(track, side, sectorId);
    }

    virtual bool isReadOnly() override
    {
        return false;
    }

    bool needsFlushing() override
    {
        return !_changedSectors.empty();
    }

private:
    std::shared_ptr<Encoder> _encoder;
    std::shared_ptr<Decoder> _decoder;
    Image _loadedSectors;
    Image _changedSectors;

    typedef std::pair<unsigned, unsigned> key_t;
    std::map<key_t, std::shared_ptr<const TrackFlux>> _tracks;
};

W_OBJECT_IMPL(Datastore);
W_OBJECT_IMPL(DatastoreImpl);

Datastore* Datastore::create(
    std::shared_ptr<Encoder>& encoder, std::shared_ptr<Decoder>& decoder)
{
    return new DatastoreImpl(encoder, decoder);
}
