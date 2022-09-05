#ifndef SECTORINTERFACE_H
#define SECTORINTERFACE_H

class Image;
class Sector;
class FluxSource;
class FluxSink;
class AbstractDecoder;
class AbstractEncoder;

class SectorInterface
{
public:
    virtual std::shared_ptr<const Sector> get(
        unsigned track, unsigned side, unsigned sectorId) = 0;
    virtual std::shared_ptr<Sector> put(
        unsigned track, unsigned side, unsigned sectorId) = 0;

    virtual bool needsFlushing()
    {
        return false;
    }

    virtual void flushChanges() {}

    virtual void discardChanges() {}

public:
    static std::unique_ptr<SectorInterface> createImageSectorInterface(
        std::shared_ptr<Image> image);
    static std::unique_ptr<SectorInterface> createFluxSectorInterface(
        std::shared_ptr<FluxSource> fluxSource,
        std::shared_ptr<FluxSink> fluxSink,
        std::shared_ptr<AbstractEncoder> encoder,
        std::shared_ptr<AbstractDecoder> decoder);
};

#endif
