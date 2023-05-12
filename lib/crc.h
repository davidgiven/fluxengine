#ifndef CRC_H
#define CRC_H

#define CCITT_POLY 0x1021
#define MODBUS_POLY 0x8005
#define MODBUS_POLY_REF 0xa001
#define BROTHER_POLY 0x000201

struct crcspec
{
    unsigned width;
    uint64_t poly;
    uint64_t init;
    uint64_t xorout;
    bool refin;
    bool refout;
};

extern uint64_t generic_crc(const struct crcspec& spec, const Bytes& bytes);

extern uint16_t sumBytes(const Bytes& bytes);
extern uint8_t xorBytes(const Bytes& bytes);
extern uint16_t crc16(uint16_t poly, uint16_t init, const Bytes& bytes);
extern uint16_t crc16ref(uint16_t poly, uint16_t init, const Bytes& bytes);
extern uint32_t crcbrother(const Bytes& bytes);

static inline uint16_t crc16(uint16_t poly, const Bytes& bytes)
{
    return crc16(poly, 0xffff, bytes);
}

static inline uint16_t crc16ref(uint16_t poly, const Bytes& bytes)
{
    return crc16ref(poly, 0xffff, bytes);
}

#endif
