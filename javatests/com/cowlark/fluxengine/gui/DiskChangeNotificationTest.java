package com.cowlark.fluxengine.gui;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.data.Disk;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import sprouts.From;
import sprouts.Var;
import sprouts.Viewable;
import java.util.concurrent.atomic.AtomicBoolean;

@RunWith(JUnit4.class)
public class DiskChangeNotificationTest
{
    @Test
    public void settingDiskFiresAllChannelListeners()
    {
        Var<Disk> disk = Var.of(new Disk());
        AtomicBoolean fired = new AtomicBoolean(false);

        Viewable.cast(disk).onChange(From.ALL, it -> fired.set(true));

        disk.set(From.VIEW_MODEL, new Disk());

        assertThat(fired.get()).isTrue();
    }
}
