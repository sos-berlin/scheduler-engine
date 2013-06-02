package com.sos.scheduler.engine.kernel.job;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.data.folder.JobPath;
import com.sos.scheduler.engine.kernel.folder.FileBasedSubsystem;

public interface JobSubsystem extends FileBasedSubsystem {
    Job job(JobPath path);
    ImmutableList<String> getNames();
    ImmutableList<String> getVisibleNames();
}
