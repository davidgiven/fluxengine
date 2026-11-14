package org.rob_smallshire.oaknut.filesystem;

import java.io.IOException;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Described repairs remain explicit: validation never changes an image. */
public record Fix(String description, ValidationFinding.Repair action)
    implements ValidationFinding.Repair
{
    @Override public void apply() throws IOException
    { action.apply(); }
}
