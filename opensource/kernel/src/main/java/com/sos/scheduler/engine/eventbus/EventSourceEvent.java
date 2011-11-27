package com.sos.scheduler.engine.eventbus;

public class EventSourceEvent implements Event {
    private final Event event;
    private final EventSource eventSource;

    EventSourceEvent(Event event, EventSource eventSource) {
        this.eventSource = eventSource;
        this.event = event;
    }

    public final Event getEvent() {
        return event;
    }

    public final EventSource getEventSource() {
        return eventSource;
    }

    @Override public String toString() {
        return EventSourceEvent.class.getSimpleName() +"("+ event +","+ eventSource +")";
    }
}
