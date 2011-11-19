package com.sos.scheduler.engine.kernel.order.jobchain;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.Job_nodeC;
import com.sos.scheduler.engine.kernel.job.Job;

public class JobNode extends OrderQueueNode {
    private final Job_nodeC cppProxy;

    JobNode(Platform platform, Job_nodeC cppProxy) {
        super(platform, cppProxy);
        this.cppProxy = cppProxy;
    }

    public final Job getJob() {
        return cppProxy.job().getSister();
    }

    public static class Type implements SisterType<JobNode, Job_nodeC> {
        @Override public final JobNode sister(Job_nodeC proxy, Sister context) {
            return new JobNode(Platform.of(context), proxy);
        }
    }
}
