package com.sos.scheduler.engine.tests.order.ordersubsystemofjob;

import com.sos.scheduler.engine.data.folder.JobChainPath;
import com.sos.scheduler.engine.data.folder.JobPath;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.job.JobSubsystem;
import com.sos.scheduler.engine.kernel.order.OrderSubsystem;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.hamcrest.Matchers;
import org.junit.Test;

import static org.junit.Assert.assertThat;

public final class OrderSubsystemOfJobTest extends SchedulerTest {
    @Test public void test() throws Exception {
        controller().startScheduler();
        doTest();
        controller().terminateScheduler();
    }

    private void doTest() {
        JobSubsystem jobSubsystem = scheduler().getJobSubsystem();
        OrderSubsystem orderSubsystem = scheduler().getOrderSubsystem();

        Job aJob = jobSubsystem.job(JobPath.of("/A"));
        Job bJob = jobSubsystem.job(JobPath.of("/B"));
        JobChain aJobchain = orderSubsystem.jobChain(JobChainPath.of("/a"));
        JobChain abJobchain = orderSubsystem.jobChain(JobChainPath.of("/ab"));

        Iterable<JobChain> a = orderSubsystem.jobchainsOfJob(aJob);
        Iterable<JobChain> ab = orderSubsystem.jobchainsOfJob(bJob);

        assertThat(a, Matchers.containsInAnyOrder(aJobchain, abJobchain));
        assertThat(ab, Matchers.containsInAnyOrder(abJobchain));
    }
}
