package com.sos.scheduler.engine.tests.jira.js1642;

import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient;
import com.sos.scheduler.engine.data.filebased.FileBasedState;
import com.sos.scheduler.engine.data.jobchain.JobChainPath;
import com.sos.scheduler.engine.data.order.OrderOverview;
import com.sos.scheduler.engine.data.order.OrderState;
import java.time.Instant;
import java.util.Collection;
import java.util.Optional;
import java.util.concurrent.ExecutionException;
import static com.google.common.base.Throwables.propagate;
import static com.sos.scheduler.engine.common.javautils.ScalaInJava.asJavaFuture;
import static com.sos.scheduler.engine.common.javautils.ScalaInJava.toJavaOptional;
import static java.time.Instant.EPOCH;
import static org.junit.Assert.assertEquals;
import static scala.collection.JavaConversions.asJavaCollection;

/**
 * @author Joacim Zschimmer
 */
final class SchedulerClientJavaTester implements AutoCloseable {

    private static final JobChainPath aJobChainPath = new JobChainPath("/aJobChain");
    private final StandardWebSchedulerClient client;

    private SchedulerClientJavaTester(String schedulerUri) {
        this.client = new StandardWebSchedulerClient(schedulerUri);
    }

    public void close() {
        client.close();
    }

    private void run() {
        testOrderOverviews();
    }

    private void testOrderOverviews() {
        try {
            Collection<OrderOverview> orderOverviews = asJavaCollection(asJavaFuture(client.orderOverviews()).get());
            testPermanentOrderOverview(orderOverviews);
            testAdHocOrderOverview(orderOverviews);
        } catch (InterruptedException | ExecutionException e) { throw propagate(e); }
    }

    private void testPermanentOrderOverview(Collection<OrderOverview> orderOverviews) {
        OrderOverview orderOverview = orderOverviews.stream()
            .filter(o -> o.orderKey().equals(aJobChainPath.orderKey("1")))
            .findFirst().get();
        assertEquals(new OrderState("100"), orderOverview.orderState());
        assertEquals(FileBasedState.active, orderOverview.fileBasedState());
        assertEquals(Optional.of(EPOCH), toJavaOptional(orderOverview.nextStepAt()));
    }

    private void testAdHocOrderOverview(Collection<OrderOverview> orderOverviews) {
        OrderOverview orderOverview = orderOverviews.stream()
            .filter(o -> o.orderKey().equals(aJobChainPath.orderKey("AD-HOC")))
            .findFirst().get();
        assertEquals(new OrderState("100"), orderOverview.orderState());
        assertEquals(FileBasedState.notInitialized, orderOverview.fileBasedState());
        assertEquals(Optional.of(Instant.parse("2038-01-01T11:22:33Z")), toJavaOptional(orderOverview.nextStepAt()));
    }

    static void run(String schedulerUri) {
        try (SchedulerClientJavaTester tester = new SchedulerClientJavaTester(schedulerUri)) {
            tester.run();
        }
    }
}
