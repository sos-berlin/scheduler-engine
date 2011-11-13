package com.sos.scheduler.engine.kernel.event;

import java.util.HashMap;
import java.util.Map;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.AbstractEvent;
import com.sos.scheduler.engine.eventbus.EventBus;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.EventSubscriberAdaptingEventSubscription;
import com.sos.scheduler.engine.eventbus.EventSubscription;
import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.Subsystem;

@ForCpp
public class EventSubsystem extends AbstractHasPlatform implements Subsystem {
    private static final Logger logger = Logger.getLogger(EventSubsystem.class);

    private final EventBus eventBus;
    private final Map<EventSubscriber,EventSubscription> oldSubscribers = new HashMap<EventSubscriber,EventSubscription>();

    public EventSubsystem(Platform platform, EventBus eventBus) {
        super(platform);
        this.eventBus = eventBus;
    }

    /** @param e {@link AbstractEvent} stat {@link com.sos.scheduler.engine.eventbus.Event}, weil C++/Java-Generator die Interface-Hierarchie nicht berücksichtig. */
    @ForCpp public final void report(AbstractEvent e) {
        eventBus.publishImmediately(e);
    }

    public final void subscribeAnnotated(EventHandlerAnnotated o) {
        eventBus.registerAnnotated(o);
    }

    public final void unsubscribeAnnotated(EventHandlerAnnotated o) {
        eventBus.unregisterAnnotated(o);
    }

    public final void subscribe(EventSubscriber old) {
        eventBus.register(new EventSubscriberAdaptingEventSubscription(old));
    }

    public final void unsubscribe(EventSubscriber old) {
        EventSubscription s = oldSubscribers.get(old);
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
