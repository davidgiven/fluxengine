#ifndef AESLANIER5_H
#define AESLANIER5_H

/* Format is FM:
 *
 *    a    a    a    a    f    b    e    f
 * 1010 1010 1010 1010 1111 1011 1110 1111
 *  0 0  0 0  0 0  0 0  1 1  0 1  1 0  1 1
 *         0         0         d         b
 * 
 * However, note that this pattern is _not_ reversed...
 */

#define AESLANIER5_RECORD_SEPARATOR 0xaaaaaaaaaaaafbefLL
#define AESLANIER5_SECTOR_LENGTH 151
#define AESLANIER5_RECORD_SIZE (AESLANIER5_SECTOR_LENGTH + 3)

extern std::unique_ptr<Decoder> createAesLanier5Decoder(
    const DecoderProto& config);

#endif
