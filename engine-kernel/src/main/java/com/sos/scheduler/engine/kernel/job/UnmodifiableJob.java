package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.data.filebased.FileBasedState;
import com.sos.scheduler.engine.data.job.JobPath;
import com.sos.scheduler.engine.kernel.log.PrefixLog;

public interface UnmodifiableJob {
    String name();
    JobPath path();
    FileBasedState fileBasedState();
    boolean fileBasedIsReread();
    PrefixLog log();
}
