package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.event.ObjectEvent;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;

@ForCpp public class TaskEndedEvent extends UnmodifiableTaskEvent {
    @ForCpp public TaskEndedEvent(UnmodifiableTask task) {
        super(task);
    }
}
