package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;

import com.google.common.collect.ImmutableList;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class BytesTest
{
    @Test
    public void boundsChecking()
    {
        Bytes bytes = Bytes.of(1, 2, 3);

        assertThrows(IndexOutOfBoundsException.class, () -> bytes.get(-1));
        assertThrows(IndexOutOfBoundsException.class, () -> bytes.get(3));
        assertThrows(IndexOutOfBoundsException.class, () -> bytes.set(-1, (byte) 0));
        assertThrows(IndexOutOfBoundsException.class, () -> bytes.set(3, (byte) 0));
        assertThrows(IndexOutOfBoundsException.class, () -> bytes.slice(-1, 1));
        assertThrows(IndexOutOfBoundsException.class, () -> bytes.slice(0, -1));
    }

    @Test
    public void sliceZeroPads()
    {
        Bytes bytes = Bytes.of(1, 2, 3);

        assertThat(bytes.slice(1, 3).toArray()).isEqualTo(new byte[] {2, 3, 0});
        assertThat(bytes.slice(5, 2).toArray()).isEqualTo(new byte[] {0, 0});
        assertThat(bytes.slice(3, 2).toArray()).isEqualTo(new byte[] {0, 0});
        assertThat(bytes.slice(2).toArray()).isEqualTo(new byte[] {3});
        assertThat(bytes.slice(5).isEmpty()).isTrue();
    }

    @Test
    public void clear()
    {
        Bytes bytes = Bytes.of(1, 2, 3);
        bytes.clear();
        assertThat(bytes.size()).isEqualTo(0);
        assertThat(bytes.isEmpty()).isTrue();
    }

    @Test
    public void split()
    {
        Bytes bytes = Bytes.of(1, 2, 0, 3, 4, 0, 5);
        ImmutableList<Bytes> pieces = bytes.split(0);

        assertThat(pieces).hasSize(3);
        assertThat(pieces.get(0).toArray()).isEqualTo(new byte[] {1, 2});
        assertThat(pieces.get(1).toArray()).isEqualTo(new byte[] {3, 4});
        assertThat(pieces.get(2).toArray()).isEqualTo(new byte[] {5});

        /* Consecutive separators and a trailing separator yield empty pieces. */
        ImmutableList<Bytes> empties = Bytes.of(0, 1, 0, 0).split(0);
        assertThat(empties).hasSize(4);
        assertThat(empties.get(0).isEmpty()).isTrue();
        assertThat(empties.get(1).toArray()).isEqualTo(new byte[] {1});
        assertThat(empties.get(2).isEmpty()).isTrue();
        assertThat(empties.get(3).isEmpty()).isTrue();
    }

    @Test
    public void swab()
    {
        assertThat(Bytes.of(1, 2, 3, 4).swab().toArray())
            .isEqualTo(new byte[] {2, 1, 4, 3});

        /* Odd length pads the trailing byte with a zero. */
        assertThat(Bytes.of(1, 2, 3).swab().toArray())
            .isEqualTo(new byte[] {2, 1, 0, 3});
    }

    @Test
    public void resizing()
    {
        Bytes bytes = Bytes.of(1, 2, 3);

        bytes.resize(5);
        assertThat(bytes.size()).isEqualTo(5);
        assertThat(bytes.get(0) & 0xff).isEqualTo(1);
        assertThat(bytes.get(2) & 0xff).isEqualTo(3);
        assertThat(bytes.get(3) & 0xff).isEqualTo(0);
        assertThat(bytes.get(4) & 0xff).isEqualTo(0);

        bytes.resize(1);
        assertThat(bytes.size()).isEqualTo(1);
        assertThat(bytes.get(0) & 0xff).isEqualTo(1);

        bytes.resize(0);
        assertThat(bytes.size()).isEqualTo(0);
        assertThat(bytes.isEmpty()).isTrue();
    }

    @Test
    public void slicesShareStorage()
    {
        Bytes parent = Bytes.of(10, 20, 30);
        Bytes view = parent.slice(1, 2);

        assertThat(view.size()).isEqualTo(2);
        assertThat(view.get(0) & 0xff).isEqualTo(20);
        assertThat(view.get(1) & 0xff).isEqualTo(30);
        assertThat(parent.refcount()).isEqualTo(2);
    }

    @Test
    public void copyOnWriteOnlyWhenShared()
    {
        Bytes lone = Bytes.of(1, 2, 3);
        lone.set(0, (byte) 9);
        lone.resize(4);
        assertThat(lone.refcount()).isEqualTo(1);
        assertThat(lone.get(0) & 0xff).isEqualTo(9);

        /* Shared bytes: a write on the parent detaches it, leaving the view
         * unchanged. */
        Bytes parent = Bytes.of(1, 2, 3);
        Bytes view = parent.slice(0, 3);
        parent.set(0, (byte) 9);
        assertThat(parent.get(0) & 0xff).isEqualTo(9);
        assertThat(view.get(0) & 0xff).isEqualTo(1);
        assertThat(parent.refcount()).isEqualTo(1);

        /* And a write on the view detaches it, leaving the parent unchanged. */
        Bytes parent2 = Bytes.of(1, 2, 3);
        Bytes view2 = parent2.slice(0, 3);
        view2.set(2, (byte) 7);
        assertThat(view2.get(2) & 0xff).isEqualTo(7);
        assertThat(parent2.get(2) & 0xff).isEqualTo(3);
        assertThat(view2.refcount()).isEqualTo(1);

        /* Resizing a shared window detaches it too. */
        Bytes parent3 = Bytes.of(1, 2, 3);
        Bytes view3 = parent3.slice(0, 3);
        parent3.resize(5);
        assertThat(parent3.size()).isEqualTo(5);
        assertThat(view3.size()).isEqualTo(3);
        assertThat(view3.get(0) & 0xff).isEqualTo(1);
    }

    @Test
    public void iteration()
    {
        Bytes bytes = Bytes.of(1, 2, 3);
        int expected = 1;
        for (Byte b : bytes)
        {
            assertThat(b.intValue()).isEqualTo(expected++);
        }
        assertThat(expected).isEqualTo(4);
    }
}
