#ifndef READER_H
#define READER_H

class Fluxmap;

class Track
{
public:
    virtual ~Track() {}

    int track;
    int side;

    Fluxmap& read();
    void forceReread();
    virtual void reallyRead() = 0;

protected:
    bool _read = false;
    std::unique_ptr<Fluxmap> _fluxmap;    
};

class CapturedTrack : public Track
{
public:
    void reallyRead();
};

class FileTrack : public Track
{
public:
    void reallyRead();
};

extern void setReaderDefaults(int minTrack, int maxTrack, int minSide, int maxSide);
extern std::vector<std::unique_ptr<Track>> readTracks();

#endif
