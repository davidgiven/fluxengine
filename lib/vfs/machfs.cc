#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "lib/vfs/applesingle.h"
#include "lib/core/utils.h"

extern "C"
{
#include "libhfs.h"
#include "os.h"
}

class MacHfsFilesystem;
static MacHfsFilesystem* currentMacHfs;

class MacHfsFilesystem : public Filesystem
{
public:
    MacHfsFilesystem(
        const MacHfsProto& config, std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    uint32_t capabilities() const override
    {
        return OP_GETFSDATA | OP_CREATE | OP_LIST | OP_GETFILE | OP_PUTFILE |
               OP_GETDIRENT | OP_MOVE | OP_CREATEDIR | OP_DELETE;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        HfsMount m(this);

        hfsvolent ve;
        if (hfs_vstat(_vol, &ve))
            throw BadFilesystemException();

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = ve.name;
        attributes[TOTAL_BLOCKS] = std::to_string(ve.totbytes / ve.alblocksz);
        attributes[USED_BLOCKS] =
            std::to_string((ve.totbytes - ve.freebytes) / ve.alblocksz);
        attributes[BLOCK_SIZE] = std::to_string(ve.alblocksz);
        return attributes;
    }

    FilesystemStatus check() override
    {
        return FS_OK;
    }

    void create(bool quick, const std::string& volumeName) override
    {
        if (!quick)
            eraseEverythingOnDisk();

        if (hfs_format((const char*)this,
                0,
                HFS_MODE_ANY,
                volumeName.c_str(),
                0,
                nullptr))
            throwError();
    }

    std::vector<std::shared_ptr<Dirent>> list(const Path& path) override
    {
        HfsMount m(this);

        std::vector<std::shared_ptr<Dirent>> results;
        auto pathstr = ":" + path.to_str(":");
        HfsDir dir(hfs_opendir(_vol, pathstr.c_str()));
        if (!dir)
            throwError();

        for (;;)
        {
            hfsdirent de;
            int r = hfs_readdir(dir, &de);
            if (r != 0)
                break;

            results.push_back(toDirent(de, path));
        }
        return results;
    }

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        HfsMount m(this);
        if (path.size() == 0)
            throw BadPathException();

        std::vector<std::shared_ptr<Dirent>> results;
        auto pathstr = ":" + path.to_str(":");
        hfsdirent de;
        if (hfs_stat(_vol, pathstr.c_str(), &de))
            throwError();

        return toDirent(de, path.parent());
    }

    Bytes getFile(const Path& path) override
    {
        HfsMount m(this);
        if (path.size() == 0)
            throw BadPathException();

        std::vector<std::shared_ptr<Dirent>> results;
        auto pathstr = ":" + path.to_str(":");
        HfsFile file(hfs_open(_vol, pathstr.c_str()));
        if (!file)
            throwError();

        AppleSingle a;

        hfsdirent de;
        hfs_fstat(file, &de);
        a.creator = Bytes(de.u.file.creator);
        a.type = Bytes(de.u.file.type);

        hfs_setfork(file, 0);
        a.data = readBytes(file);
        hfs_setfork(file, 1);
        a.rsrc = readBytes(file);

        return a.render();
    }

    void putFile(const Path& path, const Bytes& bytes) override
    {
        HfsMount m(this);
        if (path.size() == 0)
            throw BadPathException();

        AppleSingle a;
        try
        {
            a.parse(bytes);
        }
        catch (const InvalidFileException& e)
        {
            throw UnimplementedFilesystemException(
                "you can only write valid AppleSingle encoded files");
        }

        auto pathstr = ":" + path.to_str(":");
        hfs_delete(_vol, pathstr.c_str());
        HfsFile file(hfs_create(_vol,
            pathstr.c_str(),
            (const char*)a.type.cbegin(),
            (const char*)a.creator.cbegin()));
        if (!file)
            throwError();

        hfs_setfork(file, 0);
        writeBytes(file, a.data);
        hfs_setfork(file, 1);
        writeBytes(file, a.rsrc);
    }

