package com.sos.scheduler.engine.kernelcpptest.stress.order;

import org.junit.Test;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;

public final class OrderStressTest extends SchedulerTest {
    // In Maven setzen mit -DargLine=-DOrderStressTest.limit=26 (Surefire plugin 2.6), 2010-11-28
    // Zum Beispiel: mvn test -Dtest=OrderStressTest -DargLine=-DOrderStressTest.limit=26
    private static final int testLimit = Integer.parseInt(System.getProperty("OrderStressTest.limit", "100"));
    private int touchedOrderCount = 0;

    @Test public void test() throws Exception {
        controller().runScheduler(Time.of(3600));
    }
    
    @EventHandler public void handleEvent(OrderTouchedEvent e) {
        // OrderFinishedEvent wird nicht ausgelöst, weil der Auftrag vorher mit add_or_replace() ersetzt wird.
        touchedOrderCount++;
        if (touchedOrderCount > testLimit)
            controller().terminateScheduler();
    }

//    public void main(String[] args) throws Exception {
//        int limit = Integer.parseInt(args[0]);
//        new OrderStressTest().runTest(limit);
//    }
}
