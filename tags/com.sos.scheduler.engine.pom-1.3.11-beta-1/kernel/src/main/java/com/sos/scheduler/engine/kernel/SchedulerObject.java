package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.kernel.log.SchedulerLogger;


/** Schnittstelle für alle Scheduler-Objekte */
public interface SchedulerObject {
    // Für Event usw. (?)
    SchedulerLogger log();
    //? SchedulerObjectType objectType();   // Z.B. Singleton "JobObjectType"
    //? String objectName();   // Z.B. "Job /a"
}
