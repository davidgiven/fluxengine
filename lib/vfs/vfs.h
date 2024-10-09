#ifndef VFS_H
#define VFS_H

#include "lib/core/bytes.h"
#include "fmt/format.h"

class Sector;
class Image;
class Brother120Proto;
class DfsProto;
class FilesystemProto;
class SectorInterface;

class Path : public std::vector<std::string>
{
public:
    Path() {}
    Path(const std::vector<std::string> other);
    Path(const std::string& text);

public:
    Path parent() const;
    Path concat(const std::string& s) const;
    std::string to_str(const std::string sep = "/") const;
};

enum FileType
{
    TYPE__INVALID,
    TYPE_FILE,
    TYPE_DIRECTORY
};

struct Dirent
{
    Path path;
    std::string filename;
    FileType file_type;
    uint32_t length;
    std::string mode;
    std::map<std::string, std::string> attributes;
};

enum FilesystemStatus
{
    FS_OK,
    FS_OK_BUT_UNUSED_BAD_SECTORS,
    FS_OK_BUT_USED_BAD_SECTORS,
    FS_MISSING_CRITICAL_SECTORS,
    FS_BAD
};

class FilesystemException : public ErrorException
{
public:
    FilesystemException(const std::string& message): ErrorException(message) {}
};

class BadPathException : public FilesystemException
{
public:
    BadPathException(const Path& path):
        FilesystemException(fmt::format("Bad path: '{}'", path.to_str()))
    {
    }

    BadPathException(): FilesystemException("Bad path") {}

    BadPathException(const std::string& msg): FilesystemException(msg) {}
};

class FileNotFoundException : public FilesystemException
{
public:
    FileNotFoundException(): FilesystemException("File not found") {}

    FileNotFoundException(const std::string& msg): FilesystemException(msg) {}
};

class BadFilesystemException : public FilesystemException
{
public:
    BadFilesystemException(): FilesystemException("Invalid filesystem") {}

    BadFilesystemException(const std::string& msg): FilesystemException(msg) {}
};

class CannotWriteException : public FilesystemException
{
public:
    CannotWriteException(): FilesystemException("Cannot write file") {}

    CannotWriteException(const std::string& msg): FilesystemException(msg) {}
};

class DiskFullException : public CannotWriteException
{
public:
    DiskFullException(): CannotWriteException("Disk is full") {}

    DiskFullException(const std::string& msg): CannotWriteException(msg) {}
};

class ReadErrorException : public FilesystemException
{
public:
    ReadErrorException(): FilesystemException("Fatal read error") {}

    ReadErrorException(const std::string& msg): FilesystemException(msg) {}
};

class ReadOnlyFilesystemException : public FilesystemException
{
public:
    ReadOnlyFilesystemException(): FilesystemException("Read only filesystem")
    {
    }

    ReadOnlyFilesystemException(const std::string& msg):
        FilesystemException(msg)
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

class Filesystem
{
public:
    static constexpr const char* FILENAME = "filename";
    static constexpr const char* LENGTH = "length";
    static constexpr const char* MODE = "mode";
    static constexpr const char* FILE_TYPE = "file_type";
    static constexpr const char* CTIME = "ctime";

    static constexpr const char* VOLUME_NAME = "volume_name";
    static constexpr const char* TOTAL_BLOCKS = "total_blocks";
    static constexpr const char* USED_BLOCKS = "used_blocks";
    static constexpr const char* BLOCK_SIZE = "block_size";

    enum
    {
        OP_CREATE = 0b0000000000000001,
        OP_CHECK = 0b0000000000000010,
        OP_LIST = 0b0000000000000100,
        OP_GETFILE = 0b0000000000001000,
        OP_PUTFILE = 0b0000000000010000,
        OP_GETDIRENT = 0b0000000000100000,
        OP_CREATEDIR = 0b0000000001000000,
        OP_DELETE = 0b0000000010000000,
        OP_GETFSDATA = 0b0000000100000000,
        OP_PUTFSDATA = 0b0000001000000000,
        OP_PUTATTRS = 0b0000010000000000,
        OP_MOVE = 0b0000100000000000,
    };

public:
    /* Retrieve capability information. */
    virtual uint32_t capabilities() const;

    /* Create a filesystem on the disk. */
    virtual void create(bool quick, const std::string& volumeName);

    /* Are all sectors on the filesystem present and good? (Does not check
     * filesystem consistency.) */
    virtual FilesystemStatus check();

    /* Get volume metadata. */
    virtual std::map<std::string, std::string> getMetadata();

    /* Update volume metadata. */
    virtual void putMetadata(
        const std::map<std::string, std::string>& metadata);

    /* List files in a given directory. */
    virtual std::vector<std::shared_ptr<Dirent>> list(const Path& path);

    /* Read a file. */
    virtual Bytes getFile(const Path& path);

    /* Write a file. */
    virtual void putFile(const Path& path, const Bytes& data);

    /* Get a single file dirent. */
    virtual std::shared_ptr<Dirent> getDirent(const Path& path);

    /* Update file metadata. */
    virtual void putMetadata(
        const Path& path, const std::map<std::string, std::string>& metadata);

    /* Creates a directory. */
    virtual void createDirectory(const Path& path);

    /* Deletes a file or non-empty directory. */
    virtual void deleteFile(const Path& path);

    /* Moves a file (including renaming it). */
    virtual void moveFile(const Path& oldName, const Path& newName);

    /* Is this filesystem's backing store read-only? */
    bool isReadOnly();

    /* Does this filesystem need flushing? */
    bool needsFlushing();

    /* Flushes any changes back to the disk. */
    void flushChanges();

    /* Discards any pending changes. */
    void discardChanges();

public:
    Filesystem(std::shared_ptr<SectorInterface> sectors);

    Bytes getSector(unsigned track, unsigned side, unsigned sector);

    Bytes getLogicalSector(uint32_t number, uint32_t count = 1);
    void putLogicalSector(uint32_t number, const Bytes& data);

    unsigned getOffsetOfSector(unsigned track, unsigned side, unsigned sector);
    unsigned getLogicalSectorCount();
    unsigned getLogicalSectorSize(unsigned track = 0, unsigned side = 0);

    void eraseEverythingOnDisk();

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
    static std::unique_ptr<Filesystem> createProdosFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createAppledosFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createSmaky6Filesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createPhileFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createLifFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createMicrodosFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createZDosFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createRolandFsFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);

    static std::unique_ptr<Filesystem> createFilesystem(
        const FilesystemProto& config, std::shared_ptr<SectorInterface> image);
    static std::unique_ptr<Filesystem> createFilesystemFromConfig();
};

/* Used for tests only. */

class HasBitmap
{
public:
    virtual std::vector<bool> getBitmapForDebugging() = 0;
};

class HasMount
{
public:
    virtual void mount() = 0;
};

#endif
