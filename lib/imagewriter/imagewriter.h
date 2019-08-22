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
	static void verifyImageSpec(const ImageSpec& filename);

private:
	typedef 
		std::function<
			std::unique_ptr<ImageWriter>(const SectorSet& sectors, const ImageSpec& spec)
		>
		Constructor;

	static std::map<std::string, Constructor> formats;

    static std::unique_ptr<ImageWriter> createImgImageWriter(
		const SectorSet& sectors, const ImageSpec& spec);
    static std::unique_ptr<ImageWriter> createLDBSImageWriter(
		const SectorSet& sectors, const ImageSpec& spec);
    static std::unique_ptr<ImageWriter> createD64ImageWriter(
		const SectorSet& sectors, const ImageSpec& spec);

	static Constructor findConstructor(const ImageSpec& spec);

public:
	virtual void adjustGeometry();
	void printMap();
	virtual void writeImage() = 0;

protected:
	const SectorSet& sectors;
	ImageSpec spec;
};

#endif
