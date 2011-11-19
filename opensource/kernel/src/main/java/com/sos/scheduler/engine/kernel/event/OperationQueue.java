package com.sos.scheduler.engine.kernel.event;

public interface OperationQueue {
    void add(Runnable o);
}
