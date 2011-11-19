package com.sos.scheduler.engine.kernel.main.event;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.eventbus.Event;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.Rendezvous;

public class EventRendezvous extends Rendezvous<Event,Object> {
    private static final Object dummyResult = new Object();

    private boolean beginTimedOut = false;
    private boolean terminatedEventReceived = false;

    public final void unlockAndCall(Event e) {
        CppProxy.threadLock.unlock();
        try {
            call(e);
        }
        finally {
            CppProxy.threadLock.lock();
        }
    }

    @Override public final Event enter(Time timeout) {
        Event result = super.enter(timeout);
        beginTimedOut = result == null;
        if (beginTimedOut)   result = new TimeoutEvent(timeout);
        if (result instanceof TerminatedEvent)  terminatedEventReceived = true;
        return result;
    }

    public final void leave() {
        leave(dummyResult);
    }

    @Override public final void leave(Object result) {
        if (!beginTimedOut)
            super.leave(result);
    }

    public final boolean terminatedEventReceived() {
        return terminatedEventReceived;
    }
}
