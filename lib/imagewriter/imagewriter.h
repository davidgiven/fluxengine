#ifndef IMAGEWRITER_H
#define IMAGEWRITER_H

class SectorSet;
class ImageWriterProto;
class AssemblingGeometryMapper;

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
    static std::unique_ptr<ImageWriter> createDiskCopyImageWriter(
		const ImageWriterProto& config);
    static std::unique_ptr<ImageWriter> createNsiImageWriter(
		const ImageWriterProto& config);

public:
	virtual void putBlock(size_t offset, size_t length, const Bytes& data) = 0;
	virtual const AssemblingGeometryMapper* getGeometryMapper() const { return nullptr; }

protected:
	const ImageWriterProto& _config;
};

#endif
