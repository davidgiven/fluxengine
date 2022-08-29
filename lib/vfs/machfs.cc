#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"
#include <fmt/format.h>

extern "C"
{
#include "libhfs.h"
#include "os.h"
}

class MacHfsFilesystem;
static MacHfsFilesystem* currentMacHfs;

// static std::string modeToString(BYTE attrib)
//{
//     std::stringstream ss;
//     if (attrib & AM_RDO)
//         ss << 'R';
//     if (attrib & AM_HID)
//         ss << 'H';
//     if (attrib & AM_SYS)
//         ss << 'S';
//     if (attrib & AM_ARC)
//         ss << 'A';
//     return ss.str();
// }

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

    std::vector<std::unique_ptr<Dirent>> list(const Path& path)
    {
        HfsMount m(this);

        std::vector<std::unique_ptr<Dirent>> results;
        auto pathstr = ":" + path.to_str(":");
        auto* dir = hfs_opendir(_vol, pathstr.c_str());
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
                dirent->length = de.u.file.dsize + de.u.file.rsize;
            }
            dirent->mode = (de.flags & HFS_ISLOCKED) ? "L" : "";
            results.push_back(std::move(dirent));
        }
        hfs_closedir(dir);
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
        attributes["filename"] = de.name;
        attributes["length"] = "0";
        attributes["type"] = (de.flags & HFS_ISDIR) ? "dir" : "file";
        attributes["mode"] = (de.flags & HFS_ISLOCKED) ? "L" : "";
        attributes["machfs.finder.x"] = fmt::format("{}", de.fdlocation.h);
        attributes["machfs.finder.y"] = fmt::format("{}", de.fdlocation.v);
        attributes["machfs.finder.flags"] = fmt::format("0x{:x}", de.fdflags);
        if (de.flags & HFS_ISDIR)
        {
        	attributes["machfs.dir.valence"] = fmt::format("{}", de.u.dir.valence);
        	attributes["machfs.dir.x1"] = fmt::format("{}", de.u.dir.rect.left);
        	attributes["machfs.dir.y1"] = fmt::format("{}", de.u.dir.rect.top);
        	attributes["machfs.dir.x2"] = fmt::format("{}", de.u.dir.rect.right);
        	attributes["machfs.dir.y2"] = fmt::format("{}", de.u.dir.rect.bottom);
        }
        else
        {
        	attributes["machfs.file.dsize"] = fmt::format("{}", de.u.file.dsize);
        	attributes["machfs.file.rsize"] = fmt::format("{}", de.u.file.rsize);
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
        auto* file = hfs_open(_vol, pathstr.c_str());
        if (!file)
        	throw FileNotFoundException();

        Bytes bytes;
        ByteWriter bw(bytes);

        hfs_setfork(file, 0);
        for (;;)
        {
            uint8_t buffer[4096];
            unsigned long done = hfs_read(file, buffer, sizeof(buffer));

            bw += Bytes(buffer, done);

            if (done != sizeof(buffer))
            	break;
        }
        hfs_close(file);

        return bytes;
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
        return -1;
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
