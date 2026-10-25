package com.cowlark.fluxengine.gui;

import static com.cowlark.fluxengine.wiring.FluxEngine.F_BIT_PULSE;
import static swingtree.UI.of;
import static swingtree.UIFactoryMethods.menuItem;
import static swingtree.UIFactoryMethods.popupMenu;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.Utils;
import com.cowlark.fluxengine.data.FluxPosition;
import com.cowlark.fluxengine.data.FluxmapReader;
import com.cowlark.fluxengine.data.Record;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Track;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.decoders.FluxDecoder;
import com.cowlark.fluxengine.external.Crc;
import com.google.common.collect.ImmutableList;
import com.formdev.flatlaf.util.UIScale;
import com.google.common.collect.Iterables;
import javax.swing.JComponent;
import javax.swing.JInternalFrame;
import javax.swing.JPanel;
import javax.swing.JPopupMenu;
import javax.swing.JScrollPane;
import javax.swing.JViewport;
import javax.swing.Scrollable;
import javax.swing.SwingUtilities;
import javax.swing.plaf.basic.BasicInternalFrameUI;
import java.awt.Color;
import java.awt.Cursor;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.event.ComponentAdapter;
import java.awt.event.ComponentEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseMotionListener;
import java.awt.event.MouseWheelEvent;
import java.io.ByteArrayOutputStream;
import java.io.PrintStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.TreeSet;

public class FluxViewerPanel extends JInternalFrame
{
    private static final int HEIGHT_PTS = 100;
    private static final int BORDER = 4;
    private static final int CONTENT_PADDING = 5 * BORDER;
    private static final int MAX_WIDTH = Integer.MAX_VALUE - 1024;
    private static final double RENDER_LIMIT = 3000.0;
    private static final DecoderProto DECODER = DecoderProto.getDefaultInstance();
    public static final int FONT_SIZE_PTS = 7;

    private final ImmutableList<Track> tracks;
    private final ImmutableList<Capture> captures;
    private final TreeSet<Double> boundaries = new TreeSet<>();
    private final Canvas canvas;
    private final JViewport viewport;
    private final double totalDurationNs;
    private double nanosecondsPerPixel = 30;
    private boolean scaleInitialised;
    private Point dragPosition;
    private Point pointer;
    private boolean popupShown;
    private int previousScrollX;

