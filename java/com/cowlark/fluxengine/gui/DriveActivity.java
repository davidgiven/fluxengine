package com.cowlark.fluxengine.gui;

public record DriveActivity(ActivityType type, int cylinder, int head)
{
    enum ActivityType
    {
        IDLE, READING, WRITING
    }
}
