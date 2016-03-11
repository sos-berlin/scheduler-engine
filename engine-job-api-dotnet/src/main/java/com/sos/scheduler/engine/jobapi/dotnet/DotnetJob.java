package com.sos.scheduler.engine.jobapi.dotnet;

import com.sos.scheduler.engine.jobapi.dotnet.api.TaskContext;
import scala.NotImplementedError;

/**
 * @author Joacim Zschimmer
 */
public final class DotnetJob extends sos.spooler.Job_impl {
    private final Object dotnetObject;

    DotnetJob(TaskContext taskContext, Object dotnetObject) {
        spooler_log = taskContext.spoolerLog();
        spooler_task = taskContext.spoolerTask();
        spooler_job = taskContext.spoolerJob();
        spooler = taskContext.spooler();
        this.dotnetObject = dotnetObject;
    }

    @Override public boolean spooler_init() throws Exception {
        //return dotnetObject.spooler_init();   // Das ist .Net-Methode
        throw new NotImplementedError();
    }
}
