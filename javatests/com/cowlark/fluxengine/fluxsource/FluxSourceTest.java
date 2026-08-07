package com.cowlark.fluxengine.fluxsource;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.FluxSourceSinkType;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FluxSourceTest
{
    @Test
    public void createUnknownTypeReturnsNull()
    {
        FluxSourceProto config = FluxSourceProto.newBuilder()
                .setType(FluxSourceSinkType.FLUXTYPE_NOT_SET)
                .build();

        assertThat(FluxSource.create(config)).isNull();
    }

    @Test
    public void createUnportedTypeThrows()
    {
        FluxSourceProto config = FluxSourceProto.newBuilder()
                .setType(FluxSourceSinkType.FLUXTYPE_DRIVE)
                .build();

        assertThrows(FluxEngineException.class, () -> FluxSource.create(config));
    }

    @Test
    public void createEraseFluxSource()
    {
        FluxSourceProto config = FluxSourceProto.newBuilder()
                .setType(FluxSourceSinkType.FLUXTYPE_ERASE)
                .build();

        FluxSource source = FluxSource.create(config);

        assertThat(source).isInstanceOf(EraseFluxSource.class);
        assertThat(source.readFlux(0, 0).next()).isNull();
        assertThat(source.getExtraConfig().getDrive().getTracks()).isEqualTo("c0-255h0-1");
    }

    @Test
    public void trivialFluxSourceIteratorYieldsOneMap()
    {
        TrivialFluxSource source = new TrivialFluxSource()
        {
            @Override
            public Fluxmap readSingleFlux(int cylinder, int head)
            {
                return new Fluxmap();
            }
        };

        FluxSourceIterator iterator = source.readFlux(0, 0);

        assertThat(iterator.hasNext()).isTrue();
        iterator.next();
        assertThat(iterator.hasNext()).isFalse();
    }

    @Test
    public void emptyIterator()
    {
        FluxSourceIterator iterator = new EmptyFluxSourceIterator();

        assertThat(iterator.hasNext()).isFalse();
        assertThrows(FluxEngineException.class, iterator::next);
    }
}
