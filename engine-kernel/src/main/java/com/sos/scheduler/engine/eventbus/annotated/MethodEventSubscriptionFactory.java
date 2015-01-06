package com.sos.scheduler.engine.eventbus.annotated;

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;

import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSubscription;

public interface MethodEventSubscriptionFactory {
    Class<? extends Annotation> getAnnotation();
    EventSubscription newSubscription(EventHandlerAnnotated annotatedObject, Method method);
}
