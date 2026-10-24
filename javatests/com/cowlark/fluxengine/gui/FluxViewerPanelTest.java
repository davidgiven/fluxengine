package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Record;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Track;
import com.google.common.collect.ImmutableList;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import javax.swing.JComponent;
import javax.imageio.ImageIO;
import javax.swing.JMenuItem;
import javax.swing.JPopupMenu;
import javax.swing.JViewport;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;
import javax.swing.LookAndFeel;
import com.formdev.flatlaf.FlatDarkLaf;
import com.formdev.flatlaf.FlatLightLaf;
import java.awt.Component;
import java.awt.Container;
import java.awt.Graphics2D;
import java.awt.Point;
import java.awt.event.ComponentEvent;
import java.awt.event.MouseEvent;
import java.awt.event.MouseWheelEvent;
import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;

@RunWith(JUnit4.class) public class FluxViewerPanelTest
{
    /* Use a simple timed pulse stream with one index and a sector/record over its middle. */
    private static Track track(boolean pulses)
    {
        Track track = new Track();
        track.fluxmap = new Fluxmap();
        for (int i = 0; i < 200; i++)
        {
            track.fluxmap.appendInterval(2400);
            if (pulses)
                track.fluxmap.appendPulse();
            if (i == 100)
                track.fluxmap.appendIndex();
        }
        Record record = new Record();
        record.startTimeNs
        = track.fluxmap.durationNs() / 4;
        record.endTimeNs
        = track.fluxmap.durationNs() * 3 / 4;
        record.clockNs
        = 2000;
        record.rawData
        = Bytes.of(0x12, 0x34);
        track.records.add(record);
        Sector sector = new Sector(new CylinderHeadSector(1, 0, 3));
        sector.status = Sector.Status.OK;
        sector.headerStartTimeNs = record.startTimeNs;
        sector.headerEndTimeNs = record.endTimeNs;
        sector.records.add(record);
        sector.data = Bytes.of(0xab, 0xcd);
        track.allSectors.add(sector);
        return track;
    }

    private static JViewport viewport(FluxViewerPanel panel)
    {
        JViewport viewport = find(panel, JViewport.class);
        viewport.setSize(400, 200);
        viewport.dispatchEvent(new ComponentEvent(viewport, ComponentEvent.COMPONENT_RESIZED));
        viewport.doLayout();
        return viewport;
    }

    private static <T extends Component> T find(Container root, Class<T> type)
    {
        for (Component child : root.getComponents())
        {
            if (type.isInstance(child))
                return type.cast(child);
            if (child instanceof Container container)
            {
                T result = find(container, type);
                if (result != null)
                    return result;
            }
        }
        return null;
    }

    private static BufferedImage paint(JViewport viewport)
    {
        BufferedImage image = new BufferedImage(400, 200, BufferedImage.TYPE_INT_RGB);
        Graphics2D graphics = image.createGraphics();
        try
        {
            viewport.paint(graphics);
        }
        finally
        {
            graphics.dispose();
        }
        return image;
    }

    /* Retain a representative rendering as a Bazel test artifact for visual inspection. */
    private static void snapshot(String name, BufferedImage image)
    {
        String output = System.getenv("TEST_UNDECLARED_OUTPUTS_DIR");
        if (output == null)
            return;
        try
        {
            ImageIO.write(image, "png", new File(output, name + ".png"));
        }
        catch (IOException exception)
        {
            throw new AssertionError(exception);
        }
    }

    @Test public void fitZoomAnchorsAndClamps() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            FluxViewerPanel panel = new FluxViewerPanel(null, ImmutableList.of(track(true)));
            JViewport viewport = viewport(panel);
            assertThat(viewport.getView().getWidth()).isEqualTo(400);
            assertThat(viewport.getView().getHeight()).isEqualTo(200);
            assertThat(panel.getPreferredSize().width).isEqualTo(0);

            /* At the centre, one inward notch increases both content width and view offset. */
            panel.zoom(-1, 200);
            assertThat(viewport.getView().getWidth()).isEqualTo(472);
            assertThat(viewport.getViewPosition().x).isEqualTo(36);
            panel.zoom(1, 200);
            assertThat(viewport.getViewPosition().x).isEqualTo(0);
            assertThat(viewport.getView().getWidth()).isEqualTo(400);
            panel.zoom(100, 200);
            assertThat(viewport.getView().getWidth()).isEqualTo(400);

