#include "lib/globals.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/image.h"

class ImageSectorInterface : public SectorInterface
{
public:
	ImageSectorInterface(std::shared_ptr<Image> image):
		_image(image)
	{}

public:
	std::shared_ptr<const Sector> get(unsigned track, unsigned side, unsigned sectorId)
	{
		return _image->get(track, side, sectorId);
	}

	std::shared_ptr<Sector> put(unsigned track, unsigned side, unsigned sectorId)
	{
		return _image->put(track, side, sectorId);
	}

private:
	std::shared_ptr<Image> _image;
};

std::unique_ptr<SectorInterface> SectorInterface::createImageSectorInterface(std::shared_ptr<Image> image)
{
	return std::make_unique<ImageSectorInterface>(image);
}

