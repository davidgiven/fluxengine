#include "globals.h"
#include "vfs.h"
#include "lib/config.pb.h"

class DfsFilesystem : public Filesystem
{
public:
    DfsFilesystem(const DfsProto& config, std::shared_ptr<Image> image): _config(config), _image(image) {}

	void create()
	{
	}

    FilesystemStatus check()
    {
        return FS_OK;
    }

    std::vector<Dirent> list(std::vector<std::string> path)
    {
        return {};
    }

    std::unique_ptr<File> read(std::vector<std::string> path) {}

    std::vector<std::shared_ptr<const Sector>> write(
        std::vector<std::string> path, const Bytes& data)
    {
    }

private:
	const DfsProto& _config;
    std::shared_ptr<Image> _image;
};

std::unique_ptr<Filesystem> Filesystem::createDfsFilesystem(
    const DfsProto& config, std::shared_ptr<Image> image)
{
    return std::make_unique<DfsFilesystem>(config, image);
}
