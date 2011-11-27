package com.sos.scheduler.engine.eventbus.annotated;

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;

import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSubscription;
import com.sos.scheduler.engine.eventbus.HotEventHandler;

/** Für mit {@link HotEventHandler} annotierte Methoden, die im zweiten Parameter eine
 * {@link com.sos.scheduler.engine.eventbus.EventSource} bekommen können. */
public final class HotMethodEventSubscriptionFactory implements MethodEventSubscriptionFactory {
    public static final HotMethodEventSubscriptionFactory singleton = new HotMethodEventSubscriptionFactory();

    private HotMethodEventSubscriptionFactory() {}

    @Override public Class<? extends Annotation> getAnnotation() {
        return HotEventHandler.class;
    }

    @Override public EventSubscription newSubscription(EventHandlerAnnotated annotatedObject, Method method) {
        return method.getParameterTypes().length == 1? new SimpleMethodEventSubscription(annotatedObject, method)
                : new EventSourceMethodEventSubscription(annotatedObject, method);
    }
}