            /* Zoom to the minimum scale, then resize without resetting that scale. */
            panel.zoom(-100, 200);
            int width = viewport.getView().getWidth();
            panel.zoom(-100, 200);
            assertThat(viewport.getView().getWidth()).isEqualTo(width);
            viewport.setSize(300, 160);
            viewport.dispatchEvent(new ComponentEvent(viewport, ComponentEvent.COMPONENT_RESIZED));
            assertThat(viewport.getView().getWidth()).isEqualTo(width);
            assertThat(viewport.getViewPosition().y).isEqualTo(0);
            panel.dispose();
        });
    }

    @Test public void drawsLayersAndTargetsMenusInContentCoordinates() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            Track first = track(true);
            Track second = track(true);
            second.allSectors.getFirst().status = Sector.Status.BAD_CHECKSUM;
            FluxViewerPanel panel = new FluxViewerPanel(null, ImmutableList.of(first, second));
            JViewport viewport = viewport(panel);
            BufferedImage image = paint(viewport);
            snapshot("captures", image);

            /* Both timeline ends reserve blank space even where tick labels are clipped. */
            int paper = viewport.getView().getBackground().getRGB();
            for (int x : new int[]{0, 1, 18, 19, 380, 381, 398, 399})
            {
                for (int y = 0; y < image.getHeight(); y++)
                    assertThat(image.getRGB(x, y)).isEqualTo(paper);
            }

            /* Sample block interiors away from text, outlines, and the index marker. */
            assertThat(image.getRGB(80, 10) & 0xffffff).isEqualTo(UiUtils.DATA_OK_COLOUR.darker().getRGB() & 0xffffff);
            assertThat(image.getRGB(260, 10)).isEqualTo(UiUtils.DATA_BAD_COLOUR.darker().getRGB());
            assertThat(image.getRGB(200, 5) & 0xffffff).isEqualTo(UiUtils.DATA_BAD_COLOUR.getRGB() & 0xffffff);
            assertThat(image.getRGB(80, 50)).isEqualTo(UiUtils.HEADER_COLOUR.darker().getRGB());
            assertThat(image.getRGB(260, 50)).isEqualTo(UiUtils.DATA_BAD_COLOUR.darker().getRGB());
            JPopupMenu menu = panel.contextMenuAt(new Point(80, 10));
            assertThat(menu.getComponentCount()).isEqualTo(3);
            assertThat(((JMenuItem) menu.getComponent(0)).getText())
                .isEqualTo("Show sector summary");
            assertThat(panel.contextMenuAt(new Point(80, 50)).getComponentCount()).isEqualTo(1);
            assertThat(panel.contextMenuAt(new Point(80, 110))).isNull();

            /* Reports include both captures even when their statuses differ. */
            assertThat(panel.sectorSummary(first.allSectors.getFirst()))
                .contains("Number of times seen: 2");
            assertThat(panel.sectorSummary(first.allSectors.getFirst())).contains("bad checksum");
            assertThat(panel.sectorData(first.allSectors.getFirst(), false)).contains("ab cd");
            assertThat(panel.sectorData(first.allSectors.getFirst(), true)).contains("12 34");
            assertThat(FluxViewerPanel.recordData(first.records.getFirst()))
                .contains("Data CRC16:");
            panel.dispose();
        });
    }

    @Test public void wheelHoverDragAndRenderingAtEveryDetailLevel() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            FluxViewerPanel panel = new FluxViewerPanel(null, ImmutableList.of(track(true)));
            JViewport viewport = viewport(panel);
            JComponent canvas = (JComponent) viewport.getView();
            BufferedImage normal = paint(viewport);
            assertThat(normal.getRGB(120, 170) & 0xffffff).isEqualTo(UiUtils.HEADER_COLOUR.getRGB() & 0xffffff);
            canvas.dispatchEvent(
                new MouseEvent(canvas, MouseEvent.MOUSE_MOVED, 0, 0, 120, 10, 0, false));
            BufferedImage hovered = paint(viewport);
            assertThat(hovered.getRGB(120, 10)).isNotEqualTo(normal.getRGB(120, 10));
            canvas.dispatchEvent(
                new MouseEvent(canvas, MouseEvent.MOUSE_EXITED, 0, 0, 120, 10, 0, false));
            assertThat(paint(viewport).getRGB(120, 10)).isEqualTo(normal.getRGB(120, 10));

            /* A wheel event must zoom and be consumed instead of scrolling the outer panel. */
            MouseWheelEvent wheel = new MouseWheelEvent(canvas,
                MouseEvent.MOUSE_WHEEL,
                0,
                0,
                200,
                100,
                0,
                false,
                MouseWheelEvent.WHEEL_UNIT_SCROLL,
                3,
                -1);
            canvas.dispatchEvent(wheel);
            assertThat(wheel.isConsumed()).isTrue();
            assertThat(canvas.getWidth()).isEqualTo(472);
            canvas.dispatchEvent(new MouseEvent(
                canvas, MouseEvent.MOUSE_PRESSED, 0, 0, 240, 100, 1, false, MouseEvent.BUTTON1));
            canvas.dispatchEvent(new MouseEvent(canvas,
                MouseEvent.MOUSE_DRAGGED,
                0,
                MouseEvent.BUTTON1_DOWN_MASK,
                220,
                130,
                0,
                false,
                MouseEvent.NOBUTTON));
            assertThat(viewport.getViewPosition().x).isEqualTo(56);
            assertThat(viewport.getViewPosition().y).isEqualTo(0);
            canvas.dispatchEvent(new MouseEvent(
                canvas, MouseEvent.MOUSE_RELEASED, 0, 0, 240, 100, 1, false, MouseEvent.BUTTON1));

            /* Exercise density, individual pulses, bytes, and raw bits, including record EOF. */
            for (int i = 0; i < 42; i++)
            {
                panel.zoom(-1, 200);
                paint(viewport);
                if (i == 32)
                {
                    viewport.setViewPosition(new Point((int) (canvas.getWidth() * 0.539), 0));
                    snapshot("raw-bits", paint(viewport));
                }
            }
            panel.dispose();
        });
    }

    @Test
    public void paletteFollowsLightAndDarkThemes() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            LookAndFeel original = UIManager.getLookAndFeel();
            try
            {
                FluxViewerPanel panel = new FluxViewerPanel(null, ImmutableList.of(track(true)));
                for (LookAndFeel theme : ImmutableList.of(new FlatLightLaf(), new FlatDarkLaf()))
                {
                    UIManager.setLookAndFeel(theme);
                    SwingUtilities.updateComponentTreeUI(panel);
                    JViewport viewport = viewport(panel);
                    BufferedImage image = paint(viewport);
                    assertThat(image.getRGB(10, 100))
                            .isEqualTo(UIManager.getColor("Panel.background").getRGB());
                    assertThat(image.getRGB(120, 10))
                            .isEqualTo(UiUtils.DATA_OK_COLOUR.darker().getRGB());
                    /* Semantic record fills remain consistent under both themes. */
                    assertThat(image.getRGB(120, 50))
                            .isEqualTo(UiUtils.HEADER_COLOUR.darker().getRGB());
                    snapshot(theme instanceof FlatDarkLaf ? "dark-theme" : "light-theme", image);
                }
                panel.dispose();
            }
            catch (Exception exception)
            {
                throw new AssertionError(exception);
            }
            finally
            {
                try
                {
                    UIManager.setLookAndFeel(original);
                }
                catch (Exception exception)
                {
                    throw new AssertionError(exception);
                }
            }
        });
    }

    @Test public void absentZeroDurationAndPulseFreeInputsAreSafe() throws Exception
    {
        SwingUtilities.invokeAndWait(() -> {
            Track absent = new Track();
            Track empty = new Track();
            empty.fluxmap = new Fluxmap();
            for (ImmutableList<Track> tracks : ImmutableList.of(ImmutableList.<Track>of(),
                     ImmutableList.of(absent, empty),
                     ImmutableList.of(track(false))))
            {
                FluxViewerPanel panel = new FluxViewerPanel(null, tracks);
                JViewport viewport = viewport(panel);
                paint(viewport);
                panel.zoom(-100, 200);
                paint(viewport);
                panel.zoom(100, 200);
                paint(viewport);
                panel.dispose();
            }
        });
    }
}
