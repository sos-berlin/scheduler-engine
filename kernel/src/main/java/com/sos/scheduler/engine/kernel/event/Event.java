package com.sos.scheduler.engine.kernel.event;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;


@ForCpp
public abstract class Event {  // Abstrakte Klasse statt Interface, damit C++/Java-Generator Event als Oberklasse akzeptiert.
    //public abstract EventId getId();
    //public long getTimestampMs();
    //public abstract SchedulerObject getObject();
    public abstract Message getMessage();
}
