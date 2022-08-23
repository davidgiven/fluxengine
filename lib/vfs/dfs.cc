#include "globals.h"
#include "vfs.h"

class DfsFilesystem : public Filesystem
{
public:
    DfsFilesystem(std::shared_ptr<Image> image): _image(image) {}

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
    std::shared_ptr<Image> _image;
};

std::unique_ptr<Filesystem> Filesystem::createDfsFilesystem(
    std::shared_ptr<Image> image)
{
    return std::make_unique<DfsFilesystem>(image);
}
