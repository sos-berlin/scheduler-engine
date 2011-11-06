package com.sos.scheduler.engine.kernelcpptest.jira.js653;

import static org.junit.Assert.fail;

import java.util.HashSet;
import java.util.Set;

import javax.annotation.Nullable;

import org.junit.Test;

import com.google.common.base.Joiner;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Sets;
import com.sos.scheduler.engine.kernel.event.EventHandler;
import com.sos.scheduler.engine.kernel.order.Order;
import com.sos.scheduler.engine.kernel.order.OrderId;
import com.sos.scheduler.engine.kernel.order.OrderState;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;

public final class JS653Test extends SchedulerTest {
    private static final Joiner commaJoiner = Joiner.on(", ");
    private static final ImmutableSet<OrderIdAndState> expectedOrders = ImmutableSet.of(
            new OrderIdAndState(new OrderId("simpleShouldRun"), new OrderState("state.job1")),
            new OrderIdAndState(new OrderId("superShouldRun"), new OrderState("state.nestedA.job1")),
            new OrderIdAndState(new OrderId("superWithStateBShouldRun"), new OrderState("state.nestedB.job1")));

    private final Set<OrderIdAndState> startedOrders = new HashSet<OrderIdAndState>();

    @Test public void test() {
        controller().runScheduler(Time.of(5), "-e");
        if (!startedOrders.equals(expectedOrders))
            fail(differenceMessage());
    }

    @EventHandler public void handleEvent(OrderTouchedEvent e) {
        Order o = e.getOrder();
        startedOrders.add(new OrderIdAndState(o.getId(), o.getState()));
    }

    private String differenceMessage() {
        return Joiner.on("; ").skipNulls().join(wrongStartedString(), wrongNotStartedString());
    }

    @Nullable private String wrongStartedString() {
        Set<OrderIdAndState> wrongStarted = Sets.difference(startedOrders, expectedOrders);
        return wrongStarted.isEmpty()? null : "Unexpectedly started orders: "+ commaJoiner.join(wrongStarted);
    }

    @Nullable private String wrongNotStartedString() {
        Set<OrderIdAndState> wrongNotStarted = Sets.difference(expectedOrders, startedOrders);
        return wrongNotStarted.isEmpty()? null : "Missing order starts: "+ commaJoiner.join(wrongNotStarted);
    }
}
