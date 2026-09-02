package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.testing.TestHelpers;
import com.google.common.collect.ImmutableMap;
import java.io.IOException;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class AmigaFilesystemTest extends GenericTreeFilesystemTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private ConfigProto configProto;
    private DiskLayout diskLayout;
    private Image image;

    @Before
    public void setup()
    {
        configProto =
                new ConfigBuilder().loadConfigFile("amiga").withImageWriter("/tmp/out.img").build();
        diskLayout = new DiskLayout(configProto);
        image = new Image();
        createTestFilesystem();
    }

    @Override
    public void createTestFilesystem()
    {
        blockDevice = new InMemoryBlockDevice(diskLayout, image);
        impl = new AmigaFilesystem(configProto.getFilesystem().getAmigaffs(), blockDevice);
    }

    @Override
    protected Bytes getTestFileData(String contents)
    {
        return new Bytes(contents);
    }

    @Test
    public void createFilesystem() throws IOException
    {
        impl.create(true, "LABEL");
        blockDevice.commit();

        /*
         * ADF disk layout for 880KB (1760 sectors, 0-based):
         *   Boot block:  logical sector 0  → CHS(0, 0, 0)  — "DOS" magic at offset 0
         *   Root block:  logical sector 880 → CHS(40, 0, 0) — T_HEADER=2 at offset 0
         */
        Bytes sec0 = image.get(0, 0, 0).data;
        assertThat(sec0.reader().seek(0).readString(3)).isEqualTo("DOS");

        /* Root block at CHS(40, 0, 0) = logical sector 880 for 1760-sector ADF */
        Bytes rootBlock = image.get(40, 0, 0).data;
        int blockType = rootBlock.reader().seek(0).readBe32();
        assertThat(blockType).isEqualTo(2); /* T_HEADER = 2 */
    }

    @Test
    public void getFilesystemMetadata() throws IOException
    {
        impl.create(true, "LABEL");
        ImmutableMap<String, String> metadata = impl.getFilesystemMetadata();
        assertThat(metadata.get(Attributes.VOLUME_NAME)).isEqualTo("LABEL");
        assertThat(metadata.get(Attributes.BLOCK_SIZE)).isEqualTo("512");
        int totalBlocks = Integer.parseInt(metadata.get(Attributes.TOTAL_BLOCKS));
        assertThat(totalBlocks).isGreaterThan(0);
        int usedBlocks = Integer.parseInt(metadata.get(Attributes.USED_BLOCKS));
        assertThat(usedBlocks).isAtLeast(0);
    }

    @Test
    public void putFilesystemMetadata() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFilesystemMetadata(ImmutableMap.of(Attributes.VOLUME_NAME, "NEWLABEL"));
        assertThat(impl.getFilesystemMetadata().get(Attributes.VOLUME_NAME)).isEqualTo("NEWLABEL");
    }

    @Test
    public void putFilesystemMetadata_replacesLabel() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFilesystemMetadata(ImmutableMap.of(Attributes.VOLUME_NAME, "OTHER"));
        impl.putFilesystemMetadata(ImmutableMap.of(Attributes.VOLUME_NAME, "FINAL"));
        assertThat(impl.getFilesystemMetadata().get(Attributes.VOLUME_NAME)).isEqualTo("FINAL");
    }

    @Test
    public void putFilesystemMetadata_emptyRemovesLabel() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFilesystemMetadata(ImmutableMap.of(Attributes.VOLUME_NAME, ""));
        assertThat(impl.getFilesystemMetadata().get(Attributes.VOLUME_NAME)).isEqualTo("");
    }

    @Test
    public void putFilesystemMetadata_invalidKeys() throws IOException
    {
        impl.create(true, "LABEL");
        assertThrows(
                IllegalArgumentException.class,
                () -> impl.putFilesystemMetadata(ImmutableMap.of()));
        assertThrows(
                IllegalArgumentException.class,
                () -> impl.putFilesystemMetadata(ImmutableMap.of(Attributes.TOTAL_BLOCKS, "123")));
        assertThrows(
                IllegalArgumentException.class,
                () -> impl.putFilesystemMetadata(ImmutableMap.of(
                        Attributes.VOLUME_NAME,
                        "A",
                        Attributes.TOTAL_BLOCKS,
                        "123")));
        assertThrows(
                IllegalArgumentException.class,
                () -> impl.putFilesystemMetadata(ImmutableMap.of("unknown_key", "value")));
    }

    @Test
    public void putFilesystemMetadata_persistsAfterFlush() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFilesystemMetadata(ImmutableMap.of(Attributes.VOLUME_NAME, "PERSIST"));
        impl.flushChanges();

        AmigaFilesystem impl2 =
                new AmigaFilesystem(configProto.getFilesystem().getAmigaffs(), blockDevice);
        assertThat(impl2.getFilesystemMetadata().get(Attributes.VOLUME_NAME)).isEqualTo("PERSIST");
    }
}
