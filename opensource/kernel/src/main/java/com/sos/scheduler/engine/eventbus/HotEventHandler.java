package com.sos.scheduler.engine.eventbus;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/** Annotiert eine Methode mit einem Parameter einer Unterklasse von {@link Event ).
 * Rückgabe ist void.
 * Die Klasse, die die Methode definiert, muss Marker-Interface {@link com.sos.scheduler.engine.eventbus.EventHandlerAnnotated } haben. */
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface HotEventHandler {}
