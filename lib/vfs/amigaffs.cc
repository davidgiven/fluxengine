#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "lib/config/proto.h"
#include "lib/data/layout.h"
#include "lib/core/logger.h"

#include "adflib.h"
#include "adf_blk.h"
#include "adf_nativ.h"
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

    uint32_t capabilities() const override
    {
        return OP_GETFSDATA | OP_CREATE | OP_LIST | OP_GETFILE | OP_PUTFILE |
               OP_GETDIRENT | OP_DELETE | OP_MOVE | OP_CREATEDIR;
    }

    std::map<std::string, std::string> getMetadata() override
    {
        AdfMount m(this);
        auto* vol = m.mount();

        std::map<std::string, std::string> attributes;
        attributes[VOLUME_NAME] = vol->volName;

        int total = vol->lastBlock - vol->firstBlock + 1;
        attributes[TOTAL_BLOCKS] = std::to_string(total);
        attributes[USED_BLOCKS] =
            std::to_string(total - adfCountFreeBlocks(vol));
        attributes[BLOCK_SIZE] = "512";
        return attributes;
    }

    void create(bool quick, const std::string& volumeName) override
    {
        if (!quick)
            eraseEverythingOnDisk();
        AdfMount m(this);

        struct Device dev = {};
        dev.readOnly = false;
        dev.isNativeDev = true;
        dev.devType = DEVTYPE_FLOPDD;
        dev.cylinders = globalConfig()->layout().tracks();
        dev.heads = globalConfig()->layout().sides();
        dev.sectors = Layout::getLayoutOfTrack(0, 0)->numSectors;
        adfInitDevice(&dev, nullptr, false);
        int res = adfCreateFlop(&dev, (char*)volumeName.c_str(), 0);
        if (res != RC_OK)
            throw CannotWriteException();
    }

    FilesystemStatus check() override
    {
        return FS_OK;
    }

    std::vector<std::shared_ptr<Dirent>> list(const Path& path) override
    {
        AdfMount m(this);

        std::vector<std::shared_ptr<Dirent>> results;

        auto* vol = m.mount();
        changeDir(vol, path);

        auto list = AdfList(adfGetDirEnt(vol, vol->curDirPtr));
        struct List* cell = list;
        while (cell)
        {
            auto* entry = (struct Entry*)cell->content;
            cell = cell->next;

            results.push_back(toDirent(entry, path));
        }

        return results;
    }

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        AdfMount m(this);
        if (path.size() == 0)
            throw BadPathException();

        auto* vol = m.mount();
        changeDirButOne(vol, path);

        auto entry = AdfEntry(adfFindEntry(vol, (char*)path.back().c_str()));
        if (!entry)
            throw FileNotFoundException();

        return toDirent(entry, path.parent());
    }

    Bytes getFile(const Path& path) override
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

    void putFile(const Path& path, const Bytes& data) override
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

    void deleteFile(const Path& path) override
    {
        AdfMount m(this);
        if (path.size() == 0)
            throw BadPathException();

        auto* vol = m.mount();
        changeDirButOne(vol, path);

        int res =
            adfRemoveEntry(vol, vol->curDirPtr, (char*)path.back().c_str());
        if (res != RC_OK)
            throw CannotWriteException();
    }

    void moveFile(const Path& oldPath, const Path& newPath) override
    {
        AdfMount m(this);
        if ((oldPath.size() == 0) || (newPath.size() == 0))
            throw BadPathException();

        auto* vol = m.mount();

        changeDirButOne(vol, oldPath);
        auto oldDir = vol->curDirPtr;

        changeDirButOne(vol, newPath);
        auto newDir = vol->curDirPtr;

        int res = adfRenameEntry(vol,
            oldDir,
            (char*)oldPath.back().c_str(),
            newDir,
            (char*)newPath.back().c_str());
        if (res != RC_OK)
            throw CannotWriteException();
    }

    void createDirectory(const Path& path) override
    {
        AdfMount m(this);
        if (path.empty())
            throw BadPathException();

        auto* vol = m.mount();
        changeDirButOne(vol, path);
        int res = adfCreateDir(vol, vol->curDirPtr, (char*)path.back().c_str());
        if (res != RC_OK)
            throw CannotWriteException();
    }

private:
    std::shared_ptr<Dirent> toDirent(struct Entry* entry, const Path& container)
    {
        auto dirent = std::make_shared<Dirent>();

        dirent->path = container;
        dirent->path.push_back(entry->name);
        dirent->filename = entry->name;
        dirent->length = entry->size;
        dirent->file_type =
            (entry->type == ST_FILE) ? TYPE_FILE : TYPE_DIRECTORY;
        dirent->mode = modeToString(entry->access);

        dirent->attributes[FILENAME] = entry->name;
        dirent->attributes[LENGTH] = std::to_string(entry->size);
        dirent->attributes[FILE_TYPE] =
            (entry->type == ST_FILE) ? "file" : "dir";
        dirent->attributes[MODE] = modeToString(entry->access);
        dirent->attributes["amigaffs.comment"] = entry->comment;

        std::tm tm = {.tm_sec = entry->secs,
            .tm_min = entry->mins,
            .tm_hour = entry->hour,
            .tm_mday = entry->days,
            .tm_mon = entry->month,
            .tm_year = entry->year - 1900};
        std::stringstream ss;
        ss << std::put_time(&tm, "%FT%T%z");
        dirent->attributes["amigaffs.mtime"] = ss.str();

        return dirent;
    }

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
            if (vol)
                adfUnMount(vol);
            if (self->_ffs)
                adfUnMountDev(self->_ffs);
            adfEnvCleanUp();
        }

        struct Volume* mount()
        {
            self->_ffs = adfMountDev(nullptr, false);
            if (!self->_ffs)
                throw BadFilesystemException();
            vol = adfMount(self->_ffs, 0, false);
            if (!vol)
                throw BadFilesystemException();
            return vol;
        }

    private:
        AmigaFfsFilesystem* self;
        struct Volume* vol = nullptr;
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

static void onAdfWarning(char* message)
{
    log((const char*)message);
}

static void onAdfError(char* message)
{
    throw FilesystemException(message);
}

void adfInitNativeFct()
{
    auto cbs = (struct nativeFunctions*)adfEnv.nativeFct;
    cbs->adfInitDevice = ::adfInitDevice;
    cbs->adfNativeReadSector = ::adfNativeReadSector;
    cbs->adfNativeWriteSector = ::adfNativeWriteSector;
    cbs->adfIsDevNative = ::adfIsDevNative;
    cbs->adfReleaseDevice = ::adfReleaseDevice;

    adfChgEnvProp(PR_WFCT, (void*)onAdfWarning);
    adfChgEnvProp(PR_EFCT, (void*)onAdfError);
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
