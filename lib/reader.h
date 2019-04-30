#ifndef READER_H
#define READER_H

class Fluxmap;
class FluxSource;
class AbstractDecoder;
class Track;

extern void setReaderDefaultSource(const std::string& source);
extern void setReaderRevolutions(int revolutions);

extern std::vector<std::unique_ptr<Track>> readTracks();

extern void readDiskCommand(AbstractDecoder& decoder, const std::string& outputFilename);

#endif
