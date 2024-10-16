#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/usb/usb.h"
#include "lib/core/bitmap.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "protocol.h"
#include "lib/config/proto.h"
#include "lib/fluxsink/fluxsink.h"
#include "agg2d.h"
#include "stb_image_write.h"
#include <fstream>

static FlagGroup flags;

static StringFlag destFlux({"--dest", "-d"},
    "'drive:' flux destination to analyse",
    "",
    [](const auto& value)
    {
        globalConfig().setFluxSink(value);
    });

static IntFlag destTrack({"--cylinder", "-c"}, "track to write to", 0);

static IntFlag destHead({"--head", "-h"}, "head to write to", 0);

static DoubleFlag minInterval(
    {"--min-interval-us"}, "Minimum pulse interval", 2.0);

static DoubleFlag maxInterval(
    {"--max-interval-us"}, "Maximum pulse interval", 10.0);

static DoubleFlag intervalStep(
    {"--interval-step-us"}, "Interval step, approximately", 0.2);

static StringFlag writeCsv({"--write-csv"}, "Write detailed CSV data", "");

static StringFlag writeImg(
    {"--write-img"}, "Draw a graph of the response data", "analysis.png");

static IntFlag imgWidth({"--width"}, "Width of output graph", 800);

static IntFlag imgHeight({"--height"}, "Height of output graph", 600);

static IntFlag buckets({"--buckets"}, "Number of heatmap buckets", 250);

/* This is the Turbo colourmap.
 * https://ai.googleblog.com/2019/08/turbo-improved-rainbow-colormap-for.html
 */
