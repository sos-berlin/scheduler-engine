package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.folder.FileBasedState;
import com.sos.scheduler.engine.kernel.log.PrefixLog;

public interface UnmodifiableJob {
    String getName();
    AbsolutePath getPath();
    FileBasedState getFileBasedState();
    boolean isFileBasedReread();
    PrefixLog getLog();
}
