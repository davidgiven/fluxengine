package com.cowlark.fluxengine;

import com.cowlark.fluxengine.cli.CliComponent;
import com.cowlark.fluxengine.config.ConfigComponent;
import com.cowlark.fluxengine.usb.UsbComponent;
import dagger.Component;
import javax.inject.Singleton;

@Singleton
@Component
interface FluxEngineComponent
{
    static FluxEngineComponent create()
    {
        return DaggerFluxEngineComponent.create();
    }

    ConfigComponent.Factory configComponentFactory();

    UsbComponent.Factory usbComponentFactory();

    CliComponent.Factory cliComponentFactory();
}
