package com.sos.scheduler.engine.kernel.plugin;

import com.google.inject.Module;

import java.lang.annotation.Retention;
import java.lang.annotation.Target;

import static java.lang.annotation.ElementType.TYPE;
import static java.lang.annotation.RetentionPolicy.RUNTIME;

@Target(TYPE)
@Retention(RUNTIME)
public @interface UseGuiceModule {

    Class<? extends Module> value();
}
