package com.cowlark.fluxengine;

import javax.inject.Inject;

/**
 * Simple Greeter service provided via Dagger.
 */
class Greeter
{
    @Inject
    public Greeter()
    {
    }

    public void greet()
    {
        System.out.println("FluxEngine starting up...");
        System.out.println("Hello from FluxEngine Java main");
    }
}
