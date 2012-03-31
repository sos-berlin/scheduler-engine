package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.data.event.Event;

public interface EventBus {
    void registerAnnotated(EventHandlerAnnotated o);
    void unregisterAnnotated(EventHandlerAnnotated o);
    void publish(Event e);
}
