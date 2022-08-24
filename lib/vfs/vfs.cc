#include "globals.h"
#include "vfs.h"
#include "lib/config.pb.h"

std::vector<std::string> parsePath(const std::string& path)
{
	if (path == "")
		return {};
	auto p = path;
	if (p[0] == '/')
		p = p.substr(1);

    std::vector<std::string> result;
    std::stringstream ss(p);
    std::string item;

    while (std::getline(ss, item, '/'))
	{
		if (item.empty())
			throw BadPathException();
		result.push_back(item);
	}

    return result;
}


std::unique_ptr<Filesystem> Filesystem::createFilesystem(
		const FilesystemProto& config, std::shared_ptr<SectorInterface> image)
{
	switch (config.filesystem_case())
	{
		case FilesystemProto::kBrother120:
			return Filesystem::createBrother120Filesystem(config, image);

		case FilesystemProto::kDfs:
			return Filesystem::createDfsFilesystem(config, image);

		default:
			Error() << "bad filesystem config";
			return std::unique_ptr<Filesystem>();
	}
}

