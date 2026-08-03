package com.cowlark.fluxengine

import javax.inject.Inject
import javax.inject.Singleton
import dagger.Component
import dagger.Module
import dagger.Provides

/**
 * Simple Greeter service provided via Dagger.
 */
class Greeter @Inject constructor() {
    fun greet() {
        println("FluxEngine starting up...")
        println("Hello from FluxEngine Kotlin main")
    }
}

/**
 * Application module that provides application-scoped dependencies.
 */
@Module
class FluxengineModule {
    @Provides
    @Singleton
    fun provideGreeter(): Greeter = Greeter()
}

/**
 * Application component. Dagger will generate DaggerFluxengine when annotation
 * processing runs (requires dagger-compiler and Kotlin KAPT or equivalent to be enabled).
 *
 * Declared as an abstract class to allow adding convenience methods and a companion
 * factory that returns the generated implementation.
 */
@Singleton
@Component(modules = [FluxengineModule::class])
abstract class Fluxengine {
    // Abstract provider method implemented in generated subclass (DaggerFluxengine)
    abstract fun greeter(): Greeter

    // Concrete helper method available to callers
    fun start() {
        greeter().greet()
    }

    companion object {
        @JvmStatic
        fun create(): Fluxengine = DaggerFluxengine.create()
    }
}

/**
 * Entry point for the JVM application as a static main method on a class.
 */
class Main {
    companion object {
        @JvmStatic
        fun main(args: Array<String>) {
            val component = Fluxengine.create()
            component.start()
        }
    }
}
