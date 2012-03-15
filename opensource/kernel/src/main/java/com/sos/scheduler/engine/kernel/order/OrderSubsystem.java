package com.sos.scheduler.engine.kernel.order;

import static com.sos.scheduler.engine.kernel.order.jobchain.JobChains.jobChainHasJob;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.Iterables;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.scheduler.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.kernel.scheduler.Subsystem;
import com.sos.scheduler.engine.kernel.cppproxy.*;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;

//Aufteilen in JobChainSubsystem und OrderSubsystem?

public class OrderSubsystem extends AbstractHasPlatform implements Subsystem
{
    private final Order_subsystemC cppProxy;

    public OrderSubsystem(Platform platform, Order_subsystemC cppproxy) {
        super(platform);
        this.cppProxy = cppproxy;
    }

    public final ImmutableList<JobChain> jobChains() {
        return ImmutableList.copyOf(cppProxy.java_file_baseds());
    }

    public Iterable<JobChain> jobchainsOfJob(Job job) {
        return Iterables.filter(jobChains(), jobChainHasJob(job));
    }

    public final JobChain jobChain(AbsolutePath path) {
        JobChain result = cppProxy.java_file_based_or_null(path.toString());
        if (result == null)
            throw new SchedulerException("Unknown job chain '"+path+"'");
        return result;
    }
}
