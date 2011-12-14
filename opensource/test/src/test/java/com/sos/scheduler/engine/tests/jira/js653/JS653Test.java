package com.sos.scheduler.engine.tests.jira.js653;

import static java.lang.System.currentTimeMillis;
import static java.lang.Thread.sleep;

import java.util.HashSet;
import java.util.Set;

import org.apache.log4j.Logger;
import org.junit.Test;

import com.google.common.collect.ImmutableSet;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.test.SchedulerTest;

/** Ticket JS-653.
 * @see <a href='http://www.sos-berlin.com/jira/browse/JS-653'>JS-653</a>
 * @see com.sos.scheduler.engine.tests.jira.js803.JS803Test */
public final class JS653Test extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(JS653Test.class);
    private static final long idleTimeoutMs = 5*1000;
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

    @HotEventHandler public void handleEvent(OrderTouchedEvent e, UnmodifiableOrder o) {
        orderStarts.add(new OrderIdAndState(e.getKey().getId(), o.getState()));
    }

    @HotEventHandler public void handleEvent(OrderFinishedEvent e, UnmodifiableOrder o) {
        OrderKeyAndState a = new OrderKeyAndState(e.getKey(), o.getState());
        orderEnds.add(a);
        //logger.info("FINISHED: "+a);
        lastActivity = currentTimeMillis();
    }
}
