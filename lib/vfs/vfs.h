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
    uint32_t length;
    std::string mode;
};

enum FilesystemStatus
{
    FS_OK,
    FS_OK_BUT_UNUSED_BAD_SECTORS,
    FS_OK_BUT_USED_BAD_SECTORS,
    FS_MISSING_CRITICAL_SECTORS,
    FS_BAD
};

class FilesystemException
{
public:
    FilesystemException(const std::string& message): message(message) {}

public:
    std::string message;
};

class BadPathException : public FilesystemException
{
public:
    BadPathException(): FilesystemException("Bad path") {}
};

class FileNotFoundException : public FilesystemException
{
public:
    FileNotFoundException(): FilesystemException("File not found") {}
};

class BadFilesystemException : public FilesystemException
{
public:
    BadFilesystemException(): FilesystemException("Invalid filesystem") {}
};

class CannotWriteException : public FilesystemException
{
public:
    CannotWriteException(): FilesystemException("Cannot write file") {}
};

class ReadOnlyFilesystemException : public FilesystemException
{
public:
    ReadOnlyFilesystemException(): FilesystemException("Read only filesystem")
    {
    }
};

class UnimplementedFilesystemException : public FilesystemException
{
public:
    UnimplementedFilesystemException(const std::string& msg):
        FilesystemException(msg)
    {
    }

    UnimplementedFilesystemException():
        FilesystemException("Unimplemented operation")
    {
    }
};

class Path : public std::vector<std::string>
{
public:
    Path() {}
    Path(const std::string& text);

public:
    std::string to_str(const std::string sep = "/") const;
};

class Filesystem
{
public:
    virtual void create();
    virtual FilesystemStatus check();
    virtual std::vector<std::unique_ptr<Dirent>> list(const Path& path);
    virtual Bytes getFile(const Path& path);
    virtual void putFile(const Path& path, const Bytes& data);
    virtual std::map<std::string, std::string> getMetadata(const Path& path);
    virtual void putMetadata(
        const Path& path, const std::map<std::string, std::string>& metadata);
    virtual void createDirectory(const Path& path);
    virtual void deleteFile(const Path& path);
    void flush();

protected:
    Filesystem(std::shared_ptr<SectorInterface> sectors);

    Bytes getSector(unsigned track, unsigned side, unsigned sector);

    Bytes getLogicalSector(uint32_t number, uint32_t count = 1);
    void putLogicalSector(uint32_t number, const Bytes& data);

    unsigned getOffsetOfSector(unsigned track, unsigned side, unsigned sector);
    unsigned getLogicalSectorCount();
    unsigned getLogicalSectorSize(unsigned track = 0, unsigned side = 0);

private:
    typedef std::tuple<unsigned, unsigned, unsigned> location_t;
    std::vector<location_t> _locations;
    std::shared_ptr<SectorInterface> _sectors;

public:
    static std::unique_ptr<Filesystem> createBrother120Filesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createAcornDfsFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createFatFsFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createCpmFsFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createAmigaFfsFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createMacHfsFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createCbmfsFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);

    static std::unique_ptr<Filesystem> createFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
};

#endif
