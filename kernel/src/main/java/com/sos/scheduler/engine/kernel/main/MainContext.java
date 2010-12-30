package com.sos.scheduler.engine.kernel.main;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;


/** Zur Übergabe an den C++-Scheduler, der das Objekt an die Java-Schwester durchreicht. */
@ForCpp
public interface MainContext {
    void setScheduler(Scheduler scheduler);
    void onSchedulerActivated();
}
