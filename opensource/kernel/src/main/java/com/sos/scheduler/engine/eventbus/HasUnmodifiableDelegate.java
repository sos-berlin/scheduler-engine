package com.sos.scheduler.engine.eventbus;

public interface HasUnmodifiableDelegate<T extends EventSource> extends EventSource {
    T unmodifiableDelegate();
}
