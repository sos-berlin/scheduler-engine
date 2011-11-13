package com.sos.scheduler.engine.eventbus;

public interface EventBus {
    void registerAnnotated(EventHandlerAnnotated o);
    void unregisterAnnotated(EventHandlerAnnotated o);
    void publish(Event e);
}
