package com.sos.scheduler.engine.tests.jira.js611;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.util.Time;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

//TODO Wie prüfen wir das? Erstmal durch Blick ins scheduler.log: {JS-611}.
/** Test, ob Scheduler Log-Dateien verübergehen schließt, wenn mehr Logs als festgelegt geschrieben werden.
 * @see <a href="http://www.sos-berlin.com/jira/browse/JS-611">JS-611</a> */
public class JS611Test extends SchedulerTest {
    //private static final Logger logger = Logger.getLogger(JS611Test.class);
	private static final int maxOrderCount = 10;
    private static final Time myTimeout = Time.of(maxOrderCount * 1.1);
    private int finishedOrderCount = 0;

    @Test public void test() throws Exception {
        controller().startScheduler();
        scheduler().executeXml("<scheduler_log.log_categories.set category='JS-611'/>");
        addOrders(maxOrderCount);
        controller().waitForTermination(myTimeout);
    }
    
    private void addOrders(int n) {
    	for (int i = 0; i < n; i++)
    		scheduler().executeXml("<add_order job_chain='/A' id='" + i + "'/>");
    }

    @EventHandler public void handle(OrderFinishedEvent e) {
        finishedOrderCount += 1;
        if (finishedOrderCount == maxOrderCount)
            controller().terminateScheduler();
    }
}
