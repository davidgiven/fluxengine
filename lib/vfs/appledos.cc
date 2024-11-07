#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/core/utils.h"
#include "lib/config/config.pb.h"

/* This is described here:
 * http://fileformats.archiveteam.org/wiki/Apple_DOS_file_system
 * (also in Inside AppleDOS)
 */

class AppledosFilesystem : public Filesystem
{
    static constexpr int VTOC_BLOCK = 17 * 16;

    class AppledosDirent : public Dirent
    {
    public:
        AppledosDirent(const Bytes& de)
        {
            ByteReader br(de);
            track = br.read_8();
            sector = br.read_8();
            flags = br.read_8();
            filename = br.read(30);
            length = br.read_le16() * 256;

            for (char& c : filename)
                c &= 0x7f;
            filename = rightTrimWhitespace(filename);
            path = {filename};

            file_type = TYPE_FILE;

            attributes[FILENAME] = filename;
            attributes[LENGTH] = std::to_string(length);
            attributes[FILE_TYPE] = "file";
            attributes["appledos.flags"] = fmt::format("0x{:x}", flags);
        }

        uint8_t track;
        uint8_t sector;
        uint8_t flags;
    };

public:
    AppledosFilesystem(
        const AppledosProto& config, std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    uint32_t capabilities() const override
    {
        return OP_LIST | OP_GETDIRENT | OP_GETFSDATA | OP_GETFILE;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        mount();

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = "";
        attributes[TOTAL_BLOCKS] = std::to_string(_vtoc[0x34] * _vtoc[0x35]);
        attributes[USED_BLOCKS] = "0";
        attributes[BLOCK_SIZE] = "256";
        return attributes;
    }

    std::vector<std::shared_ptr<Dirent>> list(const Path& path) override
    {
        mount();
        if (path.size() != 0)
            throw BadPathException();

        std::vector<std::shared_ptr<Dirent>> results;
        for (auto& de : _dirents)
            results.push_back(de);

        return results;
    }

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        return find(path.front());
    }

    Bytes getFile(const Path& path) override
    {
        mount();
        if (path.size() != 1)
            throw BadPathException();

        auto dirent = find(path.front());
        int tstrack = dirent->track;
        int tssector = dirent->sector;

        Bytes bytes;
        ByteWriter bw(bytes);
        while (tstrack)
        {
            Bytes ts = getAppleSector(tstrack * 16 + tssector);
            ByteReader br(ts);
            br.seek(0x0c);

            while (!br.eof())
            {
                int track = br.read_8();
                int sector = br.read_8();
                if (!track)
                    goto done;

                bw += getAppleSector(track * 16 + sector);
            }

            tstrack = ts[1];
            tssector = ts[2];
        }

    done:
        return bytes;
    }

private:
    void mount()
    {
        _vtoc = getAppleSector(VTOC_BLOCK);
        if ((_vtoc[0x27] != 122) || (_vtoc[0x36] != 0) || (_vtoc[0x37] != 1))
            throw BadFilesystemException();

        _dirents.clear();
        int track = _vtoc[1];
        int sector = _vtoc[2];
        while (track)
        {
            Bytes dir = getAppleSector(track * 16 + sector);
            ByteReader br(dir);
            br.seek(0x0b);

            while (!br.eof())
            {
                Bytes fde = br.read(0x23);
                if ((fde[0] != 0) && (fde[0] != 255))
                    _dirents.push_back(std::make_shared<AppledosDirent>(fde));
            }

            track = dir[1];
            sector = dir[2];
        }
    }

    std::shared_ptr<AppledosDirent> find(const std::string filename)
    {
        for (auto& de : _dirents)
            if (de->filename == filename)
                return de;

        throw FileNotFoundException();
    }

    Bytes getAppleSector(uint32_t number, uint32_t count = 1)
    {
        return getLogicalSector(
            number + _config.filesystem_offset_sectors(), count);
    }

private:
    const AppledosProto& _config;
    Bytes _vtoc;
    std::vector<std::shared_ptr<AppledosDirent>> _dirents;
};

std::unique_ptr<Filesystem> Filesystem::createAppledosFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<AppledosFilesystem>(config.appledos(), sectors);
}
