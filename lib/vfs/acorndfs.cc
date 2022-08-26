#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"
#include <fmt/format.h>

class AcornDfsDirent : public Dirent
{
public:
	AcornDfsDirent(int inode, const Bytes& bytes0, const Bytes& bytes1)
	{
		filename += (char)(bytes0[7] & 0x7f);
		filename += '.';
		for (int j = 0; j < 7; j++)
			filename += bytes0[j] & 0x7f;
		filename = filename.substr(0, filename.find(' '));

		this->inode = inode;
		start_sector = ((bytes1[6] & 0x03) << 8) | bytes1[7];
		load_address = ((bytes1[6] & 0x0c) << 14) | (bytes1[1] << 8) | bytes1[0];
		exec_address = ((bytes1[6] & 0xc0) << 10) | (bytes1[3] << 8) | bytes1[2];
		locked = bytes0[7] & 0x80;
		length = ((bytes1[6] & 0x30) << 12) | (bytes1[5] << 8) | bytes1[4];
		file_type = TYPE_FILE;
		mode = locked ? "L" : "";
	}

public:
	int inode;
	uint32_t start_sector;
	uint32_t load_address;
	uint32_t exec_address;
	bool locked;
};

class AcornDfsFilesystem : public Filesystem
{
public:
    AcornDfsFilesystem(
        const AcornDfsProto& config, std::shared_ptr<SectorInterface> sectors):
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
		attributes["acorndfs.inode"] = fmt::format("{}", dirent->inode);
		attributes["acorndfs.start_sector"] = fmt::format("{}", dirent->start_sector);
		attributes["acorndfs.load_address"] = fmt::format("0x{:x}", dirent->load_address);
		attributes["acorndfs.exec_address"] = fmt::format("0x{:x}", dirent->exec_address);
		attributes["acorndfs.locked"] = fmt::format("{}", dirent->locked);

		return attributes;
	}

private:
    std::vector<std::unique_ptr<AcornDfsDirent>> findAllFiles()
    {
        std::vector<std::unique_ptr<AcornDfsDirent>> result;
        auto sector0 = getLogicalSector(0);
        auto sector1 = getLogicalSector(1);

        if (sector1[5] & 7)
            throw BadFilesystemException();
        int dirents = sector1[5] / 8;

        for (int i = 0; i < dirents; i++)
        {
            auto bytes0 = sector0.slice(i * 8 + 8, 8);
            auto bytes1 = sector1.slice(i * 8 + 8, 8);

            result.push_back(std::make_unique<AcornDfsDirent>(i, bytes0, bytes1));
        }

        return result;
    }

	std::unique_ptr<AcornDfsDirent> findFile(const Path& path)
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
    const AcornDfsProto& _config;
};

std::unique_ptr<Filesystem> Filesystem::createAcornDfsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<AcornDfsFilesystem>(config.acorndfs(), sectors);
}
