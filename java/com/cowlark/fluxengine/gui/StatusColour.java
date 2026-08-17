package com.cowlark.fluxengine.gui;

import javax.swing.UIManager;
import java.awt.Color;

public enum StatusColour
{
    NOT_PRESENT("Actions.Grey"), OK("Actions.Yellow"), BAD("Actions.Red"), MISSING("Actions.Blue");

    private final String name;

    StatusColour(String name)
    {
        this.name = name;
    }

    public Color getColour()
    {
        return UIManager.getColor(name);
    }
}
