package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.kernel.log.PrefixLog;


/** Sollte möglichst unabhängig von den Scheduler-Objekten sein */
public interface SchedulerObject {
    // Für Event usw. (?)
    PrefixLog log();
    //? SchedulerObjectType objectType();   // Z.B. Singleton "JobObjectType"
    //? String objectName();   // Z.B. "Job /a"
}
