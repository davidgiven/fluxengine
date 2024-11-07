#ifndef IMAGEWRITER_H
#define IMAGEWRITER_H

class ImageWriterProto;
class Image;
class Config;

class ImageWriter
{
public:
    ImageWriter(const ImageWriterProto& config);
    virtual ~ImageWriter(){};

public:
    static std::unique_ptr<ImageWriter> create(Config& config);
    static std::unique_ptr<ImageWriter> create(const ImageWriterProto& config);

    static std::unique_ptr<ImageWriter> createD64ImageWriter(
        const ImageWriterProto& config);
    static std::unique_ptr<ImageWriter> createDiskCopyImageWriter(
        const ImageWriterProto& config);
    static std::unique_ptr<ImageWriter> createDSKImageWriter(
        const ImageWriterProto& config);
    static std::unique_ptr<ImageWriter> createImgImageWriter(
        const ImageWriterProto& config);
    static std::unique_ptr<ImageWriter> createLDBSImageWriter(
        const ImageWriterProto& config);
    static std::unique_ptr<ImageWriter> createNsiImageWriter(
        const ImageWriterProto& config);
    static std::unique_ptr<ImageWriter> createRawImageWriter(
        const ImageWriterProto& config);
    static std::unique_ptr<ImageWriter> createD88ImageWriter(
        const ImageWriterProto& config);
    static std::unique_ptr<ImageWriter> createImdImageWriter(
        const ImageWriterProto& config);

public:
    void printMap(const Image& sectors);
    void writeCsv(const Image& sectors, const std::string& filename);

    /* Writes a raw image. */
    virtual void writeImage(const Image& sectors) = 0;

    /* Writes an image applying any optional mapping from disk sector numbering
     * to filesystem sector numbering. */
    void writeMappedImage(const Image& sectors);

protected:
    const ImageWriterProto& _config;
};

#endif
