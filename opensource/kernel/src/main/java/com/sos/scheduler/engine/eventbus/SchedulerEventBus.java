package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.data.event.Event;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public final class SchedulerEventBus implements EventBus, Runnable {
    private static final Logger logger = LoggerFactory.getLogger(SchedulerEventBus.class);

    private final HotEventBus hotEventBus = new HotEventBus();
    private final ColdEventBus coldEventBus = new ColdEventBus();

    @Override public void registerAnnotated(EventHandlerAnnotated o) {
        hotEventBus.registerAnnotated(o);
        coldEventBus.registerAnnotated(o);
    }

    @Override public void unregisterAnnotated(EventHandlerAnnotated o) {
        hotEventBus.unregisterAnnotated(o);
        coldEventBus.unregisterAnnotated(o);
    }

    public void register(EventSubscription s) {
        coldEventBus.register(s);
    }

    public void unregister(EventSubscription s) {
        coldEventBus.unregister(s);
    }

    public void registerHot(EventSubscription s) {
        hotEventBus.register(s);
    }

    public void unregisterHot(EventSubscription s) {
        hotEventBus.unregister(s);
    }

    public boolean isSubscribed() {
        return hotEventBus.isSubscribed() || coldEventBus.isSubscribed();
    }

    public void publish(Event e, EventSource source) {
        publish(new EventSourceEvent(e, source));
    }

    @Override public void publish(Event e) {
        logger.trace("publish {}", e);
        hotEventBus.publish(e);
        publishCold(e);
    }

    public void publishCold(Event e) {
        coldEventBus.publish(e instanceof EventSourceEvent? ((EventSourceEvent)e).getEvent() : e);
    }

    public void dispatchEvents() {
        coldEventBus.dispatchEvents();
    }

    @Override public void run() {
        coldEventBus.run();
    }

    @Override public String toString() {
        return getClass().getSimpleName();
    }
}
