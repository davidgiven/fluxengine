#ifndef F85_H
#define F85_H

#define F85_SECTOR_RECORD 0xffffce /* 1111 1111 1111 1111 1100 1110 */
#define F85_DATA_RECORD 0xffffcb   /* 1111 1111 1111 1111 1100 1101 */
#define F85_SECTOR_LENGTH 512

extern std::unique_ptr<Decoder> createDurangoF85Decoder(
    const DecoderProto& config);

#endif
