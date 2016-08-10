package com.sos.scheduler.engine.tests.jira.js653;

import com.google.common.collect.ImmutableSet;
import com.sos.scheduler.engine.data.order.OrderFinished;
import com.sos.scheduler.engine.data.order.OrderStarted;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

import java.util.HashSet;
import java.util.Set;

import static java.lang.System.currentTimeMillis;
import static java.lang.Thread.sleep;

/** Ticket JS-653.
 * <a href='http://www.sos-berlin.com/jira/browse/JS-653'>JS-653</a>
 */
public final class JS653IT extends SchedulerTest {
    private static final long idleTimeoutMs = 10*1000;
    private static final ImmutableSet<OrderIdAndState> expectedOrderStarts = ImmutableSet.of(
            OrderIdAndState.of("simpleShouldRun", "state.job1"),
            OrderIdAndState.of("simpleWithStateShouldRun", "state.job1"),
            OrderIdAndState.of("superShouldRun", "state.nestedA.job1"),
            OrderIdAndState.of("superWithStateBShouldRun", "state.nestedB.job1"));
    private static final ImmutableSet<OrderKeyAndState> expectedOrderEnds = ImmutableSet.of(
            OrderKeyAndState.of("/simple", "simpleShouldRun", "end"),
            OrderKeyAndState.of("/simple", "simpleWithStateShouldRun", "end"),
            OrderKeyAndState.of("/b", "superShouldRun", "end"),
            OrderKeyAndState.of("/b", "superWithStateBShouldRun", "end"));

    private final Set<OrderIdAndState> orderStarts = new HashSet<OrderIdAndState>();
    private final Set<OrderKeyAndState> orderEnds = new HashSet<OrderKeyAndState>();
    private volatile long lastActivity;

    @Test public void test() throws InterruptedException {
        controller().activateScheduler();
        waitUntilOrdersAreNotStarted();
        DifferenceChecker.assertNoDifference(expectedOrderStarts, orderStarts, "started");
        DifferenceChecker.assertNoDifference(expectedOrderEnds, orderEnds, "finished");
    }

    /* Warten bis wir einigermaßen sicher sind, dass Aufträge, die nicht starten sollen, nicht gestartet werden. **/
    private void waitUntilOrdersAreNotStarted() throws InterruptedException {
        lastActivity = currentTimeMillis();
        while(true) {
            long remaining = lastActivity + idleTimeoutMs - currentTimeMillis();
            if (remaining < 0) break;
            sleep(remaining);
        }
    }

    @HotEventHandler public void handleEvent(OrderStarted e, UnmodifiableOrder o) {
        orderStarts.add(new OrderIdAndState(e.orderKey().id(), o.state()));
    }

    @HotEventHandler public void handleEvent(OrderFinished e, UnmodifiableOrder o) {
        OrderKeyAndState a = new OrderKeyAndState(e.orderKey(), o.state());
        orderEnds.add(a);
        lastActivity = currentTimeMillis();
    }
}
