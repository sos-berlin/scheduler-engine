package com.sos.scheduler.engine.kernel.order.jobchain;

import com.google.inject.Injector;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.kernel.cppproxy.Job_nodeC;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.scheduler.HasInjector;

public class JobNode extends OrderQueueNode {
    private final Job_nodeC cppProxy;

    private JobNode(Job_nodeC cppProxy, Injector injector) {
        super(cppProxy, injector);
        this.cppProxy = cppProxy;
    }

    public final Job getJob() {
        return cppProxy.job().getSister();
    }

    public static class Type implements SisterType<JobNode, Job_nodeC> {
        @Override public final JobNode sister(Job_nodeC proxy, Sister context) {
            return new JobNode(proxy, ((HasInjector)context).injector());
        }
    }
}
