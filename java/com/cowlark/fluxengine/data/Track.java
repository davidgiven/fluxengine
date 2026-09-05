package com.cowlark.fluxengine.data;

import java.util.ArrayList;
import java.util.List;

/**
 * A decoded track, ported from lib/data/disk.h.
 */
public class Track
{
    public LogicalTrackLayout ltl;
    public PhysicalTrackLayout ptl;
    public Fluxmap fluxmap;
    public List<Record> records = new ArrayList<>();
    public List<Sector> allSectors = new ArrayList<>();
    public List<Sector> normalisedSectors = new ArrayList<>();
}