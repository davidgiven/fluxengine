package com.cowlark.fluxengine.vfs;

public enum FilesystemAttributes
{
    VOLUME_NAME("Volume name"),
    TOTAL_BLOCKS("Total blocks"),
    USED_BLOCKS("Used blocks"),
    BLOCK_SIZE("Block size");

    private final String humanName;

    FilesystemAttributes(String humanName)
    {
        this.humanName = humanName;
    }

}
