package com.sos.scheduler.engine.minicom.annotations;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.Target;

import static java.lang.annotation.RetentionPolicy.RUNTIME;

/**
 * Annotated Method can be dynamically invoked via {@link com.sos.scheduler.engine.minicom.Dispatcher#invoke}.
 */
@Retention(RUNTIME)
@Target(ElementType.METHOD)
public @interface invocable {}
