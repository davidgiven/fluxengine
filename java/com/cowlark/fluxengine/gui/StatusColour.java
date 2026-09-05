package com.cowlark.fluxengine.gui;

import java.awt.Color;

public enum StatusColour
{
    NOT_PRESENT(UiUtils.NO_DATA_COLOUR),
    OK(UiUtils.DATA_OK_COLOUR),
    BAD(UiUtils.DATA_BAD_COLOUR),
    MISSING(UiUtils.NO_DATA_COLOUR);

    private final Color colour;

    StatusColour(Color colour)
    {
        this.colour = colour;
    }

    public Color getColour()
    {
        return colour;
    }
}
