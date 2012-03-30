package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.eventbus.AbstractEvent;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;

public abstract class TaskEvent extends AbstractEvent {
    public abstract AbsolutePath getJobPath();
}
