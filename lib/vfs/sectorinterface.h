#ifndef SECTORINTERFACE_H
#define SECTORINTERFACE_H

class Image;
class ImageReader;
class ImageWriter;
class Sector;
class FluxSource;
class FluxSinkFactory;
class Decoder;
class DiskLayout;
class Encoder;

class SectorInterface
{
public:
    virtual ~SectorInterface() {}

public:
    virtual const Sector* get(
        unsigned track, unsigned side, unsigned sectorId) = 0;
    virtual Sector* put(unsigned track, unsigned side, unsigned sectorId) = 0;

    virtual bool isReadOnly()
    {
        return true;
    }

    virtual bool needsFlushing()
    {
        return false;
    }

    virtual void flushChanges() {}

    virtual void discardChanges() {}

public:
    static std::unique_ptr<SectorInterface> createMemorySectorInterface(
        std::shared_ptr<Image> image);
    static std::unique_ptr<SectorInterface> createImageSectorInterface(
        const DiskLayout* diskLayout,
        std::shared_ptr<ImageReader> reader,
        std::shared_ptr<ImageWriter> writer);
    static std::unique_ptr<SectorInterface> createFluxSectorInterface(
        const DiskLayout* diskLayout,
        FluxSource* fluxSource,
        FluxSinkFactory* fluxSinkFactory,
        Encoder* encoder,
        Decoder* decoder);
};

#endif
