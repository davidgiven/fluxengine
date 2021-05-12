#ifndef IMAGEWRITER_H
#define IMAGEWRITER_H

class SectorSet;
class Config_OutputFile;

class ImageWriter
{
public:
	ImageWriter(const Config_OutputFile& config);
	virtual ~ImageWriter() {};

public:
    static std::unique_ptr<ImageWriter> create(const Config_OutputFile& config);

    static std::unique_ptr<ImageWriter> createImgImageWriter(
		const Config_OutputFile& config);
    static std::unique_ptr<ImageWriter> createLDBSImageWriter(
		const Config_OutputFile& config);
    static std::unique_ptr<ImageWriter> createD64ImageWriter(
		const Config_OutputFile& config);
    static std::unique_ptr<ImageWriter> createDiskCopyImageWriter(
		const Config_OutputFile& config);

public:
	void printMap(const SectorSet& sectors);
	void writeCsv(const SectorSet& sectors, const std::string& filename);
	virtual void writeImage(const SectorSet& sectors) = 0;

protected:
	const Config_OutputFile& _config;
};

#endif
