#ifndef READER_H
#define READER_H

class Fluxmap;
class FluxSource;
class AbstractDecoder;

extern void setReaderDefaultSource(const std::string& source);
extern void setReaderRevolutions(int revolutions);

class Track
{
public:
    Track(std::shared_ptr<FluxSource>& FluxSource, unsigned track, unsigned side):
        track(track),
        side(side),
        _FluxSource(FluxSource)
    {}

public:
    std::unique_ptr<Fluxmap> read();
    void recalibrate();
    bool retryable();

    unsigned track;
    unsigned side;

private:
    std::shared_ptr<FluxSource> _FluxSource;
};

extern std::vector<std::unique_ptr<Track>> readTracks();

extern void readDiskCommand(AbstractDecoder& decoder, const std::string& outputFilename);

#endif
