package com.sos.scheduler.engine.kernel.event;

import java.lang.reflect.Method;

class EventHandlingMethod {
    private final Method method;
    private final Class<? extends Event> eventClass;

    EventHandlingMethod(Method method) {
        this.method = method;
        eventClass = eventClass(method);
        checkReturnType(method);
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
        if (!isVoid(c) && !EventResponse.class.isAssignableFrom(c))
            throw new IllegalArgumentException("Method " +m.getName() + "has an unexpected return type");
    }

    private static boolean isVoid(Class<?> c) {
        return c == Void.class || c.getName().equals("void");
    }

    Method getMethod() {
        return method;
    }

    Class<? extends Event> getEventClass() {
        return eventClass;
    }
}
