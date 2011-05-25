package com.sos.scheduler.engine.kernelcpptest.order.test1;

import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.kernel.main.event.EventThread;
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.kernel.test.JavascriptEventPredicateEngine;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import org.junit.Test;


public class ExampleWithJavascriptTest extends SchedulerTest {
    private static final Time eventTimeout = Time.of(10);

    @Test public void test() throws Exception {
        strictSubscribeEvents(new MyEventThread());
        runScheduler(shortTimeout);
    }


    private class MyEventThread extends EventThread {
        private final JavascriptEventPredicateEngine j = new JavascriptEventPredicateEngine();

        public MyEventThread() {
            setEventFilter(j.predicate(OrderStateChangedEvent.class, "event.order.id == 'id.1'"));
        }
        
        @Override protected void runEventThread() throws InterruptedException {
            expectEvent(eventTimeout, j.predicate(OrderStateChangedEvent.class, "event.order.state == 'state.2'"));
            expectEvent(eventTimeout, j.predicate(OrderStateChangedEvent.class, "event.order.state == 'state.end'"));
            Thread.sleep(1000);
        }
    }
}
