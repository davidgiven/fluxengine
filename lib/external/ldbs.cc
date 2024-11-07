#include "lib/core/globals.h"
#include <string.h>
#include "lib/core/bytes.h"
#include "lib/external/ldbs.h"

LDBS::LDBS() {}

uint32_t LDBS::put(const Bytes& data, uint32_t type)
{
    uint32_t address = top;
    Block& block = blocks[address];
    block.type = type;
    block.data = data;

    top += data.size() + 20;
    return address;
}

uint32_t LDBS::read(const Bytes& data)
{
    ByteReader br(data);

    br.seek(0);
    if ((br.read_be32() != LDBS_FILE_MAGIC) ||
        (br.read_be32() != LDBS_FILE_TYPE))
        error("not a valid LDBS file");

    uint32_t address = br.read_le32();
    br.skip(4);
    uint32_t trackDirectory = br.read_le32();

    while (address)
    {
        br.seek(address);
        if (br.read_be32() != LDBS_BLOCK_MAGIC)
            error("invalid block at address 0x{:x}", address);

        Block& block = blocks[address];
        block.type = br.read_be32();

        uint32_t size = br.read_le32();
        br.skip(4);
        address = br.read_le32();

        block.data.writer().append(br.read(size));
    }

    top = data.size();
    return trackDirectory;
}

const Bytes LDBS::write(uint32_t trackDirectory)
{
    Bytes data(top);
    ByteWriter bw(data);

    uint32_t previous = 0;
    for (const auto& e : blocks)
    {
        bw.seek(e.first);
        bw.write_be32(LDBS_BLOCK_MAGIC);
        bw.write_be32(e.second.type);
        bw.write_le32(e.second.data.size());
        bw.write_le32(e.second.data.size());
        bw.write_le32(previous);
        bw.append(e.second.data);

        previous = e.first;
    }

    bw.seek(0);
    bw.write_be32(LDBS_FILE_MAGIC);
    bw.write_be32(LDBS_FILE_TYPE);
    bw.write_le32(previous);
    bw.write_le32(0);
    bw.write_le32(trackDirectory);

    return data;
}
