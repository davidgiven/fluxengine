package com.cowlark.fluxengine.core;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class SupplierOfAutocloseableTest
{
    private static final class TestCloseable implements AutoCloseable
    {
        final AtomicInteger closes = new AtomicInteger();

        @Override
        public void close()
        {
            closes.incrementAndGet();
        }
    }

    @Test
    public void nullDelegateThrows()
    {
        assertThrows(IllegalArgumentException.class,
                () -> new SupplierOfAutocloseable<TestCloseable>(null));
    }

    @Test
    public void getReturnsInstance()
    {
        TestCloseable delegate = new TestCloseable();
        SupplierOfAutocloseable<TestCloseable> supplier =
                new SupplierOfAutocloseable<>(() -> delegate);

        assertThat(supplier.get()).isSameInstanceAs(delegate);
        assertThat(supplier.instance).isSameInstanceAs(delegate);
    }

    @Test
    public void getMemoizesInstance()
    {
        AtomicInteger calls = new AtomicInteger();
        SupplierOfAutocloseable<TestCloseable> supplier = new SupplierOfAutocloseable<>(() ->
        {
            calls.incrementAndGet();
            return new TestCloseable();
        });

        TestCloseable first = supplier.get();
        TestCloseable second = supplier.get();

        assertThat(calls.get()).isEqualTo(1);
        assertThat(second).isSameInstanceAs(first);
    }

    @Test
    public void getAfterCloseThrows()
    {
        SupplierOfAutocloseable<TestCloseable> supplier =
                new SupplierOfAutocloseable<>(TestCloseable::new);

        assertThrows(Exception.class, () ->
        {
            supplier.close();
            supplier.get();
        });
    }

    @Test
    public void closeClosesCreatedInstance()
    {
        TestCloseable delegate = new TestCloseable();
        SupplierOfAutocloseable<TestCloseable> supplier =
                new SupplierOfAutocloseable<>(() -> delegate);

        supplier.get();

        assertThat(delegate.closes.get()).isEqualTo(0);
        try
        {
            supplier.close();
        } catch (Exception e)
        {
            throw new AssertionError("close should not throw", e);
        }
        assertThat(delegate.closes.get()).isEqualTo(1);
    }

    @Test
    public void closeDoesNotCloseNeverCreatedInstance()
    {
        TestCloseable delegate = new TestCloseable();
        SupplierOfAutocloseable<TestCloseable> supplier =
                new SupplierOfAutocloseable<>(() -> delegate);

        try
        {
            supplier.close();
        } catch (Exception e)
        {
            throw new AssertionError("close should not throw", e);
        }

        assertThat(delegate.closes.get()).isEqualTo(0);
    }

    @Test
    public void closeIsIdempotent()
    {
        TestCloseable delegate = new TestCloseable();
        SupplierOfAutocloseable<TestCloseable> supplier =
                new SupplierOfAutocloseable<>(() -> delegate);
        supplier.get();

        try
        {
            supplier.close();
            supplier.close();
        } catch (Exception e)
        {
            throw new AssertionError("close should not throw", e);
        }

        /* The instance is only closed once. */
        assertThat(delegate.closes.get()).isEqualTo(1);
    }
}
