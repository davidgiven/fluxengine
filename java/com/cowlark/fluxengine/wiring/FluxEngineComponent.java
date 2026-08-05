package com.cowlark.fluxengine.wiring;

import com.cowlark.fluxengine.config.ConfigComponent;
import com.cowlark.fluxengine.usb.UsbComponent;
import com.google.common.collect.ImmutableList;
import dagger.BindsInstance;
import dagger.Component;
import java.util.List;
import javax.inject.Singleton;

@Singleton
@Component
public interface FluxEngineComponent
{
    static FluxEngineComponent create(List<String> args)
    {
        return DaggerFluxEngineComponent.builder()
            .unmatchedArgs(ImmutableList.copyOf(args))
            .build();
    }

    @Component.Builder
    interface Builder
    {
        @BindsInstance
        Builder unmatchedArgs(@UnmatchedArgs ImmutableList<String> args);

        FluxEngineComponent build();
    }

    ConfigComponent.Factory configComponentFactory();

    UsbComponent.Factory usbComponentFactory();
}
