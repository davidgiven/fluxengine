#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"
#include <fmt/format.h>

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

class Brother120Dirent : public Dirent
{
public:
	Brother120Dirent(int inode, const Bytes& bytes)
	{
		ByteReader br(bytes);
		filename = br.read(8);
		filename = filename.substr(0, filename.find(' '));

		this->inode = inode;
		brother_type = br.read_8();
		start_sector = br.read_be16(); 
		length = br.read_8() * SECTOR_SIZE;
		file_type = TYPE_FILE;
		mode = "";
	}

public:
	int inode;
	int brother_type;
	uint32_t start_sector;
};

class Brother120Filesystem : public Filesystem
{
public:
    Brother120Filesystem(
        const Brother120FsProto& config, std::shared_ptr<SectorInterface> sectors):
		Filesystem(sectors),
        _config(config)
    {
    }

    FilesystemStatus check()
    {
        return FS_OK;
    }

    std::vector<std::unique_ptr<Dirent>> list(const Path& path)
    {
		if (!path.empty())
			throw FileNotFoundException();

		std::vector<std::unique_ptr<Dirent>> result;
		for (auto& dirent : findAllFiles())
			result.push_back(std::move(dirent));

        return result;
    }

	std::map<std::string, std::string> getMetadata(const Path& path)
	{
		std::map<std::string, std::string> attributes;

		auto dirent = findFile(path);
		attributes["filename"] = dirent->filename;
		attributes["length"] = fmt::format("{}", dirent->length);
		attributes["type"] = "file";
		attributes["mode"] = dirent->mode;
		attributes["brother120.inode"] = fmt::format("{}", dirent->inode);
		attributes["brother120.start_sector"] = fmt::format("{}", dirent->start_sector);
		attributes["brother120.type"] = fmt::format("{}", dirent->brother_type);

		return attributes;
	}

private:
    std::vector<std::unique_ptr<Brother120Dirent>> findAllFiles()
    {
		std::vector<std::unique_ptr<Brother120Dirent>> result;
		int inode = 0;
		for (int block = 0; block < DIRECTORY_SECTORS; block++)
		{
			auto bytes = getLogicalSector(block);
			for (int d = 0; d < SECTOR_SIZE/16; d++, inode++)
			{
				Bytes buffer = bytes.slice(d*16, 16);
				if (buffer[0] == 0xf0)
					continue;

				result.push_back(
				std::make_unique<Brother120Dirent>(inode, buffer));
			}
		}

        return result;
    }

	std::unique_ptr<Brother120Dirent> findFile(const Path& path)
	{
		if (path.size() != 1)
			throw BadPathException();

		for (auto& dirent : findAllFiles())
        {
			if (dirent->filename == path[0])
				return std::move(dirent);
        }

		throw FileNotFoundException();
	}

private:
    const Brother120FsProto& _config;
};

std::unique_ptr<Filesystem> Filesystem::createBrother120Filesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<Brother120Filesystem>(config.brother120(), sectors);
}
