package org.rob_smallshire.oaknut.adfs;

import com.google.common.collect.ImmutableList;
import java.util.ArrayList;
import org.rob_smallshire.oaknut.discimage.SurfaceSpec;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Supported floppy formats. L_SEQUENTIAL stores all of each side consecutively. */
public enum AdfsFormat
{
    S(40, 1, 16, 256, 0, DirectoryFormat.OLD),
    M(80, 1, 16, 256, 0, DirectoryFormat.OLD),
    L(80, 2, 16, 256, 0, DirectoryFormat.OLD),
    L_SEQUENTIAL(80, 2, 16, 256, 0, DirectoryFormat.OLD),
    D(80, 2, 5, 1024, 0, DirectoryFormat.NEW),
    E(80, 2, 5, 1024, 1, DirectoryFormat.NEW),
    E_PLUS(80, 2, 5, 1024, 1, DirectoryFormat.BIG),
    F(80, 2, 10, 1024, 4, DirectoryFormat.NEW),
    F_PLUS(80, 2, 10, 1024, 4, DirectoryFormat.BIG),
    G(80, 2, 20, 1024, 8, DirectoryFormat.NEW),
    G_PLUS(80, 2, 20, 1024, 8, DirectoryFormat.BIG);

    public final int tracks, heads, sectorsPerTrack, sectorSize, zones;
    public final DirectoryFormat directoryFormat;
    AdfsFormat(int tracks, int heads, int spt, int sectorSize, int zones, DirectoryFormat directory)
    {
        this.tracks = tracks;
        this.heads = heads;
        this.sectorsPerTrack = spt;
        this.sectorSize = sectorSize;
        this.zones = zones;
        this.directoryFormat = directory;
    }
    public int size()
    { return tracks * heads * sectorsPerTrack * sectorSize; }
    public boolean newMap()
    { return zones > 0; }
    public boolean big()
    { return directoryFormat == DirectoryFormat.BIG; }
    public ImmutableList<SurfaceSpec> surfaces()
    {
        if (sectorSize == 1024)
            return ImmutableList.of(new SurfaceSpec(1, size() / 256, 256, 0, size()));
        ArrayList<SurfaceSpec> result = new ArrayList<>();
        int track = sectorsPerTrack * sectorSize;
        for (int side = 0; side < heads; side++)
            result.add(new SurfaceSpec(tracks,
                                       sectorsPerTrack,
                                       sectorSize,
                                       side * (this == L_SEQUENTIAL ? track * tracks : track),
                                       track * (this == L_SEQUENTIAL ? 1 : heads)));
        return ImmutableList.copyOf(result);
    }

    public static AdfsFormat forExtension(String extension) throws AdfsFormatException
    {
        return switch (extension.toLowerCase(java.util.Locale.ROOT))
        {
            case "ads", ".ads" -> S;
            case "adm", ".adm" -> M;
            case "adl", ".adl" -> L;
            default -> throw new AdfsFormatException("Ambiguous floppy extension: " + extension);
        };
    }
}
