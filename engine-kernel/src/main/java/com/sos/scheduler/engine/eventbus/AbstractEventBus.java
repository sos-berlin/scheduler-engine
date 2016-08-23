package com.sos.scheduler.engine.eventbus;

import com.google.common.collect.HashMultimap;
import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Multimap;
import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.event.KeyedEvent;
import com.sos.scheduler.engine.eventbus.annotated.MethodEventSubscriptionFactory;
import java.util.Map;
import java.util.Set;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import static com.google.common.collect.Maps.newHashMap;
import static com.google.common.collect.Sets.newHashSet;

public abstract class AbstractEventBus implements EventBus {
    private static final Logger logger = LoggerFactory.getLogger(AbstractEventBus.class);

    private final Multimap<Class<? extends KeyedEvent<Event>>, EventSubscription> subscribers = HashMultimap.create();
    private final AnnotatedHandlerFinder handlerFinder;
    private final Multimap<EventHandlerAnnotated,EventSubscription> annotatedEventSubscriberMap = HashMultimap.create();
    private final Map<Class<? extends KeyedEvent<Event>>, ImmutableSet<Class<? extends KeyedEvent<Event>>>> cachedSuperEventClasses = newHashMap();

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

    public final synchronized void register(EventSubscription s) {
        subscribers.put(s.eventClass(), s);
    }

    public final void unregister(EventSubscription s) {
        boolean ok;
        synchronized(this) {
            ok = subscribers.remove(s.eventClass(), s);
        }
        if (!ok) logger.debug("unregister unknown '" + s + "'");
    }

    public final boolean isSubscribed() {
        return !subscribers.isEmpty();
    }

    protected final synchronized ImmutableCollection<Call> calls(KeyedEvent<Event> e) {
//        Event realEvent = e.event() instanceof EventSourceEvent<?>
//            ? ((EventSourceEvent<Event>)e.event()).event()
//            : (Event)e.event();
//        Class<? extends Event> realEventClass = realEvent.getClass();
        Class<? extends KeyedEvent<Event>> realEventClass = (Class<? extends KeyedEvent<Event>>)e.getClass();
        ImmutableList.Builder<Call> result = null;
        for (Class<? extends KeyedEvent<Event>> c: allSuperEventClasses(realEventClass)) {
            for (EventSubscription s: subscribers.get(c)) {
                // matches() sollte nicht in einer Schleife aufgerufen werden. Das kann langsam werden. Stattdessen sollte die EventSource-Klasse direkt in einer Map abgelegt werden.
                if (matches(s, e)) {
                    if (result == null) result = ImmutableList.builder();
                    result.add(new Call(e, s));
                }
            }
        }
        if (result == null) return ImmutableList.of();
        else return result.build();
    }

    private static boolean matches(EventSubscription s, KeyedEvent<Event> e) {
        return true;  // KeyEvent is the only event class
//        return !(s instanceof EventSourceMethodEventSubscription) ||
//                e instanceof EventSourceEvent &&
//                        ((EventSourceMethodEventSubscription)s).eventSourceMatches((EventSourceEvent<?>)e);
    }

    public final boolean publishNow(KeyedEvent<Event> e) {
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
            if (t instanceof Error && t.getClass().getSimpleName().equals("TestError"))
                logger.debug("{}", call, t);
            else
                logger.error("{}", call, t);
            //Löst ein rekursives Event aus: log().error(s+": "+x);
            if (EventHandlerFailedEvent.class.isAssignableFrom(call.getEvent().event().getClass())) {
                return false;
            } else
                return publishNow(KeyedEvent.of(new EventHandlerFailedEvent(call, t)));
        }
    }

    private ImmutableSet<Class<? extends KeyedEvent<Event>>> allSuperEventClasses(Class<? extends KeyedEvent<Event>> eventClass) {
        ImmutableSet<Class<? extends KeyedEvent<Event>>> result = cachedSuperEventClasses.get(eventClass);
        if (result == null) {
            Set<Class<? extends KeyedEvent<Event>>> classes = newHashSet();
            collectAllSuperEventClasses(classes, eventClass);
            result = ImmutableSet.copyOf(classes);
            cachedSuperEventClasses.put(eventClass, result);
        }
        return result;
    }

    private static void collectAllSuperEventClasses(Set<Class<? extends KeyedEvent<Event>>> result, Class<?> clas) {
        if (clas != null && KeyedEvent.class.isAssignableFrom(clas)) {
            @SuppressWarnings("unchecked") Class<? extends KeyedEvent<Event>> eventClass = (Class<? extends KeyedEvent<Event>>)clas;
            if (!result.contains(eventClass)) {
                result.add(eventClass);
                collectAllSuperEventClasses(result, clas.getSuperclass());
                for (Class<?> c: clas.getInterfaces())
                    collectAllSuperEventClasses(result, c);
            }
        }
    }
}
