package org.rob_smallshire.oaknut.adfs;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE. */
public class AdfsDirectoryNotEmptyException extends AdfsPathException
{
    public AdfsDirectoryNotEmptyException(String message)
    {
        super(message);
    }
}
