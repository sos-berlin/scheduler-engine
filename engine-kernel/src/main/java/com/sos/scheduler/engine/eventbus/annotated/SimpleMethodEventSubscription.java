package com.sos.scheduler.engine.eventbus.annotated;

import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.event.KeyedEvent;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSourceEvent;
import com.sos.scheduler.engine.eventbus.EventSubscription;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

/** Eine {@link EventSubscription} für eine mit {@link com.sos.scheduler.engine.eventbus.EventHandler} oder
 * {@link com.sos.scheduler.engine.eventbus.HotEventHandler} annotierte Methode mit nur einem Parameter für das {@link AnyKeyedEvent}. */
public class SimpleMethodEventSubscription extends MethodEventSubscription {
    public SimpleMethodEventSubscription(EventHandlerAnnotated annotatedObject, Method method) {
        super(annotatedObject, method);
        checkMethodParameterCount(method, 1, 1);
    }

    @Override protected final void invokeHandler(KeyedEvent<Event> keyedEvent) throws InvocationTargetException, IllegalAccessException {
        KeyedEvent<Event> e = keyedEvent.event() instanceof EventSourceEvent<?>
            ? new KeyedEvent<Event>(keyedEvent.key(), (Event)((EventSourceEvent<?>)keyedEvent.event()).event())
            : keyedEvent;
        getMethod().invoke(getAnnotatedObject(), e);
    }

    @Override public String toString() {
        return getMethod().getDeclaringClass().getSimpleName() +"."+ getMethod().getName() +
                "("+ eventClass().getSimpleName() +")";
    }
}
