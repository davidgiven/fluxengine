#ifndef READER_H
#define READER_H

class AbstractDecoder;
class FluxSource;
class Fluxmap;
class ImageWriter;
class Track;

extern std::vector<std::unique_ptr<Track>> readTracks();

extern void readDiskCommand(FluxSource& source, AbstractDecoder& decoder, ImageWriter& writer);

#endif
