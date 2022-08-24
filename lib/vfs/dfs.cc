#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/sector.h"
#include "lib/image.h"
#include "lib/sectorinterface.h"
#include "lib/config.pb.h"

class DfsFilesystem : public Filesystem
{
public:
    DfsFilesystem(
        const DfsProto& config, std::shared_ptr<SectorInterface> sectors):
        _config(config),
        _sectors(sectors)
    {
    }

    void create() {}

    FilesystemStatus check()
    {
        return FS_OK;
    }

    std::vector<std::unique_ptr<Dirent>> list(std::vector<std::string> path)
    {
        if (!path.empty())
            throw FileNotFoundException();

        std::vector<std::unique_ptr<Dirent>> result;
        auto sector0 = getSector(0);
        auto sector1 = getSector(1);

        if (sector1[5] & 7)
            throw BadFilesystemException();
        int dirents = sector1[5] / 8;

        for (int i = 0; i < dirents; i++)
        {
            auto bytes0 = sector0.slice(i * 8 + 8, 8);
            auto bytes1 = sector1.slice(i * 8 + 8, 8);

            std::string filename;
            filename += (char)(bytes0[7] & 0x7f);
            filename += '.';
            for (int j = 0; j < 7; j++)
                filename += bytes0[j] & 0x7f;
            filename = filename.substr(0, filename.find(' '));

            uint32_t length =
                ((bytes1[6] & 0x30) << 12) | (bytes1[5] << 8) | bytes1[4];

            auto dirent = std::make_unique<Dirent>();
            dirent->filename = filename;
            dirent->fileType = TYPE_FILE;
            dirent->length = length;

            result.push_back(std::move(dirent));
        }

        return result;
    }

    std::unique_ptr<File> read(std::vector<std::string> path) {}

    std::vector<std::shared_ptr<const Sector>> write(
        std::vector<std::string> path, const Bytes& data)
    {
    }

private:
    Bytes getSector(uint32_t block)
    {
        uint32_t track = block / _config.sectors_per_track();
        uint32_t sector = block % _config.sectors_per_track();
        return _sectors->get(track, 0, sector)->data;
    }

    void putSector(uint32_t block, const Bytes& bytes)
    {
        uint32_t track = block / _config.sectors_per_track();
        uint32_t sector = block % _config.sectors_per_track();
        _sectors->put(track, 0, sector)->data = bytes;
    }

private:
    const DfsProto& _config;
    std::shared_ptr<SectorInterface> _sectors;
};

std::unique_ptr<Filesystem> Filesystem::createDfsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<DfsFilesystem>(config.dfs(), sectors);
}
