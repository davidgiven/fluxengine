package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.testing.TestHelpers;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

@RunWith(JUnit4.class)
public class UsbFactoryTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();
    @Rule public final MockitoRule mockitoRule = MockitoJUnit.rule();

    @Test
    public void empty()
    {
    }
}
