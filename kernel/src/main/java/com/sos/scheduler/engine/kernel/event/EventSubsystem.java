package com.sos.scheduler.engine.kernel.event;

import java.util.HashMap;
import java.util.Map;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.Callback;
import com.sos.scheduler.engine.eventbus.EventBus;
import com.sos.scheduler.engine.eventbus.EventSubscriber2;
import com.sos.scheduler.engine.eventbus.EventSubscriber2Adapter;
import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.Subsystem;

@ForCpp
public class EventSubsystem extends AbstractHasPlatform implements Subsystem {
    private static final Logger logger = Logger.getLogger(EventSubsystem.class);

    private final EventBus eventBus;
    private final Map<EventSubscriber,EventSubscriber2> oldSubscribers = new HashMap<EventSubscriber,EventSubscriber2>();

    public EventSubsystem(Platform platform, EventBus eventBus) {
        super(platform);
        this.eventBus = eventBus;
    }

    @ForCpp public final void report(Event e) {
        eventBus.publishImmediately(e);
    }

    public final void subscribeAnnotated(EventHandlerAnnotated o) {
        eventBus.registerAnnotated(o);
    }

    public final void unsubscribeAnnotated(EventHandlerAnnotated o) {
        eventBus.unregisterAnnotated(o);
    }

    public final void subscribe(EventSubscriber old) {
        eventBus.register(new EventSubscriber2Adapter(old));
    }

    public final void unsubscribe(EventSubscriber old) {
        EventSubscriber2 s = oldSubscribers.get(old);
        if (s != null) {
            oldSubscribers.remove(old);
            eventBus.unregister(s);
        }
    }

    public void dispatchEvents() {
        eventBus.dispatch();
    }

    @Override public final String toString() {
        return getClass().getSimpleName();
    }
}
