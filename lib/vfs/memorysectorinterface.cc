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
    MemorySectorInterface(Image* image): _backupImage(image)
    {
        discardChanges();
    }

public:
    const Sector* get(unsigned track, unsigned side, unsigned sectorId) override
    {
        return _liveImage->get(track, side, sectorId);
    }

    Sector* put(unsigned track, unsigned side, unsigned sectorId) override
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
        _liveImage = new Image();
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
    Image* _backupImage = nullptr;
    Image* _liveImage = nullptr;
    bool _changed = false;
};

SectorInterface* SectorInterface::createMemorySectorInterface(Image* image)
{
    return new MemorySectorInterface(image);
}
