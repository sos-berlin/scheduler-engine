package com.sos.scheduler.engine.kernel.event;

import static com.sos.scheduler.engine.kernel.util.Util.stringOrException;


public class AbstractEvent extends Event
{
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
