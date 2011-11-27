package com.sos.scheduler.engine.eventbus.annotated;

import static com.google.common.base.Throwables.propagate;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSubscription;

/** Eine {@link EventSubscription} für eine mit @{@link com.sos.scheduler.engine.eventbus.EventHandler} oder
 * @{@link com.sos.scheduler.engine.eventbus.HotEventHandler} annotierte Methode. */
abstract class MethodEventSubscription implements EventSubscription {
    private final Class<? extends Event> eventClass;
    private final EventHandlerAnnotated annotatedObject;
    private final Method method;

    protected MethodEventSubscription(EventHandlerAnnotated o, Method method) {
        this.eventClass = checkedParameterClass(method, 0, Event.class);
        this.annotatedObject = o;
        checkReturnType(method);
        this.method = method;
        method.setAccessible(true);
    }

    @SuppressWarnings("unchecked")
    protected static <T, U extends T> Class<U> checkedParameterClass(Method m, int i, Class<T> c) {
        Class<?>[] t = m.getParameterTypes();
        if (t.length < i)
            throw new IllegalArgumentException("Method "+m+" must have "+i+" parameters");
        Class<?> p = t[i];
        if (!c.isAssignableFrom(p))
            throw new IllegalArgumentException("Method "+m+" must have parameters #"+ (i+1) +" of a subtype of "+ c.getName());
        return (Class<U>)p;
    }

    protected static void checkMethodParameterCount(Method m, int min, int max) {
        int n = m.getParameterTypes().length;
        if (!(n >= min && n <= max))
            throw new IllegalArgumentException("Method "+m+" must have "+min+"..."+max+" arguments");
    }

    private static void checkReturnType(Method m) {
        Class<?> c = m.getReturnType();
        if (!isVoid(c))
            throw new IllegalArgumentException("Method "+m+" has an unexpected return type");
    }

    private static boolean isVoid(Class<?> c) {
        return c == Void.class || c.getName().equals("void");
    }

    @Override public void handleEvent(Event event) {
        try {
            invokeHandler(event);
        } catch (IllegalArgumentException x) { throw new Error("Method "+ getMethod() +" rejected argument '"+event+"'", x);
        } catch (IllegalAccessException x) { throw new Error("Method "+ getMethod() +" is inaccessible: " +event, x);
        } catch (InvocationTargetException x) {
            throw propagate(x.getCause() == null? x : x.getCause());
        }
    }

    protected abstract void invokeHandler(Event event) throws InvocationTargetException, IllegalAccessException;

    @Override public Class<? extends Event> getEventClass() {
        return eventClass;
    }

    final EventHandlerAnnotated getAnnotatedObject() {
        return annotatedObject;
    }

    final Method getMethod() {
        return method;
    }
}
