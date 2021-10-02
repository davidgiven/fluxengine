#ifndef WRITER_H
#define WRITER_H

class Fluxmap;
class AbstractDecoder;
class AbstractEncoder;
class ImageReader;
class FluxSource;
class FluxSink;

extern void writeTracks(FluxSink& fluxSink, const std::function<std::unique_ptr<Fluxmap>(int track, int side)> producer);

extern void fillBitmapTo(std::vector<bool>& bitmap,
		unsigned& cursor, unsigned terminateAt,
		const std::vector<bool>& pattern);
	
extern void writeDiskCommand(ImageReader& imageReader, AbstractEncoder& encoder, FluxSink& fluxSink,
		AbstractDecoder* decoder = nullptr, FluxSource* fluxSource = nullptr);
extern void writeRawDiskCommand(FluxSource& fluxSource, FluxSink& fluxSink);

#endif
