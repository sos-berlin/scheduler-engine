package com.sos.scheduler.engine.eventbus;

import static com.google.common.collect.Sets.newHashSet;
import static java.util.Collections.emptyList;

import java.util.ArrayDeque;
import java.util.Collection;
import java.util.Queue;
import java.util.Set;

import javax.annotation.Nullable;

import org.apache.log4j.Logger;

import com.google.common.collect.HashMultimap;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Multimap;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventHandlerAnnotated;
import com.sos.scheduler.engine.kernel.event.EventSubsystem;
import com.sos.scheduler.engine.kernel.event.SchedulerIsCallableEvent;

public class EventBus {
    private static final Logger logger = Logger.getLogger(EventBus.class);

    private final Set<EventSubscriber2> subscribers = newHashSet();
    private final Multimap<EventHandlerAnnotated,EventSubscriber2> annotatedEventSubscriberMap = HashMultimap.create();
    private final Queue<Call> callQueue = new ArrayDeque<Call>();
    @Nullable private Event currentEvent = null;

    public final void publish(Event e) {
        callQueue.addAll(calls(e));
    }

    public final void dispatch() {
        while (true) {
            Call call = callQueue.poll();
            if (call == null) break;
            dispatchCall(call);
        }
    }

    public final void publishImmediately(Event e) {
        if (currentEvent != null) {
            onRecursiveEvent(e);
        } else
        if (e instanceof SchedulerIsCallableEvent)
            publishEvent(e);
        else
            publishNonrecursiveEvent(e);
    }

    private void onRecursiveEvent(Event e) {
        try {
            // Kein log().error(), sonst gibt es wieder eine Rekursion
            throw new Exception(EventSubsystem.class.getSimpleName() + ".publish("+e+"): ignoring the event triggered by handling the event '"+currentEvent+"'");
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

    private Collection<Call> calls(Event e) {
        ImmutableList.Builder<Call> result = null;
        for (EventSubscriber2 s: subscribers) {
            if (s.getEventClass().isAssignableFrom(e.getClass())) {
                if (result == null) result = ImmutableList.builder();
                result.add(new Call(e, s));
            }
        }
        if (result == null) return emptyList();
        else return result.build();
    }

    private void publishEvent(Event e) {
        for (Call call: calls(e))
            dispatchCall(call);
    }

    private void dispatchCall(Call call) {
        try {
            call.apply();
        } catch (Throwable x) {   // TODO Der C++-Code soll wirklich keine Exception bekommen.
            logger.error(call+": "+x, x);
            //Löst ein rekursives Event aus: log().error(s+": "+x);
            if (call.getEvent().getClass() != EventHandlerFailedEvent.class)
                publishFailedEvent(new EventHandlerFailedEvent(call, x));
        }
    }

    private void publishFailedEvent(EventHandlerFailedEvent e) {
        try {
            publishEvent(e);
        } catch (Throwable x) {   // TODO Der C++-Code soll wirklich keine Exception bekommen.
            logger.error(e +": "+ x, x);
        }
    }

    public final void registerAnnotated(EventHandlerAnnotated o) {
        unregisterAnnotated(o);
        Iterable<EventSubscriber2> subscribers = AnnotatedEventSubscribers.handlers(o);
        annotatedEventSubscriberMap.putAll(o, subscribers);
        register(subscribers);
    }

    public final void unregisterAnnotated(EventHandlerAnnotated o) {
        unregister(annotatedEventSubscriberMap.get(o));
        annotatedEventSubscriberMap.removeAll(o);
    }

    private void register(Iterable<EventSubscriber2> subscribers) {
        for (EventSubscriber2 s: subscribers)
            register(s);
    }

    private void unregister(Iterable<EventSubscriber2> subscribers) {
        for (EventSubscriber2 s: subscribers)
            unregister(s);
    }

    public final void register(EventSubscriber2 s) {
        subscribers.add(s);
    }

    public final void unregister(EventSubscriber2 s) {
        subscribers.remove(s);
    }
}
