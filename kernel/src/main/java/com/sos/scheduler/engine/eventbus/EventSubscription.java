package com.sos.scheduler.engine.eventbus;

public interface EventSubscription {
    Class<? extends Event> getEventClass();
    void handleEvent(Event e);
}
