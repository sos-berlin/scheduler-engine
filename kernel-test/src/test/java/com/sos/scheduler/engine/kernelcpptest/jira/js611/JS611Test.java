package com.sos.scheduler.engine.kernelcpptest.jira.js611;

import org.junit.Test;

import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.test.SchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;

//TODO Wie prüfen wir das? Erstmal durch Blick ins scheduler.log: {JS-611}.
/** Test, ob Scheduler Log-Dateien verübergehen schließt, wenn mehr Logs als festgelegt geschrieben werden. */
public class JS611Test extends SchedulerTest {
    //private static final Logger logger = Logger.getLogger(JS611Test.class);
	private static final int orderCount = 10;
    private static final Time myTimeout = Time.of(orderCount * 1.1);


    @Test public void test() throws Exception {
        MyEventSubscriber eventSubscriber = new MyEventSubscriber();
        strictSubscribeEvents(eventSubscriber);
        startScheduler("-e");
        scheduler().executeXml("<scheduler_log.log_categories.set category='JS-611'/>");
        addOrders(orderCount);
        waitForTermination(myTimeout);
    }
    
    private void addOrders(int n) {
    	for (int i = 0; i < n; i++)
    		scheduler().executeXml("<add_order job_chain='/A' id='" + i + "'/>");
    }


    private class MyEventSubscriber implements EventSubscriber {
        @Override public void onEvent(Event event) {
        }
    }
}
