#ifndef READER_H
#define READER_H

class Fluxmap;
class BitmapDecoder;
class RecordParser;

class ReaderTrack
{
public:
    virtual ~ReaderTrack() {}

    int drive;
    int track;
    int side;

    std::unique_ptr<Fluxmap> read();
    virtual std::unique_ptr<Fluxmap> reallyRead() = 0;
    virtual void recalibrate() = 0;
};

class CapturedReaderTrack : public ReaderTrack
{
public:
    std::unique_ptr<Fluxmap> reallyRead();
    void recalibrate();
};

class FileReaderTrack : public ReaderTrack
{
public:
    std::unique_ptr<Fluxmap> reallyRead();
    void recalibrate();
};

extern void setReaderDefaultSource(const std::string& source);
extern void setReaderRevolutions(int revolutions);
extern std::vector<std::unique_ptr<ReaderTrack>> readTracks();

extern void readDiskCommand(
    const BitmapDecoder& bitmapDecoder, const RecordParser& recordParser,
    const std::string& outputFilename);

#endif
