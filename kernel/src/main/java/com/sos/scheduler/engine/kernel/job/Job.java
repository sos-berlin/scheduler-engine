package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.JobC;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;


@ForCpp
public final class Job extends AbstractHasPlatform implements Sister {
    private final JobC cppproxy;

    
    private Job(Platform platform, JobC jobC) {
        super(platform);
        this.cppproxy = jobC;
    }


    @Override public void onCppProxyInvalidated() {}


    public String getName() {
        return cppproxy.name();
    }


    public String getPath() {
        return cppproxy.path();
    }


    public static class Type implements SisterType<Job, JobC> {
        @Override public final Job sister(JobC proxy, Sister context) { return new Job(Platform.of(context), proxy); }
    }
}
