#include "lib/core/globals.h"
#include "lib/encoders/encoders.h"
#include "lib/decoders/decoders.h"
#include "lib/config/config.h"
#include "arch/agat/agat.h"
#include "arch/aeslanier/aeslanier.h"
#include "arch/amiga/amiga.h"
#include "arch/apple2/apple2.h"
#include "arch/brother/brother.h"
#include "arch/c64/c64.h"
#include "arch/f85/f85.h"
#include "arch/fb100/fb100.h"
#include "arch/ibm/ibm.h"
#include "arch/macintosh/macintosh.h"
#include "arch/micropolis/micropolis.h"
#include "arch/mx/mx.h"
#include "arch/northstar/northstar.h"
#include "arch/rolandd20/rolandd20.h"
#include "arch/smaky6/smaky6.h"
#include "arch/tartu/tartu.h"
#include "arch/tids990/tids990.h"
#include "arch/victor9k/victor9k.h"
#include "arch/zilogmcz/zilogmcz.h"
#include "arch/arch.h"

std::unique_ptr<Encoder> Arch::createEncoder(Config& config)
{
    if (!config.hasEncoder())
        error("no encoder configured");
    return createEncoder(config->encoder());
}

std::unique_ptr<Encoder> Arch::createEncoder(const EncoderProto& config)
{
    static const std::map<int,
        std::function<std::unique_ptr<Encoder>(const EncoderProto&)>>
        encoders = {
            {EncoderProto::kAgat,       createAgatEncoder       },
            {EncoderProto::kAmiga,      createAmigaEncoder      },
            {EncoderProto::kApple2,     createApple2Encoder     },
            {EncoderProto::kBrother,    createBrotherEncoder    },
            {EncoderProto::kC64,        createCommodore64Encoder},
            {EncoderProto::kIbm,        createIbmEncoder        },
            {EncoderProto::kMacintosh,  createMacintoshEncoder  },
            {EncoderProto::kMicropolis, createMicropolisEncoder },
            {EncoderProto::kNorthstar,  createNorthstarEncoder  },
            {EncoderProto::kTartu,      createTartuEncoder      },
            {EncoderProto::kTids990,    createTids990Encoder    },
            {EncoderProto::kVictor9K,   createVictor9kEncoder   },
    };

    auto encoder = encoders.find(config.format_case());
    if (encoder == encoders.end())
        error("no encoder specified");

    return (encoder->second)(config);
}

std::unique_ptr<Decoder> Arch::createDecoder(Config& config)
{
    if (!config.hasDecoder())
        error("no decoder configured");
    return createDecoder(config->decoder());
}

std::unique_ptr<Decoder> Arch::createDecoder(const DecoderProto& config)
{
    static const std::map<int,
        std::function<std::unique_ptr<Decoder>(const DecoderProto&)>>
        decoders = {
            {DecoderProto::kAgat,       createAgatDecoder       },
            {DecoderProto::kAeslanier,  createAesLanierDecoder  },
            {DecoderProto::kAmiga,      createAmigaDecoder      },
            {DecoderProto::kApple2,     createApple2Decoder     },
            {DecoderProto::kBrother,    createBrotherDecoder    },
            {DecoderProto::kC64,        createCommodore64Decoder},
            {DecoderProto::kF85,        createDurangoF85Decoder },
            {DecoderProto::kFb100,      createFb100Decoder      },
            {DecoderProto::kIbm,        createIbmDecoder        },
            {DecoderProto::kMacintosh,  createMacintoshDecoder  },
            {DecoderProto::kMicropolis, createMicropolisDecoder },
            {DecoderProto::kMx,         createMxDecoder         },
            {DecoderProto::kNorthstar,  createNorthstarDecoder  },
            {DecoderProto::kRolandd20,  createRolandD20Decoder  },
            {DecoderProto::kSmaky6,     createSmaky6Decoder     },
            {DecoderProto::kTartu,      createTartuDecoder      },
            {DecoderProto::kTids990,    createTids990Decoder    },
            {DecoderProto::kVictor9K,   createVictor9kDecoder   },
            {DecoderProto::kZilogmcz,   createZilogMczDecoder   },
    };

    auto decoder = decoders.find(config.format_case());
    if (decoder == decoders.end())
        error("no decoder specified");

    return (decoder->second)(config);
}
