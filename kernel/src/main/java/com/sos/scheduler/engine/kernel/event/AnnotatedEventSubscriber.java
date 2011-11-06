package com.sos.scheduler.engine.kernel.event;

import static com.google.common.base.Throwables.propagate;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

import com.google.common.collect.ImmutableList;

public class AnnotatedEventSubscriber implements EventSubscriber {
    private final OperationCollector operationCollector;
    private final EventHandlerAnnotated object;
    private final ImmutableList<EventHandlingMethod> methods;

    public AnnotatedEventSubscriber(OperationCollector operationCollector, EventHandlerAnnotated object,
            ImmutableList<EventHandlingMethod> methods) {
        this.operationCollector = operationCollector;
        this.object = object;
        this.methods = methods;
    }

    @Override public void onEvent(Event e) throws Exception {
        for (EventHandlingMethod m: methods) {
            if (m.getEventClass().isAssignableFrom(e.getClass()))
                callHandler(m.getMethod(), e);
        }
    }

    private void callHandler(Method m, Event e) throws IllegalAccessException {
        try {
            Object response = m.invoke(object, e);
            if (response != null)
                operationCollector.addOperation((SimpleSchedulerOperation)response);
        } catch (InvocationTargetException x) { throw propagate(x.getCause()); }
    }

    public static AnnotatedEventSubscriber of(EventHandlerAnnotated o, OperationCollector operationCollector) {
        return new AnnotatedEventSubscriber(operationCollector, o, eventHandlerMethods(o));
    }

    public static ImmutableList<EventHandlingMethod> eventHandlerMethods(EventHandlerAnnotated o) {
        ImmutableList.Builder<EventHandlingMethod> result = new ImmutableList.Builder<EventHandlingMethod>();
        for (Method m: o.getClass().getMethods()) {
            if (!Modifier.isStatic(m.getModifiers())) {
                if (m.getAnnotation(EventHandler.class) != null) {
                    result.add(new EventHandlingMethod(m));
                }
            }
        }
        return result.build();
    }

    public boolean isEmpty() {
        return methods.isEmpty();
    }
}
