#ifndef AESLANIER_H
#define AESLANIER_H

/* MFM:
 * 
 * Raw bits:
 *    5    5    5    5    5    1    2    2
 * 0101 0101 0101 0101 0101 0001 0010 0010
 * 0 0  0 0  0 0  0 0  0 0  0 0  0 1  0 1
 *        0         0         0         5
 * Decoded bits.
 */

#define AESLANIER_RECORD_SEPARATOR 0x55555122
#define AESLANIER_SECTOR_LENGTH 256
#define AESLANIER_RECORD_SIZE (AESLANIER_SECTOR_LENGTH + 4)

extern std::unique_ptr<Decoder> createAesLanierDecoder(
    const DecoderProto& config);

#endif
