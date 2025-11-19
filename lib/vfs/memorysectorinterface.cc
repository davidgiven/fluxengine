#include "lib/core/globals.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/data/image.h"
#include "lib/data/layout.h"
#include "lib/data/sector.h"
#include "lib/core/bytes.h"

class MemorySectorInterface : public SectorInterface
{
public:
    MemorySectorInterface(std::shared_ptr<Image> image): _backupImage(image)
    {
        discardChanges();
    }

public:
    std::shared_ptr<const Sector> get(
        unsigned track, unsigned side, unsigned sectorId) override
    {
        return _liveImage->get(track, side, sectorId);
    }

    std::shared_ptr<Sector> put(
        unsigned track, unsigned side, unsigned sectorId) override
    {
        _changed = true;
        return _liveImage->put(track, side, sectorId);
    }

    bool isReadOnly() override
    {
        return false;
    }

    bool needsFlushing() override
    {
        return _changed;
    }

    void flushChanges() override
    {
        for (const auto& sector : *_liveImage)
        {
            const auto& s = _backupImage->put(sector->logicalCylinder,
                sector->logicalHead,
                sector->logicalSector);
            *s = *sector;
        }
        _changed = false;
    }

    void discardChanges() override
    {
        _liveImage = std::make_unique<Image>();
        for (auto sector : *_backupImage)
        {
            auto s = _liveImage->put(sector->logicalCylinder,
                sector->logicalHead,
                sector->logicalSector);
            *s = *sector;
        }
        _changed = false;
    }

private:
    std::shared_ptr<Image> _backupImage;
    std::shared_ptr<Image> _liveImage;
    bool _changed = false;
};

std::unique_ptr<SectorInterface> SectorInterface::createMemorySectorInterface(
    std::shared_ptr<Image> image)
{
    return std::make_unique<MemorySectorInterface>(image);
}
