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
import static scala.collection.JavaConversions.asJavaIterable;

public final class OrderSubsystemOfJobIT extends SchedulerTest {
    @Test public void test() throws Exception {
        controller().activateScheduler();
        doTest();
        controller().terminateScheduler();
    }

    private void doTest() {
        JobSubsystem jobSubsystem = instance(JobSubsystem.class);
        OrderSubsystem orderSubsystem = instance(OrderSubsystem.class);

        Job aJob = jobSubsystem.job(JobPath.of("/A"));
        Job bJob = jobSubsystem.job(JobPath.of("/B"));
        JobChain aJobchain = orderSubsystem.jobChain(JobChainPath.of("/a"));
        JobChain abJobchain = orderSubsystem.jobChain(JobChainPath.of("/ab"));

        Iterable<JobChain> a = asJavaIterable(orderSubsystem.jobChainsOfJob(aJob));
        Iterable<JobChain> ab = asJavaIterable(orderSubsystem.jobChainsOfJob(bJob));

        assertThat(a, Matchers.containsInAnyOrder(aJobchain, abJobchain));
        assertThat(ab, Matchers.containsInAnyOrder(abJobchain));
    }
}