    public FluxViewerPanel(ImageViewModel model, ImmutableList<Track> tracks)
    {
        super(getTitle(tracks), false, true, false, false);

        /* Retain the captures supplied by the flux-view request. */
        this.tracks = tracks;

        /* Keep captures in input order and accumulate offsets in time, rather than pixels. */
        ImmutableList.Builder<Capture> captureBuilder = ImmutableList.builder();
        double offset = 0;
        for (Track track : tracks)
        {
            if (track.fluxmap == null)
                continue;

            /* Ignore captures without a usable duration. */
            double duration = track.fluxmap.durationNs();
            if (!(duration > 0) || !Double.isFinite(duration))
                continue;

            /* Index record boundaries for the ruler before advancing to the next capture. */
            captureBuilder.add(new Capture(track, offset, duration));
            for (Record record : track.records)
            {
                if (Double.isFinite(record.startTimeNs) && Double.isFinite(record.endTimeNs) &&
                        record.endTimeNs >= record.startTimeNs)
                {
                    boundaries.add(offset + record.startTimeNs);
                    boundaries.add(offset + record.endTimeNs);
                }
            }
            offset += duration;
        }
        captures = captureBuilder.build();
        totalDurationNs = offset;

        /* The viewport supplies horizontal translation and the canvas follows its height. */
        canvas = new Canvas();
        canvas.setFont(new Font(
                Font.SANS_SERIF,
                Font.PLAIN,
                UIScale.scale(UiUtils.pointsToPixels(FONT_SIZE_PTS))));
        JScrollPane scrollPane = new JScrollPane(
                canvas,
                JScrollPane.VERTICAL_SCROLLBAR_NEVER,
                JScrollPane.HORIZONTAL_SCROLLBAR_ALWAYS);
        scrollPane.setWheelScrollingEnabled(false);
        viewport = scrollPane.getViewport();

        /* Fit the timeline on first layout and preserve its scale on subsequent resizes. */
        viewport.addComponentListener(new ComponentAdapter()
        {
            @Override
            public void componentResized(ComponentEvent event)
            {
                if (!scaleInitialised && viewport.getWidth() > 0)
                {
                    scaleInitialised = true;
                    updateScale(fitScale());
                } else
                    updateScale(nanosecondsPerPixel);
            }
        });

        /* Keep the pointer at the same screen position when the viewport scrolls. */
        viewport.addChangeListener(event -> {
            int scrollX = viewport.getViewPosition().x;
            if (pointer != null)
                pointer.translate(scrollX - previousScrollX, 0);
            previousScrollX = scrollX;
            canvas.repaint();
        });

        /* Track pointer movement, horizontal dragging, and native popup gestures. */
        MouseAdapter mouse = new MouseAdapter()
        {
            @Override
            public void mouseMoved(MouseEvent event)
            {
                trackPointer(event);
            }

            @Override
            public void mouseExited(MouseEvent event)
            {
                pointer = null;
                canvas.repaint();
            }

            @Override
            public void mousePressed(MouseEvent event)
            {
                popupShown = false;
                trackPointer(event);

                /* Distinguish native popup gestures from the start of a primary-button drag. */
                if (event.isPopupTrigger())
                    showPopup(event);
                else if (SwingUtilities.isLeftMouseButton(event))
                {
                    dragPosition = SwingUtilities.convertPoint(canvas, event.getPoint(), viewport);
                    canvas.setCursor(Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR));
                }
            }

            @Override
            public void mouseDragged(MouseEvent event)
            {
                if (dragPosition != null)
                {
                    Point position =
                            SwingUtilities.convertPoint(canvas, event.getPoint(), viewport);
                    setScrollX((long) viewport.getViewPosition().x + dragPosition.x - position.x);
                    dragPosition = position;

                    /* Restore pointer coordinates after moving the canvas beneath the mouse. */
                    pointer = new Point(viewport.getViewPosition().x + position.x, position.y);
                    canvas.repaint();
                } else
                    trackPointer(event);
            }

            @Override
            public void mouseReleased(MouseEvent event)
            {
                if (event.isPopupTrigger() && !popupShown)
                    showPopup(event);

                /* Finish any active drag and restore the normal cursor. */
                dragPosition = null;
                canvas.setCursor(Cursor.getDefaultCursor());
                trackPointer(event);
            }

            @Override
            public void mouseWheelMoved(MouseWheelEvent event)
            {
                Point anchor = SwingUtilities.convertPoint(canvas, event.getPoint(), viewport);
                zoom(event.getPreciseWheelRotation(), anchor.x);
                pointer = new Point(viewport.getViewPosition().x + anchor.x, event.getY());

                /* Reserve the wheel for zooming rather than scrolling the surrounding panel. */
                event.consume();
                canvas.repaint();
            }
        };
        canvas.addMouseListener(mouse);
        canvas.addMouseMotionListener(mouse);
        canvas.addMouseWheelListener(mouse);

        /* Keep each row a fixed height without contributing to the window's preferred width. */
        int height = UiUtils.pointsToPixels(UIScale.scale(HEIGHT_PTS));
        JPanel content = new JPanel();
        of(content).withLayout("fill, insets 0")
                .add("grow, push", of(scrollPane));
        content.setPreferredSize(new Dimension(0, height));
        setContentPane(content);

