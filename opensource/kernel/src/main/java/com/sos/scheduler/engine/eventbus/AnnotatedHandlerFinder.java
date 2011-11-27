package com.sos.scheduler.engine.eventbus;

import java.lang.reflect.Method;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.eventbus.annotated.MethodEventSubscriptionFactory;

final class AnnotatedHandlerFinder {
    private final MethodEventSubscriptionFactory factory;

    AnnotatedHandlerFinder(MethodEventSubscriptionFactory factory) {
        this.factory = factory;
    }

    ImmutableList<EventSubscription> handlers(EventHandlerAnnotated o) {
        ImmutableList.Builder<EventSubscription> result = new ImmutableList.Builder<EventSubscription>();
        for (Method m: o.getClass().getMethods()) {
            if (methodIsAnnotated(m))
                result.add(factory.newSubscription(o, m));
        }
        return result.build();
    }

    private boolean methodIsAnnotated(Method m) {
        return m.getAnnotation(factory.getAnnotation()) != null;
    }
}
