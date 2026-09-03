package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.testing.TestHelpers;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FluxBlockDeviceTest
{
    public static final Bytes DATA1 = Bytes.of(1, 2, 3, 4).slice(0, 512);
    public static final Bytes DATA2 = Bytes.of(4, 3, 2, 1).slice(0, 512);

    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private Path tempFlux;
    private ConfigProto configProto;

    @Before
    public void setup() throws Exception
    {
        tempFlux = Files.createTempFile("flux", ".flux");
        Files.deleteIfExists(tempFlux);

        configProto = new ConfigBuilder()
                .loadConfigFile("ibm")
                .set("drive.rotational_period_ms", "200")
                .set("verify_writes", "false")
                .withFluxSource(tempFlux.toString())
                .withFluxSink(tempFlux.toString())
                .build();
    }

    private FilesystemOperation createOperation()
    {
        FilesystemOperation fso = new FilesystemOperation(fs -> {});
        fso.setConfig(configProto);
        fso.init();
        return fso;
    }

    private void writeFluxWithBlock0(Bytes data) throws Exception
    {
        FilesystemOperation fso = createOperation();
        FluxBlockDevice device = new FluxBlockDevice(fso);
        device.putBlock(0, data);
        device.commit();
        fso.dispose();
    }

    @Test
    public void testBlockCountAndBlockSize() throws Exception
    {
        FilesystemOperation fso = createOperation();
        FluxBlockDevice device = new FluxBlockDevice(fso);

        assertThat(device.getBlockCount()).isEqualTo(2880);
        assertThat(device.getBlockSize()).isEqualTo(512);
        fso.dispose();
    }

    @Test
    public void readBlocks() throws Exception
    {
        writeFluxWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        FluxBlockDevice device = new FluxBlockDevice(fso);

        Bytes block0 = device.getBlock(0);
        assertThat(block0).isEqualTo(DATA1);
        fso.dispose();
    }

    @Test
    public void writeThenReadBlocks() throws Exception
    {
        writeFluxWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        FluxBlockDevice device = new FluxBlockDevice(fso);

        assertThat(device.getBlock(0)).isEqualTo(DATA1);
        device.putBlock(0, DATA2);
        assertThat(device.getBlock(0)).isEqualTo(DATA2);
        fso.dispose();
    }

    @Test
    public void writeThenCommit() throws Exception
    {
        writeFluxWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        FluxBlockDevice device = new FluxBlockDevice(fso);

        assertThat(device.getBlock(0)).isEqualTo(DATA1);
        device.putBlock(0, DATA2);
        assertThat(device.getBlock(0)).isEqualTo(DATA2);
        assertThat(device.needsCommit()).isTrue();
        device.commit();
        assertThat(device.getBlock(0)).isEqualTo(DATA2);
        assertThat(device.needsCommit()).isFalse();
        fso.dispose();
    }

    @Test
    public void writeThenRevert() throws Exception
    {
        writeFluxWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        FluxBlockDevice device = new FluxBlockDevice(fso);

        assertThat(device.getBlock(0)).isEqualTo(DATA1);
        device.putBlock(0, DATA2);
        assertThat(device.getBlock(0)).isEqualTo(DATA2);
        device.revert();
        assertThat(device.getBlock(0)).isEqualTo(DATA1);
        assertThat(device.needsCommit()).isFalse();
        fso.dispose();
    }

    @Test
    public void needsCommitTracksChanges() throws Exception
    {
        writeFluxWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        FluxBlockDevice device = new FluxBlockDevice(fso);

        assertThat(device.needsCommit()).isFalse();
        device.putBlock(0, DATA2);
        assertThat(device.needsCommit()).isTrue();
        device.commit();
        assertThat(device.needsCommit()).isFalse();
        device.putBlock(0, DATA1);
        assertThat(device.needsCommit()).isTrue();
        device.revert();
        assertThat(device.needsCommit()).isFalse();
        fso.dispose();
    }

    @Test
    public void commitPersistsToFlux() throws Exception
    {
        writeFluxWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        FluxBlockDevice device = new FluxBlockDevice(fso);
        device.putBlock(0, DATA2);
        device.commit();
        fso.dispose();

        // New device reading same flux file should see committed data
        FilesystemOperation fso2 = createOperation();
        FluxBlockDevice device2 = new FluxBlockDevice(fso2);
        assertThat(device2.getBlock(0)).isEqualTo(DATA2);
        fso2.dispose();
    }

    @Test
    public void revertDoesNotPersistToFlux() throws Exception
    {
        writeFluxWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        FluxBlockDevice device = new FluxBlockDevice(fso);
        device.putBlock(0, DATA2);
        device.revert();
        fso.dispose();

        FilesystemOperation fso2 = createOperation();
        FluxBlockDevice device2 = new FluxBlockDevice(fso2);
        assertThat(device2.getBlock(0)).isEqualTo(DATA1);
        fso2.dispose();
    }
}
