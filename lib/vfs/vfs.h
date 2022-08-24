#ifndef VFS_H
#define VFS_H

#include "lib/bytes.h"

class Sector;
class Image;
class Brother120Proto;
class DfsProto;
class FilesystemProto;

struct File
{
    Bytes data;
    std::vector<std::shared_ptr<const Sector>> sectors;
};

enum FileType
{
    TYPE_FILE,
    TYPE_DIRECTORY
};

struct Dirent
{
    std::string filename;
    FileType fileType;
    uint64_t length;
};

enum FilesystemStatus
{
    FS_OK,
    FS_OK_BUT_UNUSED_BAD_SECTORS,
    FS_OK_BUT_USED_BAD_SECTORS,
    FS_MISSING_CRITICAL_SECTORS,
    FS_BAD
};

class Filesystem
{
public:
	virtual void create();
    virtual FilesystemStatus check();
    virtual std::vector<Dirent> list(std::vector<std::string> path);

    virtual std::unique_ptr<File> read(std::vector<std::string> path);
    virtual std::vector<std::shared_ptr<const Sector>> write(
        std::vector<std::string> path, const Bytes& data);

public:
    static std::unique_ptr<Filesystem> createBrother120Filesystem(
        const Brother120Proto& config, std::shared_ptr<Image> image);
    static std::unique_ptr<Filesystem> createDfsFilesystem(
        const DfsProto& config, std::shared_ptr<Image> image);

	static std::unique_ptr<Filesystem> createFilesystem(
		const FilesystemProto& config, std::shared_ptr<Image> image);
};

#endif
