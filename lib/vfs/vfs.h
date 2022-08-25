#ifndef VFS_H
#define VFS_H

#include "lib/bytes.h"

class Sector;
class Image;
class Brother120Proto;
class DfsProto;
class FilesystemProto;
class SectorInterface;

enum FileType
{
    TYPE_FILE,
    TYPE_DIRECTORY
};

struct Dirent
{
    std::string filename;
    FileType file_type;
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

class FilesystemException {};
class BadPathException : public FilesystemException {};
class FileNotFoundException : public FilesystemException {};
class BadFilesystemException : public FilesystemException {};
class ReadOnlyFilesystemException : public FilesystemException {};
class UnimplementedFilesystemException : public FilesystemException {};

class Path : public std::vector<std::string>
{
public:
	Path() {}
	Path(const std::string& text);
};

class Filesystem
{
public:
	virtual void create()
	{ throw UnimplementedFilesystemException(); }
	
    virtual FilesystemStatus check()
	{ throw UnimplementedFilesystemException(); }

    virtual std::vector<std::unique_ptr<Dirent>> list(const Path& path)
	{ throw UnimplementedFilesystemException(); }

    virtual Bytes read(const Path& path)
	{ throw UnimplementedFilesystemException(); }

    virtual void write(const Path& path, const Bytes& data)
	{ throw UnimplementedFilesystemException(); }

	virtual std::map<std::string, std::string> getMetadata(const Path& path)
	{ throw UnimplementedFilesystemException(); }

	virtual void setMetadata(const Path& path, const std::map<std::string, std::string>& metadata)
	{ throw UnimplementedFilesystemException(); }

public:
    static std::unique_ptr<Filesystem> createBrother120Filesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createAcornDfsFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);

	static std::unique_ptr<Filesystem> createFilesystem(
		const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
};

#endif
