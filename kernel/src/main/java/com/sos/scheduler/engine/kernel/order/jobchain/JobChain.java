package com.sos.scheduler.engine.kernel.order.jobchain;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.Job_chainC;
import com.sos.scheduler.engine.kernel.folder.FileBased;
import com.sos.scheduler.engine.kernel.order.OrderId;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;


public final class JobChain extends FileBased implements UnmodifiableJobchain {
    private final Job_chainC jobChainC;


    private JobChain(Platform platform, Job_chainC jobChainC) {
        super(platform);
        this.jobChainC = jobChainC;
    }
    
    @Override public void onCppProxyInvalidated() {
    }

	public String getName() { 
        return jobChainC.name();
    }

	public ImmutableList<Node> getNodes() { 
        return ImmutableList.copyOf(jobChainC.java_nodes());
    }

	public UnmodifiableOrder getUnmodifiableOrder(OrderId id) {
        return getOrder(id);
    }

	public UnmodifiableOrder getOrder(OrderId id) {
        return jobChainC.order(id.getString()).getSister();
    }

    public static class Type implements SisterType<JobChain, Job_chainC> {
        @Override public final JobChain sister(Job_chainC proxy, Sister context) { return new JobChain(Platform.of(context), proxy); }
    }
}
