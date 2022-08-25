#ifndef SECTORINTERFACE_H
#define SECTORINTERFACE_H

class Image;

class SectorInterface
{
public:
	virtual std::shared_ptr<const Sector> get(unsigned track, unsigned side, unsigned sectorId) = 0;
	virtual std::shared_ptr<Sector> put(unsigned track, unsigned side, unsigned sectorId) = 0;

	virtual void flush() {}
};

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

#endif

