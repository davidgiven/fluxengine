package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.imagewriter.ImageWriter;
import com.cowlark.fluxengine.testing.TestHelpers;
import com.google.common.collect.ImmutableMap;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.IOException;

@RunWith(JUnit4.class)
public class FatFilesystemTest extends GenericTreeFilesystemTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private ConfigProto configProto;
    private DiskLayout diskLayout;
    private Image image;

    @Before
    public void setup()
    {
        configProto =
                new ConfigBuilder().loadConfigFile("ibm").withImageWriter("/tmp/out.img").build();
        diskLayout = new DiskLayout(configProto);
        image = new Image();
        createTestFilesystem();
    }

    @Override
    public void createTestFilesystem()
    {
        blockDevice = new InMemoryBlockDevice(diskLayout, image);
        impl = new FatFilesystem(configProto.getFilesystem().getFatfs(), blockDevice);
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
        assertThat(image.get(0, 0, 1).data.reader().seek(3).readString(8)).isEqualTo("MSDOS5.0");
        assertThat(image.get(0, 0, 2).data
                .reader()
                .read(3)
                .toByteArray()).isEqualTo(new byte[]{(byte) 0xf8, (byte) 0xff, (byte) 0xff});
    }

    @Test
    public void getFilesystemMetadata() throws IOException
    {
        impl.create(true, "LABEL");
        ImmutableMap<String, String> metadata = impl.getFilesystemMetadata();
        assertThat(metadata).isEqualTo(ImmutableMap
                .builder()
                .put(FilesystemAttributes.VOLUME_NAME.name(), "LABEL")
                .put(FilesystemAttributes.TOTAL_BLOCKS.name(), "2880")
                .put(FilesystemAttributes.USED_BLOCKS.name(), "42")
                .put(FilesystemAttributes.BLOCK_SIZE.name(), "512")
                .build());
    }

    @Test
    public void putFilesystemMetadata() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFilesystemMetadata(ImmutableMap.of(FilesystemAttributes.VOLUME_NAME.name(), "NEWLABEL"));
        assertThat(impl.getFilesystemMetadata().get(FilesystemAttributes.VOLUME_NAME.name())).isEqualTo(
                "NEWLABEL");
    }

    @Test
    public void putFilesystemMetadata_replacesLabel() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFilesystemMetadata(ImmutableMap.of(FilesystemAttributes.VOLUME_NAME.name(), "OTHER"));
        impl.putFilesystemMetadata(ImmutableMap.of(FilesystemAttributes.VOLUME_NAME.name(), "FINAL"));
        assertThat(impl.getFilesystemMetadata().get(FilesystemAttributes.VOLUME_NAME.name())).isEqualTo(
                "FINAL");
    }

    @Test
    public void putFilesystemMetadata_emptyRemovesLabel() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFilesystemMetadata(ImmutableMap.of(FilesystemAttributes.VOLUME_NAME.name(), ""));
        assertThat(impl
                .getFilesystemMetadata()
                .get(FilesystemAttributes.VOLUME_NAME.name())).isEqualTo("");
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
                () -> impl.putFilesystemMetadata(ImmutableMap.of(
                        FilesystemAttributes.TOTAL_BLOCKS.name(),
                        "123")));
        assertThrows(
                IllegalArgumentException.class, () -> impl.putFilesystemMetadata(ImmutableMap.of(
                        FilesystemAttributes.VOLUME_NAME.name(),
                        "A",
                        FilesystemAttributes.TOTAL_BLOCKS.name(),
                        "123")));
        assertThrows(
                IllegalArgumentException.class,
                () -> impl.putFilesystemMetadata(ImmutableMap.of("unknown_key", "value")));
    }

    @Test
    public void putFilesystemMetadata_persistsAfterFlush() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFilesystemMetadata(ImmutableMap.of(FilesystemAttributes.VOLUME_NAME.name(), "PERSIST"));
        impl.flushChanges();

        FatFilesystem impl2 =
                new FatFilesystem(configProto.getFilesystem().getFatfs(), blockDevice);
        assertThat(impl2.getFilesystemMetadata().get(FilesystemAttributes.VOLUME_NAME.name())).isEqualTo(
                "PERSIST");
    }

    @Test
    public void putFilesystemMetadata_invalidLabel() throws IOException
    {
        impl.create(true, "LABEL");

        /* Label with bad character '*' should be rejected (maps to InvalidPathException via
        FR_INVALID_NAME) */
        assertThrows(
                java.nio.file.InvalidPathException.class,
                () -> impl.putFilesystemMetadata(ImmutableMap.of(
                        FilesystemAttributes.VOLUME_NAME.name(),
                        "BAD*LABEL")));
    }

    /* Do not use --- for debugging the test only */
    private void writeImage()
    {
        ImageWriter.create(configProto).writeImage(image);
    }
}
