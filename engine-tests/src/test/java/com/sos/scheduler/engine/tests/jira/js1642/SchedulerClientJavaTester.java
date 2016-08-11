package com.sos.scheduler.engine.tests.jira.js1642;

import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient;
import com.sos.scheduler.engine.data.compounds.OrdersComplemented;
import com.sos.scheduler.engine.data.filebased.FileBasedState;
import com.sos.scheduler.engine.data.job.JobObstacle;
import com.sos.scheduler.engine.data.job.JobOverview;
import com.sos.scheduler.engine.data.job.JobPath;
import com.sos.scheduler.engine.data.job.JobState;
import com.sos.scheduler.engine.data.job.ProcessClassOverview;
import com.sos.scheduler.engine.data.job.TaskId;
import com.sos.scheduler.engine.data.job.TaskOverview;
import com.sos.scheduler.engine.data.job.TaskState;
import com.sos.scheduler.engine.data.jobchain.JobChainPath;
import com.sos.scheduler.engine.data.order.OrderOverview;
import com.sos.scheduler.engine.data.order.OrderSourceType;
import com.sos.scheduler.engine.data.jobchain.NodeId;
import com.sos.scheduler.engine.data.processclass.ProcessClassPath;
import com.sos.scheduler.engine.data.queries.OrderQuery;
import java.time.Instant;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.ExecutionException;
import java.util.stream.Collectors;
import scala.Option;
import scala.collection.immutable.Set;
import static com.google.common.base.Throwables.propagate;
import static com.sos.scheduler.engine.common.javautils.ScalaInJava.asJavaFuture;
import static com.sos.scheduler.engine.common.javautils.ScalaInJava.toJavaOptional;
import static com.sos.scheduler.engine.data.queries.PathQueries.toPathQuery;
import static java.time.Instant.EPOCH;
import static java.util.Arrays.asList;
import static java.util.Collections.singletonList;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsInAnyOrder;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static scala.collection.JavaConversions.asJavaCollection;
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
        testOrdersOrdersComplemented();
        testOrdersComplementedByQuery();
    }

    private void testOrderOverviews() {
        try {
            testOrderOverviews(seqAsJavaList(asJavaFuture(client.orderOverviews()).get().value()));
        } catch (InterruptedException | ExecutionException e) { throw propagate(e); }
    }

    private void testOrdersOrdersComplemented() {
        try {
            OrdersComplemented ordersComplemented = asJavaFuture(client.ordersComplemented()).get().value();
            testOrderOverviews(seqAsJavaList(ordersComplemented.orders()));
            List<TaskOverview> orderedTasks = seqAsJavaList(ordersComplemented.usedTasks()).stream()
                .sorted(SchedulerClientJavaTester::compareTaskOverview)
                .collect(Collectors.toList());
            assertEquals(orderedTasks, asList(
                new TaskOverview(new TaskId(3), new JobPath("/test"), TaskState.running, ProcessClassPath.Default(), Option.empty()),
                new TaskOverview(new TaskId(4), new JobPath("/test"), TaskState.running, ProcessClassPath.Default(), Option.empty()),
                new TaskOverview(new TaskId(5), new JobPath("/test"), TaskState.running, ProcessClassPath.Default(), Option.empty())));
            assertEquals(seqAsJavaList(ordersComplemented.usedJobs()),
                singletonList(
                    new JobOverview(
                        new JobPath("/test"),
                        FileBasedState.active,
                        Option.empty(),
                        JobState.running,
                        true,  // isInPeriod
                        10,    // taskLimit
                        3,
                        (Set<JobObstacle>)scala.collection.immutable.Set$.MODULE$.<JobObstacle>empty())));  // usedTaskCount
            assertEquals(seqAsJavaList(ordersComplemented.usedProcessClasses()), singletonList(
                new ProcessClassOverview(ProcessClassPath.Default(), FileBasedState.active, 30/*processLimit*/, 3/*usedProcessCount*/)));
        } catch (InterruptedException | ExecutionException e) { throw propagate(e); }
    }

    private void testOrdersComplementedByQuery() {
        try {
            OrderQuery query = OrderQuery.All()
                .withJobChainPathQuery(toPathQuery("/xFolder/"))
                .withIsDistributed(true)
                .withIsSuspended(false)
                .withOrderSourceTypes(singletonList(OrderSourceType.fileBased));
            OrdersComplemented ordersComplemented = asJavaFuture(client.ordersComplementedBy(query)).get().value();
            assertThat(
                asJavaCollection(ordersComplemented.orders()).stream().map(OrderOverview::orderKey).collect(Collectors.toList()),
                containsInAnyOrder(
                    new JobChainPath("/xFolder/x-bJobChain").orderKey("1")));
            assertTrue(ordersComplemented.usedTasks().isEmpty());
            assertTrue(ordersComplemented.usedJobs().isEmpty());
            assertTrue(ordersComplemented.usedProcessClasses().isEmpty());
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
        assertEquals(new NodeId("100"), orderOverview.nodeId());
        assertEquals(FileBasedState.active, orderOverview.fileBasedState());
        assertEquals(OrderSourceType.fileBased, orderOverview.sourceType());
        assertEquals(Optional.of(EPOCH), toJavaOptional(orderOverview.nextStepAt()));
    }

    private static void testAdHocOrderOverview(List<OrderOverview> orderOverviews) {
        OrderOverview orderOverview = orderOverviews.stream()
            .filter(o -> o.orderKey().equals(aJobChainPath.orderKey("AD-HOC")))
            .findFirst().get();
        assertEquals(new NodeId("100"), orderOverview.nodeId());
        assertEquals(FileBasedState.not_initialized, orderOverview.fileBasedState());
        assertEquals(OrderSourceType.adHoc, orderOverview.sourceType());
        assertEquals(Optional.of(Instant.parse("2038-01-01T11:22:33Z")), toJavaOptional(orderOverview.nextStepAt()));
    }

    private static int compareTaskOverview(TaskOverview a, TaskOverview b) {
        return Integer.compare(a.taskId().number(), b.taskId().number());
    }

    static void run(String schedulerUri) {
        try (SchedulerClientJavaTester tester = new SchedulerClientJavaTester(schedulerUri)) {
            tester.run();
        }
    }
}
