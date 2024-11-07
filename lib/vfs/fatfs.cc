#include "lib/core/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config/config.pb.h"
#include "lib/core/utils.h"

extern "C"
{
#include "ff.h"
#include "diskio.h"
}

class FatFsFilesystem;
static FatFsFilesystem* currentFatFs;

static std::string modeToString(BYTE attrib)
{
    std::stringstream ss;
    if (attrib & AM_RDO)
        ss << 'R';
    if (attrib & AM_HID)
        ss << 'H';
    if (attrib & AM_SYS)
        ss << 'S';
    if (attrib & AM_ARC)
        ss << 'A';
    return ss.str();
}

class FatFsFilesystem : public Filesystem
{
public:
    FatFsFilesystem(
        const FatFsProto& config, std::shared_ptr<SectorInterface> sectors):
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
        mount();

        std::map<std::string, std::string> attributes;

        {
            char buffer[34];
            FRESULT res = f_getlabel("", buffer, nullptr);
            throwError(res);
            attributes[VOLUME_NAME] = buffer;
        }

        {
            FATFS* fs;
            DWORD free_clusters;
            FRESULT res = f_getfree("", &free_clusters, &fs);
            throwError(res);
            int total = (fs->n_fatent - 2) + (fs->database / fs->csize);
            attributes[TOTAL_BLOCKS] = std::to_string(total);
            attributes[USED_BLOCKS] = std::to_string(total - free_clusters);
            attributes[BLOCK_SIZE] =
                std::to_string(fs->csize * getLogicalSectorSize(0));
        }

        return attributes;
    }

    void create(bool quick, const std::string& volumeName) override
    {
        if (!quick)
            eraseEverythingOnDisk();

        char buffer[FF_MAX_SS * 2];
        currentFatFs = this;
        MKFS_PARM parm = {
            .fmt = FM_SFD | FM_ANY,
            .n_root = _config.root_directory_entries(),
            .au_size = _config.cluster_size(),
        };
        FRESULT res = f_mkfs("", &parm, buffer, sizeof(buffer));
        throwError(res);

        mount();
        f_setlabel(volumeName.c_str());
    }

    FilesystemStatus check() override
    {
        return FS_OK;
    }

    std::vector<std::shared_ptr<Dirent>> list(const Path& path) override
    {
        mount();

        DIR dir;
        auto pathstr = toUpper(path.to_str());
        FRESULT res = f_opendir(&dir, pathstr.c_str());
        std::vector<std::shared_ptr<Dirent>> results;

        for (;;)
        {
            FILINFO filinfo;
            res = f_readdir(&dir, &filinfo);
            if (res != FR_OK)
                throw BadFilesystemException();
            if (filinfo.fname[0] == 0)
                break;

            results.push_back(toDirent(filinfo, path));
        }

        f_closedir(&dir);
        return results;
    }

    std::shared_ptr<Dirent> getDirent(const Path& path) override
    {
        std::map<std::string, std::string> attributes;

        mount();
        auto pathstr = toUpper(path.to_str());
        FILINFO filinfo;
        FRESULT res = f_stat(pathstr.c_str(), &filinfo);
        throwError(res);

        return toDirent(filinfo, path.parent());
    }

    Bytes getFile(const Path& path) override
    {
        mount();
        auto pathstr = toUpper(path.to_str());
        FIL fil;
        FRESULT res = f_open(&fil, pathstr.c_str(), FA_READ);
        throwError(res);

        Bytes bytes;
        ByteWriter bw(bytes);

        while (!f_eof(&fil))
        {
            uint8_t buffer[4096];
            UINT done;
            res = f_read(&fil, buffer, sizeof(buffer), &done);
            throwError(res);

            bw += Bytes(buffer, done);
        }

        f_close(&fil);

        return bytes;
    }

    void putFile(const Path& path, const Bytes& bytes) override
    {
        mount();
        auto pathstr = toUpper(path.to_str());
        FIL fil;
        FRESULT res =
            f_open(&fil, pathstr.c_str(), FA_WRITE | FA_CREATE_ALWAYS);
        throwError(res);

        unsigned remaining = bytes.size();
        char* ptr = (char*)bytes.cbegin();
        while (remaining != 0)
        {
            UINT done;
            res = f_write(&fil, ptr, remaining, &done);
            throwError(res);

            remaining -= done;
            ptr += done;
        }

        f_close(&fil);
    }

    void deleteFile(const Path& path) override
    {
        mount();
        auto pathstr = toUpper(path.to_str());
        FRESULT res = f_unlink(pathstr.c_str());
        throwError(res);
    }

    void moveFile(const Path& oldPath, const Path& newPath) override
    {
        mount();
        auto oldPathStr = toUpper(oldPath.to_str());
        auto newPathStr = toUpper(newPath.to_str());
        FRESULT res = f_rename(oldPathStr.c_str(), newPathStr.c_str());
        throwError(res);
    }

    void createDirectory(const Path& path) override
    {
        mount();
        auto pathStr = path.to_str();
        FRESULT res = f_mkdir(pathStr.c_str());
        throwError(res);
    }

