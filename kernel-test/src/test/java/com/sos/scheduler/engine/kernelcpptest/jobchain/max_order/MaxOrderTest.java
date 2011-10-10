package com.sos.scheduler.engine.kernelcpptest.jobchain.max_order;

import static java.lang.Math.max;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.lessThanOrEqualTo;

import org.apache.log4j.Logger;
import org.junit.Test;

import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.OrderTouchedEvent;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;


public class MaxOrderTest extends SchedulerTest {
    private static final int maxOrders = 3;         // Derselbe Wert wie <job_chain max_orders="">
    private static final int addedOrderCount = 9;   // Anzahl der <add_order>
    private static final Logger log = Logger.getLogger(MaxOrderTest.class);


    @Test public void test() throws Exception {
        controller().strictSubscribeEvents(new MyEventSubscriber());
        controller().runScheduler(shortTimeout);
    }


    class MyEventSubscriber implements EventSubscriber {
        private int touchedOrderCount = 0;
        private int maxTouchedOrderCount = 0;
        private int finishedOrderCount = 0;
        
        @Override public void onEvent(Event e) {
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
            controller().terminateScheduler();
        }

        private void check() {
            assertThat("touchedOrderCount != 0", touchedOrderCount, equalTo(0));
            assertThat(maxTouchedOrderCount, lessThanOrEqualTo(maxOrders));
            assertThat(maxTouchedOrderCount, equalTo(maxOrders));   // Strenger, das Maximum soll genutzt worden sein
        }
    }
}
