package com.sos.scheduler.engine.kernelcpptest.order.test1;

import org.junit.Test;

import com.sos.scheduler.engine.kernel.main.event.EventThread;
import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent;
import com.sos.scheduler.engine.test.JavascriptEventPredicateEngine;
import com.sos.scheduler.engine.test.SchedulerTest;

public class ExampleWithJavascriptTest extends SchedulerTest {
    @Test public void test() throws Exception {
        controller().subscribeEvents(new MyEventThread());
        controller().runScheduler(shortTimeout);
    }

    private class MyEventThread extends EventThread {
        private final JavascriptEventPredicateEngine j = new JavascriptEventPredicateEngine();

        MyEventThread() {
            super(controller());
            setEventFilter(j.predicate(OrderStateChangedEvent.class, "event.order.id == 'id.1'"));
        }
        
        @Override protected void runEventThread() throws InterruptedException {
            expectEvent(shortTimeout, j.predicate(OrderStateChangedEvent.class, "event.order.state == 'state.2'"));
            expectEvent(shortTimeout, j.predicate(OrderStateChangedEvent.class, "event.order.state == 'state.end'"));
            Thread.sleep(1000);
        }
    }
}
