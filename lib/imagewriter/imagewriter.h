#ifndef IMAGEWRITER_H
#define IMAGEWRITER_H

class SectorSet;
class OutputFileProto;

class ImageWriter
{
public:
	ImageWriter(const OutputFileProto& config);
	virtual ~ImageWriter() {};

public:
    static std::unique_ptr<ImageWriter> create(const OutputFileProto& config);

    static std::unique_ptr<ImageWriter> createImgImageWriter(
		const OutputFileProto& config);
    static std::unique_ptr<ImageWriter> createLDBSImageWriter(
		const OutputFileProto& config);
    static std::unique_ptr<ImageWriter> createD64ImageWriter(
		const OutputFileProto& config);
    static std::unique_ptr<ImageWriter> createDiskCopyImageWriter(
		const OutputFileProto& config);

public:
	void printMap(const SectorSet& sectors);
	void writeCsv(const SectorSet& sectors, const std::string& filename);
	virtual void writeImage(const SectorSet& sectors) = 0;

protected:
	const OutputFileProto& _config;
};

#endif
