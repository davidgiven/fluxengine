package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.when;

import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.DiskLayout;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;
import sprouts.Var;

@RunWith(MockitoJUnitRunner.class)
public class SummaryPanelTest
{
    @Mock private ImagerViewModel model;

    @Test
    public void addPhysicalAndLogicalViews()
    {
        Disk disk = new Disk();
        disk.diskLayout = new DiskLayout(80, 2, 18, 512);
        when(model.getDisk()).thenReturn(Var.of(disk));
        when(model.getDriveActivity()).thenReturn(Var.of(new DriveActivity(
                DriveActivity.ActivityType.IDLE,
                0,
                0)));

        SummaryPanel panel = new SummaryPanel(model);

        /* Physical label + header + 2 button rows, then logical label + 2
         * button rows + header. Each header/button row spans 82 columns
         * (2 indicators + 80 cylinders). */
        int header = 82;
        int buttonRow = 82;
        int expected = 1 + header + 2 * buttonRow + 1 + 2 * buttonRow + header;
        assertThat(panel.getComponentCount()).isEqualTo(expected);
    }
}
