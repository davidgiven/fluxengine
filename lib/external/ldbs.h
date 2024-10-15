#ifndef LDBS_H
#define LDBS_H

class Bytes;

/* A very simple interface to John Elliott's LDBS image format:
 * http://www.seasip.info/Unix/LibDsk/ldbs.html
 */

#define LDBS_FILE_MAGIC 0x4C425301  /* "LBS\01" */
#define LDBS_FILE_TYPE 0x44534B02   /* "DSK\02" */
#define LDBS_BLOCK_MAGIC 0x4C444201 /* "LDB\01" */
#define LDBS_TRACK_BLOCK 0x44495201 /* "DIR\01" */

class LDBS
{
public:
    LDBS();

public:
    const Bytes& get(uint32_t address) const
    {
        return blocks.at(address).data;
    }

    uint32_t put(const Bytes& data, uint32_t type);

public:
    const Bytes write(uint32_t trackDirectory);
    uint32_t read(const Bytes& bytes);

private:
    struct Block
    {
        uint32_t type;
        Bytes data;
    };

    std::map<uint32_t, Block> blocks;
    unsigned top = 20;
};

#endif