static const uint8_t turbo_srgb_bytes[256][3] = {
  // clang-format off
    {48,  18,  59 }, {50,  21,  67 }, {51,  24,  74 }, {52,  27,  81 },
    {53,  30,  88 }, {54,  33,  95 }, {55,  36,  102}, {56,  39,  109},
    {57,  42,  115}, {58,  45,  121}, {59,  47,  128}, {60,  50,  134},
    {61,  53,  139}, {62,  56,  145}, {63,  59,  151}, {63,  62,  156},
    {64,  64,  162}, {65,  67,  167}, {65,  70,  172}, {66,  73,  177},
    {66,  75,  181}, {67,  78,  186}, {68,  81,  191}, {68,  84,  195},
    {68,  86,  199}, {69,  89,  203}, {69,  92,  207}, {69,  94,  211},
    {70,  97,  214}, {70,  100, 218}, {70,  102, 221}, {70,  105, 224},
    {70,  107, 227}, {71,  110, 230}, {71,  113, 233}, {71,  115, 235},
    {71,  118, 238}, {71,  120, 240}, {71,  123, 242}, {70,  125, 244},
    {70,  128, 246}, {70,  130, 248}, {70,  133, 250}, {70,  135, 251},
    {69,  138, 252}, {69,  140, 253}, {68,  143, 254}, {67,  145, 254},
    {66,  148, 255}, {65,  150, 255}, {64,  153, 255}, {62,  155, 254},
    {61,  158, 254}, {59,  160, 253}, {58,  163, 252}, {56,  165, 251},
    {55,  168, 250}, {53,  171, 248}, {51,  173, 247}, {49,  175, 245},
    {47,  178, 244}, {46,  180, 242}, {44,  183, 240}, {42,  185, 238},
    {40,  188, 235}, {39,  190, 233}, {37,  192, 231}, {35,  195, 228},
    {34,  197, 226}, {32,  199, 223}, {31,  201, 221}, {30,  203, 218},
    {28,  205, 216}, {27,  208, 213}, {26,  210, 210}, {26,  212, 208},
    {25,  213, 205}, {24,  215, 202}, {24,  217, 200}, {24,  219, 197},
    {24,  221, 194}, {24,  222, 192}, {24,  224, 189}, {25,  226, 187},
    {25,  227, 185}, {26,  228, 182}, {28,  230, 180}, {29,  231, 178},
    {31,  233, 175}, {32,  234, 172}, {34,  235, 170}, {37,  236, 167},
    {39,  238, 164}, {42,  239, 161}, {44,  240, 158}, {47,  241, 155},
    {50,  242, 152}, {53,  243, 148}, {56,  244, 145}, {60,  245, 142},
    {63,  246, 138}, {67,  247, 135}, {70,  248, 132}, {74,  248, 128},
    {78,  249, 125}, {82,  250, 122}, {85,  250, 118}, {89,  251, 115},
    {93,  252, 111}, {97,  252, 108}, {101, 253, 105}, {105, 253, 102},
    {109, 254, 98 }, {113, 254, 95 }, {117, 254, 92 }, {121, 254, 89 },
    {125, 255, 86 }, {128, 255, 83 }, {132, 255, 81 }, {136, 255, 78 },
    {139, 255, 75 }, {143, 255, 73 }, {146, 255, 71 }, {150, 254, 68 },
    {153, 254, 66 }, {156, 254, 64 }, {159, 253, 63 }, {161, 253, 61 },
    {164, 252, 60 }, {167, 252, 58 }, {169, 251, 57 }, {172, 251, 56 },
    {175, 250, 55 }, {177, 249, 54 }, {180, 248, 54 }, {183, 247, 53 },
    {185, 246, 53 }, {188, 245, 52 }, {190, 244, 52 }, {193, 243, 52 },
    {195, 241, 52 }, {198, 240, 52 }, {200, 239, 52 }, {203, 237, 52 },
    {205, 236, 52 }, {208, 234, 52 }, {210, 233, 53 }, {212, 231, 53 },
    {215, 229, 53 }, {217, 228, 54 }, {219, 226, 54 }, {221, 224, 55 },
    {223, 223, 55 }, {225, 221, 55 }, {227, 219, 56 }, {229, 217, 56 },
    {231, 215, 57 }, {233, 213, 57 }, {235, 211, 57 }, {236, 209, 58 },
    {238, 207, 58 }, {239, 205, 58 }, {241, 203, 58 }, {242, 201, 58 },
    {244, 199, 58 }, {245, 197, 58 }, {246, 195, 58 }, {247, 193, 58 },
    {248, 190, 57 }, {249, 188, 57 }, {250, 186, 57 }, {251, 184, 56 },
    {251, 182, 55 }, {252, 179, 54 }, {252, 177, 54 }, {253, 174, 53 },
    {253, 172, 52 }, {254, 169, 51 }, {254, 167, 50 }, {254, 164, 49 },
    {254, 161, 48 }, {254, 158, 47 }, {254, 155, 45 }, {254, 153, 44 },
    {254, 150, 43 }, {254, 147, 42 }, {254, 144, 41 }, {253, 141, 39 },
    {253, 138, 38 }, {252, 135, 37 }, {252, 132, 35 }, {251, 129, 34 },
    {251, 126, 33 }, {250, 123, 31 }, {249, 120, 30 }, {249, 117, 29 },
    {248, 114, 28 }, {247, 111, 26 }, {246, 108, 25 }, {245, 105, 24 },
    {244, 102, 23 }, {243, 99,  21 }, {242, 96,  20 }, {241, 93,  19 },
    {240, 91,  18 }, {239, 88,  17 }, {237, 85,  16 }, {236, 83,  15 },
    {235, 80,  14 }, {234, 78,  13 }, {232, 75,  12 }, {231, 73,  12 },
    {229, 71,  11 }, {228, 69,  10 }, {226, 67,  10 }, {225, 65,  9  },
    {223, 63,  8  }, {221, 61,  8  }, {220, 59,  7  }, {218, 57,  7  },
    {216, 55,  6  }, {214, 53,  6  }, {212, 51,  5  }, {210, 49,  5  },
    {208, 47,  5  }, {206, 45,  4  }, {204, 43,  4  }, {202, 42,  4  },
    {200, 40,  3  }, {197, 38,  3  }, {195, 37,  3  }, {193, 35,  2  },
    {190, 33,  2  }, {188, 32,  2  }, {185, 30,  2  }, {183, 29,  2  },
    {180, 27,  1  }, {178, 26,  1  }, {175, 24,  1  }, {172, 23,  1  },
    {169, 22,  1  }, {167, 20,  1  }, {164, 19,  1  }, {161, 18,  1  },
    {158, 16,  1  }, {155, 15,  1  }, {152, 14,  1  }, {149, 13,  1  },
    {146, 11,  1  }, {142, 10,  1  }, {139, 9,   2  }, {136, 8,   2  },
    {133, 7,   2  }, {129, 6,   2  }, {126, 5,   2  }, {122, 4,   3  }
  // clang-format on
};

static void palette(double value, agg::srgba8* pixel)
{
    int index = std::min((int)(value * 256.0), 255);
    pixel->r = turbo_srgb_bytes[index][0];
    pixel->g = turbo_srgb_bytes[index][1];
    pixel->b = turbo_srgb_bytes[index][2];
    pixel->a = 255;
}

