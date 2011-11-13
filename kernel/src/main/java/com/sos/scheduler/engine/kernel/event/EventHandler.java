package com.sos.scheduler.engine.kernel.event;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/** Annotiert eine Methode mit einem Parameter einer Unterklasse von {@link Event).
 * Rückgabe ist void.
 * Die Klasse, die die Methode defineirt, muss Marker-Interface {@link EventHandlerAnnotated} haben. */
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface EventHandler {}
