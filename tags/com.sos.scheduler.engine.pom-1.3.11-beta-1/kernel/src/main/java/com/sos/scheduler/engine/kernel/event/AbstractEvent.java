package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.kernel.NullSchedulerObject;
import com.sos.scheduler.engine.kernel.SchedulerObject;
import static com.sos.scheduler.engine.kernel.util.Util.*;


public class AbstractEvent extends Event
{
    @Override public SchedulerObject getObject() {
        return NullSchedulerObject.singleton;
    }


    @Override public Message getMessage() {
        return EmptyMessage.singleton;
    }

    
    @Override public String toString() {
        StringBuilder result = new StringBuilder(200);
        result.append(getClass().getName());

        Message m = getMessage();
        if (m != EmptyMessage.singleton)
            result.append(", message=").append(stringOrException(getMessage()));

        return result.toString();
    }
}