static void do_in_steps(double c1,
    double c2,
    double lo,
    double hi,
    double step,
    std::function<void(double c, double v)> func)
{
    double scale = (c2 - c1) / (hi - lo);
    double v = lo;
    while (v <= hi + step / 10.0)
    {
        double c = c1 + scale * (v - lo);
        func(c, v);
        v += step;
    }
}

static void draw_y_axis(Agg2D& painter,
    double x,
    double y1,
    double y2,
    double lo,
    double hi,
    double step,
    const std::string& format)
{
    painter.noFill();
    painter.lineColor(0, 0, 0);
    painter.line(x, y1, x, y2);
    painter.textSize(10.0);
    painter.textAlignment(Agg2D::AlignRight);

    do_in_steps(y1,
        y2,
        lo,
        hi,
        step,
        [&](double y, double v)
        {
            painter.line(x, y, x - 5, y);
            painter.text(x - 8, y + 5.0, fmt::format(format, v));
        });
}

static void draw_x_axis(Agg2D& painter,
    double x1,
    double x2,
    double y,
    double lo,
    double hi,
    double step,
    const std::string& format)
{
    painter.noFill();
    painter.lineColor(0, 0, 0);
    painter.line(x1, y, x2, y);
    painter.textSize(10.0);
    painter.textAlignment(Agg2D::AlignCenter);

    do_in_steps(x1,
        x2,
        lo,
        hi,
        step,
        [&](double x, double v)
        {
            painter.line(x, y, x, y + 5);
            painter.text(x, y + 18, fmt::format(format, v));
        });
}

static void draw_y_graticules(Agg2D& painter,
    double x1,
    double y1,
    double x2,
    double y2,
    double lo,
    double hi,
    double step)
{
    painter.noFill();
    painter.lineColor(0, 0, 0, 128);

    do_in_steps(y1,
        y2,
        lo,
        hi,
        step,
        [&](double y, double v)
        {
            painter.line(x1, y, x2, y);
        });
}

static void draw_x_graticules(Agg2D& painter,
    double x1,
    double y1,
    double x2,
    double y2,
    double lo,
    double hi,
    double step)
{
    painter.noFill();
    painter.lineColor(0, 0, 0, 128);

    do_in_steps(x1,
        x2,
        lo,
        hi,
        step,
        [&](double x, double v)
        {
            painter.line(x, y1, x, y2);
        });
}

