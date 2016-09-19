package com.sos.scheduler.engine.eventbus.annotated;

import com.sos.scheduler.engine.eventbus.EventHandler;
import java.lang.annotation.Annotation;

/** Für mit {@link EventHandler} annotierte Methoden. */
public class ColdMethodEventSubscriptionFactory extends MethodEventSubscriptionFactory {
    public static final ColdMethodEventSubscriptionFactory singleton = new ColdMethodEventSubscriptionFactory();

    private ColdMethodEventSubscriptionFactory() {}

    @Override public final Class<? extends Annotation> getAnnotation() {
        return EventHandler.class;
    }
}
