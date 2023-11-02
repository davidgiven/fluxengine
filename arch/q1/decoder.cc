#include "lib/globals.h"
#include "lib/fluxmap.h"
#include "lib/decoders/fluxmapreader.h"
#include "lib/decoders/decoders.h"
#include "lib/sector.h"
#include "arch/q1/q1.h"
#include "lib/bytes.h"
#include "lib/decoders/decoders.pb.h"
#include "fmt/format.h"

static const FluxPattern ADDRESS_RECORD(32, Q1_ADDRESS_RECORD);
static const FluxPattern DATA_RECORD(32, Q1_DATA_RECORD);

const FluxMatchers ANY_RECORD_PATTERN({&ADDRESS_RECORD, &DATA_RECORD});

class Q1Decoder : public Decoder
{
public:
    Q1Decoder(const DecoderProto& config): Decoder(config), _config(config.q1())
    {
    }

    /* Search for FM or MFM sector record */
    nanoseconds_t advanceToNextRecord() override
    {
        return seekToPattern(ANY_RECORD_PATTERN);
    }

    void decodeSectorRecord() override {}

private:
    const Q1DecoderProto& _config;
};

std::unique_ptr<Decoder> createQ1Decoder(const DecoderProto& config)
{
    return std::unique_ptr<Decoder>(new Q1Decoder(config));
}
