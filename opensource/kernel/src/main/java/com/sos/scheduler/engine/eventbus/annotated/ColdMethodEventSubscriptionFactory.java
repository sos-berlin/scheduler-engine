package com.sos.scheduler.engine.eventbus.annotated;

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSubscription;

/** Für mit {@link EventHandler} annotierte Methoden. */
public class ColdMethodEventSubscriptionFactory implements MethodEventSubscriptionFactory {
    public static final ColdMethodEventSubscriptionFactory singleton = new ColdMethodEventSubscriptionFactory();

    private ColdMethodEventSubscriptionFactory() {}

    @Override public Class<? extends Annotation> getAnnotation() {
        return EventHandler.class;
    }

    @Override public EventSubscription newSubscription(EventHandlerAnnotated annotatedObject, Method method) {
        return new SimpleMethodEventSubscription(annotatedObject, method);
    }
}