    void deleteFile(const Path& path) override
    {
        HfsMount m(this);
        if (path.size() == 0)
            throw BadPathException();

        auto pathstr = ":" + path.to_str(":");
        if (hfs_delete(_vol, pathstr.c_str()))
            throwError();
    }

    void moveFile(const Path& oldPath, const Path& newPath) override
    {
        HfsMount m(this);
        if (oldPath.empty() || newPath.empty())
            throw BadPathException();

        auto oldPathStr = ":" + oldPath.to_str(":");
        auto newPathStr = ":" + newPath.to_str(":");
        if (hfs_rename(_vol, oldPathStr.c_str(), newPathStr.c_str()))
            throwError();
    }

    void createDirectory(const Path& path) override
    {
        HfsMount m(this);

        auto pathStr = ":" + path.to_str(":");
        if (hfs_mkdir(_vol, pathStr.c_str()))
            throwError();
    }

private:
    std::shared_ptr<Dirent> toDirent(hfsdirent& de, const Path& parent)
    {
        auto dirent = std::make_shared<Dirent>();
        dirent->filename = de.name;
        dirent->path = parent;
        dirent->path.push_back(de.name);
        if (de.flags & HFS_ISDIR)
            dirent->file_type = TYPE_DIRECTORY;
        else
        {
            dirent->file_type = TYPE_FILE;
            dirent->length =
                de.u.file.dsize + de.u.file.rsize + AppleSingle::OVERHEAD;
        }
        dirent->mode = (de.flags & HFS_ISLOCKED) ? "L" : "";

        dirent->attributes[FILENAME] = de.name;
        dirent->attributes[LENGTH] = "0";
        dirent->attributes[FILE_TYPE] = (de.flags & HFS_ISDIR) ? "dir" : "file";
        dirent->attributes[MODE] = (de.flags & HFS_ISLOCKED) ? "L" : "";
        dirent->attributes["machfs.ctime"] = toIso8601(de.crdate);
        dirent->attributes["machfs.mtime"] = toIso8601(de.mddate);
        dirent->attributes["machfs.last_backup"] = toIso8601(de.bkdate);
        dirent->attributes["machfs.finder.x"] = std::to_string(de.fdlocation.h);
        dirent->attributes["machfs.finder.y"] = std::to_string(de.fdlocation.v);
        dirent->attributes["machfs.finder.flags"] =
            fmt::format("0x{:x}", de.fdflags);
        if (de.flags & HFS_ISDIR)
        {
            dirent->attributes["machfs.dir.valence"] =
                std::to_string(de.u.dir.valence);
            dirent->attributes["machfs.dir.x1"] =
                std::to_string(de.u.dir.rect.left);
            dirent->attributes["machfs.dir.y1"] =
                std::to_string(de.u.dir.rect.top);
            dirent->attributes["machfs.dir.x2"] =
                std::to_string(de.u.dir.rect.right);
            dirent->attributes["machfs.dir.y2"] =
                std::to_string(de.u.dir.rect.bottom);
        }
        else
        {
            dirent->attributes["length"] = fmt::format("{}",
                de.u.file.dsize + de.u.file.rsize + AppleSingle::OVERHEAD);
            dirent->attributes["machfs.file.dsize"] =
                std::to_string(de.u.file.dsize);
            dirent->attributes["machfs.file.rsize"] =
                std::to_string(de.u.file.rsize);
            dirent->attributes["machfs.file.type"] = de.u.file.type;
            dirent->attributes["machfs.file.creator"] = de.u.file.creator;
        }

        return dirent;
    }

