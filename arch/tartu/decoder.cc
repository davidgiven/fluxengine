#include "lib/globals.h"
#include "lib/decoders/decoders.h"
#include "arch/tartu/tartu.h"
#include "lib/crc.h"
#include "lib/fluxmap.h"
#include "lib/decoders/fluxmapreader.h"
#include "lib/sector.h"
#include <string.h>

class TartuDecoder : public Decoder
{
public:
    TartuDecoder(const DecoderProto& config):
        Decoder(config),
        _config(config.tartu())
    {
    }

    nanoseconds_t advanceToNextRecord() override
    {
        return 0;
    }

    void decodeSectorRecord() override
    {
    }

private:
    const TartuDecoderProto& _config;
};

std::unique_ptr<Decoder> createTartuDecoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new TartuDecoder(config));
}

