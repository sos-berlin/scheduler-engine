package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;

@ForCpp public class TaskStartedEvent extends UnmodifiableTaskEvent {
    @ForCpp public TaskStartedEvent(UnmodifiableTask task) {
        super(task);
    }
}