int mainAnalyseDriveResponse(int argc, const char* argv[])
{
    globalConfig().overrides()->mutable_flux_source()->set_type(FLUXTYPE_DRIVE);
    flags.parseFlagsWithConfigFiles(argc, argv, {});

    if (globalConfig()->flux_sink().type() != FLUXTYPE_DRIVE)
        error("this only makes sense with a real disk drive");

    usbSetDrive(globalConfig()->drive().drive(),
        globalConfig()->drive().high_density(),
        globalConfig()->drive().index_mode());
    usbSeek(destTrack);

    std::cout << "Measuring rotational speed...\n";
    nanoseconds_t period = usbGetRotationalPeriod(0);
    if (period == 0)
        error("Unable to measure rotational speed (try fluxengine rpm).");

    std::ofstream csv;
    if (writeCsv.get() != "")
        csv.open(writeCsv);

    int numRows = (maxInterval - minInterval) / intervalStep;
    const int numColumns = buckets;
    std::vector<std::vector<double>> frequencies(
        numRows, std::vector<double>(numColumns, 0.0));

    double interval;
    for (int row = 0; row < numRows; row++)
    {
        interval = minInterval + (double)row * intervalStep;

        unsigned ticks = (unsigned)(interval * TICKS_PER_US);
        std::cout << fmt::format("Interval {:.2f}: ", ticks * US_PER_TICK);
        std::cout << std::flush;

        /* Write the test pattern. */

        if (interval >= 2.0)
        {
            Fluxmap outFluxmap;
            while (outFluxmap.duration() < period)
            {
                outFluxmap.appendInterval(ticks);
                outFluxmap.appendPulse();
            }

            usbWrite(destHead, outFluxmap.rawBytes(), 0);

            /* Read the test pattern in again. */

            Fluxmap inFluxmap;
            inFluxmap.appendBytes(usbRead(destHead, true, period, 0));

            /* Compute histogram. */

            FluxmapReader fmr(inFluxmap);
            fmr.seek((double)period *
                     0.1); /* skip first 10% and last 10% as contains junk */
            fmr.skipToEvent(F_BIT_PULSE);
            while (fmr.tell().ns() < ((double)period * 0.9))
            {
                unsigned ticks;
                fmr.findEvent(F_BIT_PULSE, ticks);
                if (ticks < numColumns)
                    frequencies[row][ticks]++;
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
            max = std::max(max, frequencies[row][i]);
        }
        if (max != 0.0)
            for (int i = 0; i < numColumns; i++)
                frequencies[row][i] /= max;

        if (sum == 0)
            std::cout << "failed\n";
        else
        {
            double mean = prod / sum;
            std::cout << fmt::format("{:.4f}\n", mean / TICKS_PER_US);
        }

        if (writeCsv.get() != "")
        {
            csv << interval;
            for (double d : frequencies[row])
                csv << "," << d;
            csv << '\n';
        }
    }

    Bitmap bitmap(writeImg, imgWidth, imgHeight);
    if (!bitmap.filename.empty())
    {
        Agg2D& painter = bitmap.painter();
        painter.clearAll(0xdd, 0xdd, 0xdd);

        const double MARGIN = 30;
        agg::rect_d drawableBounds = {MARGIN * 1.5,
            MARGIN,
            bitmap.width - MARGIN,
            bitmap.height - MARGIN};
        agg::rect_d colourbarBounds = {drawableBounds.x2 - MARGIN,
            drawableBounds.y1,
            drawableBounds.x2,
            drawableBounds.y2};
        agg::rect_d graphBounds = {drawableBounds.x1,
            drawableBounds.y1,
            colourbarBounds.x1 - MARGIN * 2,
            drawableBounds.y2};
        double blockWidth = (graphBounds.x2 - graphBounds.x1) / numColumns;
        double blockHeight = (graphBounds.y2 - graphBounds.y1) / numRows;
        painter.imageFilter(Agg2D::NoFilter);
        painter.imageResample(Agg2D::NoResample);
        painter.imageBlendMode(Agg2D::BlendDst);

        /* Create the off-screen buffer which the actual bitmap goes into, and
         * draw it. */

        {
            const int width = numRows;     /* input interval on X axis */
            const int height = numColumns; /* response spread on Y axis */
            std::vector<agg::srgba8> rbufdata(height * width);
            for (int y = 0; y < height; y++)
                for (int x = 0; x < width; x++)
                    palette(frequencies[x][y], &rbufdata[x + y * width]);

            Agg2D::Image image((uint8_t*)&rbufdata[0],
                width,
                height,
                width * sizeof(agg::srgba8));
            painter.transformImage(image,
                graphBounds.x1,
                graphBounds.y2,
                graphBounds.x2,
                graphBounds.y1);
        }

        /* Likewise for the colour bar. */

        {
            const int height = graphBounds.y2 - graphBounds.y1;
            std::vector<agg::srgba8> rbufdata(height);
            for (int y = 0; y < height; y++)
                palette((double)y / height, &rbufdata[y]);

            Agg2D::Image image(
                (uint8_t*)&rbufdata[0], 1, height, sizeof(agg::srgba8));
            painter.transformImage(image,
                colourbarBounds.x1,
                colourbarBounds.y2,
                colourbarBounds.x2,
                colourbarBounds.y1);
        }

        draw_y_axis(painter,
            colourbarBounds.x1 - 5,
            colourbarBounds.y2,
            colourbarBounds.y1,
            0.0,
            1.0,
            0.1,
            "{:.1f}");
        draw_y_axis(painter,
            graphBounds.x1 - 5,
            graphBounds.y2,
            graphBounds.y1,
            0.0,
            buckets / TICKS_PER_US,
            5.0,
            "{:.0f}");
        draw_y_graticules(painter,
            graphBounds.x1,
            graphBounds.y2,
            graphBounds.x2,
            graphBounds.y1,
            0.0,
            buckets / TICKS_PER_US,
            5.0);
        draw_x_axis(painter,
            graphBounds.x1,
            graphBounds.x2,
            graphBounds.y2 + 5,
            minInterval,
            maxInterval,
            5.0,
            "{:.0f}");
        draw_x_graticules(painter,
            graphBounds.x1,
            graphBounds.y1,
            graphBounds.x2,
            graphBounds.y2,
            minInterval,
            maxInterval,
            5.0);

        painter.noFill();
        painter.lineColor(0, 0, 0);
        painter.rectangle(
            graphBounds.x1, graphBounds.y1, graphBounds.x2, graphBounds.y2);
        painter.rectangle(colourbarBounds.x1,
            drawableBounds.y1,
            drawableBounds.x2,
            drawableBounds.y2);
        bitmap.save();
    }

    return 0;
}
