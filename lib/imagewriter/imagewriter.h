#ifndef IMAGEWRITER_H
#define IMAGEWRITER_H

class SectorSet;
class ImageWriterProto;

class ImageWriter
{
public:
	ImageWriter(const ImageWriterProto& config);
	virtual ~ImageWriter() {};

public:
    static std::unique_ptr<ImageWriter> create(const ImageWriterProto& config);
	static void updateConfigForFilename(ImageWriterProto* proto, const std::string& filename);

    static std::unique_ptr<ImageWriter> createImgImageWriter(
		const ImageWriterProto& config);
    static std::unique_ptr<ImageWriter> createLDBSImageWriter(
		const ImageWriterProto& config);
    static std::unique_ptr<ImageWriter> createD64ImageWriter(
		const ImageWriterProto& config);
    static std::unique_ptr<ImageWriter> createDiskCopyImageWriter(
		const ImageWriterProto& config);

public:
	void printMap(const SectorSet& sectors);
	void writeCsv(const SectorSet& sectors, const std::string& filename);
	virtual void writeImage(const SectorSet& sectors) = 0;

protected:
	const ImageWriterProto& _config;
};

#endif
