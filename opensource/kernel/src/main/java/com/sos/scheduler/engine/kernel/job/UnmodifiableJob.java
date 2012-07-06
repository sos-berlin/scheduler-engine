package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.data.folder.JobPath;
import com.sos.scheduler.engine.kernel.folder.FileBasedState;
import com.sos.scheduler.engine.kernel.log.PrefixLog;

public interface UnmodifiableJob {
    String getName();
    JobPath getPath();
    FileBasedState getFileBasedState();
    boolean isFileBasedReread();
    PrefixLog getLog();
}
