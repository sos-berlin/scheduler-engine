package com.sos.scheduler.engine.eventbus.annotated;

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;

import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSubscription;

public abstract class MethodEventSubscriptionFactory {

    public abstract Class<? extends Annotation> getAnnotation();

    public final EventSubscription newSubscription(EventHandlerAnnotated annotatedObject, Method method) {
        return new SimpleMethodEventSubscription(annotatedObject, method);
    }
}
