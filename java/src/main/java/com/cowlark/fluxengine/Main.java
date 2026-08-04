package com.cowlark.fluxengine;

import javax.inject.Inject;
import javax.inject.Singleton;

import dagger.Component;
import dagger.Module;
import dagger.Provides;

/**
 * Simple Greeter service provided via Dagger.
 */
class Greeter {
    @Inject
    public Greeter() {
    }

    public void greet() {
        System.out.println("FluxEngine starting up...");
        System.out.println("Hello from FluxEngine Java main");
    }
}

/**
 * Application module that provides application-scoped dependencies.
 */
@Module
class FluxengineModule {
    @Provides
    @Singleton
    Greeter provideGreeter() {
        return new Greeter();
    }
}

/**
 * Concrete application class with injected dependencies (Guice-style).
 */
@Singleton
class Fluxengine {
    private final Greeter greeter;

    @Inject
    public Fluxengine(Greeter greeter) {
        this.greeter = greeter;
    }

    public void start() {
        greeter.greet();
    }
}

/**
 * Component that exposes the concrete Fluxengine type.
 * Dagger will generate DaggerFluxengineComponent when annotation processing runs.
 */
@Singleton
@Component(modules = FluxengineModule.class)
interface FluxengineComponent {
    Fluxengine fluxengine();

    // Convenience factory that delegates to the generated implementation
    static FluxengineComponent create() {
        return DaggerFluxengineComponent.create();
    }
}

/**
 * JVM entrypoint that obtains the Fluxengine instance from the component and runs it.
 */
public class Main {
    public static void main(String[] args) {
        FluxengineComponent component = FluxengineComponent.create();
        component.fluxengine().start();
    }
}
