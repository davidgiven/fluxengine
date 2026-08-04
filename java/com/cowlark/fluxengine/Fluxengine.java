package com.cowlark.fluxengine;

import javax.inject.Inject;
import javax.inject.Singleton;

/**
 * Concrete application class with injected dependencies (Guice-style).
 */
@Singleton
class Fluxengine
{
    private final Greeter greeter;

    @Inject
    public Fluxengine(Greeter greeter)
    {
        this.greeter = greeter;
    }

    public void start()
    {
        greeter.greet();
    }
}
