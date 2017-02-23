package com.sos.scheduler.engine.eventbus;

import com.sos.jobscheduler.data.event.KeyedEvent;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/** Annotiert eine Methode mit einem Parameter einer Unterklasse von {@link KeyedEvent ).
 * Rückgabe ist void.
 * Die Klasse, die die Methode definiert, muss Marker-Interface {@link EventHandlerAnnotated} haben. */
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface HotEventHandler {}
