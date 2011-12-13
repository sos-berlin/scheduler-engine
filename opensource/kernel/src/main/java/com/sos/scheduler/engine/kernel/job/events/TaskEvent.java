package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.kernel.event.ObjectEvent;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;

public abstract class TaskEvent extends ObjectEvent {
    public abstract AbsolutePath getJobPath();
}
