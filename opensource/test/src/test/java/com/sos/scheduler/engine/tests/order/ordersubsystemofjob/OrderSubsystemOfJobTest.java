package com.sos.scheduler.engine.tests.order.ordersubsystemofjob;

import static org.junit.Assert.assertThat;

import org.hamcrest.Matchers;
import org.junit.Test;

import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.job.JobSubsystem;
import com.sos.scheduler.engine.kernel.order.OrderSubsystem;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.test.SchedulerTest;

public class OrderSubsystemOfJobTest extends SchedulerTest {
    @Test public void test() throws Exception {
        controller().startScheduler();
        doTest();
        controller().terminateScheduler();
    }

    private void doTest() {
        JobSubsystem jobSubsystem = scheduler().getJobSubsystem();
        OrderSubsystem orderSubsystem = scheduler().getOrderSubsystem();

        Job aJob = jobSubsystem.job(new AbsolutePath("/A"));
        Job bJob = jobSubsystem.job(new AbsolutePath("/B"));
        JobChain aJobchain = orderSubsystem.jobChain(new AbsolutePath("/a"));
        JobChain abJobchain = orderSubsystem.jobChain(new AbsolutePath("/ab"));

        Iterable<JobChain> a = orderSubsystem.jobchainsOfJob(aJob);
        Iterable<JobChain> ab = orderSubsystem.jobchainsOfJob(bJob);

        assertThat(a, Matchers.containsInAnyOrder(aJobchain, abJobchain));
        assertThat(ab, Matchers.containsInAnyOrder(abJobchain));
    }
}
