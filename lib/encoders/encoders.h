#ifndef ENCODERS_H
#define ENCODERS_H

class Config;
class CylinderHead;
class EncoderProto;
class Fluxmap;
class Image;
class Layout;
class LogicalTrackLayout;
class Sector;

class Encoder
{
public:
    Encoder(const EncoderProto& config) {}
    virtual ~Encoder() {}

    static std::unique_ptr<Encoder> create(Config& config);

public:
    virtual std::shared_ptr<const Sector> getSector(
        const CylinderHead& ch, const Image& image, unsigned sectorId);

    virtual std::vector<std::shared_ptr<const Sector>> collectSectors(
        const LogicalTrackLayout& ltl, const Image& image);

    virtual std::unique_ptr<Fluxmap> encode(const LogicalTrackLayout& ltl,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) = 0;

    nanoseconds_t calculatePhysicalClockPeriod(
        nanoseconds_t targetClockPeriod, nanoseconds_t targetRotationalPeriod);
};

#endif
