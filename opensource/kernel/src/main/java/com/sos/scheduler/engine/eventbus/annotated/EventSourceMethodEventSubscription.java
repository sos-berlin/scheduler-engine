package com.sos.scheduler.engine.eventbus.annotated;

import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.eventbus.EventSourceEvent;
import com.sos.scheduler.engine.data.event.Event;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/** Eine {@link com.sos.scheduler.engine.eventbus.EventSubscription} für eine mit
 * @{@link com.sos.scheduler.engine.eventbus.HotEventHandler} annotierte Methode mit einem
 * Parameter für das {@link Event} und einem zweiten Parameter für {@link EventSource}
 * mit dem auslösenden Objekt. Das auslösende Objekt (zum Beispiel eine Order) ist nur in einem
 * @{@code HotEventHandler} gültig, weshalb es nicht ins {@code Event} aufgenommen wird. */
public class EventSourceMethodEventSubscription extends MethodEventSubscription {
    private final Class<? extends EventSource> eventSourceClass;

    EventSourceMethodEventSubscription(EventHandlerAnnotated annotatedObject, Method method) {
        super(annotatedObject, method);
        this.eventSourceClass = checkedParameterClass(method, 1, EventSource.class);
        checkMethodParameterCount(method, 1, 2);
    }

    public final boolean eventSourceMatches(EventSourceEvent e) {
        return eventSourceClass.isAssignableFrom(e.getEventSource().getClass());
    }

    @Override protected final void invokeHandler(Event event) throws InvocationTargetException, IllegalAccessException {
        EventSourceEvent e = (EventSourceEvent)event;
        getMethod().invoke(getAnnotatedObject(), e.getEvent(), e.getEventSource());
    }

    @Override public String toString() {
        return getMethod().getDeclaringClass().getSimpleName() +"."+ getMethod().getName() +
                "("+ getEventClass().getSimpleName() +","+ eventSourceClass.getSimpleName() +")";
    }
}
