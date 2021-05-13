#ifndef IMAGEREADER_H
#define IMAGEREADER_H

class SectorSet;
class ImageSpec;
class InputFileProto;

class ImageReader
{
public:
	ImageReader(const InputFileProto& config);
	virtual ~ImageReader() {};

public:
    static std::unique_ptr<ImageReader> create(const InputFileProto& config);

public:
    static std::unique_ptr<ImageReader> createDiskCopyImageReader(const InputFileProto& config);
    static std::unique_ptr<ImageReader> createImgImageReader(const InputFileProto& config);
    static std::unique_ptr<ImageReader> createJv3ImageReader(const InputFileProto& config);
    static std::unique_ptr<ImageReader> createIMDImageReader(const InputFileProto& config);

public:
	virtual SectorSet readImage() = 0;

protected:
	const InputFileProto& _config;
};

#endif

