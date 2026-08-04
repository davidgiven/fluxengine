package com.cowlark.fluxengine;

import com.cowlark.fluxengine.wiring.CliParameters;
import javax.inject.Inject;
import javax.inject.Singleton;

/**
 * Concrete application class with injected dependencies (Guice-style).
 */
@Singleton
class Fluxengine
{
    private final Greeter greeter;
    private final String[] args;

    @Inject
    public Fluxengine(Greeter greeter, @CliParameters String[] args)
    {
        this.greeter = greeter;
        this.args = args;
    }

    public void start()
    {
        greeter.greet();
        System.out.println("CLI arguments: " + String.join(" ", args));
    }
}
