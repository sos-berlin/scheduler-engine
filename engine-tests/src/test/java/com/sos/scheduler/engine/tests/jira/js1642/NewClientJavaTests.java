package com.sos.scheduler.engine.tests.jira.js1642;

import com.sos.scheduler.engine.data.filebased.FileBasedState;
import com.sos.scheduler.engine.data.jobchain.JobChainPath;
import com.sos.scheduler.engine.data.order.OrderOverview;
import com.sos.scheduler.engine.data.order.OrderState;
import com.sos.scheduler.engine.client.WebSchedulerClient;
import java.time.Instant;
import java.util.Collection;
import java.util.Optional;
import static com.sos.scheduler.engine.common.javautils.ScalaInJava.asJavaFuture;
import static com.sos.scheduler.engine.common.javautils.ScalaInJava.toJavaOptional;
import static java.time.Instant.EPOCH;
import static org.junit.Assert.assertEquals;
import static scala.collection.JavaConversions.asJavaCollection;

/**
 * @author Joacim Zschimmer
 */
final class NewClientJavaTests implements AutoCloseable {

    private final WebSchedulerClient.Standard client;

    NewClientJavaTests(String schedulerUri) {
        this.client = new WebSchedulerClient.Standard(schedulerUri);
    }

    public void close() {
        client.close();
    }

    void test() throws Exception {
        testOrderOverviews();
    }

    private void testOrderOverviews() throws Exception {
        Collection<OrderOverview> orderOverviews = asJavaCollection(asJavaFuture(client.orderOverviews()).get());
        testPermanentOrderOverview(orderOverviews);
        testAdHocOrderOverview(orderOverviews);
    }

    private void testPermanentOrderOverview(Collection<OrderOverview> orderOverviews) {
        OrderOverview orderOverview = orderOverviews.stream().filter(o ->
            o.path().equals(new JobChainPath("/aJobChain").orderKey("1"))
        ).findFirst().get();
        assertEquals(orderOverview.orderState(), new OrderState("100"));
        assertEquals(orderOverview.fileBasedState(), FileBasedState.active);
        assertEquals(toJavaOptional(orderOverview.nextStepAt()), Optional.of(EPOCH));
    }

    private void testAdHocOrderOverview(Collection<OrderOverview> orderOverviews) {
        OrderOverview orderOverview = orderOverviews.stream().filter(o ->
            o.path().equals(new JobChainPath("/aJobChain").orderKey("AD-HOC"))
        ).findFirst().get();
        assertEquals(orderOverview.orderState(), new OrderState("100"));
        assertEquals(orderOverview.fileBasedState(), FileBasedState.notInitialized);
        assertEquals(toJavaOptional(orderOverview.nextStepAt()), Optional.of(Instant.parse("2038-01-01T11:22:33Z")));
    }
}
