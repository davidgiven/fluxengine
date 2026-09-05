package com.cowlark.fluxengine.cli;

import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DRIVE;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_BIT_PULSE;
import static com.cowlark.fluxengine.wiring.FluxEngine.NS_PER_TICK;
import static com.cowlark.fluxengine.wiring.FluxEngine.TICKS_PER_US;
import static com.cowlark.fluxengine.wiring.FluxEngine.US_PER_TICK;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.flags.DoubleFlag;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.IntFlag;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.FluxmapReader;
import com.cowlark.fluxengine.data.Locations;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.google.common.collect.ImmutableList;
import org.apache.commons.csv.CSVFormat;
import org.apache.commons.csv.CSVPrinter;
import javax.imageio.ImageIO;
import java.awt.Color;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.geom.Line2D;
import java.awt.geom.Rectangle2D;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

/**
 * Measures the drive's ability to read and write pulses, and produces a
 * heatmap of pulse response over a range of intervals, ported from
 * src/fe-analysedriveresponse.cc. The Agg2D graphics of the original are
 * replaced with an AWT BufferedImage.
 */
public class AnalyseDriveResponse implements Command
{
    /* This is the Turbo colourmap.
     * https://ai.googleblog.com/2019/08/turbo-improved-rainbow-colormap-for.html
     */
    private static final int[][] TURBO_SRGB = {{48, 18, 59},
            {50, 21, 67},
            {51, 24, 74},
            {52, 27, 81},
            {53, 30, 88},
            {54, 33, 95},
            {55, 36, 102},
            {56, 39, 109},
            {57, 42, 115},
            {58, 45, 121},
            {59, 47, 128},
            {60, 50, 134},
            {61, 53, 139},
            {62, 56, 145},
            {63, 59, 151},
            {63, 62, 156},
            {64, 64, 162},
            {65, 67, 167},
            {65, 70, 172},
            {66, 73, 177},
            {66, 75, 181},
            {67, 78, 186},
            {68, 81, 191},
            {68, 84, 195},
            {68, 86, 199},
            {69, 89, 203},
            {69, 92, 207},
            {69, 94, 211},
            {70, 97, 214},
            {70, 100, 218},
            {70, 102, 221},
            {70, 105, 224},
            {70, 107, 227},
            {71, 110, 230},
            {71, 113, 233},
            {71, 115, 235},
            {71, 118, 238},
            {71, 120, 240},
            {71, 123, 242},
            {70, 125, 244},
            {70, 128, 246},
            {70, 130, 248},
            {70, 133, 250},
            {70, 135, 251},
            {69, 138, 252},
            {69, 140, 253},
            {68, 143, 254},
            {67, 145, 254},
            {66, 148, 255},
            {65, 150, 255},
            {64, 153, 255},
            {62, 155, 254},
            {61, 158, 254},
            {59, 160, 253},
            {58, 163, 252},
            {56, 165, 251},
            {55, 168, 250},
            {53, 171, 248},
            {51, 173, 247},
            {49, 175, 245},
            {47, 178, 244},
            {46, 180, 242},
            {44, 183, 240},
            {42, 185, 238},
            {40, 188, 235},
            {39, 190, 233},
            {37, 192, 231},
            {35, 195, 228},
            {34, 197, 226},
            {32, 199, 223},
            {31, 201, 221},
            {30, 203, 218},
            {28, 205, 216},
            {27, 208, 213},
            {26, 210, 210},
            {26, 212, 208},
            {25, 213, 205},
            {24, 215, 202},
            {24, 217, 200},
            {24, 219, 197},
            {24, 221, 194},
            {24, 222, 192},
            {24, 224, 189},
            {25, 226, 187},
            {25, 227, 185},
            {26, 228, 182},
            {28, 230, 180},
            {29, 231, 178},
            {31, 233, 175},
            {32, 234, 172},
            {34, 235, 170},
            {37, 236, 167},
            {39, 238, 164},
            {42, 239, 161},
            {44, 240, 158},
            {47, 241, 155},
            {50, 242, 152},
            {53, 243, 148},
            {56, 244, 145},
            {60, 245, 142},
            {63, 246, 138},
            {67, 247, 135},
            {70, 248, 132},
            {74, 248, 128},
            {78, 249, 125},
            {82, 250, 122},
            {85, 250, 118},
            {89, 251, 115},
            {93, 252, 111},
            {97, 252, 108},
            {101, 253, 105},
            {105, 253, 102},
            {109, 254, 98},
            {113, 254, 95},
            {117, 254, 92},
            {121, 254, 89},
            {125, 255, 86},
            {128, 255, 83},
            {132, 255, 81},
            {136, 255, 78},
            {139, 255, 75},
            {143, 255, 73},
            {146, 255, 71},
            {150, 254, 68},
            {153, 254, 66},
            {156, 254, 64},
            {159, 253, 63},
            {161, 253, 61},
            {164, 252, 60},
            {167, 252, 58},
            {169, 251, 57},
            {172, 251, 56},
            {175, 250, 55},
            {177, 249, 54},
            {180, 248, 54},
            {183, 247, 53},
            {185, 246, 53},
            {188, 245, 52},
            {190, 244, 52},
            {193, 243, 52},
            {195, 241, 52},
            {198, 240, 52},
            {200, 239, 52},
            {203, 237, 52},
            {205, 236, 52},
            {208, 234, 52},
            {210, 233, 53},
            {212, 231, 53},
            {215, 229, 53},
            {217, 228, 54},
            {219, 226, 54},
            {221, 224, 55},
            {223, 223, 55},
            {225, 221, 55},
            {227, 219, 56},
            {229, 217, 56},
            {231, 215, 57},
            {233, 213, 57},
            {235, 211, 57},
            {236, 209, 58},
            {238, 207, 58},
            {239, 205, 58},
            {241, 203, 58},
            {242, 201, 58},
            {244, 199, 58},
            {245, 197, 58},
            {246, 195, 58},
            {247, 193, 58},
            {248, 190, 57},
            {249, 188, 57},
            {250, 186, 57},
            {251, 184, 56},
            {251, 182, 55},
            {252, 179, 54},
            {252, 177, 54},
            {253, 174, 53},
            {253, 172, 52},
            {254, 169, 51},
            {254, 167, 50},
            {254, 164, 49},
            {254, 161, 48},
            {254, 158, 47},
            {254, 155, 45},
            {254, 153, 44},
            {254, 150, 43},
            {254, 147, 42},
            {254, 144, 41},
            {253, 141, 39},
            {253, 138, 38},
            {252, 135, 37},
            {252, 132, 35},
            {251, 129, 34},
            {251, 126, 33},
            {250, 123, 31},
            {249, 120, 30},
            {249, 117, 29},
            {248, 114, 28},
            {247, 111, 26},
            {246, 108, 25},
            {245, 105, 24},
            {244, 102, 23},
            {243, 99, 21},
            {242, 96, 20},
            {241, 93, 19},
            {240, 91, 18},
            {239, 88, 17},
            {237, 85, 16},
            {236, 83, 15},
            {235, 80, 14},
            {234, 78, 13},
            {232, 75, 12},
            {231, 73, 12},
            {229, 71, 11},
            {228, 69, 10},
            {226, 67, 10},
            {225, 65, 9},
            {223, 63, 8},
            {221, 61, 8},
            {220, 59, 7},
            {218, 57, 7},
            {216, 55, 6},
            {214, 53, 6},
            {212, 51, 5},
            {210, 49, 5},
            {208, 47, 5},
            {206, 45, 4},
            {204, 43, 4},
            {202, 42, 4},
            {200, 40, 3},
            {197, 38, 3},
            {195, 37, 3},
            {193, 35, 2},
            {190, 33, 2},
            {188, 32, 2},
            {185, 30, 2},
            {183, 29, 2},
            {180, 27, 1},
            {178, 26, 1},
            {175, 24, 1},
            {172, 23, 1},
            {169, 22, 1},
            {167, 20, 1},
            {164, 19, 1},
            {161, 18, 1},
            {158, 16, 1},
            {155, 15, 1},
            {152, 14, 1},
            {149, 13, 1},
            {146, 11, 1},
            {142, 10, 1},
            {139, 9, 2},
            {136, 8, 2},
            {133, 7, 2},
            {129, 6, 2},
            {126, 5, 2},
            {122, 4, 3},};
    private final FlagGroup flags = new FlagGroup();
    private final StringFlag destFlux = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--dest")
            .setName("-d")
            .setHelpText("'drive:' flux destination to analyse")
            .build();
    private final StringFlag destTracks = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--tracks")
            .setName("-t")
            .setHelpText("tracks to write to")
            .setDefaultValue("c0h0")
            .build();
    private final DoubleFlag minInterval = DoubleFlag
            .builder()
            .setGroup(flags)
            .setName("--min-interval-us")
            .setHelpText("Minimum pulse interval")
            .setDefaultValue(2.0)
            .build();
    private final DoubleFlag maxInterval = DoubleFlag
            .builder()
            .setGroup(flags)
            .setName("--max-interval-us")
            .setHelpText("Maximum pulse interval")
            .setDefaultValue(10.0)
            .build();
    private final DoubleFlag intervalStep = DoubleFlag
            .builder()
            .setGroup(flags)
            .setName("--interval-step-us")
            .setHelpText("Interval step, approximately")
            .setDefaultValue(0.2)
            .build();
    private final StringFlag writeCsv = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--write-csv")
            .setHelpText("Write detailed CSV data")
            .build();
    private final StringFlag writeImg = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--write-img")
            .setHelpText("Draw a graph of the response data")
            .setDefaultValue("analysis.png")
            .build();
    private final IntFlag imgWidth = IntFlag
            .builder()
            .setGroup(flags)
            .setName("--width")
            .setHelpText("Width of output graph")
            .setDefaultValue(800)
            .build();
    private final IntFlag imgHeight = IntFlag
            .builder()
            .setGroup(flags)
            .setName("--height")
            .setHelpText("Height of output graph")
            .setDefaultValue(600)
            .build();
    private final IntFlag buckets = IntFlag
            .builder()
            .setGroup(flags)
            .setName("--buckets")
            .setHelpText("Number of heatmap buckets")
            .setDefaultValue(250)
            .build();

