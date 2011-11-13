package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.kernel.event.Event;

public interface EventSubscription {
    Class<? extends Event> getEventClass();
    void handleEvent(Event e);
}
