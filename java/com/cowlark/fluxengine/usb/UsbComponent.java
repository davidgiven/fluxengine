package com.cowlark.fluxengine.usb;

import dagger.Subcomponent;
import com.cowlark.fluxengine.wiring.Scoped;

/**
 * Dagger subcomponent for USB-related accessors.
 */
@Scoped
@Subcomponent
public interface UsbComponent
{
    UsbFactory usbFactory();

    @Subcomponent.Factory
    interface Factory
    {
        UsbComponent create();
    }
}
