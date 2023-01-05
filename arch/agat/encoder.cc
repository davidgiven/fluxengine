#include "globals.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "agat.h"
#include "crc.h"
#include "readerwriter.h"
#include "image.h"
#include "arch/agat/agat.pb.h"
#include "lib/encoders/encoders.pb.h"

class AgatEncoder : public Encoder
{
public:
    AgatEncoder(const EncoderProto& config):
        Encoder(config),
        _config(config.agat())
    {
    }

public:
    std::unique_ptr<Fluxmap> encode(std::shared_ptr<const TrackInfo>& trackInfo,
        const std::vector<std::shared_ptr<const Sector>>& sectors,
        const Image& image) override
    {
        Error() << "unimplemented";
    }

private:
    const AgatEncoderProto& _config;
};


std::unique_ptr<Encoder> createAgatEncoder(const EncoderProto& config)
{
    return std::unique_ptr<Encoder>(new AgatEncoder(config));
}

