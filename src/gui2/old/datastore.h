#pragma once

#include "lib/vfs/sectorinterface.h"

class Datastore : public SectorInterface, public QObject
{
    W_OBJECT(Datastore);

public:
    virtual void setDiskData(std::shared_ptr<const DiskFlux> diskData) = 0;
    W_SLOT(setDiskData)

    virtual void setTrackData(std::shared_ptr<const TrackFlux> trackData) = 0;
    W_SLOT(setTrackData)

    virtual void setImageData(std::shared_ptr<const Image> imageData) = 0;
    W_SLOT(setImageData)

    virtual void clear() = 0;
    W_SLOT(clear)

public:
    static Datastore* create(
        std::shared_ptr<Encoder>& encoder, std::shared_ptr<Decoder>& decoder);
};
