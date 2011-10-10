package com.sos.scheduler.engine.kernel.event;

public abstract class AbstractEvent extends Event {
    @Override public String toString() {
        return getClass().getName();
    }
}
