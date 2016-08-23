package com.sos.scheduler.engine.eventbus.annotated;

import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.event.KeyedEvent;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.eventbus.EventSourceEvent;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/** Eine {@link com.sos.scheduler.engine.eventbus.EventSubscription} für eine mit
 * {@link com.sos.scheduler.engine.eventbus.HotEventHandler} annotierte Methode mit einem
 * Parameter für das {@link KeyedEvent} und einem zweiten Parameter für {@link EventSource}
 * mit dem auslösenden Objekt. Das auslösende Objekt (zum Beispiel eine Order) ist nur in einem
 * {@code HotEventHandler} gültig, weshalb es nicht ins {@code AnyKeyedEvent} aufgenommen wird. */
public class EventSourceMethodEventSubscription extends MethodEventSubscription {
    private final Class<? extends EventSource> eventSourceClass;

    EventSourceMethodEventSubscription(EventHandlerAnnotated annotatedObject, Method method) {
        super(annotatedObject, method);
        this.eventSourceClass = checkedParameterClass(method, 1, EventSource.class);
        checkMethodParameterCount(method, 1, 2);
    }

    public final boolean eventSourceMatches(EventSourceEvent<?> e) {
        return eventSourceClass.isAssignableFrom(e.eventSource().getClass());
    }

    @Override protected final void invokeHandler(KeyedEvent<Event> keyedEvent) throws InvocationTargetException, IllegalAccessException {
        EventSourceEvent<?> sourceEvent = (EventSourceEvent<?>)keyedEvent.event();
        KeyedEvent<Event> cleanedKeyedEvent = new KeyedEvent<>(keyedEvent.key(), sourceEvent);
        getMethod().invoke(getAnnotatedObject(), cleanedKeyedEvent, sourceEvent.eventSource());
    }

    @Override public String toString() {
        return getMethod().getDeclaringClass().getSimpleName() +"."+ getMethod().getName() +
                "("+ eventClass().getSimpleName() +","+ eventSourceClass.getSimpleName() +")";
    }
}
