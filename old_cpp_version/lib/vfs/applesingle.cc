#include "lib/core/globals.h"
#include "lib/core/bytes.h"
#include "lib/vfs/applesingle.h"

static constexpr uint32_t APPLESINGLE_MAGIC = 0x00051600;
static constexpr uint32_t APPLESINGLE_VERSION = 0x00020000;

void AppleSingle::parse(const Bytes& bytes)
{
    ByteReader br(bytes);
    if (br.read_be32() != APPLESINGLE_MAGIC)
        throw InvalidFileException();
    if (br.read_be32() > APPLESINGLE_VERSION)
        throw InvalidFileException();

    br.skip(16);
    int entries = br.read_be16();
    for (int i = 0; i < entries; i++)
    {
        uint32_t entryType = br.read_be32();
        uint32_t offset = br.read_be32();
        uint32_t length = br.read_be32();

        switch (entryType)
        {
            case 1:
                data = bytes.slice(offset, length);
                break;

            case 2:
                rsrc = bytes.slice(offset, length);
                break;

            case 9:
            {
                Bytes finderinfo = bytes.slice(offset, length);
                type = finderinfo.slice(0, 4);
                creator = finderinfo.slice(4, 4);
                break;
            }

            default:
                break;
        }
    }
}

Bytes AppleSingle::render()
{
    Bytes result;
    ByteWriter bw(result);

    bw.write_be32(APPLESINGLE_MAGIC);
    bw.write_be32(APPLESINGLE_VERSION);
    bw.append(Bytes(16)); /* padding */
    bw.write_be16(3);     /* number of entries */

    bw.write_be32(9);    /* finder info */
    bw.write_be32(0x3e); /* finder info offset */
    bw.write_be32(32);   /* finder info length */

    bw.write_be32(1);           /* data fork */
    bw.write_be32(OVERHEAD);    /* data fork offset */
    bw.write_be32(data.size()); /* data fork length */

    bw.write_be32(2);                      /* resource fork */
    bw.write_be32(OVERHEAD + data.size()); /* resource fork offset */
    bw.write_be32(rsrc.size());            /* resource fork length */

    bw.append(type.slice(0, 4));
    bw.append(creator.slice(0, 4));
    bw.append(Bytes(32 - 8));

    bw.append(data);
    bw.append(rsrc);

    return result;
}
