#ifndef ENCODERS_H
#define ENCODERS_H

class Fluxmap;
class EncoderProto;
class Image;
class Sector;

class AbstractEncoder
{
public:
    AbstractEncoder(const EncoderProto& config) {}
	virtual ~AbstractEncoder() {}

	static std::unique_ptr<AbstractEncoder> create(const EncoderProto& config);

public:
	virtual std::vector<std::shared_ptr<Sector>> collectSectors(
		int physicalCylinder, int physicalHead, const Image& image) = 0;

	virtual std::unique_ptr<Fluxmap> encode(
		int physicalCylinder, int physicalHead, const std::vector<std::shared_ptr<Sector>>& sectors, const Image& image) = 0;
};

#endif

