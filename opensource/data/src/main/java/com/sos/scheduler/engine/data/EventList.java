package com.sos.scheduler.engine.data;

import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.folder.FileBasedActivatedEvent;
import com.sos.scheduler.engine.data.folder.FileBasedRemovedEvent;
import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.data.job.TaskStartedEvent;
import com.sos.scheduler.engine.data.order.*;

public class EventList {

    public static final Class<? extends Event>[] eventClassList = new Class[]
            {
                    FileBasedActivatedEvent.class,
                    FileBasedRemovedEvent.class,
                    TaskStartedEvent.class,
                    TaskEndedEvent.class,
                    OrderTouchedEvent.class,
                    OrderFinishedEvent.class,
                    OrderSuspendedEvent.class,
                    OrderResumedEvent.class,
                    OrderStepStartedEvent.class,
                    OrderStepEndedEvent.class,
                    OrderStateChangedEvent.class
            };

}
