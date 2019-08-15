#ifndef IMAGEWRITER_H
#define IMAGEWRITER_H

class SectorSet;
class ImageSpec;

class ImageWriter
{
public:
	ImageWriter(const SectorSet& sectors, const ImageSpec& spec);
	virtual ~ImageWriter() {};

public:
    static std::unique_ptr<ImageWriter> create(const SectorSet& sectors, const ImageSpec& spec);

private:
    static std::unique_ptr<ImageWriter> createImgImageWriter(
		const SectorSet& sectors, const ImageSpec& spec);
    static std::unique_ptr<ImageWriter> createLDBSImageWriter(
		const SectorSet& sectors, const ImageSpec& spec);
    static std::unique_ptr<ImageWriter> createD64ImageWriter(
		const SectorSet& sectors, const ImageSpec& spec);

public:
	virtual void adjustGeometry();
	void printMap();
	virtual void writeImage() = 0;

protected:
	const SectorSet& sectors;
	ImageSpec spec;
};

#endif
