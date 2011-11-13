package com.sos.scheduler.engine.eventbus;

import java.lang.reflect.Method;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.kernel.event.EventHandler;
import com.sos.scheduler.engine.kernel.event.EventHandlerAnnotated;

public final class AnnotatedEventSubscribers {
    private AnnotatedEventSubscribers() {}

    public static ImmutableList<EventSubscriber2> handlers(EventHandlerAnnotated o) {
        ImmutableList.Builder<EventSubscriber2> result = new ImmutableList.Builder<EventSubscriber2>();
        for (Method m: o.getClass().getMethods()) {
            if (m.getAnnotation(EventHandler.class) != null)
                result.add(new MethodEventSubscriber(o, m));
        }
        return result.build();
    }
}
