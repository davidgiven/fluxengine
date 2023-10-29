#ifndef SECTORINTERFACE_H
#define SECTORINTERFACE_H

class ImageReader;
class ImageWriter;
class Sector;
class FluxSource;
class FluxSink;
class Decoder;
class Encoder;

class SectorInterface
{
public:
    virtual ~SectorInterface() {}

public:
    virtual std::shared_ptr<const Sector> get(
        unsigned track, unsigned side, unsigned sectorId) = 0;
    virtual std::shared_ptr<Sector> put(
        unsigned track, unsigned side, unsigned sectorId) = 0;

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
    static std::unique_ptr<SectorInterface> createImageSectorInterface(
        std::shared_ptr<ImageReader> reader,
        std::shared_ptr<ImageWriter> writer);
    static std::unique_ptr<SectorInterface> createFluxSectorInterface(
        std::shared_ptr<FluxSource> fluxSource,
        std::shared_ptr<FluxSink> fluxSink,
        std::shared_ptr<Encoder> encoder,
        std::shared_ptr<Decoder> decoder);
};

#endif
