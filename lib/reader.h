#ifndef READER_H
#define READER_H

class Fluxmap;
class FluxReader;
class AbstractDecoder;

extern void setReaderDefaultSource(const std::string& source);
extern void setReaderRevolutions(int revolutions);

class Track
{
public:
    Track(std::shared_ptr<FluxReader>& fluxReader, unsigned track, unsigned side):
        track(track),
        side(side),
        _fluxReader(fluxReader)
    {}

public:
    std::unique_ptr<Fluxmap> read();
    void recalibrate();

    unsigned track;
    unsigned side;

private:
    std::shared_ptr<FluxReader> _fluxReader;
};

extern std::vector<std::unique_ptr<Track>> readTracks();

extern void readDiskCommand(AbstractDecoder& decoder, const std::string& outputFilename);

#endif
