#include "lib/globals.h"
#include "lib/vfs/vfs.h"
#include "lib/config.pb.h"
#include <fmt/format.h>

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

    FilesystemStatus check()
    {
        return FS_OK;
    }

    std::vector<std::unique_ptr<Dirent>> list(const Path& path)
    {
        mount();

        DIR dir;
        auto pathstr = path.to_str();
        FRESULT res = f_opendir(&dir, pathstr.c_str());
        std::vector<std::unique_ptr<Dirent>> results;

        for (;;)
        {
            FILINFO filinfo;
            res = f_readdir(&dir, &filinfo);
            if (res != FR_OK)
                throw BadFilesystemException();
            if (filinfo.fname[0] == 0)
                break;

            auto dirent = std::make_unique<Dirent>();
            dirent->filename = filinfo.fname;
            dirent->length = filinfo.fsize;
            dirent->file_type =
                (filinfo.fattrib & AM_DIR) ? TYPE_DIRECTORY : TYPE_FILE;
            dirent->mode = modeToString(filinfo.fattrib);
            results.push_back(std::move(dirent));
        }

        f_closedir(&dir);
        return results;
    }

    std::map<std::string, std::string> getMetadata(const Path& path)
    {
        std::map<std::string, std::string> attributes;

        mount();
        auto pathstr = path.to_str();
        FILINFO filinfo;
        FRESULT res = f_stat(pathstr.c_str(), &filinfo);
        throwError(res);

        attributes["filename"] = filinfo.fname;
        attributes["length"] = fmt::format("{}", filinfo.fsize);
        attributes["type"] = (filinfo.fattrib & AM_DIR) ? "dir" : "file";
        attributes["mode"] = modeToString(filinfo.fattrib);

        return attributes;
    }

    Bytes getFile(const Path& path)
    {
        mount();
        auto pathstr = path.to_str();
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

public:
    DRESULT diskRead(BYTE* buffer, LBA_t sector, UINT count)
    {
        auto bytes = getLogicalSector(sector, count);
        memcpy(buffer, bytes.cbegin(), bytes.size());
        return RES_OK;
    }

    DRESULT diskWrite(const BYTE* buffer, LBA_t sector, UINT count)
    {
        return RES_WRPRT;
    }

    DRESULT diskIoctl(BYTE cmd, void* buffer)
    {
        switch (cmd)
        {
            case GET_SECTOR_SIZE:
                *(DWORD*)buffer = getLogicalSectorSize();
                break;
            case GET_SECTOR_COUNT:
                *(DWORD*)buffer = getLogicalSectorCount();
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

            case FR_NO_FILESYSTEM:
                throw BadFilesystemException();

            default:
                throw FilesystemException();
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
