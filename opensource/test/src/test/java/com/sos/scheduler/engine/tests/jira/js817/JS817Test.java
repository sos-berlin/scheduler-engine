package com.sos.scheduler.engine.tests.jira.js817;

import org.junit.Test;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.order.OrderId;
import com.sos.scheduler.engine.test.SchedulerTest;

/** JS-801 "JobScheduler crashes if order_state is set to a delayed jobchain_node by api".
 * @see <a href="http://www.sos-berlin.com/jira/browse/JS-801">JS-801</a> */
public class JS817Test extends SchedulerTest {
    private static final OrderId orderId = new OrderId("test");

    @Test public void testJobChainWithoutDelayShell() {
        runSchedulerWithJobChain("simple_chain");
    }

    private void runSchedulerWithJobChain(String jobChainName) {
        controller().activateScheduler("-e");
        scheduler().executeXml("<order job_chain='"+jobChainName+"' id='"+orderId+"'/>");
        controller().waitForTermination(shortTimeout);
    }

    @EventHandler public void handleEvent(OrderFinishedEvent e) {
        if (e.getKey().getId().equals(orderId))
            controller().terminateScheduler();
    }
}
