package com.sos.scheduler.engine.eventbus;

import static com.google.common.collect.Sets.newHashSet;
import static java.util.Collections.emptyList;

import java.lang.annotation.Annotation;
import java.util.Collection;
import java.util.Set;

import org.apache.log4j.Level;
import org.apache.log4j.Logger;

import com.google.common.collect.HashMultimap;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Multimap;

public abstract class AbstractEventBus implements EventBus {
    private static final Logger logger = Logger.getLogger(AbstractEventBus.class);

    private final Set<EventSubscription> subscribers = newHashSet();
    private final AnnotatedHandlerFinder handlerFinder;
    private final Multimap<EventHandlerAnnotated,EventSubscription> annotatedEventSubscriberMap = HashMultimap.create();

    protected AbstractEventBus(ImmutableList<Class<? extends Annotation>> annotations) {
        this.handlerFinder = new AnnotatedHandlerFinder(annotations);
    }

    protected AbstractEventBus(Class<? extends Annotation> annotation) {
        this(new ImmutableList.Builder<Class<? extends Annotation>>().add(annotation).build());
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
        ImmutableList.Builder<Call> result = null;
        for (EventSubscription s: subscribers) {
            if (s.getEventClass().isAssignableFrom(e.getClass())) {
                if (result == null) result = ImmutableList.builder();
                result.add(new Call(e, s));
            }
        }
        if (result == null) return emptyList();
        else return result.build();
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
            logger.log(t instanceof Error? Level.FATAL : Level.ERROR, call+": "+t, t);
            //Löst ein rekursives Event aus: log().error(s+": "+x);
            if (call.getEvent().getClass() == EventHandlerFailedEvent.class)
                return false;
            else
                return publishNow(new EventHandlerFailedEvent(call, t));
        }
    }
}
