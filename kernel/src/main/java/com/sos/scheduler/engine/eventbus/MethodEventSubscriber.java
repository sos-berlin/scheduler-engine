package com.sos.scheduler.engine.eventbus;

import static com.google.common.base.Throwables.propagate;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import com.sos.scheduler.engine.kernel.event.Event;

public class MethodEventSubscriber implements EventSubscriber2 {
    private final Class<? extends Event> eventClass;
    private final Object object;
    private final Method method;

    public MethodEventSubscriber(Object object, Method method) {
        eventClass = eventClass(method);
        checkMethod(method);
        this.object = object;
        this.method = method;
        method.setAccessible(true);
    }

    private static void checkMethod(Method m) {
        checkReturnType(m);
    }

    @SuppressWarnings("unchecked")
    private static Class<? extends Event> eventClass(Method m) {
        Class<?>[] p = m.getParameterTypes();
        if (p.length != 1 && !Event.class.isAssignableFrom(p.getClass()))
            throw new IllegalArgumentException("Method " + m.getName() + " must have exactly one argument of a subtype of " + Event.class.getName());
        return (Class<? extends Event>)p[0];
    }

    private static void checkReturnType(Method m) {
        Class<?> c = m.getReturnType();
        if (!isVoid(c))
            throw new IllegalArgumentException("Method " +m.getName() + " has an unexpected return type");
    }

    private static boolean isVoid(Class<?> c) {
        return c == Void.class || c.getName().equals("void");
    }


    @Override public void onEvent(Event event) {
        try {
            method.invoke(object, event);
        } catch (IllegalArgumentException x) { throw new Error("Method "+method+" rejected argument '"+event+"'", x);
        } catch (IllegalAccessException x) { throw new Error("Method "+method+" is inaccessible: " +event, x);
        } catch (InvocationTargetException x) {
//            if (x.getCause() instanceof Error)
//                throw (Error)x.getCause();
            throw propagate(x.getCause() == null? x : x.getCause());
        }
    }

    public Class<? extends Event> getEventClass() {
        return eventClass;
    }

    public Object getObject() {
        return object;
    }

    public Method getMethod() {
        return method;
    }

    @Override public String toString() {
        return method.getDeclaringClass().getSimpleName() +"."+ method.getName() +"("+ eventClass.getSimpleName() +")";
    }
}
