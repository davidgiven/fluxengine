package org.rob_smallshire.oaknut.dfs;

import com.google.common.collect.ImmutableList;
import org.rob_smallshire.oaknut.discimage.SurfaceSpec;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE. */
public enum DfsFormat
{
    ACORN_DFS_40T_SINGLE_SIDED(40, 1, false, false),
    ACORN_DFS_40T_DOUBLE_SIDED_INTERLEAVED(40, 2, false, false),
    ACORN_DFS_40T_DOUBLE_SIDED_SEQUENTIAL(40, 2, true, false),
    ACORN_DFS_80T_SINGLE_SIDED(80, 1, false, false),
    ACORN_DFS_80T_DOUBLE_SIDED_INTERLEAVED(80, 2, false, false),
    ACORN_DFS_80T_DOUBLE_SIDED_SEQUENTIAL(80, 2, true, false),
    WATFORD_DFS_40T_SINGLE_SIDED(40, 1, false, true),
    WATFORD_DFS_40T_DOUBLE_SIDED_INTERLEAVED(40, 2, false, true),
    WATFORD_DFS_40T_DOUBLE_SIDED_SEQUENTIAL(40, 2, true, true),
    WATFORD_DFS_80T_SINGLE_SIDED(80, 1, false, true),
    WATFORD_DFS_80T_DOUBLE_SIDED_INTERLEAVED(80, 2, false, true),
    WATFORD_DFS_80T_DOUBLE_SIDED_SEQUENTIAL(80, 2, true, true);

    public final int tracks, sides;
    public final boolean sequential, watford;

    DfsFormat(int tracks, int sides, boolean sequential, boolean watford)
    {
        this.tracks = tracks;
        this.sides = sides;
        this.sequential = sequential;
        this.watford = watford;
    }

    /* An extension identifies the image layout; the catalogue distinguishes Acorn and Watford. */
    public static DfsFormat detect(String filename, int size) throws DfsFormatException
    {
        String lower = filename.toLowerCase(java.util.Locale.ROOT);
        int sides = lower.endsWith(".ssd") ? 1 : lower.endsWith(".dsd") ? 2 : 0;
        if (sides == 0 || size <= 0 || size > sides * 204800)
            throw new DfsFormatException("Cannot infer DFS format; specify the format explicitly");
        int tracks = size <= sides * 102400 ? 40 : 80;
        return valueOf("ACORN_DFS_" + tracks + "T_" +
                (sides == 1 ? "SINGLE_SIDED" : "DOUBLE_SIDED_INTERLEAVED"));
    }

    public int size()
    {
        return tracks * sides * 10 * 256;
    }

    public ImmutableList<SurfaceSpec> surfaces()
    {
        ImmutableList.Builder<SurfaceSpec> result = ImmutableList.builder();
        for (int side = 0; side < sides; side++)
            result.add(new SurfaceSpec(
                    tracks,
                    10,
                    256,
                    side * 2560 * (sequential ? tracks : 1),
                    2560 * (sequential ? 1 : sides)));
        return result.build();
    }

    public DfsFormat withWatford(boolean value)
    {
        return valueOf((value ? "WATFORD" : "ACORN") + name().substring(name().indexOf("_DFS_")));
    }
}
