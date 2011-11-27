package com.sos.scheduler.engine.eventbus.annotated;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.eventbus.EventSourceEvent;

/** Eine {@link com.sos.scheduler.engine.eventbus.EventSubscription} für eine mit
 * @{@link com.sos.scheduler.engine.eventbus.HotEventHandler} annotierte Methode mit einem
 * Parameter für das {@link Event} und einem zweiten Parameter für {@link EventSource}
 * mit dem auslösenden Objekt. Das auslösende Objekt (zum Beispiel eine Order) ist nur in einem
 * @{@link com.sos.scheduler.engine.eventbus.HotEventHandler} gültig, weshalb es nicht ins {@link Event} aufgenommen wird. */
public class EventSourceMethodEventSubscription extends MethodEventSubscription {
    private final Class<? extends EventSource> eventSourceClass;

    EventSourceMethodEventSubscription(EventHandlerAnnotated annotatedObject, Method method) {
        super(annotatedObject, method);
        this.eventSourceClass = checkedParameterClass(method, 1, EventSource.class);
        checkMethodParameterCount(method, 1, 2);
    }

    public boolean matches(EventSourceEvent e) {
        return getEventClass().isAssignableFrom(e.getEvent().getClass()) &&
            getEventSourceClass().isAssignableFrom(e.getEventSource().getClass());
    }

    @Override protected void invokeHandler(Event event) throws InvocationTargetException, IllegalAccessException {
        EventSourceEvent e = (EventSourceEvent)event;
        getMethod().invoke(getAnnotatedObject(), e.getEvent(), e.getEventSource());
    }

    @Override public String toString() {
        return getMethod().getDeclaringClass().getSimpleName() +"."+ getMethod().getName() +
                "("+ getEventClass().getSimpleName() +","+ eventSourceClass.getSimpleName() +")";
    }

    final Class<? extends EventSource> getEventSourceClass() {
        return eventSourceClass;
    }
}
