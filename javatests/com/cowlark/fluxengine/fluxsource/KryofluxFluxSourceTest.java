package com.cowlark.fluxengine.fluxsource;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.stream.Collectors;

@RunWith(JUnit4.class)
public class KryofluxFluxSourceTest
{
    @org.junit.Rule public final org.junit.rules.TestRule loggerRule =
            com.cowlark.fluxengine.testing.TestHelpers.loggerRule();

    @Rule public TemporaryFolder folder = new TemporaryFolder();

    @Test
    public void readsSingleFluxFromDirectory() throws Exception
    {
        Path dir = folder.getRoot().toPath();
        Files.write(dir.resolve("track80.0.raw"), new byte[]{0x20});
        Files.write(dir.resolve("track81.1.raw"), new byte[]{0x20});

        KryofluxFluxSourceProto config =
                KryofluxFluxSourceProto.newBuilder().setDirectory(dir.toString()).build();
        KryofluxFluxSource source = new KryofluxFluxSource(config);

        assertThat(source
                .readSingleFlux(FluxReadParameters.builder().setCylinder(80).setHead(0).build())
                .rawBytes()
                .toByteArray()).isEqualTo(new byte[]{(byte) 0x8f});

        ConfigBuilder configBuilder = new ConfigBuilder().set("usb.serial", "test-serial");
        source.adjustConfig(configBuilder);
        String tracks = configBuilder.build().getDrive().getTracks();
        String sorted = Arrays.stream(tracks.split(" ")).sorted().collect(Collectors.joining(" "));
        assertThat(sorted).isEqualTo("c80h0 c81h1");
    }
}