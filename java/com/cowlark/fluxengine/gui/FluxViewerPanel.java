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
import javax.swing.JComponent;
import javax.swing.JInternalFrame;
import javax.swing.JPanel;
import javax.swing.JPopupMenu;
import javax.swing.JScrollPane;
import javax.swing.JViewport;
import javax.swing.Scrollable;
import javax.swing.SwingUtilities;
import javax.swing.plaf.basic.BasicInternalFrameUI;
import java.awt.BasicStroke;
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
    /* Geometry and detail thresholds from the C++ flux viewer. */
    private static final int HEIGHT_PTS = 150;
    private static final int BORDER = 4;
    private static final int MAX_WIDTH = Integer.MAX_VALUE - 1024;
    private static final double RENDER_LIMIT = 3000.0;
    private static final Color BACKGROUND = new Color(192, 192, 192);
    private static final Color FLUX = new Color(64, 64, 255);
    private static final Color BAD_SECTOR = new Color(213, 94, 0);
    private static final DecoderProto DECODER = DecoderProto.getDefaultInstance();

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
        super("Flux view", false, true, false, false);
        this.tracks = tracks;

        /* Keep captures in input order and accumulate offsets in time, rather than pixels. */
        ImmutableList.Builder<Capture> captureBuilder = ImmutableList.builder();
        double offset = 0;
        for (Track track : tracks)
        {
            if (track.fluxmap == null)
                continue;
            double duration = track.fluxmap.durationNs();
            if (!(duration > 0) || !Double.isFinite(duration))
                continue;
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
        canvas.setBackground(BACKGROUND);
        canvas.setFont(new Font(Font.SANS_SERIF, Font.PLAIN, UiUtils.pointsToPixels(6)));
        JScrollPane scrollPane = new JScrollPane(canvas,
            JScrollPane.VERTICAL_SCROLLBAR_NEVER,
            JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
        scrollPane.setWheelScrollingEnabled(false);
        viewport = scrollPane.getViewport();
        viewport.addComponentListener(new ComponentAdapter() {
            @Override public void componentResized(ComponentEvent event)
            {
                if (!scaleInitialised && viewport.getWidth() > 0)
                {
                    scaleInitialised = true;
                    updateScale(fitScale());
                }
                else
                    updateScale(nanosecondsPerPixel);
            }
        });
        viewport.addChangeListener(event -> {
            int scrollX = viewport.getViewPosition().x;
            if (pointer != null)
                pointer.translate(scrollX - previousScrollX, 0);
            previousScrollX = scrollX;
            canvas.repaint();
        });

        /* Track pointer movement, horizontal dragging, and native popup gestures. */
        MouseAdapter mouse = new MouseAdapter() {
            @Override public void mouseMoved(MouseEvent event)
            { trackPointer(event); }

            @Override public void mouseExited(MouseEvent event)
            {
                pointer = null;
                canvas.repaint();
            }

            @Override public void mousePressed(MouseEvent event)
            {
                popupShown = false;
                trackPointer(event);
                if (event.isPopupTrigger())
                    showPopup(event);
                else if (SwingUtilities.isLeftMouseButton(event))
                {
                    dragPosition = SwingUtilities.convertPoint(canvas, event.getPoint(), viewport);
                    canvas.setCursor(Cursor.getPredefinedCursor(Cursor.MOVE_CURSOR));
                }
            }

            @Override public void mouseDragged(MouseEvent event)
            {
                if (dragPosition != null)
                {
                    Point position =
                        SwingUtilities.convertPoint(canvas, event.getPoint(), viewport);
                    setScrollX((long) viewport.getViewPosition().x + dragPosition.x - position.x);
                    dragPosition = position;
                    pointer = new Point(viewport.getViewPosition().x + position.x, position.y);
                    canvas.repaint();
                }
                else
                    trackPointer(event);
            }

            @Override public void mouseReleased(MouseEvent event)
            {
                if (event.isPopupTrigger() && !popupShown)
                    showPopup(event);
                dragPosition = null;
                canvas.setCursor(Cursor.getDefaultCursor());
                trackPointer(event);
            }

            @Override public void mouseWheelMoved(MouseWheelEvent event)
            {
                Point anchor = SwingUtilities.convertPoint(canvas, event.getPoint(), viewport);
                zoom(event.getPreciseWheelRotation(), anchor.x);
                pointer = new Point(viewport.getViewPosition().x + anchor.x, event.getY());
                event.consume();
                canvas.repaint();
            }
        };
        canvas.addMouseListener(mouse);
        canvas.addMouseMotionListener(mouse);
        canvas.addMouseWheelListener(mouse);

        /* Keep each row a fixed height without contributing to the window's preferred width. */
        int height = UiUtils.pointsToPixels(HEIGHT_PTS);
        JPanel content = new JPanel();
        of(content).withLayout("fill, insets 0").add("grow, push", of(scrollPane));
        content.setPreferredSize(new Dimension(0, height));
        setContentPane(content);
        setFrameIcon(null);
        setDefaultCloseOperation(DISPOSE_ON_CLOSE);
        int frameHeight = getPreferredSize().height;
        setMinimumSize(new Dimension(0, frameHeight));
        setPreferredSize(new Dimension(0, frameHeight));
        setMaximumSize(new Dimension(Integer.MAX_VALUE, frameHeight));
        setVisible(true);
    }

    /* Keep the look and feel's title bar, but leave positioning to the row layout. */
    @Override public void updateUI()
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
    { return Math.max(minimumScale(), totalDurationNs / Math.max(1, viewport.getWidth())); }

    private double minimumScale()
    { return Math.max(30, totalDurationNs / (MAX_WIDTH - 1.0)); }

    private void updateScale(double scale)
    {
        nanosecondsPerPixel = Math.max(minimumScale(), scale);
        for (Capture capture : captures)
            capture.updateDensity();
        double maximum = 0;
        for (Capture capture : captures)
        {
            for (double value : capture.density)
                maximum = Math.max(maximum, value);
        }
        if (maximum > 0)
        {
            for (Capture capture : captures)
            {
                for (int i = 0; i < capture.density.length; i++)
                    capture.density[i] /= maximum;
            }
        }
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
        double time = (viewport.getViewPosition().x + (double) anchorX) * nanosecondsPerPixel;
        updateScale(Math.min(fitScale(), nanosecondsPerPixel * Math.pow(1.2, rotation)));
        setScrollX(Math.round(time / nanosecondsPerPixel - anchorX));
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
    { return (int) Math.min(MAX_WIDTH, Math.max(0, time / nanosecondsPerPixel)); }

    private int centre(int row)
    {
        int rowHeight = canvas.getHeight() / 5;
        return row * rowHeight + rowHeight / 2;
    }

    private int blockHeight()
    { return Math.max(1, (canvas.getHeight() / 5) * 2 / 3); }

    private Rectangle block(Capture capture, double start, double end, int row)
    {
        int left = pixel(capture.offset + start);
        int right = pixel(capture.offset + end);
        return new Rectangle(
            left, centre(row) - blockHeight() / 2, Math.max(0, right - left), blockHeight());
    }

    /* Paint only the visible part of the concatenated timeline, in the original layer order. */
    protected void redraw(Graphics2D graphics)
    {
        Rectangle visible = viewport.getViewRect();
        graphics.setColor(BACKGROUND);
        graphics.fillRect(visible.x, 0, visible.width, canvas.getHeight());
        graphics.setFont(canvas.getFont());
        if (totalDurationNs <= 0 || visible.width <= 0)
            return;
        for (Capture capture : captures)
        {
            int left = pixel(capture.offset);
            int right = pixel(capture.offset + capture.duration);
            if (right < visible.x || left > visible.x + visible.width)
                continue;
            if (capture.offset != 0)
            {
                graphics.setColor(Color.RED);
                graphics.drawLine(left, 0, left, canvas.getHeight());
            }
            graphics.setColor(Color.BLACK);
            for (int row : new int[] {0, 1, 3})
                graphics.drawLine(left, centre(row), right, centre(row));
            graphics.setColor(Color.YELLOW);
            for (double index : capture.track.fluxmap.getIndexMarks())
            {
                int x = pixel(capture.offset + index);
                if (x > visible.x + visible.width)
                    break;
                if (x >= visible.x)
                    graphics.drawLine(x, 0, x, canvas.getHeight());
            }
            drawScale(graphics, capture, visible);
            for (Sector sector : capture.track.allSectors)
            {
                double end =
                    sector.dataEndTimeNs != 0 ? sector.dataEndTimeNs : sector.headerEndTimeNs;
                Rectangle rect = block(capture, sector.headerStartTimeNs, end, 0);
                drawBlock(graphics,
                    rect,
                    sector.status == Sector.Status.OK ? Color.WHITE : BAD_SECTOR,
                    sector.logicalLocation + " " + Sector.statusToString(sector.status));
            }
            for (Record record : capture.track.records)
            {
                Rectangle rect = block(capture, record.startTimeNs, record.endTimeNs, 1);
                if (!rect.intersects(visible))
                    continue;
                drawBlock(graphics,
                    rect,
                    new Color(200, 200, 200),
                    nanosecondsPerPixel > RENDER_LIMIT / 4
                        ? String.format(Locale.ROOT, "+%.3fms", record.startTimeNs / 1e6)
                        : "");
                if (nanosecondsPerPixel <= RENDER_LIMIT / 4)
                    drawDecoded(graphics, capture, record, rect, visible, false);
                if (nanosecondsPerPixel < RENDER_LIMIT / 8)
                    drawDecoded(graphics, capture, record, rect, visible, true);
            }
            drawFlux(graphics, capture, visible);
        }
        drawRuler(graphics);
    }

    /* Draw tick marks in capture-relative time and labels at the viewport's two edges. */
    private void drawScale(Graphics2D graphics, Capture capture, Rectangle visible)
    {
        graphics.setColor(Color.BLACK);
        double step = 1000;
        while (step / nanosecondsPerPixel < 10)
            step *= 10;
        double start = Math.max(0, visible.x * nanosecondsPerPixel - capture.offset);
        long first = (long) Math.floor(start / step);
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
                text(graphics, milliseconds(time), x, centre(3) - half);
        }
        int top = centre(2) + blockHeight() / 4;
        double leftTime = visible.x * nanosecondsPerPixel - capture.offset;
        if (leftTime >= 0 && leftTime < capture.duration)
            text(graphics, milliseconds(leftTime), visible.x + BORDER, top);
        double rightTime = (visible.x + visible.width) * nanosecondsPerPixel - capture.offset;
        if (rightTime >= 0 && rightTime <= capture.duration)
        {
            String label = milliseconds(rightTime);
            text(graphics,
                label,
                visible.x + visible.width - BORDER - graphics.getFontMetrics().stringWidth(label),
                top);
        }
    }

    /* Clip text to its block while keeping hover strokes and caller graphics state isolated. */
    private void drawBlock(Graphics2D graphics, Rectangle rect, Color colour, String label)
    {
        if (rect.width <= 0 || !rect.intersects(viewport.getViewRect()))
            return;
        Graphics2D g = (Graphics2D) graphics.create();
        try
        {
            g.setColor(colour);
            g.fill(rect);
            g.setColor(Color.BLACK);
            g.setStroke(new BasicStroke(pointer != null && rect.contains(pointer) ? 2 : 1));
            g.draw(rect);
            g.clip(rect);
            text(g,
                label,
                rect.x + BORDER,
                rect.y + (rect.height - g.getFontMetrics().getHeight()) / 2);
        }
        finally
        {
            g.dispose();
        }
    }

    /* Decode from a bytecode position with a record-relative clock, as in the C++ viewer. */
    private void drawDecoded(Graphics2D graphics,
        Capture capture,
        Record record,
        Rectangle rect,
        Rectangle visible,
        boolean rawBits)
    {
        if (!(record.clockNs > 0) || !Double.isFinite(record.clockNs) ||
            record.endTimeNs <= record.startTimeNs || record.position < 0 ||
            record.position >= capture.track.fluxmap.bytes())
            return;
        FluxmapReader reader = new FluxmapReader(capture.track.fluxmap, DECODER);
        reader.seek(new FluxPosition(record.position, 0, 0));
        FluxDecoder decoder = new FluxDecoder(reader, record.clockNs, DECODER);
        Graphics2D g = (Graphics2D) graphics.create();
        try
        {
            g.setColor(Color.BLACK);
            if (!rawBits)
                g.clip(rect);
            StringBuilder bits = new StringBuilder();
            while (!reader.eof() &&
                   reader.tell().getDurationNs() <= record.endTimeNs - record.startTimeNs)
            {
                String label;
                if (rawBits)
                {
                    boolean bit = decoder.readBit();
                    bits.append(bit ? '1' : '0');
                    if (!bit)
                        continue;
                    label = bits.toString();
                    bits.setLength(0);
                }
                else
                {
                    Bits value = decoder.readBits(8);
                    if (value.size() < 8)
                        break;
                    label = String.format(Locale.ROOT, "%02x", value.toBytes().getByte(0) & 0xff);
                }
                int x = pixel(capture.offset + record.startTimeNs + reader.tell().getDurationNs());
                if (x > visible.x + visible.width + 50)
                    break;
                if (x < visible.x)
                    continue;
                if (!rawBits)
                    g.drawLine(x, rect.y, x, rect.y + rect.height);
                text(g,
                    label,
                    x - g.getFontMetrics().stringWidth(label) - BORDER,
                    rawBits ? centre(3) : centre(1) - g.getFontMetrics().getHeight() / 2);
            }
        }
        finally
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
            int first = Math.max(visible.x, pixel(capture.offset));
            int last =
                Math.min(visible.x + visible.width, pixel(capture.offset + capture.duration));
            for (int x = first; x < last; x++)
            {
                int index = x - pixel(capture.offset);
                double density = index < capture.density.length ? capture.density[index] : 0;
                graphics.setColor(new Color(interpolate(192, 64, density),
                    interpolate(192, 64, density),
                    interpolate(192, 255, density)));
                graphics.fillRect(x, centre(4) - half, 1, blockHeight());
            }
        }
        else
        {
            graphics.setColor(FLUX);
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
        graphics.setColor(FLUX);
        graphics.drawLine(
            pixel(capture.offset), centre(4), pixel(capture.offset + capture.duration), centre(4));
    }

    private static int interpolate(int low, int high, double factor)
    { return (int) (low + (high - low) * factor); }

    private void drawRuler(Graphics2D graphics)
    {
        if (pointer == null)
            return;
        double time = pointer.x * nanosecondsPerPixel;
        Double right = boundaries.ceiling(time);
        Double left = right == null ? null : boundaries.lower(right);
        if (left == null)
            return;
        int lx = pixel(left);
        int rx = pixel(right);
        int y = centre(2) - canvas.getHeight() / 10;
        graphics.setColor(Color.BLACK);
        graphics.drawLine(lx, y, rx, y);
        String label = milliseconds(right - left);
        text(graphics,
            label,
            lx + (rx - lx) / 2 - graphics.getFontMetrics().stringWidth(label) / 2,
            y);
    }

    /* Java draws text at a baseline; wxWidgets' text coordinates refer to the top edge. */
    private static void text(Graphics2D graphics, String text, int x, int top)
    { graphics.drawString(text, x, top + graphics.getFontMetrics().getAscent()); }

    private static String milliseconds(double time)
    { return String.format(Locale.ROOT, "%.3fms", time / 1e6); }

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
        for (int i = captures.size() - 1; i >= 0; i--)
        {
            Capture capture = captures.get(i);
            List<Record> records = capture.track.records;
            for (int j = records.size() - 1; j >= 0; j--)
            {
                Record record = records.get(j);
                if (block(capture, record.startTimeNs, record.endTimeNs, 1).contains(position))
                    return popupMenu()
                        .add(menuItem("Show record data")
                                .onClick(event
                                    -> showText(
                                        "Raw record data " +
                                            (capture.track.ptl == null
                                                    ? ""
                                                    : "c" + capture.track.ptl.physicalCylinder +
                                                          "h" + capture.track.ptl.physicalHead) +
                                            " + " + milliseconds(record.startTimeNs),
                                        recordData(record))))
                        .get(JPopupMenu.class);
            }
            List<Sector> sectors = capture.track.allSectors;
            for (int j = sectors.size() - 1; j >= 0; j--)
            {
                Sector sector = sectors.get(j);
                double end =
                    sector.dataEndTimeNs != 0 ? sector.dataEndTimeNs : sector.headerEndTimeNs;
                if (block(capture, sector.headerStartTimeNs, end, 0).contains(position))
                    return popupMenu()
                        .add(menuItem("Show sector summary")
                                .onClick(event
                                    -> showText("Sector summary " + sector.logicalLocation,
                                        sectorSummary(sector))))
                        .add(menuItem("Show decoded data")
                                .onClick(event
                                    -> showText("User data " + sector.logicalLocation,
                                        sectorData(sector, false))))
                        .add(menuItem("Show raw data")
                                .onClick(event
                                    -> showText("Raw data " + sector.logicalLocation,
                                        sectorData(sector, true))))
                        .get(JPopupMenu.class);
            }
        }
        return null;
    }

    private void showText(String title, String text)
    { TextViewerDialogue.show(this, title, text); }

    /* Reports retain all occurrences of the same logical sector, including duplicate captures. */
    String sectorSummary(Sector selected)
    {
        List<Sector> matches = new ArrayList<>();
        for (Track track : tracks)
        {
            for (Sector sector : track.allSectors)
            {
                if (selected.logicalLocation.equals(sector.logicalLocation))
                    matches.add(sector);
            }
        }
        StringBuilder result = new StringBuilder("Number of times seen: " + matches.size() + "\n");
        for (int i = 0; i < matches.size(); i++)
            result.append("\nInstance ")
                .append(i)
                .append(":\n\n")
                .append(sectorMetadata(matches.get(i)));
        return result.toString();
    }

    String sectorData(Sector sector, boolean raw)
    {
        StringBuilder result = new StringBuilder(sectorMetadata(sector));
        if (!raw)
            result.append('\n').append(hexDump(sector.data));
        else
        {
            result.append("Number of records: ").append(sector.records.size()).append('\n');
            for (int i = 0; i < sector.records.size(); i++)
                result.append("\nRecord ")
                    .append(i)
                    .append(":\n\n")
                    .append(recordData(sector.records.get(i)));
        }
        return result.toString();
    }

    private static String sectorMetadata(Sector sector)
    {
        return String.format(Locale.ROOT,
            "Sector status:     %s\nPhysical location: %s\nClock:             %.2fus / %s\n"
                + "Bytecode position: %d\nData CRC16:        %04x\n",
            Sector.statusToString(sector.status),
            sector.physicalLocation == null ? "unknown" : sector.physicalLocation,
            sector.clockNs / 1000,
            sector.clockNs > 0 ? String.format(Locale.ROOT, "%.0fkHz", 1e6 / sector.clockNs)
                               : "unknown",
            sector.position,
            Crc.crc16(Crc.CCITT_POLY, sector.data));
    }

    static String recordData(Record record)
    {
        return String.format(Locale.ROOT,
            "Bytecode position: %d\nStart:             %.2fms\nEnd:               %.2fms\n"
                + "Data CRC16:        %04x\n\n%s",
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
            densityScale = nanosecondsPerPixel;
            density = new double[pixel(offset + duration) - pixel(offset) + 1];
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
        @Override protected void paintComponent(Graphics graphics)
        {
            super.paintComponent(graphics);
            Graphics2D copy = (Graphics2D) graphics.create();
            try
            {
                redraw(copy);
            }
            finally
            {
                copy.dispose();
            }
        }

        @Override public Dimension getPreferredSize()
        {
            return new Dimension(
                Math.max(1, (int) Math.ceil(totalDurationNs / nanosecondsPerPixel)),
                UiUtils.pointsToPixels(HEIGHT_PTS));
        }

        @Override public Dimension getPreferredScrollableViewportSize()
        { return new Dimension(0, UiUtils.pointsToPixels(HEIGHT_PTS)); }

        @Override public boolean getScrollableTracksViewportHeight()
        { return true; }

        @Override public boolean getScrollableTracksViewportWidth()
        { return false; }

        @Override
        public int getScrollableUnitIncrement(Rectangle visible, int orientation, int direction)
        { return UiUtils.pointsToPixels(16); }

        @Override
        public int getScrollableBlockIncrement(Rectangle visible, int orientation, int direction)
        { return Math.max(1, visible.width - UiUtils.pointsToPixels(16)); }
    }
}
