package com.sos.scheduler.engine.kernel.order;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.Iterables;
import com.sos.scheduler.engine.data.folder.JobChainPath;
import com.sos.scheduler.engine.data.order.OrderKey;
import com.sos.scheduler.engine.kernel.cppproxy.Order_subsystemC;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.scheduler.Subsystem;

import javax.annotation.Nullable;

import static com.sos.scheduler.engine.kernel.order.jobchain.JobChains.jobChainHasJob;

public class OrderSubsystem implements Subsystem {
    private final Order_subsystemC cppProxy;

    public OrderSubsystem(Order_subsystemC cppproxy) {
        this.cppProxy = cppproxy;
    }

    public final ImmutableList<JobChain> jobChains() {
        return ImmutableList.copyOf(cppProxy.java_file_baseds());
    }

    public final Iterable<JobChain> jobchainsOfJob(Job job) {
        return Iterables.filter(jobChains(), jobChainHasJob(job));
    }

    public final JobChain jobChain(JobChainPath o) {
        JobChain result = cppProxy.java_file_based_or_null(o.asString());
        if (result == null)
            throw new SchedulerException("Unknown '"+o+"'");
        return result;
    }

    @Nullable public final Order orderOrNull(OrderKey orderKey) {
        return jobChain(orderKey.getJobChainPath()).orderOrNull(orderKey.getId());
    }

    public final Order order(OrderKey orderKey) {
        return jobChain(orderKey.getJobChainPath()).order(orderKey.getId());
    }

    public  final void tryRemoveOrder(OrderKey k) {
        Order o = orderOrNull(k);
        if (o != null)
            o.remove();
    }
}
