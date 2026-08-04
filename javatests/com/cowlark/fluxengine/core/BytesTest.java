package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;

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
        assertThrows(IndexOutOfBoundsException.class, () -> bytes.slice(1, 3));
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
    public void slicesAreReadOnlyViews()
    {
        Bytes parent = Bytes.of(10, 20, 30);
        Bytes view = parent.slice(1, 2);

        /* A slice reads the shared data. */
        assertThat(view.size()).isEqualTo(2);
        assertThat(view.get(0) & 0xff).isEqualTo(20);
        assertThat(view.get(1) & 0xff).isEqualTo(30);

        /* ...and reflects later writes to the parent. */
        parent.set(1, (byte) 99);
        assertThat(view.get(0) & 0xff).isEqualTo(99);

        /* But it cannot itself be mutated. */
        assertThrows(UnsupportedOperationException.class, () -> view.set(0, (byte) 1));
        assertThrows(UnsupportedOperationException.class, () -> view.resize(4));
    }
}
