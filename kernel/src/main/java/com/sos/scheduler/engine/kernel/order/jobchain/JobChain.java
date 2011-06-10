package com.sos.scheduler.engine.kernel.order.jobchain;

import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.Job_chainC;
import com.sos.scheduler.engine.kernel.folder.FileBased;
import com.sos.scheduler.engine.kernel.order.Order;
import com.sos.scheduler.engine.kernel.order.OrderId;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import java.util.List;


public final class JobChain extends FileBased {
    private final Job_chainC jobChainC;


    private JobChain(Platform platform, Job_chainC jobChainC) {
        super(platform);
        this.jobChainC = jobChainC;
    }

    
    @Override public void onCppProxyInvalidated() {}


    public String getName() { 
        return jobChainC.name();
    }


    public List<Node> getNodes() { 
        return jobChainC.java_nodes();
    }


    public Order getOrder(OrderId id) {
        return jobChainC.order(id.getString()).getSister();
    }


    public static class Type implements SisterType<JobChain, Job_chainC> {
        @Override public final JobChain sister(Job_chainC proxy, Sister context) { return new JobChain(Platform.of(context), proxy); }
    }
}
