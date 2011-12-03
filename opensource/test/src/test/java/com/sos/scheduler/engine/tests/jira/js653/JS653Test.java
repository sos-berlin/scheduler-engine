package com.sos.scheduler.engine.tests.jira.js653;

import static java.lang.System.currentTimeMillis;
import static java.lang.Thread.sleep;
import static org.junit.Assert.fail;

import java.util.HashSet;
import java.util.Set;

import javax.annotation.Nullable;

import org.junit.Test;

import com.google.common.base.Joiner;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Sets;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.Order;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.OrderId;
import com.sos.scheduler.engine.kernel.order.OrderState;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;

/** @see <a href='http://www.sos-berlin.com/jira/browse/JS-653'>JS-653</a> */
public final class JS653Test extends SchedulerTest {
    private static final long idleTimeoutMs = 5*1000;
    private static final Joiner commaJoiner = Joiner.on(", ");
    private static final ImmutableSet<OrderIdAndState> expectedOrderStarts = ImmutableSet.of(
            new OrderIdAndState(new OrderId("simpleShouldRun"), new OrderState("state.job1")),
            new OrderIdAndState(new OrderId("superShouldRun"), new OrderState("state.nestedA.job1")),
            new OrderIdAndState(new OrderId("superWithStateBShouldRun"), new OrderState("state.nestedB.job1")));

    private final Set<OrderIdAndState> orderStarts = new HashSet<OrderIdAndState>();
    private volatile long lastActivity;

    @Test public void test() throws InterruptedException {
        controller().activateScheduler("-e");
        waitUntilOrdersAreNotStarted();
        controller().terminateScheduler();
        if (!orderStarts.equals(expectedOrderStarts))
            fail(differenceMessage());
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

    @HotEventHandler public void handleEvent(OrderTouchedEvent e, Order o) {
        orderStarts.add(new OrderIdAndState(o.getId(), o.getState()));
    }

    @HotEventHandler public void handleEvent(OrderFinishedEvent e) {
        lastActivity = currentTimeMillis();
    }

    private String differenceMessage() {
        return Joiner.on("; ").skipNulls().join(wrongStartedString(), wrongNotStartedString());
    }

    @Nullable private String wrongStartedString() {
        Set<OrderIdAndState> wrongStarted = Sets.difference(orderStarts, expectedOrderStarts);
        return wrongStarted.isEmpty()? null : "Unexpectedly started orders: "+ commaJoiner.join(wrongStarted);
    }

    @Nullable private String wrongNotStartedString() {
        Set<OrderIdAndState> wrongNotStarted = Sets.difference(expectedOrderStarts, orderStarts);
        return wrongNotStarted.isEmpty()? null : "Missing order starts: "+ commaJoiner.join(wrongNotStarted);
    }
}
