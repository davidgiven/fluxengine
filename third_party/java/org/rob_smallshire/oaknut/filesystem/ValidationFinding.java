package org.rob_smallshire.oaknut.filesystem;

import java.io.IOException;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * A validation diagnostic optionally carrying an explicit repair. */
public record ValidationFinding(String path, String message, Repair repair)
{
    @Override
    public String toString()
    {
        return path + ": " + message;
    }

    public boolean repairable()
    {
        return repair != null;
    }

    public void fix() throws IOException
    {
        if (repair == null)
            throw new UnsupportedOperationException("No repair for " + message);
        repair.apply();
    }

    @FunctionalInterface
    public interface Repair
    {
        void apply() throws IOException;
    }
}
