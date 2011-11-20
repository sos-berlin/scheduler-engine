package com.sos.scheduler.engine.tests.jobchain.max_order;

import static java.lang.Math.max;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.lessThanOrEqualTo;

import org.junit.Test;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;

public class MaxOrderTest extends SchedulerTest {
    private static final int maxOrders = 3;         // Derselbe Wert wie <job_chain max_orders="">
    private static final int addedOrderCount = 9;   // Anzahl der <add_order>

    private int runningOrderCount = 0;
    private int maxTouchedOrderCount = 0;
    private int finishedOrderCount = 0;

    @Test public void test() throws Exception {
        controller().runScheduler(shortTimeout);
    }

    @EventHandler public void handleEvent(OrderTouchedEvent e) {
        runningOrderCount++;
        maxTouchedOrderCount = max(maxTouchedOrderCount, runningOrderCount);
    }

    @EventHandler public void handleEvent(OrderFinishedEvent e) {
        runningOrderCount--;
        finishedOrderCount++;
        if (finishedOrderCount == addedOrderCount)
            finish();
    }

    private void finish() {
        check();
        controller().terminateScheduler();
    }

    private void check() {
        assertThat("runningOrderCount != 0", runningOrderCount, equalTo(0));
        assertThat(maxTouchedOrderCount, lessThanOrEqualTo(maxOrders));
        assertThat(maxTouchedOrderCount, equalTo(maxOrders));   // Strenger, das Maximum soll genutzt worden sein
    }
}
