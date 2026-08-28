package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;

import com.google.common.collect.ImmutableList;
import io.kaitai.formats.AppleSingleDouble;
import io.kaitai.struct.ByteBufferKaitaiStream;
import io.kaitai.struct.KaitaiStruct;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.function.Consumer;
import java.util.function.Supplier;

@RunWith(JUnit4.class)
public class AppleSingleDoubleTest
{
    @Test
    public void testCreation()
    {
        AppleSingleDouble.Entry entry = new AppleSingleDouble.Entry();
        entry.setType(AppleSingleDouble.Entry.Types.DATA_FORK);
        entry.setBody("Hello, world!".getBytes(StandardCharsets.UTF_8));
        entry.setLenBody(13);
        entry._check();

        AppleSingleDouble data = new AppleSingleDouble();
        data.setMagic(AppleSingleDouble.FileType.APPLE_DOUBLE);
        data.setVersion(AppleSingleDouble.FileVersion.VERSION_2);
        data.setReserved(new byte[16]);
        addItem(
                data,
                entry,
                data::entries,
                data::setEntries,
                data::setNumEntries,
                data::_root,
                entry::set_root,
                entry::set_parent);
        data._check();

        ByteBuffer bb = ByteBuffer.allocate(16384);
        ByteBufferKaitaiStream bbks = new ByteBufferKaitaiStream(bb);
        data._write(bbks);
        assertThat(bb.position()).isEqualTo(38);
    }

    private static <C extends KaitaiStruct.ReadWrite, T extends KaitaiStruct.ReadWrite,
            R extends KaitaiStruct.ReadWrite> C addItem(
            C container,
            T value,
            Supplier<List<T>> getItems,
            Consumer<List<T>> setItems,
            Consumer<Integer> setLen,
            Supplier<R> getRoot,
            Consumer<R> setRoot,
            Consumer<C> setParent)
    {
        setRoot.accept(getRoot.get());
        setParent.accept(container);
        value._check();

        List<T> items = getItems.get();
        ImmutableList newItems = (items == null) ?
                ImmutableList.of(value) :
                ImmutableList.builder().addAll(items).add(value).build();
        setItems.accept(newItems);
        setLen.accept(newItems.size());
        return container;
    }
}
