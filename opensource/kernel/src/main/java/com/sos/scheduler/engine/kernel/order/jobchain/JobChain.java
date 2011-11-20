package com.sos.scheduler.engine.kernel.order.jobchain;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.Job_chainC;
import com.sos.scheduler.engine.kernel.folder.FileBased;
import com.sos.scheduler.engine.kernel.order.Order;
import com.sos.scheduler.engine.kernel.order.OrderId;

public final class JobChain extends FileBased implements UnmodifiableJobchain {
    private final Job_chainC cppProxy;

    private JobChain(Platform platform, Job_chainC cppProxy) {
        super(platform);
        this.cppProxy = cppProxy;
    }
    
    @Override public void onCppProxyInvalidated() {}

    public String getName() {
        return cppProxy.name();
    }

//    @Override public AbsolutePath getPath() {
//        return cppProxy.getPath();
//    }

    /** Markiert, dass das {@link FileBased} beim nächsten Verzeichnisabgleich neu geladen werden soll. */
    public void setForceFileReread() {
        cppProxy.set_force_file_reread();
    }

	public ImmutableList<Node> getNodes() { 
        return ImmutableList.copyOf(cppProxy.java_nodes());
    }

	public Order order(OrderId id) {
        return cppProxy.order(id.getString()).getSister();
    }

    @Override public String toString() {
        return JobChain.class.getSimpleName() + " " + getName();
    }

    public static class Type implements SisterType<JobChain, Job_chainC> {
        @Override public final JobChain sister(Job_chainC proxy, Sister context) {
            return new JobChain(Platform.of(context), proxy);
        }
    }
}
