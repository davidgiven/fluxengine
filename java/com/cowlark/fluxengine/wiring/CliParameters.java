package com.cowlark.fluxengine.wiring;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import javax.inject.Qualifier;

/**
 * Qualifier marking a {@code String[]} as the command-line parameters passed
 * to {@link com.cowlark.fluxengine.Main}.
 */
@Qualifier
@Retention(RetentionPolicy.RUNTIME)
public @interface CliParameters {
}