    private static Color palette(double value)
    {
        int index = Math.min((int) (value * 256.0), 255);
        return new Color(TURBO_SRGB[index][0], TURBO_SRGB[index][1], TURBO_SRGB[index][2]);
    }

    private static void doInSteps(
            double c1,
            double c2,
            double lo,
            double hi,
            double step,
            StepVisitor visitor)
    {
        double scale = (c2 - c1) / (hi - lo);
        double v = lo;
        while (v <= hi + step / 10.0)
        {
            double c = c1 + scale * (v - lo);
            visitor.step(c, v);
            v += step;
        }
    }

    private static void drawYAxis(
            Graphics2D painter,
            double x,
            double y1,
            double y2,
            double lo,
            double hi,
            double step,
            String format)
    {
        painter.setColor(new Color(0, 0, 0));
        painter.draw(new Line2D.Double(x, y1, x, y2));
        painter.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, 10));

        doInSteps(
                y1, y2, lo, hi, step, (y, v) -> {
                    painter.draw(new Line2D.Double(x, y, x - 5, y));
                    drawTextRightAligned(painter, x - 8, y + 5.0, String.format(format, v));
                });
    }

    private static void drawXAxis(
            Graphics2D painter,
            double x1,
            double x2,
            double y,
            double lo,
            double hi,
            double step,
            String format)
    {
        painter.setColor(new Color(0, 0, 0));
        painter.draw(new Line2D.Double(x1, y, x2, y));
        painter.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, 10));

        doInSteps(
                x1, x2, lo, hi, step, (x, v) -> {
                    painter.draw(new Line2D.Double(x, y, x, y + 5));
                    drawTextCentred(painter, x, y + 18, String.format(format, v));
                });
    }

    private static void drawYGraticules(
            Graphics2D painter,
            double x1,
            double y1,
            double x2,
            double y2,
            double lo,
            double hi,
            double step)
    {
        painter.setColor(new Color(0, 0, 0, 128));

        doInSteps(y1, y2, lo, hi, step, (y, v) -> painter.draw(new Line2D.Double(x1, y, x2, y)));
    }

    private static void drawXGraticules(
            Graphics2D painter,
            double x1,
            double y1,
            double x2,
            double y2,
            double lo,
            double hi,
            double step)
    {
        painter.setColor(new Color(0, 0, 0, 128));

        doInSteps(x1, x2, lo, hi, step, (x, v) -> painter.draw(new Line2D.Double(x, y1, x, y2)));
    }

    private static void drawTextLeftAligned(Graphics2D painter, double x, double y, String text)
    {
        painter.drawString(text, (float) x, (float) y);
    }

    private static void drawTextCentred(Graphics2D painter, double x, double y, String text)
    {
        FontMetrics metrics = painter.getFontMetrics();
        painter.drawString(text, (float) (x - metrics.stringWidth(text) / 2.0), (float) y);
    }

    private static void drawTextRightAligned(Graphics2D painter, double x, double y, String text)
    {
        FontMetrics metrics = painter.getFontMetrics();
        painter.drawString(text, (float) (x - metrics.stringWidth(text)), (float) y);
    }

    @Override
    public String getHelp()
    {
        return "Measures the drive's ability to read and write pulses.";
    }

    private void measure(
            UsbDevice device,
            ConfigProto config,
            double rotationalPeriodNs,
            CylinderHead track,
            double[][] frequencies) throws IOException
    {
        double minInterval = this.minInterval.get();
        double maxInterval = this.maxInterval.get();
        double intervalStep = this.intervalStep.get();
        int numColumns = buckets.get();

        int numRows = frequencies.length;
        try (CSVPrinter csv = openCsv())
        {
            for (int row = 0; row < numRows; row++)
            {
                double interval = minInterval + (double) row * intervalStep;

                int ticks = (int) (interval * TICKS_PER_US);
                System.out.printf("Interval %.2f: ", ticks * US_PER_TICK);
                System.out.flush();

                /* Write the test pattern. */

                if (interval >= 2.0)
                {
                    Fluxmap outFluxmap = new Fluxmap();
                    while (outFluxmap.durationNs() < rotationalPeriodNs)
                    {
                        outFluxmap.appendInterval(ticks);
                        outFluxmap.appendPulse();
                    }

                    device.write(track.cylinder(), track.head(), outFluxmap.rawBytes());

                    /* Read the test pattern in again. */

                    Bytes raw = device.read(
                            track.cylinder(),
                            track.head(),
                            device.getRotationalPeriod());
                    Fluxmap inFluxmap = new Fluxmap(raw);

                    /* Compute histogram. */

                    FluxmapReader fmr = new FluxmapReader(inFluxmap, config.getDecoder());
                    double period = device.getRotationalPeriod();
                    fmr.seek((long) (period * 0.1 / NS_PER_TICK)); /* skip first
                     * 10% and last
                     * 10% as
                     * contains junk */
                    fmr.skipToEvent(F_BIT_PULSE);
                    while (fmr.tell().getDurationNs() < period * 0.9)
                    {
                        FluxmapReader.EventResult r = fmr.findEvent(F_BIT_PULSE);
                        if (!r.found())
                            break;
                        if (r.ticks() < numColumns)
                            frequencies[row][(int) r.ticks()]++;
                    }
                }

                /* Compute mean and normalise. */

                double sum = 0.0;
                double prod = 0.0;
                double max = 0.0;
                for (int i = 0; i < numColumns; i++)
                {
                    sum += frequencies[row][i];
                    prod += i * frequencies[row][i];
                    max = Math.max(max, frequencies[row][i]);
                }
                if (max != 0.0)
                    for (int i = 0; i < numColumns; i++)
                        frequencies[row][i] /= max;

                if (sum == 0)
                    System.out.println("failed");
                else
                {
                    double mean = prod / sum;
                    System.out.printf("%.4f%n", mean / TICKS_PER_US);
                }

                if (csv != null)
                {
                    Object[] values = new Object[numColumns + 1];
                    values[0] = interval;
                    for (int i = 0; i < numColumns; i++)
                        values[i + 1] = frequencies[row][i];
                    try
                    {
                        csv.printRecord(values);
                    } catch (IOException e)
                    {
                        throw new FluxEngineException("can't write CSV data: " + e.getMessage());
                    }
                }
            }
        }
    }

    private void drawGraph(double[][] frequencies)
    {
        double minInterval = this.minInterval.get();
        double maxInterval = this.maxInterval.get();
        int numRows = frequencies.length;
        int numColumns = buckets.get();

        BufferedImage image =
                new BufferedImage(imgWidth.get(), imgHeight.get(), BufferedImage.TYPE_INT_RGB);
        Graphics2D painter = image.createGraphics();
        painter.setRenderingHint(
                RenderingHints.KEY_ANTIALIASING,
                RenderingHints.VALUE_ANTIALIAS_ON);
        painter.setColor(new Color(0xdd, 0xdd, 0xdd));
        painter.fillRect(0, 0, image.getWidth(), image.getHeight());

        final double MARGIN = 30;
        Rectangle2D.Double drawableBounds = new Rectangle2D.Double(
                MARGIN * 1.5,
                MARGIN,
                imgWidth.get() - MARGIN - MARGIN * 1.5,
                imgHeight.get() - MARGIN * 2);
        Rectangle2D.Double colourbarBounds = new Rectangle2D.Double(
                drawableBounds.x + drawableBounds.width - MARGIN,
                drawableBounds.y,
                MARGIN,
                drawableBounds.height);
        Rectangle2D.Double graphBounds = new Rectangle2D.Double(
                drawableBounds.x,
                drawableBounds.y,
                colourbarBounds.x - MARGIN * 2 - drawableBounds.x,
                drawableBounds.height);
        double blockWidth = graphBounds.width / numColumns;
        double blockHeight = graphBounds.height / numRows;

        /* Create the off-screen buffer which the actual bitmap goes into,
         * drawn flipped vertically so that row 0 ends up at the bottom, as
         * transformImage did in the C++. */

        {
            final int width = numRows;     /* input interval on X axis */
            final int height = numColumns; /* response spread on Y axis */
            BufferedImage data = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
            for (int y = 0; y < height; y++)
                for (int x = 0; x < width; x++)
                    data.setRGB(x, height - 1 - y, palette(frequencies[x][y]).getRGB());
            painter.drawImage(
                    data,
                    (int) Math.round(graphBounds.x),
                    (int) Math.round(graphBounds.y),
                    (int) Math.round(graphBounds.x + graphBounds.width),
                    (int) Math.round(graphBounds.y + graphBounds.height),
                    0,
                    0,
                    width,
                    height,
                    null);
        }

        /* Likewise for the colour bar. */

        {
            final int height = (int) graphBounds.height;
            BufferedImage bar = new BufferedImage(1, height, BufferedImage.TYPE_INT_RGB);
            for (int y = 0; y < height; y++)
                bar.setRGB(0, height - 1 - y, palette((double) y / height).getRGB());
            painter.drawImage(
                    bar,
                    (int) Math.round(colourbarBounds.x),
                    (int) Math.round(colourbarBounds.y),
                    (int) Math.round(colourbarBounds.x + colourbarBounds.width),
                    (int) Math.round(colourbarBounds.y + colourbarBounds.height),
                    0,
                    0,
                    1,
                    height,
                    null);
        }

        drawYAxis(
                painter,
                colourbarBounds.x - 5,
                colourbarBounds.y + colourbarBounds.height,
                colourbarBounds.y,
                0.0,
                1.0,
                0.1,
                "%.1f");
        drawYAxis(
                painter,
                graphBounds.x - 5,
                graphBounds.y + graphBounds.height,
                graphBounds.y,
                0.0,
                buckets.get() / TICKS_PER_US,
                5.0,
                "%.0f");
        drawYGraticules(
                painter,
                graphBounds.x,
                graphBounds.y + graphBounds.height,
                graphBounds.x + graphBounds.width,
                graphBounds.y,
                0.0,
                buckets.get() / TICKS_PER_US,
                5.0);
        drawXAxis(
                painter,
                graphBounds.x,
                graphBounds.x + graphBounds.width,
                graphBounds.y + graphBounds.height + 5,
                minInterval,
                maxInterval,
                5.0,
                "%.0f");
        drawXGraticules(
                painter,
                graphBounds.x,
                graphBounds.y,
                graphBounds.x + graphBounds.width,
                graphBounds.y + graphBounds.height,
                minInterval,
                maxInterval,
                5.0);

        painter.setColor(new Color(0, 0, 0));
        painter.draw(graphBounds);
        painter.draw(new Rectangle2D.Double(
                graphBounds.x,
                graphBounds.y,
                graphBounds.width,
                graphBounds.height));
        painter.dispose();

        saveImage(image, writeImg.get());
    }

    private void saveImage(BufferedImage image, String filename)
    {
        String lower = filename.toLowerCase();
        String format;
        if (lower.endsWith(".png"))
            format = "png";
        else if (lower.endsWith(".bmp"))
            format = "bmp";
        else if (lower.endsWith(".jpg") || lower.endsWith(".jpeg"))
            format = "jpg";
        else
            throw new FluxEngineException("don't know how to write that image format");

        try
        {
            ImageIO.write(image, format, new File(filename));
        } catch (IOException e)
        {
            throw new FluxEngineException("can't write output file: " + e.getMessage());
        }
    }

    /* Opens the CSV output file, or returns null if none was requested.
     * try-with-resources skips null resources. */
    private CSVPrinter openCsv() throws IOException
    {
        if (writeCsv.get().isEmpty())
            return null;
        return new CSVPrinter(new FileWriter(writeCsv.get()), CSVFormat.DEFAULT);
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        System.setProperty("java.awt.headless", "true");

        ConfigProto config =
                new ConfigBuilder().fromFlags(args, flags).withFluxSink(destFlux.get()).build();

        if (config.getFluxSink().getType() != FLUXTYPE_DRIVE)
            throw new FluxEngineException("this only makes sense with a real disk drive");

        ImmutableList<CylinderHead> tracks = Locations.parseCylinderHeadsString(destTracks.get());
        if (tracks.size() != 1)
            throw new FluxEngineException("you must specify exactly one track");

        int numRows = (int) ((maxInterval.get() - minInterval.get()) / intervalStep.get());
        int numColumns = buckets.get();
        double[][] frequencies = new double[numRows][numColumns];

        try (UsbFactory usbFactory = new UsbFactory(config))
        {
            usbFactory.perform(device -> {
                System.out.println("Measuring rotational speed...");
                device.seek(tracks.get(0).cylinder());

                double period = device.getRotationalPeriod();
                if (period == 0)
                    throw new FluxEngineException(
                            "Unable to measure rotational speed (try fluxengine rpm).");

                try
                {
                    measure(device, config, period, tracks.get(0), frequencies);
                } catch (IOException e)
                {
                    throw new FluxEngineException("I/O error during analysis: " + e.getMessage());
                }

                if (!writeImg.get().isEmpty())
                    drawGraph(frequencies);
            });
        }
    }

    private interface StepVisitor
    {
        void step(double c, double v);
    }
}
