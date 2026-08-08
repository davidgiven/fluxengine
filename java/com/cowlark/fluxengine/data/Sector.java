package com.cowlark.fluxengine.data;

import com.cowlark.fluxengine.core.Bytes;
import java.util.ArrayList;
import java.util.List;

/**
 * A sector, ported from lib/data/sector.h.
 */
public class Sector
{
    public enum Status
    {
        OK,
        BAD_CHECKSUM,
        MISSING,
        DATA_MISSING,
        CONFLICT,
        INTERNAL_ERROR
    }

    /* The logical location of this sector. */

    public LogicalLocation location;

    public Status status = Status.INTERNAL_ERROR;
    public int position = 0;
    public double clockNs = 0.0;
    public double headerStartTimeNs = 0.0;
    public double headerEndTimeNs = 0.0;
    public double dataStartTimeNs = 0.0;
    public double dataEndTimeNs = 0.0;
    public CylinderHead physicalLocation = null;
    public Bytes data = new Bytes();
    public List<Record> records = new ArrayList<>();

    public Sector(LogicalLocation location)
    {
        this.location = location;
    }

    public static String statusToString(Status status)
    {
        switch (status)
        {
            case OK:
                return "OK";
            case BAD_CHECKSUM:
                return "bad checksum";
            case MISSING:
                return "sector not found";
            case DATA_MISSING:
                return "present but no data found";
            case CONFLICT:
                return "conflicting data";
            default:
                return String.format("unknown error %d", status.ordinal());
        }
    }

    public static String statusToChar(Status status)
    {
        switch (status)
        {
            case OK:
                return "";
            case MISSING:
                return "?";
            case BAD_CHECKSUM:
            case DATA_MISSING:
                return "!";
            case CONFLICT:
                return "*";
            default:
                return "?";
        }
    }

    public static Status stringToStatus(String value)
    {
        if (value.equals("OK"))
            return Status.OK;
        if (value.equals("bad checksum"))
            return Status.BAD_CHECKSUM;
        if (value.equals("sector not found") || value.equals("MISSING"))
            return Status.MISSING;
        if (value.equals("present but no data found"))
            return Status.DATA_MISSING;
        if (value.equals("conflicting data"))
            return Status.CONFLICT;
        return Status.INTERNAL_ERROR;
    }
}
