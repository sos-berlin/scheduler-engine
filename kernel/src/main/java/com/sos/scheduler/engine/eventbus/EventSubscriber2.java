package com.sos.scheduler.engine.eventbus;

import com.sos.scheduler.engine.kernel.event.Event;

public interface EventSubscriber2 {
    Class<? extends Event> getEventClass();
    void onEvent(Event e);
}
