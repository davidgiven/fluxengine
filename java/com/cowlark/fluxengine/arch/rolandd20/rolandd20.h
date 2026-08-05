#pragma once

extern std::unique_ptr<Decoder> createRolandD20Decoder(
    const DecoderProto& config);
