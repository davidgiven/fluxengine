package com.cowlark.fluxengine.config;

import static com.cowlark.fluxengine.config.ImageFormats.Mode.MODE_RO;
import static com.cowlark.fluxengine.config.ImageFormats.Mode.MODE_RW;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_D64;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_D88;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_DIM;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_DISKCOPY;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_FDI;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_IMD;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_IMG;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_JV3;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_NFD;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_NSI;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_TD0;

import com.google.common.collect.ImmutableList;

public class ImageFormats
{
    public enum Mode
    {
        MODE_RO, MODE_RW;
    }

    public record ImageFormat(String extension, ImageReaderWriterType type, Mode mode)
    {
    }

    public static ImmutableList<ImageFormat> imageFormats = ImmutableList.of(
            new ImageFormat(".adf", IMAGETYPE_IMG, MODE_RW),
            new ImageFormat(".d64", IMAGETYPE_D64, MODE_RW),
            new ImageFormat(".d81", IMAGETYPE_IMG, MODE_RW),
            new ImageFormat(".d88", IMAGETYPE_D88, MODE_RW),
            new ImageFormat(".dim", IMAGETYPE_DIM, MODE_RO),
            new ImageFormat(".diskcopy", IMAGETYPE_DISKCOPY, MODE_RW),
            new ImageFormat(".dsk", IMAGETYPE_IMG, MODE_RW),
            new ImageFormat(".fdi", IMAGETYPE_FDI, MODE_RO),
            new ImageFormat(".imd", IMAGETYPE_IMD, MODE_RW),
            new ImageFormat(".img", IMAGETYPE_IMG, MODE_RW),
            new ImageFormat(".jv3", IMAGETYPE_JV3, MODE_RO),
            new ImageFormat(".nfd", IMAGETYPE_NFD, MODE_RO),
            new ImageFormat(".nsi", IMAGETYPE_NSI, MODE_RW),
            new ImageFormat(".st", IMAGETYPE_IMG, MODE_RW),
            new ImageFormat(".td0", IMAGETYPE_TD0, MODE_RO),
            new ImageFormat(".trd", IMAGETYPE_IMG, MODE_RW),
            new ImageFormat(".vgi", IMAGETYPE_IMG, MODE_RW),
            new ImageFormat(".xdf", IMAGETYPE_IMG, MODE_RW));

    private ImageFormats()
    {
    }
}
