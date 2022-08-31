#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"
#include "lib/utils.h"
#include <fmt/format.h>

enum
{
    DEL,
    SEQ,
    PRG,
    USR,
    REL
};

static std::string fromPetscii(const Bytes& bytes)
{
    std::stringstream ss;

    for (uint8_t b : bytes)
    {
        if ((b >= 32) && (b <= 126))
            ss << (char)tolower(b);
        else
            ss << fmt::format("%{:2x}", b);
    }

    return ss.str();
}

static std::string toMode(uint8_t cbm_type)
{
    std::stringstream ss;
    if (cbm_type & 0x40)
        ss << 'L';
    if (cbm_type & 0x80)
        ss << 'S';
    return ss.str();
}

static std::string toFileType(uint8_t cbm_type)
{
    switch (cbm_type & 0x0f)
    {
        case DEL:
            return "DEL";
        case SEQ:
            return "SEQ";
        case PRG:
            return "PRG";
        case USR:
            return "USR";
        case REL:
            return "REL";
        default:
            return fmt::format("[bad type {:x}]", cbm_type & 0x0f);
    }
}

class CbmfsDirent : public Dirent
{
public:
    CbmfsDirent(const Bytes& dbuf)
    {
        ByteReader br(dbuf);

        br.skip(2); /* t/s field */
        cbm_type = br.read_8();
        start_track = br.read_8();
        start_sector = br.read_8();

        auto filenameBytes = br.read(16).split(0xa0)[0];
        filename = fromPetscii(filenameBytes);
        side_track = br.read_8();
        side_sector = br.read_8();
        recordlen = br.read_8();
        br.skip(6);
        sectors = br.read_le16();

        file_type = TYPE_FILE;
        length = sectors * 254;
        mode = "";
    }

public:
    unsigned cbm_type;
    unsigned start_track;
    unsigned start_sector;
    unsigned side_track;
    unsigned side_sector;
    unsigned recordlen;
    unsigned sectors;
};

class CbmfsFilesystem : public Filesystem
{
public:
    CbmfsFilesystem(
        const CbmfsProto& config, std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    FilesystemStatus check()
    {
        return FS_OK;
    }

    std::vector<std::shared_ptr<Dirent>> list(const Path& path)
    {
        if (path.size() != 0)
            throw BadPathException();

        std::vector<std::shared_ptr<Dirent>> results;
        uint8_t t = _config.directory_track();
        uint8_t s = 1;
        while (t != 0xff)
        {
            auto b = getSector(t, 0, s);

            for (int i = 0; i < 8; i++)
            {
                auto dbuf = b.slice(i * 32, 32);
                if (dbuf[2] == 0)
                    continue;

                results.push_back(std::make_shared<CbmfsDirent>(dbuf));
            }

            t = b[0] - 1;
            s = b[1];
        }

        return results;
    }

    std::map<std::string, std::string> getMetadata(const Path& path)
    {
        if (path.size() != 1)
            throw BadPathException();
        auto de = findFile(unhex(path[0]));
        if (!de)
            throw FileNotFoundException();

        std::map<std::string, std::string> attributes;
        attributes[FILENAME] = de->filename;
        attributes[LENGTH] = fmt::format("{}", de->length);
        attributes[FILE_TYPE] = "file";
        attributes[MODE] = de->mode;
        attributes["cbmfs.type"] = toFileType(de->cbm_type);
        attributes["cbmfs.start_track"] = fmt::format("{}", de->start_track);
        attributes["cbmfs.start_sector"] = fmt::format("{}", de->start_sector);
        attributes["cbmfs.side_track"] = fmt::format("{}", de->side_track);
        attributes["cbmfs.side_sector"] = fmt::format("{}", de->side_sector);
        attributes["cbmfs.recordlen"] = fmt::format("{}", de->recordlen);
        attributes["cbmfs.sectors"] = fmt::format("{}", de->sectors);
        return attributes;
    }

    Bytes getFile(const Path& path)
    {
        if (path.size() != 1)
            throw BadPathException();
        auto de = findFile(unhex(path[0]));
        if (!de)
            throw FileNotFoundException();
        if (de->cbm_type == REL)
            throw UnimplementedFilesystemException("cannot read .REL files");

        Bytes bytes;
        ByteWriter bw(bytes);

        uint8_t t = de->start_track - 1;
        uint8_t s = de->start_sector;
        for (;;)
        {
            auto b = getSector(t, 0, s);

            if (b[0])
                bw += b.slice(2);
            else
            {
                bw += b.slice(2, b[1]);
                break;
            }

            t = b[0] - 1;
            s = b[1];
        }

        return bytes;
    }

private:
    std::shared_ptr<CbmfsDirent> findFile(const std::string& filename)
    {
        uint8_t t = _config.directory_track();
        uint8_t s = 1;
        while (t != 0xff)
        {
            auto b = getSector(t, 0, s);

            for (int i = 0; i < 8; i++)
            {
                auto dbuf = b.slice(i * 32, 32);
                if (dbuf[2] == 0)
                    continue;

                auto de = std::make_shared<CbmfsDirent>(dbuf);
                if (de->filename == filename)
                    return de;
            }

            t = b[0] - 1;
            s = b[1];
        }

        return nullptr;
    }

private:
    const CbmfsProto& _config;
};

std::unique_ptr<Filesystem> Filesystem::createCbmfsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<CbmfsFilesystem>(config.cbmfs(), sectors);
}
