package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.kernel.NullSchedulerObject;
import com.sos.scheduler.engine.kernel.SchedulerObject;


abstract public class Event
{
    public SchedulerObject getObject() {
        return NullSchedulerObject.singleton;
    }


    public Message getMessage() {
        return Message.empty;
    }

    
    @Override public String toString() {
        return getClass().getName();
    }
}
