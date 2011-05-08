package com.sos.scheduler.engine.kernelcpptest.stress.order;

import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernelcpptest.SchedulerTest;
import org.junit.Test;


public class OrderStressTest extends SchedulerTest {
    // In Maven setzen mit -DargLine=-DOrderStressTest.limit=26 (Surefire plugin 2.6), 2010-11-28
    // Zum Beispiel: mvn test -Dtest=OrderStressTest -DargLine=-DOrderStressTest.limit=26
    private static final int testLimit = Integer.parseInt(System.getProperty("OrderStressTest.limit", "100"));


    @Test public void test() throws Exception {
        runTest(testLimit);
    }


    private void runTest(int limit) {
        strictSubscribeEvents(new MyEventSubscriber(limit));
        runScheduler(Time.of(3600));
    }
    

    private class MyEventSubscriber implements EventSubscriber {
        private final int limit;
        private int touchedOrderCount = 0;
        
        public MyEventSubscriber(int limit) { 
            this.limit = limit;
        }

        @Override public void onEvent(Event e) {
            if (e instanceof OrderTouchedEvent) {   // OrderFinishedEvent wird nicht ausgelöst, weil der Auftrag vorher mit add_or_replace() ersetzt wird.
                touchedOrderCount++;
                if (touchedOrderCount > limit)
                    schedulerController.terminateScheduler();
            }
        }
    }


    public void main(String[] args) throws Exception {
        int limit = Integer.parseInt(args[0]);
        new OrderStressTest().runTest(limit);
    }
}
