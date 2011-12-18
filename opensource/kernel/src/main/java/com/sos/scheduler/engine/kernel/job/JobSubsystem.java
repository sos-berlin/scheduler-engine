package com.sos.scheduler.engine.kernel.job;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.kernel.cppproxy.Job_subsystemC;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.folder.FileBasedSubsystem;
import com.sos.scheduler.engine.kernel.scheduler.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.scheduler.Platform;

public final class JobSubsystem extends AbstractHasPlatform implements FileBasedSubsystem {
    private final Job_subsystemC cppproxy;
    
    public JobSubsystem(Platform platform, Job_subsystemC cppproxy) {
        super(platform);
        this.cppproxy = cppproxy;
    }
    
    public Job job(AbsolutePath path) {
        return cppproxy.job_by_string(path.getString()).getSister();
    }

    public ImmutableList<String> getNames() {
        return names(false);
    }

    public ImmutableList<String> getVisibleNames() {
        return names(true);
    }

    private ImmutableList<String> names(boolean visibleOnly) {
        ImmutableList.Builder<String> result = ImmutableList.builder();
        for (String name: cppproxy.file_based_names(visibleOnly))
            result.add(name);
        return result.build();
    }
}
