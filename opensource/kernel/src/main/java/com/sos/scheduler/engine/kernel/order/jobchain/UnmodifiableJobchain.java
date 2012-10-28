package com.sos.scheduler.engine.kernel.order.jobchain;

import com.sos.scheduler.engine.data.folder.JobChainPath;
import com.sos.scheduler.engine.data.order.OrderId;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;

public interface UnmodifiableJobchain {
    String getName();
    JobChainPath getPath();
    boolean refersToJob(Job o);
    UnmodifiableOrder order(OrderId id);
}