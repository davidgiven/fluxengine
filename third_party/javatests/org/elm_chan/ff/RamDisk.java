package org.elm_chan.ff;

/**
 * Bespoke in-memory block device for tests. Backed by a byte[] of
 * {@code sectorCount * 512} bytes. Implements {@link DiskIo} without any
 * external dependency.
 */
public final class RamDisk implements DiskIo {

    private final byte[] data;
    private final int sectorCount;
    private boolean writeProtected = false;

    public RamDisk(int sectorCount) {
        this.sectorCount = sectorCount;
        this.data = new byte[sectorCount * 512];
    }

    public int getSectorCount() {
        return sectorCount;
    }

    public byte[] getData() {
        return data;
    }

    public void setWriteProtected(boolean wp) {
        this.writeProtected = wp;
    }

    @Override
    public int diskInitialize() {
        return 0;
    }

    @Override
    public int diskStatus() {
        if (writeProtected) {
            return STA_PROTECT;
        }
        return 0;
    }

    @Override
    public DResult diskRead(long sector, byte[] buff, int count) {
        if (sector < 0 || sector + count > sectorCount) {
            return DResult.RES_PARERR;
        }
        if (buff.length < count * 512) {
            return DResult.RES_PARERR;
        }
        System.arraycopy(data, (int) sector * 512, buff, 0, count * 512);
        return DResult.RES_OK;
    }

    @Override
    public DResult diskWrite(long sector, byte[] buff, int count) {
        if (writeProtected) {
            return DResult.RES_WRPRT;
        }
        if (sector < 0 || sector + count > sectorCount) {
            return DResult.RES_PARERR;
        }
        if (buff.length < count * 512) {
            return DResult.RES_PARERR;
        }
        System.arraycopy(buff, 0, data, (int) sector * 512, count * 512);
        return DResult.RES_OK;
    }

    @Override
    public DResult diskIoctl(int cmd, Object buff) {
        if (cmd == CTRL_SYNC) {
            return DResult.RES_OK;
        }
        if (cmd == GET_SECTOR_COUNT && buff instanceof int[]) {
            ((int[]) buff)[0] = sectorCount;
            return DResult.RES_OK;
        }
        if (cmd == GET_SECTOR_SIZE && buff instanceof int[]) {
            ((int[]) buff)[0] = 512;
            return DResult.RES_OK;
        }
        if (cmd == GET_BLOCK_SIZE && buff instanceof int[]) {
            ((int[]) buff)[0] = 1;
            return DResult.RES_OK;
        }
        return DResult.RES_PARERR;
    }
}
