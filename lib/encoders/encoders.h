#ifndef ENCODERS_H
#define ENCODERS_H

class EncoderProto;
class Fluxmap;
class Image;
class Layout;
class Sector;
class TrackInfo;
class Config;

class Encoder
{
public:
    Encoder(const EncoderProto& config) {}
    virtual ~Encoder() {}

    static std::unique_ptr<Encoder> create(Config& config);

public:
    virtual std::shared_ptr<const Sector> getSector(
        std::shared_ptr<const TrackInfo>&,
        const Image& image,
        unsigned sectorId);

    virtual std::vector<std::shared_ptr<const Sector>> collectSectors(
        std::shared_ptr<const TrackInfo>&, const Image& image);

    virtual std::unique_ptr<Fluxmap> encode(
        std::shared_ptr<const TrackInfo>& trackInfo,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) = 0;

    nanoseconds_t calculatePhysicalClockPeriod(
        nanoseconds_t targetClockPeriod, nanoseconds_t targetRotationalPeriod);
};

#endif
