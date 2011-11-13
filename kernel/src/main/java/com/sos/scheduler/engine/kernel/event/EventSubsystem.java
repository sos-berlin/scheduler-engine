package com.sos.scheduler.engine.kernel.event;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.Subsystem;

@ForCpp
public class EventSubsystem extends AbstractHasPlatform implements Subsystem {
    private static final Logger logger = Logger.getLogger(EventSubsystem.class);

    private final OperationQueue operationQueue;
    private final Set<EventSubscriber> subscribers = new HashSet<EventSubscriber>();
    private final Map<Object,AnnotatedEventSubscriber> annotatedEventSubscriberMap = new HashMap<Object,AnnotatedEventSubscriber>();
    private Event currentEvent = null;

    public EventSubsystem(Platform platform, OperationQueue operationQueue) {
        super(platform);
        this.operationQueue = operationQueue;
    }

    @ForCpp public final void report(Event e) {
        if (currentEvent != null) {
            onRecursiveEvent(e);
        } else {
            if (e instanceof SchedulerIsCallableEvent)
                publishEvent(e);
            else
                publishNonrecursiveEvent(e);
        }
    }

    private void onRecursiveEvent(Event e) {
        try {
            // Kein log().error(), sonst gibt es wieder eine Rekursion
            throw new Exception(EventSubsystem.class.getSimpleName() + ".report(" +e+ "): ignoring the event triggered by handling the event '"+currentEvent+"'");
        }
        catch (Exception x) {
            logger.error(x, x);
        }
    }

    private void publishNonrecursiveEvent(Event e) {
        currentEvent = e;
        try {
            publishEvent(e);
        }
        finally {
            currentEvent = null;
        }
    }

    private void publishEvent(Event e) {
        logger.trace(e);
        for (EventSubscriber s: subscribers) publishEvent(e, s);
    }

    private void publishEvent(Event e, EventSubscriber s) {
        try {
            s.onEvent(e);
        }
        catch (Throwable x) {   // Der C++-Code soll wirklich keine Exception bekommen.
            logger.error(s+": "+x, x);
            //log().error(s+": "+x);   Löst ein rekursives Event aus. Events sollten in Warteschlange, vielleicht mit Guava-EventBus implementieren
        }
    }

    public final void subscribeAnnotated(EventHandlerAnnotated o) {
        AnnotatedEventSubscriber s = AnnotatedEventSubscriber.of(o);
        if (!s.isEmpty()) {
            annotatedEventSubscriberMap.put(o, s);
            subscribe(s);
        }
    }

    public final void unsubscribeAnnotated(Object o) {
        AnnotatedEventSubscriber s =  annotatedEventSubscriberMap.get(o);
        if (s != null) {
            unsubscribe(s);
            annotatedEventSubscriberMap.remove(o);
        }
    }

    public final void subscribe(EventSubscriber s) {
        subscribers.add(s);
    }

    public final void unsubscribe(EventSubscriber s) {
        subscribers.remove(s);
    }

    @Override public final String toString() {
        return getClass().getSimpleName();
    }
}
