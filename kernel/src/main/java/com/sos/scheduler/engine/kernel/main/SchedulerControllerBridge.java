package com.sos.scheduler.engine.kernel.main;

import javax.annotation.Nullable;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.EventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.settings.Settings;

/** Zur Übergabe an {@link SchedulerThread} und den C++-Scheduler, der das Objekt an die Java-Schwester
 * {@link Scheduler} durchreicht. */
@ForCpp
public interface SchedulerControllerBridge {
    Settings getSettings();
    void onSchedulerStarted(Scheduler scheduler);
    void onSchedulerActivated();
    void onSchedulerTerminated(int exitCode, @Nullable Throwable t);
    EventBus getEventBus();
}
