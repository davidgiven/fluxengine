#ifndef IMAGEREADER_H
#define IMAGEREADER_H

class SectorSet;
class ImageSpec;
class Config_InputFile;

class ImageReader
{
public:
	ImageReader(const Config_InputFile& config);
	virtual ~ImageReader() {};

public:
    static std::unique_ptr<ImageReader> create(const Config_InputFile& config);

public:
    static std::unique_ptr<ImageReader> createDiskCopyImageReader(const Config_InputFile& config);
    static std::unique_ptr<ImageReader> createImgImageReader(const Config_InputFile& config);
    static std::unique_ptr<ImageReader> createJv3ImageReader(const Config_InputFile& config);
    static std::unique_ptr<ImageReader> createIMDImageReader(const Config_InputFile& config);

public:
	virtual SectorSet readImage() = 0;

protected:
	const Config_InputFile& _config;
};

#endif

