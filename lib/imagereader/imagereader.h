#ifndef IMAGEREADER_H
#define IMAGEREADER_H

class SectorSet;
class ImageSpec;

class ImageReader
{
public:
	ImageReader(const ImageSpec& spec);
	virtual ~ImageReader() {};

public:
    static std::unique_ptr<ImageReader> create(const ImageSpec& spec);

private:
    static std::unique_ptr<ImageReader> createImgImageReader(const ImageSpec& spec);

public:
	virtual SectorSet readImage() = 0;

protected:
	ImageSpec spec;
};

#endif

