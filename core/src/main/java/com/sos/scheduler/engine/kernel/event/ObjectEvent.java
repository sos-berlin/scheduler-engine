package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.kernel.NullSchedulerObject;
import com.sos.scheduler.engine.kernel.SchedulerObject;


public class ObjectEvent<T extends SchedulerObject> extends Event {
    private final T object;


    public ObjectEvent(T o) {
        this.object = o;
    }


    @Override public String toString() {
        StringBuilder result = new StringBuilder(200);
        if (object != NullSchedulerObject.singleton)  result.append(" object=").append(stringOrException(object));

        Message m = getMessage();
        if (m != Message.empty)
            result.append(", message=").append(stringOrException(getMessage()));
        
        return result.toString();
    }


    private static String stringOrException(Object o) {
        try {
            return o == null? "null" : o.toString();
        }
        catch(Throwable x) { return x.toString(); }
    }


    /** Das Objekt kann bereits weg sein, dann gibt es den Fehler Z-JAVA-111. */
    @Override public T getObject() {
        return object;
    }
}
