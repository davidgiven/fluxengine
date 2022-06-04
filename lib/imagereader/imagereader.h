#ifndef IMAGEREADER_H
#define IMAGEREADER_H

#include "lib/image.h"

class ImageSpec;
class ImageReaderProto;

class ImageReader
{
public:
	ImageReader(const ImageReaderProto& config);
	virtual ~ImageReader() {};

public:
    static std::unique_ptr<ImageReader> create(const ImageReaderProto& config);
	static void updateConfigForFilename(ImageReaderProto* proto, const std::string& filename);

public:
    static std::unique_ptr<ImageReader> createD64ImageReader(const ImageReaderProto& config);
    static std::unique_ptr<ImageReader> createDiskCopyImageReader(const ImageReaderProto& config);
    static std::unique_ptr<ImageReader> createImgImageReader(const ImageReaderProto& config);
    static std::unique_ptr<ImageReader> createJv3ImageReader(const ImageReaderProto& config);
    static std::unique_ptr<ImageReader> createIMDImageReader(const ImageReaderProto& config);
    static std::unique_ptr<ImageReader> createNsiImageReader(const ImageReaderProto& config);
    static std::unique_ptr<ImageReader> createTd0ImageReader(const ImageReaderProto& config);
    static std::unique_ptr<ImageReader> createDimImageReader(const ImageReaderProto& config);
    static std::unique_ptr<ImageReader> createFdiImageReader(const ImageReaderProto& config);
    static std::unique_ptr<ImageReader> createD88ImageReader(const ImageReaderProto& config);
    static std::unique_ptr<ImageReader> createNFDImageReader(const ImageReaderProto& config);

public:
	virtual std::unique_ptr<Image> readImage() = 0;

protected:
	const ImageReaderProto& _config;
};

#endif

