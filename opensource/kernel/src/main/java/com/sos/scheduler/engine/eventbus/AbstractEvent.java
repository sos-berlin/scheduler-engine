package com.sos.scheduler.engine.eventbus;

public abstract class AbstractEvent implements Event {
    @Override public String toString() {
        return getClass().getName();
    }
}
