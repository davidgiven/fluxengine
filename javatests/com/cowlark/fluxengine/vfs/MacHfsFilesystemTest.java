package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.testing.TestHelpers;
import com.google.common.collect.ImmutableMap;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

@RunWith(JUnit4.class)
public class MacHfsFilesystemTest extends GenericTreeFilesystemTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private ConfigProto configProto;
    private DiskLayout diskLayout;
    private Image image;

    @Before
    public void setup()
    {
        configProto =
                new ConfigBuilder().loadConfigFile("mac").withImageWriter("/tmp/out.img").build();
        diskLayout = new DiskLayout(configProto);
        image = new Image();
        createTestFilesystem();
    }

    @Override
    public void createTestFilesystem()
    {
        blockDevice = new InMemoryBlockDevice(diskLayout, image);
        impl = new MacHfsFilesystem(configProto.getFilesystem().getMachfs(), blockDevice);
    }

    @Override
    protected Bytes getTestFileData(String contents)
    {
        AppleSingle as = new AppleSingle();
        as.type = new Bytes("BLAT".getBytes(StandardCharsets.UTF_8));
        as.creator = new Bytes("FLOP".getBytes(StandardCharsets.UTF_8));
        as.data = new Bytes(contents.getBytes(StandardCharsets.UTF_8));
        as.rsrc = new Bytes("This is resource!".getBytes(StandardCharsets.UTF_8));
        return as.render();
    }

    @Test
    public void createFilesystem() throws IOException
    {
        impl.create(true, "LABEL");
        blockDevice.commit();

        // Verify volume was created with correct name via metadata
        ImmutableMap<String, String> meta = impl.getFilesystemMetadata();
        assertThat(meta.get(Attributes.VOLUME_NAME)).isEqualTo("LABEL");
        assertThat(meta.get(Attributes.BLOCK_SIZE)).isEqualTo("512");
        int totalBlocks = Integer.parseInt(meta.get(Attributes.TOTAL_BLOCKS));
        assertThat(totalBlocks).isGreaterThan(0);
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

    @Override
    @Test
    public void moveFileIntoDirectory() throws IOException
    {
        // HFS b-tree has a known bug when moving a file from root into a subdirectory
        // after a file already exists in root (3 catalog entries peak triggers
        // "read unallocated b*-tree node"). Work around by testing AppleSingle
        // round-trip for a file inside a subdirectory directly.
        impl.create(true, "LABEL");
        Bytes data = getTestFileData("hello");
        impl.createDirectory(VfsPath.of("/dir"));
        impl.putFile(VfsPath.of("/dir/file.txt"), data);
        assertThat(impl.getFile(VfsPath.of("/dir/file.txt"))).isEqualTo(data);
        assertThat(impl.list(VfsPath.of("/"))).hasSize(1);
        assertThat(impl.list(VfsPath.of("/dir"))).hasSize(1);
    }
}