    void throwError()
    {
        auto message =
            fmt::format("HFS error: {}", hfs_error ? hfs_error : "unknown");
        switch (errno)
        {
            case ENOTDIR:
            case ENAMETOOLONG:
                if (hfs_error)
                    throw BadPathException(message);
                else
                    throw BadPathException();

            case ENOENT:
                if (hfs_error)
                    throw FileNotFoundException(message);
                else
                    throw FileNotFoundException();

            case EEXIST:
                throw BadPathException("That already exists");

            case ENOTEMPTY:
                throw BadPathException("Directory is not empty");

            case EISDIR:
                throw BadPathException("That's a directory");

            case EIO:
            case EINVAL:
                if (hfs_error)
                    throw BadFilesystemException(message);
                else
                    throw BadFilesystemException();

            case ENOSPC:
            case ENOMEM:
                if (hfs_error)
                    throw DiskFullException(message);
                else
                    throw DiskFullException();

            case EROFS:
                if (hfs_error)
                    throw ReadOnlyFilesystemException(message);
                else
                    throw ReadOnlyFilesystemException();
        }
    }

private:
    Bytes readBytes(hfsfile* file)
    {
        Bytes bytes;
        ByteWriter bw(bytes);

        for (;;)
        {
            uint8_t buffer[4096];
            unsigned long done = hfs_read(file, buffer, sizeof(buffer));

            bw += Bytes(buffer, done);

            if (done != sizeof(buffer))
                break;
        }

        return bytes;
    }

    void writeBytes(hfsfile* file, const Bytes& bytes)
    {
        unsigned pos = 0;
        while (pos != bytes.size())
        {
            unsigned long done =
                hfs_write(file, bytes.cbegin() + pos, bytes.size() - pos);
            pos += done;
        }
    }

private:
    class HfsMount
    {
    public:
        friend MacHfsFilesystem;
        HfsMount(MacHfsFilesystem* self): _self(self)
        {
            _self->_vol = hfs_mount((const char*)self, 0, HFS_MODE_RDWR);
        }

        ~HfsMount()
        {
            hfs_umount(_self->_vol);
        }

    private:
        MacHfsFilesystem* _self;
    };

    class HfsFile
    {
    public:
        HfsFile(hfsfile* file): _file(file) {}
        ~HfsFile()
        {
            if (_file)
                hfs_close(_file);
        }

        operator hfsfile*() const
        {
            return _file;
        }

    private:
        hfsfile* _file;
    };

    class HfsDir
    {
    public:
        HfsDir(hfsdir* dir): _dir(dir) {}
        ~HfsDir()
        {
            if (_dir)
                hfs_closedir(_dir);
        }

        operator hfsdir*() const
        {
            return _dir;
        }

    private:
        hfsdir* _dir;
    };

private:
    friend unsigned long os_seek(void** priv, unsigned long offset);
    unsigned long hfsSeek(unsigned long offset)
    {
        unsigned count = getLogicalSectorCount();
        if ((offset == -1) || (offset > count))
            offset = count;

        _seek = offset;
        return offset;
    }

    friend unsigned long os_read(void** priv, void* buffer, unsigned long len);
    unsigned long hfsRead(void* buffer, unsigned long len)
    {
        auto bytes = getLogicalSector(_seek, len);
        _seek += len;
        memcpy(buffer, bytes.cbegin(), bytes.size());
        return len;
    }

    friend unsigned long os_write(
        void** priv, const void* buffer, unsigned long len);
    unsigned long hfsWrite(const void* buffer, unsigned long len)
    {
        Bytes bytes((const uint8_t*)buffer, len * 512);
        putLogicalSector(_seek, bytes);
        _seek += len;
        return len;
    }

private:
    const MacHfsProto& _config;
    hfsvol* _vol;
    unsigned long _seek;
};

int os_open(void** priv, const char* path, int mode)
{
    *priv = (void*)path;
    return 0;
}

int os_close(void** priv)
{
    return 0;
}

int os_same(void** priv, const char* path)
{
    return *priv == (void*)path;
}

unsigned long os_seek(void** priv, unsigned long offset)
{
    auto* self = (MacHfsFilesystem*)*priv;
    return self->hfsSeek(offset);
}

unsigned long os_read(void** priv, void* buffer, unsigned long len)
{
    auto* self = (MacHfsFilesystem*)*priv;
    return self->hfsRead(buffer, len);
}

unsigned long os_write(void** priv, const void* buffer, unsigned long len)
{
    auto* self = (MacHfsFilesystem*)*priv;
    return self->hfsWrite(buffer, len);
}

std::unique_ptr<Filesystem> Filesystem::createMacHfsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<MacHfsFilesystem>(config.machfs(), sectors);
}
