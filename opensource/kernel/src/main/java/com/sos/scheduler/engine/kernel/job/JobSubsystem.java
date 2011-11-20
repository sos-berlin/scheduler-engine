package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.kernel.scheduler.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.scheduler.Subsystem;
import com.sos.scheduler.engine.kernel.cppproxy.*;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;

public final class JobSubsystem extends AbstractHasPlatform implements Subsystem {
    private final Job_subsystemC cppproxy;
    
    
    public JobSubsystem(Platform platform, Job_subsystemC cppproxy) {
        super(platform);
        this.cppproxy = cppproxy;
    }
    

    public Job job(AbsolutePath path) {
        return cppproxy.job_by_string(path.getString()).getSister();
    }
}