        /* Include the title bar in the fixed row height and keep only the close control. */
        setFrameIcon(null);
        setDefaultCloseOperation(DISPOSE_ON_CLOSE);
        int frameHeight = getPreferredSize().height;
        setMinimumSize(new Dimension(0, frameHeight));
        setPreferredSize(new Dimension(0, frameHeight));
        setMaximumSize(new Dimension(Integer.MAX_VALUE, frameHeight));
        setVisible(true);
    }

    /* Keep the look and feel's title bar, but leave positioning to the row layout. */
    @Override
    public void updateUI()
    {
        super.updateUI();
        if (getUI() instanceof BasicInternalFrameUI frameUI)
        {
            JComponent titleBar = frameUI.getNorthPane();
            if (titleBar != null)
            {
                for (MouseMotionListener listener : titleBar.getMouseMotionListeners())
                    titleBar.removeMouseMotionListener(listener);
            }
        }
    }

    /* Scaling and view positioning use a bounded pixel range, with time retained as doubles. */
    private double fitScale()
    {
        return Math.max(
                minimumScale(),
                totalDurationNs / Math.max(1, viewport.getWidth() - 2 * CONTENT_PADDING));
    }

    private double minimumScale()
    {
        return Math.max(30, totalDurationNs / (MAX_WIDTH - 2 * CONTENT_PADDING - 1.0));
    }

    private void updateScale(double scale)
    {
        /* Apply the bounded scale and refresh any density caches that depend on it. */
        nanosecondsPerPixel = Math.max(minimumScale(), scale);
        for (Capture capture : captures)
            capture.updateDensity();

        /* Find a shared density maximum so captures use a consistent colour scale. */
        double maximum = 0;
        for (Capture capture : captures)
        {
            for (double value : capture.density)
                maximum = Math.max(maximum, value);
        }

        /* Normalize pulse counts while leaving pulse-free captures at zero. */
        if (maximum > 0)
        {
            for (Capture capture : captures)
            {
                for (int i = 0; i < capture.density.length; i++)
                    capture.density[i] /= maximum;
            }
        }

        /* Lay out the resized canvas and clamp the viewport to its new extent. */
        canvas.revalidate();
        viewport.doLayout();
        setScrollX(viewport.getViewPosition().x);
        canvas.repaint();
    }

    /* Keep the time beneath the mouse stationary except where a timeline edge clamps it. */
    void zoom(double rotation, int anchorX)
    {
        if (totalDurationNs <= 0 || viewport.getWidth() <= 0)
            return;

        /* Record the time under the pointer before changing scale. */
        double time = (viewport.getViewPosition().x + (double) anchorX - CONTENT_PADDING) *
                nanosecondsPerPixel;

        /* Apply wheel zoom and restore the anchor within the available scroll range. */
        updateScale(Math.min(fitScale(), nanosecondsPerPixel * Math.pow(1.2, rotation)));
        setScrollX(Math.round(CONTENT_PADDING + time / nanosecondsPerPixel - anchorX));
    }

    private void setScrollX(long x)
    {
        int maximum = Math.max(0, canvas.getWidth() - viewport.getWidth());
        viewport.setViewPosition(new Point((int) Math.max(0, Math.min(x, maximum)), 0));
    }

    private void trackPointer(MouseEvent event)
    {
        pointer = event.getPoint();
        canvas.repaint();
    }

    /* Coordinates shared by drawing and hit testing prevent drift between blocks and menus. */
    private int pixel(double time)
    {
        return CONTENT_PADDING + (int) Math.min(
                MAX_WIDTH - 2 * CONTENT_PADDING,
                Math.max(0, time / nanosecondsPerPixel));
    }

    private int centre(int row)
    {
        int rowHeight = canvas.getHeight() / 5;
        return row * rowHeight + rowHeight / 2;
    }

    private int blockHeight()
    {
        return Math.max(1, (canvas.getHeight() / 5) * 2 / 3);
    }

    private Rectangle block(Capture capture, double start, double end, int row)
    {
        int left = pixel(capture.offset + start);
        int right = pixel(capture.offset + end);
        return new Rectangle(
                left,
                centre(row) - blockHeight() / 2,
                Math.max(0, right - left),
                blockHeight());
    }

    /* Paint only the visible part of the concatenated timeline, in the original layer order. */
    protected void redraw(Graphics2D graphics)
    {
        /* Clear the visible canvas and leave empty timelines with just their background. */
        Rectangle visible = viewport.getViewRect();
        graphics.setColor(canvas.getBackground());
        graphics.fillRect(visible.x, 0, visible.width, canvas.getHeight());
        graphics.setFont(canvas.getFont());
        if (totalDurationNs <= 0 || visible.width <= 0)
            return;

        /* Keep all drawing, including labels, inside the padded timeline extent. */
        graphics.clipRect(
                CONTENT_PADDING,
                0,
                Math.max(0, canvas.getWidth() - 2 * CONTENT_PADDING),
                canvas.getHeight());

        /* Render each capture in timeline order, skipping captures outside the viewport. */
        for (Capture capture : captures)
        {
            int left = pixel(capture.offset);
            int right = pixel(capture.offset + capture.duration);
            if (right < visible.x || left > visible.x + visible.width)
                continue;

            /* Separate consecutive captures and match connecting lines to their box fills. */
            if (capture.offset != 0)
            {
                graphics.setColor(UiUtils.DATA_BAD_COLOUR);
                graphics.drawLine(left, 0, left, canvas.getHeight());
            }
            graphics.setColor(UiUtils.DATA_OK_COLOUR.darker());
            graphics.drawLine(left, centre(0), right, centre(0));
            graphics.setColor(UiUtils.HEADER_COLOUR.darker());
            graphics.drawLine(left, centre(1), right, centre(1));
            graphics.setColor(canvas.getForeground());
            graphics.drawLine(left, centre(3), right, centre(3));

            /* Mark index events across all five bands. */
            graphics.setColor(UiUtils.INDEX_LINE_COLOUR);
            for (double index : capture.track.fluxmap.getIndexMarks())
            {
                int x = pixel(capture.offset + index);
                if (x > visible.x + visible.width)
                    break;
                if (x >= visible.x)
                    graphics.drawLine(x, 0, x, canvas.getHeight());
            }

            /* Draw the capture-relative time scale before overlaying data blocks. */
            drawScale(graphics, capture, visible);

            /* Show sector extents, including headers whose data was not recovered. */
            for (Sector sector : capture.track.allSectors)
            {
                double end =
                        sector.dataEndTimeNs != 0 ? sector.dataEndTimeNs : sector.headerEndTimeNs;
                Rectangle rect = block(capture, sector.headerStartTimeNs, end, 0);
                drawBlock(
                        graphics,
                        rect,
                        (sector.status == Sector.Status.OK ?
                                UiUtils.DATA_OK_COLOUR :
                                UiUtils.DATA_BAD_COLOUR).darker(),
                        sector.logicalLocation + " " + Sector.statusToString(sector.status));
            }

            /* Show record extents, decoded bytes, and raw bits at their respective detail levels. */
            for (Record record : capture.track.records)
            {
                Rectangle rect = block(capture, record.startTimeNs, record.endTimeNs, 1);
                if (!rect.intersects(visible))
                    continue;
                drawBlock(
                        graphics,
                        rect,
                        recordColour(capture.track, record),
                        nanosecondsPerPixel > RENDER_LIMIT / 4 ?
                                String.format(Locale.ROOT, "+%.3fms", record.startTimeNs / 1e6) :
                                "");
                if (nanosecondsPerPixel <= RENDER_LIMIT / 4)
                    drawDecodedBytes(graphics, capture, record, rect, visible);
                if (nanosecondsPerPixel < RENDER_LIMIT / 8)
                    drawRawBits(graphics, capture, record, visible);
            }

            /* Finish the capture with its pulse-density or discrete-flux chart. */
            drawFlux(graphics, capture, visible);
        }

        /* Keep the timeline names at the visible left edge when the canvas is scrolled. */
        int labelX = Math.max(CONTENT_PADDING, visible.x) + BORDER;
        int labelHeight = graphics.getFontMetrics().getHeight();
        graphics.setColor(UiUtils.DATA_OK_COLOUR.darker());
        text(graphics, "Sectors", labelX, centre(0) - labelHeight);
        graphics.setColor(UiUtils.HEADER_COLOUR.darker());
        text(graphics, "Raw records", labelX, centre(1) - labelHeight);

        /* Overlay the interval ruler for the current pointer position. */
        drawRuler(graphics);
    }

    /* Draw tick marks in capture-relative time and labels at the viewport's two edges. */
    private void drawScale(Graphics2D graphics, Capture capture, Rectangle visible)
    {
        /* Choose a power-of-ten tick interval with enough room between marks. */
        graphics.setColor(canvas.getForeground());
        double step = 1000;
        while (step / nanosecondsPerPixel < 10)
            step *= 10;

        /* Start at the first potentially visible tick and draw progressively larger marks. */
        double start =
                Math.max(0, (visible.x - CONTENT_PADDING) * nanosecondsPerPixel - capture.offset);
        long first = (long) Math.floor(start / step);
        int top = centre(2) + blockHeight() / 4;
        for (long index = first; index * step < capture.duration; index++)
        {
            double time = index * step;
            int x = pixel(capture.offset + time);
            if (x > visible.x + visible.width)
                break;
            int half = blockHeight() / 2;
            int tick = index % 100 == 0 ? half : index % 10 == 0 ? half / 2 : half / 3;
            graphics.drawLine(x, centre(3) - tick, x, centre(3) + tick);
            if (index % 10 == 0)
            {
                /* Centre each label above its tick, leaving room for the full text height. */
                String label = milliseconds(time);
                text(graphics, label, x - graphics.getFontMetrics().stringWidth(label) / 2, top);
            }
        }

        /* Label the times at the two viewport edges relative to this capture. */
        int leftEdge = Math.max(CONTENT_PADDING, visible.x);
        int rightEdge = Math.min(canvas.getWidth() - CONTENT_PADDING, visible.x + visible.width);
        double leftTime = (leftEdge - CONTENT_PADDING) * nanosecondsPerPixel - capture.offset;
        if (leftTime >= 0 && leftTime < capture.duration)
            text(graphics, milliseconds(leftTime), leftEdge + BORDER, top);
        double rightTime = (rightEdge - CONTENT_PADDING) * nanosecondsPerPixel - capture.offset;
        if (rightTime >= 0 && rightTime <= capture.duration)
        {
            String label = milliseconds(rightTime);
            text(
                    graphics,
                    label,
                    rightEdge - BORDER - graphics.getFontMetrics().stringWidth(label),
                    top);
        }
    }

    /* Records inherit an error colour when they belong to a sector with a bad status. */
    private static Color recordColour(Track track, Record record)
    {
        for (Sector sector : track.allSectors)
        {
            if (sector.status != Sector.Status.OK && sector.records.contains(record))
                return UiUtils.DATA_BAD_COLOUR.darker();
        }
        return UiUtils.HEADER_COLOUR.darker();
    }

    /* Clip text to its block while keeping hover shading and caller graphics state isolated. */
    private void drawBlock(Graphics2D graphics, Rectangle rect, Color colour, String label)
    {
        if (rect.width <= 0 || !rect.intersects(viewport.getViewRect()))
            return;

        /* Highlight hovered blocks through their fill rather than an outline. */
        Graphics2D g = UiUtils.getGraphics2D(graphics.create());
        try
        {
            g.setColor(pointer != null && rect.contains(pointer) ?
                    blend(colour, canvas.getForeground(), 0.1) :
                    colour);
            g.fill(rect);
            g.setColor(canvas.getForeground());

            /* Keep the label inside the block and centre it vertically. */
            g.clip(rect);
            text(
                    g,
                    label,
                    rect.x + BORDER,
                    rect.y + (rect.height - g.getFontMetrics().getHeight()) / 2);
        } finally
        {
            g.dispose();
        }
    }

    /* Draw decoded byte values within their record boxes. */
    private void drawDecodedBytes(
            Graphics2D graphics,
            Capture capture,
            Record record,
            Rectangle rect,
            Rectangle visible)
    {
        /* Reject records that cannot be decoded safely. */
        if (!(record.clockNs > 0) || !Double.isFinite(record.clockNs) ||
                record.endTimeNs <= record.startTimeNs || record.position < 0 ||
                record.position >= capture.track.fluxmap.bytes())
            return;

        /* Seek directly to the record bytecode with a record-relative time origin. */
        FluxmapReader reader = new FluxmapReader(capture.track.fluxmap, DECODER);
        reader.seek(new FluxPosition(record.position, 0, 0));
        FluxDecoder decoder = new FluxDecoder(reader, record.clockNs, DECODER);

        /* Keep byte labels and endpoint marks within the record box. */
        Graphics2D g = UiUtils.getGraphics2D(graphics.create());
        try
        {
            g.setColor(canvas.getForeground());
            g.clip(rect);

            /* Read labels until the record ends or the reader reaches EOF. */
            while (!reader.eof() &&
                    reader.tell().getDurationNs() <= record.endTimeNs - record.startTimeNs)
            {
                /* Decode the next complete byte and format it as two hexadecimal digits. */
                Bits value = decoder.readBits(8);
                if (value.size() < 8)
                    break;
                String label = String.format(Locale.ROOT, "%02x", value.toBytes().getByte(0) & 0xff);

                /* Stop beyond the visible region and skip labels that are still to its left. */
                int x = pixel(capture.offset + record.startTimeNs + reader.tell().getDurationNs());
                if (x > visible.x + visible.width + 50)
                    break;
                if (x < visible.x)
                    continue;

                /* Place each label immediately before its decoded endpoint. */
                g.drawLine(x, rect.y, x, rect.y + rect.height);
                text(
                        g,
                        label,
                        x - g.getFontMetrics().stringWidth(label) - BORDER,
                        centre(1) - g.getFontMetrics().getHeight() / 2);
            }
        } finally
        {
            g.dispose();
        }
    }

    /* Draw raw bit values above the flux timeline. */
    private void drawRawBits(
            Graphics2D graphics,
            Capture capture,
            Record record,
            Rectangle visible)
    {
        /* Reject records that cannot be decoded safely. */
        if (!(record.clockNs > 0) || !Double.isFinite(record.clockNs) ||
                record.endTimeNs <= record.startTimeNs || record.position < 0 ||
                record.position >= capture.track.fluxmap.bytes())
            return;

        /* Seek directly to the record bytecode with a record-relative time origin. */
        FluxmapReader reader = new FluxmapReader(capture.track.fluxmap, DECODER);
        reader.seek(new FluxPosition(record.position, 0, 0));
        FluxDecoder decoder = new FluxDecoder(reader, record.clockNs, DECODER);

        /* Isolate raw-bit labels from the record boxes and the caller's graphics state. */
        Graphics2D g = UiUtils.getGraphics2D(graphics.create());
        try
        {
            g.setColor(UiUtils.HEADER_COLOUR);

            /* Read labels until the record ends or the reader reaches EOF. */
            StringBuilder bits = new StringBuilder();
            while (!reader.eof() &&
                    reader.tell().getDurationNs() <= record.endTimeNs - record.startTimeNs)
            {
                /* Accumulate raw bits through the next pulse before placing their label. */
                boolean bit = decoder.readBit();
                bits.append(bit ? '1' : '0');
                if (!bit)
                    continue;
                String label = bits.toString();
                bits.setLength(0);

                /* Stop beyond the visible region and skip labels that are still to its left. */
                int x = pixel(capture.offset + record.startTimeNs + reader.tell().getDurationNs());
                if (x > visible.x + visible.width + 50)
                    break;
                if (x < visible.x)
                    continue;

                /* Place each label immediately before its decoded endpoint. */
                text(
                        g,
                        label,
                        x - g.getFontMetrics().stringWidth(label) - BORDER,
                        centre(4) - g.getFontMetrics().getHeight());
            }
        } finally
        {
            g.dispose();
        }
    }

    /* Density is normalized across all captures; close views draw individual transitions. */
    private void drawFlux(Graphics2D graphics, Capture capture, Rectangle visible)
    {
        int half = blockHeight() / 2;
        if (nanosecondsPerPixel > RENDER_LIMIT)
        {
            /* Shade visible columns using the cached, normalized pulse density. */
            int first = Math.max(visible.x, pixel(capture.offset));
            int last =
                    Math.min(visible.x + visible.width, pixel(capture.offset + capture.duration));
            for (int x = first; x < last; x++)
            {
                int index = x - pixel(capture.offset);
                double density = index < capture.density.length ? capture.density[index] : 0;
                graphics.setColor(blend(canvas.getBackground(), UiUtils.HEADER_COLOUR, density));
                graphics.fillRect(x, centre(4) - half, 1, blockHeight());
            }
        } else
        {
            /* At close scales, draw each visible transition as an individual pulse. */
            graphics.setColor(UiUtils.HEADER_COLOUR);
            FluxmapReader reader = new FluxmapReader(capture.track.fluxmap, DECODER);
            while (reader.findEvent(F_BIT_PULSE).found())
            {
                int x = pixel(capture.offset + reader.tell().getDurationNs());
                if (x > visible.x + visible.width)
                    break;
                if (x >= visible.x)
                    graphics.drawLine(x, centre(4) - half, x, centre(4) + half);
            }
        }

        /* Draw the flux baseline across the full capture extent. */
        graphics.setColor(UiUtils.HEADER_COLOUR);
        graphics.drawLine(
                pixel(capture.offset),
                centre(4),
                pixel(capture.offset + capture.duration),
                centre(4));
    }

    /* Blend semantic highlights into the current theme's background rather than a fixed grey. */
    private static Color blend(Color background, Color highlight, double factor)
    {
        return new Color(
                interpolate(background.getRed(), highlight.getRed(), factor),
                interpolate(background.getGreen(), highlight.getGreen(), factor),
                interpolate(background.getBlue(), highlight.getBlue(), factor));
    }

    private static int interpolate(int low, int high, double factor)
    {
        return (int) (low + (high - low) * factor);
    }

    private void drawRuler(Graphics2D graphics)
    {
        if (pointer == null)
            return;

        /* Find the record boundaries immediately surrounding the pointer time. */
        double time = (pointer.x - CONTENT_PADDING) * nanosecondsPerPixel;
        Double right = boundaries.ceiling(time);
        Double left = right == null ? null : boundaries.lower(right);
        if (left == null)
            return;

        /* Draw the interval and centre its duration label between the two boundaries. */
        int lx = pixel(left);
        int rx = pixel(right);
        int y = centre(2) - canvas.getHeight() / 10;
        graphics.setColor(canvas.getForeground());
        graphics.drawLine(lx, y, rx, y);
        String label = milliseconds(right - left);
        text(
                graphics,
                label,
                lx + (rx - lx) / 2 - graphics.getFontMetrics().stringWidth(label) / 2,
                y);
    }

    /* Java draws text at a baseline; wxWidgets' text coordinates refer to the top edge. */
    private static void text(Graphics2D graphics, String text, int x, int top)
    {
        graphics.drawString(text, x, top + graphics.getFontMetrics().getAscent());
    }

    private static String milliseconds(double time)
    {
        return String.format(Locale.ROOT, "%.3fms", time / 1e6);
    }

    /* Popup hit testing runs on the mouse event, never during painting. */
    private void showPopup(MouseEvent event)
    {
        popupShown = true;
        JPopupMenu menu = contextMenuAt(event.getPoint());
        if (menu != null)
            menu.show(canvas, event.getX(), event.getY());
    }

    JPopupMenu contextMenuAt(Point position)
    {
        /* Search in reverse painting order so the uppermost block receives the menu. */
        for (int i = captures.size() - 1; i >= 0; i--)
        {
            Capture capture = captures.get(i);

            /* Offer raw-record data when the pointer is over a record block. */
            List<Record> records = capture.track.records;
            for (int j = records.size() - 1; j >= 0; j--)
            {
                Record record = records.get(j);
                if (block(capture, record.startTimeNs, record.endTimeNs, 1).contains(position))
                    return popupMenu()
                            .add(menuItem("Show record data").onClick(event -> showText(
                                    "Raw record data " + (capture.track.ptl == null ?
                                            "" :
                                            "c" + capture.track.ptl.physicalCylinder + "h" +
                                                    capture.track.ptl.physicalHead) + " + " +
                                            milliseconds(record.startTimeNs), recordData(record))))
                            .get(JPopupMenu.class);
            }

            /* Offer sector summaries and decoded or raw data for a sector block. */
            List<Sector> sectors = capture.track.allSectors;
            for (int j = sectors.size() - 1; j >= 0; j--)
            {
                Sector sector = sectors.get(j);
                double end =
                        sector.dataEndTimeNs != 0 ? sector.dataEndTimeNs : sector.headerEndTimeNs;
                if (block(capture, sector.headerStartTimeNs, end, 0).contains(position))
                    return popupMenu()
                            .add(menuItem("Show sector summary").onClick(event -> showText(
                                    "Sector summary " + sector.logicalLocation,
                                    sectorSummary(sector))))
                            .add(menuItem("Show decoded data").onClick(event -> showText(
                                    "User data " + sector.logicalLocation,
                                    sectorData(sector, false))))
                            .add(menuItem("Show raw data").onClick(event -> showText(
                                    "Raw data " + sector.logicalLocation,
                                    sectorData(sector, true)))).get(JPopupMenu.class);
            }
        }
        return null;
    }

    private void showText(String title, String text)
    {
        TextViewerDialogue.show(this, title, text);
    }

    /* Reports retain all occurrences of the same logical sector, including duplicate captures. */
    String sectorSummary(Sector selected)
    {
        /* Collect every occurrence of the selected logical sector across the supplied captures. */
        List<Sector> matches = new ArrayList<>();
        for (Track track : tracks)
        {
            for (Sector sector : track.allSectors)
            {
                if (selected.logicalLocation.equals(sector.logicalLocation))
                    matches.add(sector);
            }
        }

        /* Report the occurrence count followed by metadata for each matching sector. */
        StringBuilder result = new StringBuilder("Number of times seen: " + matches.size() + "\n");
        for (int i = 0; i < matches.size(); i++)
            result
                    .append("\nInstance ")
                    .append(i)
                    .append(":\n\n")
                    .append(sectorMetadata(matches.get(i)));
        return result.toString();
    }

    String sectorData(Sector sector, boolean raw)
    {
        /* Start with sector metadata, then append decoded bytes or the underlying records. */
        StringBuilder result = new StringBuilder(sectorMetadata(sector));
        if (!raw)
            result.append('\n').append(hexDump(sector.data));
        else
        {
            result.append("Number of records: ").append(sector.records.size()).append('\n');

            /* Keep each raw record separate so its timing and bytecode position remain visible. */
            for (int i = 0; i < sector.records.size(); i++)
                result
                        .append("\nRecord ")
                        .append(i)
                        .append(":\n\n")
                        .append(recordData(sector.records.get(i)));
        }
        return result.toString();
    }

    private static String sectorMetadata(Sector sector)
    {
        return String.format(
                Locale.ROOT,
                "Sector status:     %s\nPhysical location: %s\nClock:             %.2fus / %s\n" +
                        "Bytecode position: %d\nData CRC16:        %04x\n",
                Sector.statusToString(sector.status),
                sector.physicalLocation == null ? "unknown" : sector.physicalLocation,
                sector.clockNs / 1000,
                sector.clockNs > 0 ?
                        String.format(Locale.ROOT, "%.0fkHz", 1e6 / sector.clockNs) :
                        "unknown",
                sector.position,
                Crc.crc16(Crc.CCITT_POLY, sector.data));
    }

    static String recordData(Record record)
    {
        return String.format(
                Locale.ROOT,
                "Bytecode position: %d\nStart:             %.2fms\nEnd:               %.2fms\n" +
                        "Data CRC16:        %04x\n\n%s",
                record.position,
                record.startTimeNs / 1e6,
                record.endTimeNs / 1e6,
                Crc.crc16(Crc.CCITT_POLY, record.rawData),
                hexDump(record.rawData));
    }

    private static String hexDump(Bytes bytes)
    {
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        try (PrintStream stream = new PrintStream(output, false, StandardCharsets.UTF_8))
        {
            Utils.hexdump(stream, bytes);
        }
        return output.toString(StandardCharsets.UTF_8);
    }

    /* Cache coarse-scale pulse counts outside painting, normalized across captures. */
    private final class Capture
    {
        final Track track;
        final double offset;
        final double duration;
        double densityScale = -1;
        double[] density = new double[0];

        Capture(Track track, double offset, double duration)
        {
            this.track = track;
            this.offset = offset;
            this.duration = duration;
        }

        void updateDensity()
        {
            if (nanosecondsPerPixel <= RENDER_LIMIT || densityScale == nanosecondsPerPixel)
                return;

            /* Allocate fresh bins only when a coarse-scale density cache needs rebuilding. */
            densityScale = nanosecondsPerPixel;
            density = new double[pixel(offset + duration) - pixel(offset) + 1];

            /* Count pulse endpoints in timeline-aligned pixel bins for this capture. */
            FluxmapReader reader = new FluxmapReader(track.fluxmap, DECODER);
            while (reader.findEvent(F_BIT_PULSE).found())
            {
                int index = pixel(offset + reader.tell().getDurationNs()) - pixel(offset);
                density[index]++;
            }
        }
    }

    /* A wide timeline whose height always matches the viewport rather than scrolling vertically. */
    private final class Canvas extends JPanel implements Scrollable
    {
        @Override
        protected void paintComponent(Graphics graphics)
        {
            super.paintComponent(graphics);
            Graphics2D copy = UiUtils.getGraphics2D(graphics.create());
            try
            {
                redraw(copy);
            } finally
            {
                copy.dispose();
            }
        }

        @Override
        public Dimension getPreferredSize()
        {
            return new Dimension(
                    2 * CONTENT_PADDING + (int) Math.ceil(totalDurationNs / nanosecondsPerPixel),
                    UiUtils.pointsToPixels(HEIGHT_PTS));
        }

        @Override
        public Dimension getPreferredScrollableViewportSize()
        {
            return new Dimension(0, UiUtils.pointsToPixels(HEIGHT_PTS));
        }

        @Override
        public boolean getScrollableTracksViewportHeight()
        {
            return true;
        }

        @Override
        public boolean getScrollableTracksViewportWidth()
        {
            return false;
        }

        @Override
        public int getScrollableUnitIncrement(Rectangle visible, int orientation, int direction)
        {
            return UiUtils.pointsToPixels(16);
        }

        @Override
        public int getScrollableBlockIncrement(Rectangle visible, int orientation, int direction)
        {
            return Math.max(1, visible.width - UiUtils.pointsToPixels(16));
        }
    }

    private static String getTitle(ImmutableList<Track> tracks){
        var ptl = Iterables.getFirst(tracks,null).ptl;
        return String.format("Physical track c%dh%d", ptl.physicalCylinder, ptl.physicalHead);
    }
}
