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
	static void verifyImageSpec(const ImageSpec& spec);

private:
	typedef 
		std::function<
			std::unique_ptr<ImageReader>(const ImageSpec& spec)
		>
		Constructor;

	static std::map<std::string, Constructor> formats;

    static std::unique_ptr<ImageReader> createImgImageReader(const ImageSpec& spec);

	static Constructor findConstructor(const ImageSpec& spec);

public:
	virtual SectorSet readImage() = 0;

protected:
	ImageSpec spec;
};

#endif

