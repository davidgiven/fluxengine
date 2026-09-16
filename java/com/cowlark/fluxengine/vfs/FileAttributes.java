package com.cowlark.fluxengine.vfs;

import lombok.Getter;

public enum FileAttributes
{
    FILENAME("Filename"),
    LENGTH("Length (bytes)"),
    MODE("Mode"),
    FILE_TYPE("File type"),
    CTIME("Creation time");

    @Getter private final String humanName;

    FileAttributes(String humanName)
    {
        this.humanName = humanName;
    }

    public static String getHumanName(String attr)
    {
        try
        {
            return FileAttributes.valueOf(attr).getHumanName();
        } catch (IllegalArgumentException e)
        {
            return attr;
        }
    }
}
