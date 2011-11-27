package com.sos.scheduler.engine.eventbus.annotated;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSourceEvent;
import com.sos.scheduler.engine.eventbus.EventSubscription;

/** Eine {@link EventSubscription} für eine mit @{@link com.sos.scheduler.engine.eventbus.EventHandler} oder
 * @{@link com.sos.scheduler.engine.eventbus.HotEventHandler} annotierte Methode mit nur einem Parameter für das {@link Event}. */
public class SimpleMethodEventSubscription extends MethodEventSubscription {
    public SimpleMethodEventSubscription(EventHandlerAnnotated annotatedObject, Method method) {
        super(annotatedObject, method);
        checkMethodParameterCount(method, 1, 1);
    }

    @Override protected final void invokeHandler(Event event) throws InvocationTargetException, IllegalAccessException {
        Event e = event instanceof EventSourceEvent? ((EventSourceEvent)event).getEvent() : event;
        getMethod().invoke(getAnnotatedObject(), e);
    }

    @Override public String toString() {
        return getMethod().getDeclaringClass().getSimpleName() +"."+ getMethod().getName() +
                "("+ getEventClass().getSimpleName() +")";
    }
}
