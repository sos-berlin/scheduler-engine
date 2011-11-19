package com.sos.scheduler.engine.eventbus;

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.util.Collection;

import com.google.common.collect.ImmutableList;

public final class AnnotatedHandlerFinder {
    private final Collection<Class<? extends Annotation>> annotations;

    AnnotatedHandlerFinder(Collection<Class<? extends Annotation>> annotations) {
        this.annotations = annotations;
    }

    public ImmutableList<EventSubscription> handlers(EventHandlerAnnotated o) {
        ImmutableList.Builder<EventSubscription> result = new ImmutableList.Builder<EventSubscription>();
        for (Method m: o.getClass().getMethods()) {
            if (methodIsAnnotated(m))
                result.add(new MethodEventSubscription(o, m));
        }
        return result.build();
    }

    private boolean methodIsAnnotated(Method m) {
        for (Class<? extends Annotation> a: annotations)
            if (m.getAnnotation(a) != null) return true;
        return false;
    }
}
