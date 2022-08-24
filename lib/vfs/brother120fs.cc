#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/sector.h"
#include "lib/image.h"
#include "lib/sectorinterface.h"
#include "lib/config.pb.h"

/* Number of sectors on a 120kB disk. */
static constexpr int SECTOR_COUNT = 468;

/* Start sector for data (after the directory */
static constexpr int DATA_START_SECTOR = 14;

/* Size of a sector */
static constexpr int SECTOR_SIZE = 256;

/* Number of dirents in a directory. */
static constexpr int DIRECTORY_SIZE = 128;

/* Number of sectors in a directory. */
static constexpr int DIRECTORY_SECTORS = 8;

class Brother120Filesystem : public Filesystem
{
public:
    Brother120Filesystem(
        const Brother120FsProto& config, std::shared_ptr<SectorInterface> sectors):
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
		for (int block = 0; block < DIRECTORY_SECTORS; block++)
		{
			auto bytes = getSector(block);
			for (int d = 0; d < SECTOR_SIZE/16; d++)
			{
				Bytes buffer = bytes.slice(d*16, 16);
				if (buffer[0] == 0xf0)
					continue;

				ByteReader br(buffer);
				std::string filename = br.read(8);
				filename = filename.substr(0, filename.find(' '));
				br.read_8(); /* type */
				br.read_be16(); /* start sector */
				uint32_t length = br.read_8() * SECTOR_SIZE;

				auto dirent = std::make_unique<Dirent>();
				dirent->filename = filename;
				dirent->fileType = TYPE_FILE;
				dirent->length = length;

				result.push_back(std::move(dirent));
			}
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
		uint32_t track = block / 12;
		uint32_t sector = block % 12;
		return _sectors->get(track, 0, sector)->data;
	}

	void putSector(uint32_t block, const Bytes& bytes)
	{
		uint32_t track = block / 12;
		uint32_t sector = block % 12;
		_sectors->put(track, 0, sector)->data = bytes;
	}

private:
    const Brother120FsProto& _config;
    std::shared_ptr<SectorInterface> _sectors;
};

std::unique_ptr<Filesystem> Filesystem::createBrother120Filesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<Brother120Filesystem>(config.brother120(), sectors);
}
