package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.*;


public class JobSubsystem extends AbstractHasPlatform {
    private final Job_subsystemC job_subsystemC;
    
    
    public JobSubsystem(Platform platform, Job_subsystemC job_subsystemC) {
        super(platform);
        this.job_subsystemC = job_subsystemC;
    }
    

    public Job job(String absolutePath) { return job_subsystemC.job_by_string(absolutePath).getSister(); }
}
