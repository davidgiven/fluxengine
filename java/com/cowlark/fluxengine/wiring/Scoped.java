package com.cowlark.fluxengine.wiring;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import javax.inject.Scope;

/**
 * Dagger scope for subcomponents.
 */
@Scope
@Retention(RetentionPolicy.RUNTIME)
public @interface Scoped
{
}
