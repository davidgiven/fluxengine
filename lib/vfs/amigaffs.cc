#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"
#include "lib/proto.h"
#include "lib/layout.h"
#include <fmt/format.h>

#include "adflib.h"
#include "adf_blk.h"
#include "dep/adflib/adf_nativ.h"
#undef min

#include <ctime>
#include <iomanip>

class AmigaFfsFilesystem;
static AmigaFfsFilesystem* currentAmigaFfs;

extern struct Env adfEnv;

static RETCODE adfInitDevice(struct Device* dev, char* name, BOOL ro);
static RETCODE adfNativeReadSector(struct Device*, int32_t, int, uint8_t*);
static RETCODE adfNativeWriteSector(struct Device*, int32_t, int, uint8_t*);
static BOOL adfIsDevNative(char*);
static RETCODE adfReleaseDevice(struct Device*);

static std::string modeToString(long access)
{
    std::stringstream ss;
    if (hasH(access))
        ss << 'H';
    if (hasS(access))
        ss << 'S';
    if (hasP(access))
        ss << 'P';
    if (hasA(access))
        ss << 'A';
    if (hasR(access))
        ss << 'R';
    if (hasW(access))
        ss << 'W';
    if (hasE(access))
        ss << 'E';
    if (hasD(access))
        ss << 'D';
    return ss.str();
}

class AmigaFfsFilesystem : public Filesystem
{
public:
    AmigaFfsFilesystem(
        const AmigaFfsProto& config, std::shared_ptr<SectorInterface> sectors):
        Filesystem(sectors),
        _config(config)
    {
    }

    void create()
    {
        eraseEverythingOnDisk();
        AdfMount m(this);

        struct Device dev = {};
        dev.readOnly = false;
        dev.isNativeDev = true;
        dev.devType = DEVTYPE_FLOPDD;
        dev.cylinders = config.layout().tracks();
        dev.heads = config.layout().sides();
        dev.sectors = Layout::getSectorsInTrack( Layout::getLayoutOfTrack(0, 0)).size();
        adfInitDevice(&dev, nullptr, false);
        int res = adfCreateFlop(&dev,
            (char*)"FluxEngine FFS", 0);
        if (res != RC_OK)
            throw CannotWriteException();
    }

    FilesystemStatus check()
    {
        return FS_OK;
    }

    std::vector<std::unique_ptr<Dirent>> list(const Path& path)
    {
        AdfMount m(this);

        std::vector<std::unique_ptr<Dirent>> results;

        auto* vol = m.mount();
        changeDir(vol, path);

        auto list = AdfList(adfGetDirEnt(vol, vol->curDirPtr));
        struct List* cell = list;
        while (cell)
        {
            auto* entry = (struct Entry*)cell->content;
            cell = cell->next;

            auto dirent = std::make_unique<Dirent>();
            dirent->filename = entry->name;
            dirent->length = entry->size;
            dirent->file_type =
                (entry->type == ST_FILE) ? TYPE_FILE : TYPE_DIRECTORY;
            dirent->mode = modeToString(entry->access);
            results.push_back(std::move(dirent));
        }

        return results;
    }

    std::map<std::string, std::string> getMetadata(const Path& path)
    {
        AdfMount m(this);
        if (path.size() == 0)
            throw BadPathException();

        auto* vol = m.mount();
        changeDirButOne(vol, path);

        auto entry = AdfEntry(adfFindEntry(vol, (char*)path.back().c_str()));
        if (!entry)
            throw BadPathException();

        std::map<std::string, std::string> attributes;
        attributes["filename"] = entry->name;
        attributes["length"] = fmt::format("{}", entry->size);
        attributes["type"] = (entry->type == ST_FILE) ? "file" : "dir";
        attributes["mode"] = modeToString(entry->access);
        attributes["amigaffs.comment"] = entry->comment;
        attributes["amigaffs.sector"] = entry->sector;

        std::tm tm = {.tm_sec = entry->secs,
            .tm_min = entry->mins,
            .tm_hour = entry->hour,
            .tm_mday = entry->days,
            .tm_mon = entry->month,
            .tm_year = entry->year - 1900};
        std::stringstream ss;
        ss << std::put_time(&tm, "%FT%T%z");
        attributes["amigaffs.mtime"] = ss.str();
        return attributes;
    }

    Bytes getFile(const Path& path)
    {
        AdfMount m(this);
        if (path.size() == 0)
            throw BadPathException();

        auto* vol = m.mount();
        changeDirButOne(vol, path);

        auto* file = adfOpenFile(vol, (char*)path.back().c_str(), (char*)"r");
        if (!file)
            throw FileNotFoundException();

        Bytes bytes;
        ByteWriter bw(bytes);
        while (!adfEndOfFile(file))
        {
            uint8_t buffer[4096];
            long done = adfReadFile(file, sizeof(buffer), buffer);

            bw += Bytes(buffer, done);
        }

        adfCloseFile(file);
        return bytes;
    }

