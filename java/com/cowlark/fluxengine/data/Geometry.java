package com.cowlark.fluxengine.data;

/**
 * The geometry of a disk image, ported from lib/data/image.h.
 */
public class Geometry
{
    public int numCylinders = 0;
    public int numHeads = 0;
    public int firstSector = Integer.MAX_VALUE;
    public int numSectors = 0;
    public int sectorSize = 0;
    public boolean irregular = false;
    public int totalBytes = 0;
}