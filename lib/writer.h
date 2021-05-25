#ifndef WRITER_H
#define WRITER_H

class Fluxmap;
class AbstractEncoder;
class ImageReader;
class FluxSource;
class FluxSink;

extern void writeTracks(FluxSink& fluxSink, const std::function<std::unique_ptr<Fluxmap>(int track, int side)> producer);

extern void fillBitmapTo(std::vector<bool>& bitmap,
		unsigned& cursor, unsigned terminateAt,
		const std::vector<bool>& pattern);
	
extern void writeDiskCommand(AbstractEncoder& encoder, FluxSink& fluxSink);
extern void writeRawDiskCommand(FluxSource& fluxSource, FluxSink& fluxSink);

#endif
