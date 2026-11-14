package org.rob_smallshire.oaknut.file;

import java.time.Instant;
import java.time.Duration;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * The timezone-free Acorn epoch is represented as UTC in the Java API. */
public final class Datestamp
{
    public static final Instant EPOCH = Instant.parse("1900-01-01T00:00:00Z");
    private Datestamp() {}
    public static long encode(Instant when)
    {
        Duration delta = Duration.between(EPOCH, when);
        long cs =
            Math.addExact(Math.multiplyExact(delta.getSeconds(), 100), delta.getNano() / 10000000);
        if (cs < 0 || cs >= (1L << 40))
            throw new IllegalArgumentException("Datestamp out of range");
        return cs;
    }
}
