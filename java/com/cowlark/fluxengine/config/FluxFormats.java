package com.cowlark.fluxengine.config;

import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_A2R;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_AU;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_CWF;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DMK;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DRIVE;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_ERASE;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_FLUX;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_FLX;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_KRYOFLUX;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_NOP;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_SCP;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_TEST_PATTERN;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_VCD;

import com.google.common.collect.ImmutableList;
import lombok.Builder;
import java.util.function.BiConsumer;
import java.util.regex.Pattern;

public class FluxFormats
{
    private FluxFormats()
    {
    }

    private static BiConsumer<String, ConfigProto.Builder> UNSUPPORTED_SINK = (filename, proto) -> {
        throw new ConfigException("this flux format can't be used as a sink");
    };

    private static BiConsumer<String, ConfigProto.Builder> UNSUPPORTED_SOURCE =
            (filename, proto) -> {
                throw new ConfigException("this flux format can't be used as a source");
            };

    public static ImmutableList<FluxFormat> formats = ImmutableList.of(
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("(.*\\.flux)"))
                    .setSourceBuilder((filename, proto) -> {
                        proto.getFluxSourceBuilder().setType(FLUXTYPE_FLUX);
                        proto.getFluxSourceBuilder().getFl2Builder().setFilename(filename);
                    })
                    .setSinkBuilder((filename, proto) -> {
                        proto.getFluxSinkBuilder().setType(FLUXTYPE_FLUX);
                        proto.getFluxSinkBuilder().getFl2Builder().setFilename(filename);
                    })
                    .build(),
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("(.*\\.scp)"))
                    .setSourceBuilder((filename, proto) -> {
                        proto.getFluxSourceBuilder().setType(FLUXTYPE_SCP);
                        proto.getFluxSourceBuilder().getScpBuilder().setFilename(filename);
                    })
                    .setSinkBuilder((filename, proto) -> {
                        proto.getFluxSinkBuilder().setType(FLUXTYPE_SCP);
                        proto.getFluxSinkBuilder().getScpBuilder().setFilename(filename);
                    })
                    .build(),
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("(.*\\.a2r)"))
                    .setSourceBuilder((filename, proto) -> {
                        proto.getFluxSourceBuilder().setType(FLUXTYPE_A2R);
                        proto.getFluxSourceBuilder().getA2RBuilder().setFilename(filename);
                    })
                    .setSinkBuilder((filename, proto) -> {
                        proto.getFluxSinkBuilder().setType(FLUXTYPE_A2R);
                        proto.getFluxSinkBuilder().getA2RBuilder().setFilename(filename);
                    })
                    .build(),
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("(.*\\.cwf)"))
                    .setSourceBuilder((filename, proto) -> {
                        proto.getFluxSourceBuilder().setType(FLUXTYPE_CWF);
                        proto.getFluxSourceBuilder().getCwfBuilder().setFilename(filename);
                    })
                    .setSinkBuilder(UNSUPPORTED_SINK)
                    .build(),
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("dmk:(.*)"))
                    .setSourceBuilder((directory, proto) -> {
                        proto.getFluxSourceBuilder().setType(FLUXTYPE_DMK);
                        proto.getFluxSourceBuilder().getDmkBuilder().setDirectory(directory);
                    })
                    .setSinkBuilder(UNSUPPORTED_SINK)
                    .build(),
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("flx:(.*)"))
                    .setSourceBuilder((directory, proto) -> {
                        proto.getFluxSourceBuilder().setType(FLUXTYPE_FLX);
                        proto.getFluxSourceBuilder().getFlxBuilder().setDirectory(directory);
                    })
                    .setSinkBuilder(UNSUPPORTED_SINK)
                    .build(),
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("kryoflux:(.*)"))
                    .setSourceBuilder((directory, proto) -> {
                        proto.getFluxSourceBuilder().setType(FLUXTYPE_KRYOFLUX);
                        proto.getFluxSourceBuilder().getKryofluxBuilder().setDirectory(directory);
                    })
                    .setSinkBuilder(UNSUPPORTED_SINK)
                    .build(),
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("drive:(.*)"))
                    .setSourceBuilder((filename, proto) -> {
                        proto.getFluxSourceBuilder().setType(FLUXTYPE_DRIVE);
                        proto.getDriveBuilder().setDrive(Integer.parseInt(filename));
                    })
                    .setSinkBuilder((filename, proto) -> {
                        proto.getFluxSinkBuilder().setType(FLUXTYPE_DRIVE);
                        proto.getDriveBuilder().setDrive(Integer.parseInt(filename));
                    })
                    .build(),
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("erase:(.*)"))
                    .setSourceBuilder(UNSUPPORTED_SOURCE)
                    .setSinkBuilder((filename, proto) -> {
                        proto.getFluxSinkBuilder().setType(FLUXTYPE_ERASE);
                    })
                    .build(),
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("testpattern:(.*)"))
                    .setSourceBuilder(UNSUPPORTED_SOURCE)
                    .setSinkBuilder((filename, proto) -> {
                        proto.getFluxSinkBuilder().setType(FLUXTYPE_TEST_PATTERN);
                    })
                    .build(),
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("nop:(.*)"))
                    .setSourceBuilder((filename, proto) -> {
                        proto.getFluxSourceBuilder().setType(FLUXTYPE_NOP);
                    })
                    .setSinkBuilder((filename, proto) -> {
                        proto.getFluxSinkBuilder().setType(FLUXTYPE_NOP);
                    })
                    .build(),
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("vcd:(.*)"))
                    .setSourceBuilder(UNSUPPORTED_SOURCE)
                    .setSinkBuilder((filename, proto) -> {
                        proto.getFluxSinkBuilder().setType(FLUXTYPE_VCD);
                        proto.getFluxSinkBuilder().getVcdBuilder().setDirectory(filename);
                    })
                    .build(),
            FluxFormat
                    .builder()
                    .setMatcher(Pattern.compile("au:(.*)"))
                    .setSourceBuilder(UNSUPPORTED_SOURCE)
                    .setSinkBuilder((filename, proto) -> {
                        proto.getFluxSinkBuilder().setType(FLUXTYPE_AU);
                        proto.getFluxSinkBuilder().getAuBuilder().setDirectory(filename);
                    })
                    .build());

    @Builder(setterPrefix = "set")
    public record FluxFormat(Pattern matcher, BiConsumer<String, ConfigProto.Builder> sourceBuilder,
                             BiConsumer<String, ConfigProto.Builder> sinkBuilder)
    {
    }

}
