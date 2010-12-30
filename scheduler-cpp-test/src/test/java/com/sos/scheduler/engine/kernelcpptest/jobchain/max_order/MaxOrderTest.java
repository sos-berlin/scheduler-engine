package com.sos.scheduler.engine.kernelcpptest.jobchain.max_order;

import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernelcpptest.SchedulerTest;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;
import org.junit.Test;
import org.apache.log4j.*;
import static java.lang.Math.max;
import static org.junit.Assert.*;


public class MaxOrderTest extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(MaxOrderTest.class);
    private static final Time eventTimeout = Time.of(5);
    private static final int maxOrders = 3;         // Derselbe Wert wie <job_chain max_orders="">
    private static final int addedOrderCount = 9;   // Anzahl der <add_order>


    @Test public void test() throws Exception {
        strictSubscribeEvents(new MyEventSubscriber());
        runScheduler(shortTimeout);
    }


    class MyEventSubscriber implements EventSubscriber {
        private int touchedOrderCount = 0;
        private int maxTouchedOrderCount = 0;
        private int finishedOrderCount = 0;
        
        @Override public void onEvent(Event e) {
            //logger.info(e);
            if (e instanceof OrderTouchedEvent) {
                touchedOrderCount++;
                maxTouchedOrderCount = max(maxTouchedOrderCount, touchedOrderCount);
            }
            else
            if (e instanceof OrderFinishedEvent) {
                touchedOrderCount--;
                finishedOrderCount++;
                if (finishedOrderCount == addedOrderCount)
                    finish();
            }
        }

        private void finish() {
            check();
            schedulerController.terminateScheduler();
        }

        private void check() {
            assert touchedOrderCount >= 0 : "touchedOrderCount >= 0 failed, activeOrderCount=" + touchedOrderCount;
            assert touchedOrderCount <= maxOrders : "touchedOrderCount=" + touchedOrderCount + " > maxOrders=" + maxOrders;
            assertEquals(maxOrders, maxTouchedOrderCount);
        }
    }
}
