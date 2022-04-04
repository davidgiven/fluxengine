#ifndef ENCODERS_H
#define ENCODERS_H

class EncoderProto;
class Fluxmap;
class Image;
class Location;
class Sector;

class AbstractEncoder
{
public:
    AbstractEncoder(const EncoderProto& config) {}
    virtual ~AbstractEncoder() {}

    static std::unique_ptr<AbstractEncoder> create(const EncoderProto& config);

public:
    virtual std::vector<std::shared_ptr<const Sector>> collectSectors(
        const Location& location, const Image& image) = 0;

    virtual std::unique_ptr<Fluxmap> encode(const Location& location,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) = 0;
};

#endif
