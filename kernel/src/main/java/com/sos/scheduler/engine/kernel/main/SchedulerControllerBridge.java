package com.sos.scheduler.engine.kernel.main;

import javax.annotation.Nullable;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.Scheduler;

/** Zur Übergabe an {@link SchedulerThread} und den C++-Scheduler, der das Objekt an die Java-Schwester durchreicht. */
@ForCpp
public interface SchedulerControllerBridge {
    void onSchedulerStarted(Scheduler scheduler);
    void onSchedulerActivated();
    void onSchedulerTerminated(int exitCode, @Nullable Throwable t);

    SchedulerControllerBridge empty = new SchedulerControllerBridge() {
        @Override public void onSchedulerStarted(Scheduler scheduler) {}
        @Override public void onSchedulerActivated() {}
        @Override public void onSchedulerTerminated(int exitCode, Throwable t) {}
    };
}