    void putFile(const Path& path, const Bytes& data)
    {
        AdfMount m(this);
        if (path.size() == 0)
            throw BadPathException();

        auto* vol = m.mount();
        changeDirButOne(vol, path);

        auto* file = adfOpenFile(vol, (char*)path.back().c_str(), (char*)"w");
        if (!file)
            throw CannotWriteException();

        unsigned pos = 0;
        while (pos != data.size())
        {
            long done = adfWriteFile(
                file, data.size() - pos, (uint8_t*)data.cbegin() + pos);
            pos += done;
        }

        adfCloseFile(file);
    }

private:
    class AdfEntry
    {
    public:
        AdfEntry(struct Entry* entry): _entry(entry) {}

        ~AdfEntry()
        {
            if (_entry)
                adfFreeEntry(_entry);
        }

        operator struct Entry *() const
        {
            return _entry;
        }

        struct Entry* operator->() const
        {
            return _entry;
        }

    private:
        struct Entry* _entry;
    };

    class AdfList
    {
    public:
        AdfList(struct List* list): _list(list) {}

        ~AdfList()
        {
            if (_list)
                adfFreeDirList(_list);
        }

        operator struct List *() const
        {
            return _list;
        }

        struct List* operator->()
        {
            return _list;
        }

    private:
        struct List* _list;
    };

    class AdfMount
    {
    public:
        friend AmigaFfsFilesystem;
        AdfMount(AmigaFfsFilesystem* self): self(self)
        {
            currentAmigaFfs = self;
			self->_ffs = nullptr;

            adfEnvInitDefault();
        }

        ~AdfMount()
        {
            if (self->_ffs)
                adfUnMountDev(self->_ffs);
            adfEnvCleanUp();
        }

        struct Volume* mount()
        {
            self->_ffs = adfMountDev(nullptr, false);
            return adfMount(self->_ffs, 0, false);
        }

    private:
        AmigaFfsFilesystem* self;
    };

    void changeDir(struct Volume* vol, const Path& path)
    {
        adfToRootDir(vol);
        for (const auto& p : path)
        {
            RETCODE r = adfChangeDir(vol, (char*)p.c_str());
            if (r != RC_OK)
                throw FileNotFoundException();
        }
    }

    void changeDirButOne(struct Volume* vol, const Path& path)
    {
        adfToRootDir(vol);
        for (int i = 0; i < path.size() - 1; i++)
        {
            RETCODE r = adfChangeDir(vol, (char*)path[i].c_str());
            if (r != RC_OK)
                throw FileNotFoundException();
        }
    }

public:
    RETCODE adfInitDevice(struct Device* dev, char* name, BOOL ro)
    {
        dev->size = getLogicalSectorCount() * getLogicalSectorSize();
        return RC_OK;
    }

    RETCODE adfReleaseDevice(struct Device* dev)
    {
        return RC_OK;
    }

    RETCODE adfNativeReadSector(
        struct Device* dev, int32_t sector, int size, uint8_t* buffer)
    {
        auto bytes = getLogicalSector(sector, size / 512);
        memcpy(buffer, bytes.cbegin(), bytes.size());
        return RC_OK;
    }

    RETCODE adfNativeWriteSector(
        struct Device* dev, int32_t sector, int size, uint8_t* buffer)
    {
        Bytes bytes(buffer, size);
        putLogicalSector(sector, bytes);
        return RC_OK;
    }

private:
    const AmigaFfsProto& _config;
    struct Device* _ffs;
};

void adfInitNativeFct()
{
    auto cbs = (struct nativeFunctions*)adfEnv.nativeFct;
    cbs->adfInitDevice = ::adfInitDevice;
    cbs->adfNativeReadSector = ::adfNativeReadSector;
    cbs->adfNativeWriteSector = ::adfNativeWriteSector;
    cbs->adfIsDevNative = ::adfIsDevNative;
    cbs->adfReleaseDevice = ::adfReleaseDevice;
}

static RETCODE adfInitDevice(struct Device* dev, char* name, BOOL ro)
{
    return currentAmigaFfs->adfInitDevice(dev, name, ro);
}

static RETCODE adfReleaseDevice(struct Device* dev)
{
    return currentAmigaFfs->adfReleaseDevice(dev);
}

static RETCODE adfNativeReadSector(
    struct Device* dev, int32_t count, int size, uint8_t* buffer)
{
    return currentAmigaFfs->adfNativeReadSector(dev, count, size, buffer);
}

static RETCODE adfNativeWriteSector(
    struct Device* dev, int32_t count, int size, uint8_t* buffer)
{
    return currentAmigaFfs->adfNativeWriteSector(dev, count, size, buffer);
}

static BOOL adfIsDevNative(char*)
{
    return true;
}

std::unique_ptr<Filesystem> Filesystem::createAmigaFfsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<AmigaFfsFilesystem>(config.amigaffs(), sectors);
}
