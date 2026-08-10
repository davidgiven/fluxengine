package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.gui.Gui;
import com.google.common.collect.ImmutableList;

public class GuiCommand implements Command
{
    @Override
    public String getHelp()
    {
        return "Launch the GUI.";
    }

    @Override
    public void run(ImmutableList<String> args) throws Exception
    {
        new Gui().run(args);
    }
}
