package com.sos.scheduler.engine.eventbus;

import java.util.ArrayDeque;
import java.util.Collection;
import java.util.Queue;

import com.google.common.collect.ImmutableList;

public class ColdEventBus extends AbstractEventBus {
    private final Queue<Call> callQueue = new ArrayDeque<Call>();

    public ColdEventBus() {
        super(EventHandler.class);
    }

    @Override public final void publish(Event e) {
        publish(calls(e));
    }

    final void publish(Collection<Call> c) {
        callQueue.addAll(c);
    }

    public final void dispatchEvents() {
        while (true) {
            Call call = callQueue.poll();
            if (call == null) break;
            dispatchCall(call);
        }
    }
}
