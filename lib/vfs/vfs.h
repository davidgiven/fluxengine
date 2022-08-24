#ifndef VFS_H
#define VFS_H

#include "lib/bytes.h"

class Sector;
class Image;
class Brother120Proto;
class DfsProto;
class FilesystemProto;
class SectorInterface;

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

extern std::vector<std::string> parsePath(const std::string& path);

class FilesystemException {};
class BadPathException : public FilesystemException {};
class FileNotFoundException : public FilesystemException {};
class BadFilesystemException : public FilesystemException {};

class Filesystem
{
public:
	virtual void create() = 0;
    virtual FilesystemStatus check() = 0;
    virtual std::vector<std::unique_ptr<Dirent>> list(std::vector<std::string> path) = 0;

    virtual std::unique_ptr<File> read(std::vector<std::string> path) = 0;
    virtual std::vector<std::shared_ptr<const Sector>> write(
        std::vector<std::string> path, const Bytes& data) = 0;

public:
    static std::unique_ptr<Filesystem> createBrother120Filesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createDfsFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);

	static std::unique_ptr<Filesystem> createFilesystem(
		const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
};

#endif
