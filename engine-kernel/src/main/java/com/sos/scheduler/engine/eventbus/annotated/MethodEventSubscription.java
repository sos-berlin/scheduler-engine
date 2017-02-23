package com.sos.scheduler.engine.eventbus.annotated;

import com.sos.jobscheduler.data.event.Event;
import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSubscription;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.ParameterizedType;
import java.lang.reflect.Type;
import static com.google.common.base.Throwables.propagate;

/** Eine {@link EventSubscription} für eine mit {@link com.sos.scheduler.engine.eventbus.EventHandler} oder
 * {@link com.sos.scheduler.engine.eventbus.HotEventHandler} annotierte Methode. */
abstract class MethodEventSubscription implements EventSubscription {
    private final Class<? extends Event> eventClass;
    private final EventHandlerAnnotated annotatedObject;
    private final Method method;

    MethodEventSubscription(EventHandlerAnnotated o, Method method) {
        this.eventClass = checkedParameterClass(method);
        this.annotatedObject = o;
        checkReturnType(method);
        this.method = method;
        method.setAccessible(true);
    }

    @SuppressWarnings("unchecked")
    static <T, U extends T> Class<U> checkedParameterClass(Method method) {
        Type[] types = method.getGenericParameterTypes();
        if (types.length != 1)
            throw new IllegalArgumentException("Method "+method+" must have exactly 1 parameter");
        try {
            ParameterizedType p = (ParameterizedType)types[0];
            return (Class<U>)p.getActualTypeArguments()[0];
        }
        catch (Exception e) {
            throw new IllegalArgumentException("Method "+method+" must have parameter of a type KeyedEvent<Event>: " + e, e);
        }
    }

    static void checkMethodParameterCount(Method m, int min, int max) {
        int n = m.getParameterTypes().length;
        if (!(n >= min && n <= max))
            throw new IllegalArgumentException("Method "+m+" must have "+(min == max? min : min+" through "+max)+" arguments");
    }

    private static void checkReturnType(Method m) {
        Class<?> c = m.getReturnType();
        if (!isVoid(c))
            throw new IllegalArgumentException("Method "+m+" has an unexpected return type");
    }

    private static boolean isVoid(Class<?> c) {
        return c == Void.class || c.getName().equals("void");
    }

    @Override public final void handleEvent(KeyedEvent<Event> event) {
        try {
            invokeHandler(event);
        } catch (IllegalArgumentException x) { throw new Error("Method "+ method +" rejected argument '"+event+"'", x);
        } catch (IllegalAccessException x) { throw new Error("Method "+ method +" is inaccessible: " +event, x);
        } catch (InvocationTargetException x) {
            throw propagate(x.getCause() == null? x : x.getCause());
        }
    }

    protected abstract void invokeHandler(KeyedEvent<Event> event) throws InvocationTargetException, IllegalAccessException;

    @Override public final Class<? extends Event> eventClass() {
        return eventClass;
    }

    final EventHandlerAnnotated getAnnotatedObject() {
        return annotatedObject;
    }

    final Method getMethod() {
        return method;
    }
}
