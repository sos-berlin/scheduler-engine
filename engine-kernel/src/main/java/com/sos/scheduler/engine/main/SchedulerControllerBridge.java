package com.sos.scheduler.engine.main;

import com.google.inject.Injector;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.settings.CppSettings;
import javax.annotation.Nullable;

/** Zur Übergabe an {@link SchedulerThread} und den C++-Scheduler, der das Objekt an die Java-Schwester
 * {@link Scheduler} durchreicht. */
@ForCpp
public interface SchedulerControllerBridge {
    String getName();
    CppSettings cppSettings();
    void onSchedulerStarted(Scheduler scheduler);
    void onSchedulerActivated();
    void onSchedulerTerminated(int exitCode, @Nullable Throwable t);
    SchedulerEventBus getEventBus();
    void setInjector(Injector injector);
}
