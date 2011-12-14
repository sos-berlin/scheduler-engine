package com.sos.scheduler.engine.eventbus;

import static java.util.Collections.emptyList;
import static java.util.Collections.synchronizedSet;
import static org.apache.log4j.Level.DEBUG;
import static org.apache.log4j.Level.ERROR;
import static org.apache.log4j.Level.FATAL;

import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

import org.apache.log4j.Level;
import org.apache.log4j.Logger;

import com.google.common.collect.HashMultimap;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Multimap;
import com.sos.scheduler.engine.eventbus.annotated.EventSourceMethodEventSubscription;
import com.sos.scheduler.engine.eventbus.annotated.MethodEventSubscriptionFactory;

public abstract class AbstractEventBus implements EventBus {
    private static final Logger logger = Logger.getLogger(AbstractEventBus.class);

    private final Set<EventSubscription> subscribers = synchronizedSet(new HashSet<EventSubscription>());
    private final AnnotatedHandlerFinder handlerFinder;
    private final Multimap<EventHandlerAnnotated,EventSubscription> annotatedEventSubscriberMap = HashMultimap.create();

    protected AbstractEventBus(MethodEventSubscriptionFactory factory) {
        this.handlerFinder = new AnnotatedHandlerFinder(factory);
    }

    @Override public final void registerAnnotated(EventHandlerAnnotated o) {
        unregisterAnnotated(o);
        Iterable<EventSubscription> subscribers = handlerFinder.handlers(o);
        annotatedEventSubscriberMap.putAll(o, subscribers);
        registerAll(subscribers);
    }

    @Override public final void unregisterAnnotated(EventHandlerAnnotated o) {
        unregisterAll(annotatedEventSubscriberMap.get(o));
        annotatedEventSubscriberMap.removeAll(o);
    }

    private void registerAll(Iterable<EventSubscription> subscribers) {
        for (EventSubscription s: subscribers)
            register(s);
    }

    private void unregisterAll(Iterable<EventSubscription> subscribers) {
        for (EventSubscription s: subscribers)
            unregister(s);
    }

    public final void register(EventSubscription s) {
        subscribers.add(s);
    }

    public final void unregister(EventSubscription s) {
        subscribers.remove(s);
    }

    protected final Collection<Call> calls(Event e) {
        Class<? extends Event> realEventClass = (e instanceof EventSourceEvent? ((EventSourceEvent)e).getEvent() : e).getClass();
        ImmutableList.Builder<Call> result = null;
        for (EventSubscription s: subscribers) {
            if (matches(s, e, realEventClass)) {
                if (result == null) result = ImmutableList.builder();
                result.add(new Call(e, s));
            }
        }
        if (result == null) return emptyList();
        else return result.build();
    }

    private static boolean matches(EventSubscription s, Event e, Class<? extends Event> realEventClass) {
        return s instanceof EventSourceMethodEventSubscription?
            e instanceof EventSourceEvent && ((EventSourceMethodEventSubscription)s).matches((EventSourceEvent)e)
            : s.getEventClass().isAssignableFrom(realEventClass);
    }

    public final boolean publishNow(Event e) {
        boolean published = false;
        for (Call call: calls(e)) {
            dispatchCall(call);
            published = true;
        }
        return published;
    }

    protected final boolean dispatchCall(Call call) {
        try {
            call.apply();
            return true;
        } catch (Throwable t) {   // Der C++-Code soll wirklich keine Exception bekommen.
            Level level = t instanceof Error? t.getClass().getSimpleName().equals("TestError")? DEBUG : FATAL : ERROR;
                logger.log(level, call+": "+t, t);
            //Löst ein rekursives Event aus: log().error(s+": "+x);
            if (call.getEvent().getClass() == EventHandlerFailedEvent.class) {
                return false;
            } else
                return publishNow(new EventHandlerFailedEvent(call, t));
        }
    }
}
