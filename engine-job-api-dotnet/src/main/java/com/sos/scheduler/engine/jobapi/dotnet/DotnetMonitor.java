package com.sos.scheduler.engine.jobapi.dotnet;

import com.sos.scheduler.engine.jobapi.dotnet.api.TaskContext;
import scala.NotImplementedError;

/**
 * @author Joacim Zschimmer
 */
public final class DotnetMonitor extends sos.spooler.Monitor_impl {
    private final Object dotnetObject;

    DotnetMonitor(TaskContext taskContext, Object dotnetObject) {
        spooler_log = taskContext.spoolerLog();
        spooler_task = taskContext.spoolerTask();
        spooler_job = taskContext.spoolerJob();
        spooler = taskContext.spooler();
        this.dotnetObject = dotnetObject;
    }

    @Override
    public boolean spooler_task_before() throws Exception {
        //return dotnetObject.spooler_task_before();
        throw new NotImplementedError();
    }
}
