package com.sos.scheduler.engine.eventbus;

import java.lang.reflect.Method;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.kernel.event.EventHandler;
import com.sos.scheduler.engine.kernel.event.EventHandlerAnnotated;

public final class AnnotatedEventSubscribers {
    private AnnotatedEventSubscribers() {}

    public static ImmutableList<EventSubscription> handlers(EventHandlerAnnotated o) {
        ImmutableList.Builder<EventSubscription> result = new ImmutableList.Builder<EventSubscription>();
        for (Method m: o.getClass().getMethods()) {
            if (m.getAnnotation(EventHandler.class) != null)
                result.add(new MethodEventSubscription(o, m));
        }
        return result.build();
    }
}
