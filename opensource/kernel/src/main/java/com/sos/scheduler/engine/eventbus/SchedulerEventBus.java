package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.data.event.Event;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class SchedulerEventBus implements EventBus {
    private static final Logger logger = LoggerFactory.getLogger(SchedulerEventBus.class);

    private final HotEventBus hotEventBus = new HotEventBus();
    private final ColdEventBus coldEventBus = new ColdEventBus();

    @Override public final void registerAnnotated(EventHandlerAnnotated o) {
        hotEventBus.registerAnnotated(o);
        coldEventBus.registerAnnotated(o);
    }

    @Override public final void unregisterAnnotated(EventHandlerAnnotated o) {
        hotEventBus.unregisterAnnotated(o);
        coldEventBus.unregisterAnnotated(o);
    }

    public final void register(EventSubscription s) {
        coldEventBus.register(s);
    }

    public final void unregister(EventSubscription s) {
        coldEventBus.unregister(s);
    }

    public final void registerHot(EventSubscription s) {
        hotEventBus.register(s);
    }

    public final void unregisterHot(EventSubscription s) {
        hotEventBus.unregister(s);
    }

    public final void publish(Event e, EventSource source) {
        publish(new EventSourceEvent(e, source));
    }

    @Override public final void publish(Event e) {
        logger.trace("publish {}", e);
        hotEventBus.publish(e);
        coldEventBus.publish(e instanceof EventSourceEvent? ((EventSourceEvent)e).getEvent() : e);
    }

    public final void publishCold(Event e) {
        coldEventBus.publish(e instanceof EventSourceEvent? ((EventSourceEvent)e).getEvent() : e);
    }

    public final void dispatchEvents() {
        coldEventBus.dispatchEvents();
    }
}
