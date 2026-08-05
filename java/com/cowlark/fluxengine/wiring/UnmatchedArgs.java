package com.cowlark.fluxengine.wiring;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import javax.inject.Qualifier;

/**
 * Qualifier for the unmatched command-line arguments.
 */
@Qualifier
@Retention(RetentionPolicy.RUNTIME)
public @interface UnmatchedArgs
{
}
