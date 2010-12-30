package com.sos.scheduler.engine.kernel.main.event;

import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.util.sync.Rendezvous;
import com.sos.scheduler.kernel.cplusplus.runtime.ThreadLock;


public class EventRendezvous extends Rendezvous<Event,Object>
{
    private static final Object dummyResult = new Object();

    private boolean beginTimedOut = false;
    private boolean terminatedEventReceived = false;


    public void unlockAndCall(Event e) {
        ThreadLock.unlock();
        try {
            call(e);
        }
        finally {
            ThreadLock.lock();
        }
    }


    @Override public Event enter(Time timeout) {
        Event result = super.enter(timeout);
        beginTimedOut = result == null;
        if (beginTimedOut)   result = new TimeoutEvent(timeout);
        if (result instanceof TerminatedEvent)  terminatedEventReceived = true;
        return result;
    }


    public void leave() {
        leave(dummyResult);
    }


    @Override public void leave(Object result) {
        if (!beginTimedOut)
            super.leave(result);
    }


    public boolean terminatedEventReceived() {
        return terminatedEventReceived;
    }
}
