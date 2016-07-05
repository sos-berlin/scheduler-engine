package com.sos.scheduler.engine.tests.jira.js1642;

import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient;
import com.sos.scheduler.engine.data.agent.AgentAddress;
import com.sos.scheduler.engine.data.compounds.OrdersFullOverview;
import com.sos.scheduler.engine.data.filebased.FileBasedState;
import com.sos.scheduler.engine.data.job.JobOverview;
import com.sos.scheduler.engine.data.job.JobPath;
import com.sos.scheduler.engine.data.job.JobState;
import com.sos.scheduler.engine.data.job.ProcessClassOverview;
import com.sos.scheduler.engine.data.job.TaskId;
import com.sos.scheduler.engine.data.job.TaskOverview;
import com.sos.scheduler.engine.data.job.TaskState;
import com.sos.scheduler.engine.data.jobchain.JobChainPath;
import com.sos.scheduler.engine.data.order.OrderOverview;
import com.sos.scheduler.engine.data.order.OrderState;
import com.sos.scheduler.engine.data.processclass.ProcessClassPath;
import java.time.Instant;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.ExecutionException;
import java.util.stream.Collectors;
import scala.Option;
import static com.google.common.base.Throwables.propagate;
import static com.sos.scheduler.engine.common.javautils.ScalaInJava.asJavaFuture;
import static com.sos.scheduler.engine.common.javautils.ScalaInJava.toJavaOptional;
import static java.time.Instant.EPOCH;
import static java.util.Arrays.asList;
import static java.util.Collections.singletonList;
import static org.junit.Assert.assertEquals;
import static scala.collection.JavaConversions.seqAsJavaList;

/**
 * @author Joacim Zschimmer
 */
final class SchedulerClientJavaTester implements AutoCloseable {

    private static final JobChainPath aJobChainPath = new JobChainPath("/aJobChain");
    private final StandardWebSchedulerClient client;

    private SchedulerClientJavaTester(String schedulerUri) {
        this.client = new StandardWebSchedulerClient(schedulerUri);
    }

    @Override
    public void close() {
        client.close();
    }

    private void run() {
        testOrderOverviews();
        testOrdersFullOverview();
    }

    private void testOrderOverviews() {
        try {
            testOrderOverviews(seqAsJavaList(asJavaFuture(client.orderOverviews()).get()));
        } catch (InterruptedException | ExecutionException e) { throw propagate(e); }
    }

    private void testOrdersFullOverview() {
        try {
            OrdersFullOverview fullOverview = asJavaFuture(client.ordersFullOverview()).get();
            testOrderOverviews(seqAsJavaList(fullOverview.orders()));
            List<TaskOverview> orderedTasks = seqAsJavaList(fullOverview.usedTasks()).stream()
                .sorted(SchedulerClientJavaTester::compareTaskOverview)
                .collect(Collectors.toList());
            assertEquals(orderedTasks, asList(
                new TaskOverview(new TaskId(3), new JobPath("/test"), TaskState.running, ProcessClassPath.Default(), Option.<AgentAddress>empty()),
                new TaskOverview(new TaskId(4), new JobPath("/test"), TaskState.running, ProcessClassPath.Default(), Option.<AgentAddress>empty()),
                new TaskOverview(new TaskId(5), new JobPath("/test"), TaskState.running, ProcessClassPath.Default(), Option.<AgentAddress>empty())));
            assertEquals(seqAsJavaList(fullOverview.usedJobs()),
                singletonList(
                    new JobOverview(
                        new JobPath("/test"),
                        FileBasedState.active,
                        Option.<ProcessClassPath>empty(),
                        JobState.running,
                        true,  // isInPeriod
                        10,    // taskLimit
                        3)));  // usedTaskCount
            assertEquals(seqAsJavaList(fullOverview.usedProcessClasses()), singletonList(
                new ProcessClassOverview(ProcessClassPath.Default(), FileBasedState.active, 30/*processLimit*/, 3/*usedProcessCount*/)));
        } catch (InterruptedException | ExecutionException e) { throw propagate(e); }
    }

    private static void testOrderOverviews(List<OrderOverview> orderOverviews) {
        testPermanentOrderOverview(orderOverviews);
        testAdHocOrderOverview(orderOverviews);
    }

    private static void testPermanentOrderOverview(List<OrderOverview> orderOverviews) {
        OrderOverview orderOverview = orderOverviews.stream()
            .filter(o -> o.orderKey().equals(aJobChainPath.orderKey("1")))
            .findFirst().get();
        assertEquals(new OrderState("100"), orderOverview.orderState());
        assertEquals(FileBasedState.active, orderOverview.fileBasedState());
        assertEquals(Optional.of(EPOCH), toJavaOptional(orderOverview.nextStepAt()));
    }

    private static void testAdHocOrderOverview(List<OrderOverview> orderOverviews) {
        OrderOverview orderOverview = orderOverviews.stream()
            .filter(o -> o.orderKey().equals(aJobChainPath.orderKey("AD-HOC")))
            .findFirst().get();
        assertEquals(new OrderState("100"), orderOverview.orderState());
        assertEquals(FileBasedState.notInitialized, orderOverview.fileBasedState());
        assertEquals(Optional.of(Instant.parse("2038-01-01T11:22:33Z")), toJavaOptional(orderOverview.nextStepAt()));
    }

    private static int compareTaskOverview(TaskOverview a, TaskOverview b) {
        return Integer.compare(a.id().number(), b.id().number());
    }

    static void run(String schedulerUri) {
        try (SchedulerClientJavaTester tester = new SchedulerClientJavaTester(schedulerUri)) {
            tester.run();
        }
    }
}
