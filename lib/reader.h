#ifndef READER_H
#define READER_H

#include "flags.h"

class AbstractDecoder;
class FluxSource;
class Fluxmap;
class ImageWriter;
class Track;

extern FlagGroup readerFlags;

extern void setReaderDefaultSource(const std::string& source);
extern void setReaderDefaultOutput(const std::string& output);
extern void setReaderRevolutions(int revolutions);
extern void setReaderHardSectorCount(int sectorCount);

extern std::vector<std::unique_ptr<Track>> readTracks();

extern void readDiskCommand(FluxSource& source, AbstractDecoder& decoder, ImageWriter& writer);

#endif
