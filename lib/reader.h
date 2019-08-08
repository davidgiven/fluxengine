#ifndef READER_H
#define READER_H

#include "flags.h"

class Fluxmap;
class FluxSource;
class AbstractDecoder;
class Track;

extern FlagGroup readerFlags;

extern void setReaderDefaultSource(const std::string& source);
extern void setReaderDefaultOutput(const std::string& output);
extern void setReaderRevolutions(int revolutions);

extern std::vector<std::unique_ptr<Track>> readTracks();

extern void readDiskCommand(AbstractDecoder& decoder);

#endif
