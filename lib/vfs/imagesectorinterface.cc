#include "lib/core/globals.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/data/image.h"
#include "lib/data/layout.h"
#include "lib/data/sector.h"
#include "lib/core/bytes.h"

class ImageSectorInterface : public SectorInterface
{
public:
    ImageSectorInterface(std::shared_ptr<ImageReader> reader,
        std::shared_ptr<ImageWriter> writer):
        _reader(reader),
        _writer(writer)
    {
        discardChanges();
    }

public:
    std::shared_ptr<const Sector> get(
        unsigned track, unsigned side, unsigned sectorId) override
    {
        return _image->get(track, side, sectorId);
    }

    std::shared_ptr<Sector> put(
        unsigned track, unsigned side, unsigned sectorId) override
    {
        _changed = true;
        return _image->put(track, side, sectorId);
    }

    virtual bool isReadOnly() override
    {
        return (_writer == nullptr);
    }

    bool needsFlushing() override
    {
        return _changed;
    }

    void flushChanges() override
    {
        _writer->writeMappedImage(*_image);
        _changed = false;
    }

    void discardChanges() override
    {
        if (_reader)
            _image = _reader->readMappedImage();
        else
        {
            _image = std::make_shared<Image>();
            _image->createBlankImage();
        }
        _changed = false;
    }

private:
    std::shared_ptr<Image> _image;
    std::shared_ptr<ImageReader> _reader;
    std::shared_ptr<ImageWriter> _writer;
    bool _changed = false;
};

std::unique_ptr<SectorInterface> SectorInterface::createImageSectorInterface(
    std::shared_ptr<ImageReader> reader, std::shared_ptr<ImageWriter> writer)
{
    return std::make_unique<ImageSectorInterface>(reader, writer);
}
