package com.sos.scheduler.engine.kernel.order.jobchain;

import com.google.common.base.Predicate;
import com.sos.scheduler.engine.kernel.job.Job;

public class JobChains {
    private JobChains() {}

    public static Predicate<JobChain> jobChainHasJob(final Job job) {
        return new Predicate<JobChain>() {
            @Override public boolean apply(JobChain jobChain) {
                for (Node node: jobChain.getNodes()) {
                     if (node instanceof JobNode) {
                         JobNode n = (JobNode)node;
                         if (n.getJob() == job) return true;
                     }
                }
                return false;
            }
        };
    }
}
