#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"
#include "lib/vfs/applesingle.h"
#include "lib/utils.h"
#include <fmt/format.h>

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

    FilesystemStatus check()
    {
        return FS_OK;
    }

    void create(bool quick, const std::string& volumeName)
    {
        if (!quick)
            eraseEverythingOnDisk();

        hfs_format(
            (const char*)this, 0, HFS_MODE_ANY, volumeName.c_str(), 0, nullptr);
    }

    std::vector<std::unique_ptr<Dirent>> list(const Path& path)
    {
        HfsMount m(this);

        std::vector<std::unique_ptr<Dirent>> results;
        auto pathstr = ":" + path.to_str(":");
        HfsDir dir(hfs_opendir(_vol, pathstr.c_str()));
        if (!dir)
            throw FileNotFoundException();

        for (;;)
        {
            hfsdirent de;
            int r = hfs_readdir(dir, &de);
            if (r != 0)
                break;

            auto dirent = std::make_unique<Dirent>();
            dirent->filename = de.name;
            if (de.flags & HFS_ISDIR)
            {

                dirent->file_type = TYPE_DIRECTORY;
            }
            else
            {
                dirent->file_type = TYPE_FILE;
                dirent->length =
                    de.u.file.dsize + de.u.file.rsize + AppleSingle::OVERHEAD;
            }
            dirent->mode = (de.flags & HFS_ISLOCKED) ? "L" : "";
            results.push_back(std::move(dirent));
        }
        return results;
    }

    std::map<std::string, std::string> getMetadata(const Path& path)
    {
        HfsMount m(this);
        if (path.size() == 0)
            throw BadPathException();

        std::vector<std::unique_ptr<Dirent>> results;
        auto pathstr = ":" + path.to_str(":");
        hfsdirent de;
        if (hfs_stat(_vol, pathstr.c_str(), &de))
            throw FileNotFoundException();

        std::map<std::string, std::string> attributes;
        attributes[FILENAME] = de.name;
        attributes[LENGTH] = "0";
        attributes[FILE_TYPE] = (de.flags & HFS_ISDIR) ? "dir" : "file";
        attributes[MODE] = (de.flags & HFS_ISLOCKED) ? "L" : "";
        attributes["machfs.ctime"] = toIso8601(de.crdate);
        attributes["machfs.mtime"] = toIso8601(de.mddate);
        attributes["machfs.last_backup"] = toIso8601(de.bkdate);
        attributes["machfs.finder.x"] = fmt::format("{}", de.fdlocation.h);
        attributes["machfs.finder.y"] = fmt::format("{}", de.fdlocation.v);
        attributes["machfs.finder.flags"] = fmt::format("0x{:x}", de.fdflags);
        if (de.flags & HFS_ISDIR)
        {
            attributes["machfs.dir.valence"] =
                fmt::format("{}", de.u.dir.valence);
            attributes["machfs.dir.x1"] = fmt::format("{}", de.u.dir.rect.left);
            attributes["machfs.dir.y1"] = fmt::format("{}", de.u.dir.rect.top);
            attributes["machfs.dir.x2"] =
                fmt::format("{}", de.u.dir.rect.right);
            attributes["machfs.dir.y2"] =
                fmt::format("{}", de.u.dir.rect.bottom);
        }
        else
        {
            attributes["length"] = fmt::format("{}",
                de.u.file.dsize + de.u.file.rsize + AppleSingle::OVERHEAD);
            attributes["machfs.file.dsize"] =
                fmt::format("{}", de.u.file.dsize);
            attributes["machfs.file.rsize"] =
                fmt::format("{}", de.u.file.rsize);
            attributes["machfs.file.type"] = de.u.file.type;
            attributes["machfs.file.creator"] = de.u.file.creator;
        }

        return attributes;
    }

    Bytes getFile(const Path& path)
    {
        HfsMount m(this);
        if (path.size() == 0)
            throw BadPathException();

        std::vector<std::unique_ptr<Dirent>> results;
        auto pathstr = ":" + path.to_str(":");
        HfsFile file(hfs_open(_vol, pathstr.c_str()));
        if (!file)
            throw FileNotFoundException();

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

    void putFile(const Path& path, const Bytes& bytes)
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
            throw CannotWriteException();

        hfs_setfork(file, 0);
        writeBytes(file, a.data);
        hfs_setfork(file, 1);
        writeBytes(file, a.rsrc);
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
