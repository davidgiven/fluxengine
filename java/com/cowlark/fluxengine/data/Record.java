package com.cowlark.fluxengine.data;

import com.cowlark.fluxengine.core.Bytes;
import java.util.ArrayList;
import java.util.List;

/**
 * A single record on a track, ported from lib/data/disk.h.
 */
public class Record
{
    public double clockNs = 0.0;
    public double startTimeNs = 0.0;
    public double endTimeNs = 0.0;
    public int position = 0;
    public Bytes rawData = new Bytes();

    public Record()
    {
    }
}