private:
    std::shared_ptr<Dirent> toDirent(FILINFO& filinfo, const Path& parent)
    {
        auto dirent = std::make_shared<Dirent>();
        dirent->filename = toUpper(filinfo.fname);
        dirent->path = parent;
        dirent->path.push_back(dirent->filename);
        dirent->length = filinfo.fsize;
        dirent->file_type =
            (filinfo.fattrib & AM_DIR) ? TYPE_DIRECTORY : TYPE_FILE;
        dirent->mode = modeToString(filinfo.fattrib);

        dirent->attributes[Filesystem::FILENAME] = filinfo.fname;
        dirent->attributes[Filesystem::LENGTH] = std::to_string(filinfo.fsize);
        dirent->attributes[Filesystem::FILE_TYPE] =
            (filinfo.fattrib & AM_DIR) ? "dir" : "file";
        dirent->attributes[Filesystem::MODE] = modeToString(filinfo.fattrib);
        return dirent;
    }

public:
    DRESULT diskRead(BYTE* buffer, LBA_t sector, UINT count)
    {
        auto bytes = getLogicalSector(sector, count);
        memcpy(buffer, bytes.cbegin(), bytes.size());
        return RES_OK;
    }

    DRESULT diskWrite(const BYTE* buffer, LBA_t sector, UINT count)
    {
        Bytes bytes(buffer, count * getLogicalSectorSize());
        putLogicalSector(sector, bytes);
        return RES_OK;
    }

    DRESULT diskIoctl(BYTE cmd, void* buffer)
    {
        switch (cmd)
        {
            case GET_SECTOR_SIZE:
                *(WORD*)buffer = getLogicalSectorSize();
                break;

            case GET_SECTOR_COUNT:
                *(LBA_t*)buffer = getLogicalSectorCount();
                break;

            case CTRL_SYNC:
                break;

            default:
                return RES_PARERR;
        }
        return RES_OK;
    }

private:
    void mount()
    {
        currentFatFs = this;
        f_mount(&_fs, "", 1);
    }

    void throwError(FRESULT res)
    {
        switch (res)
        {
            case FR_OK:
                return;

            case FR_NO_FILE:
            case FR_NO_PATH:
                throw FileNotFoundException();

            case FR_INVALID_NAME:
                throw BadPathException();

            case FR_DENIED:
            case FR_EXIST:
            case FR_WRITE_PROTECTED:
            case FR_MKFS_ABORTED:
            case FR_LOCKED:
                throw CannotWriteException();

            case FR_NO_FILESYSTEM:
                throw BadFilesystemException();

            default:
                throw FilesystemException(
                    fmt::format("unknown fatfs error {}", (int)res));
        }
    }

private:
    const FatFsProto& _config;
    FATFS _fs;
};

DSTATUS disk_initialize(BYTE)
{
    return RES_OK;
}

DSTATUS disk_status(BYTE)
{
    return 0;
}

DRESULT disk_read(BYTE, BYTE* buffer, LBA_t sector, UINT count)
{
    return currentFatFs->diskRead(buffer, sector, count);
}

DRESULT disk_write(BYTE, const BYTE* buffer, LBA_t sector, UINT count)
{
    return currentFatFs->diskWrite(buffer, sector, count);
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buffer)
{
    return currentFatFs->diskIoctl(cmd, buffer);
}

DWORD get_fattime(void)
{
    return 0;
}

std::unique_ptr<Filesystem> Filesystem::createFatFsFilesystem(
    const FilesystemProto& config, std::shared_ptr<SectorInterface> sectors)
{
    return std::make_unique<FatFsFilesystem>(config.fatfs(), sectors);
}
