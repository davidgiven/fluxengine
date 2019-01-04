#ifndef READER_H
#define READER_H

class Fluxmap;

class ReaderTrack
{
public:
    virtual ~ReaderTrack() {}

    int track;
    int side;

    Fluxmap& read();
    void forceReread();
    virtual void reallyRead() = 0;

protected:
    bool _read = false;
    std::unique_ptr<Fluxmap> _fluxmap;    
};

class CapturedReaderTrack : public ReaderTrack
{
public:
    void reallyRead();
};

class FileReaderTrack : public ReaderTrack
{
public:
    void reallyRead();
};

extern void setReaderDefaults(int minTrack, int maxTrack, int minSide, int maxSide);
extern std::vector<std::unique_ptr<ReaderTrack>> readTracks();

#endif
