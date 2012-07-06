package com.sos.scheduler.engine.tests.jira.js801;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.data.order.OrderId;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

import static org.junit.Assert.fail;

/** JS-801 "JobScheduler crashes if order_state is set to a delayed jobchain_node by api".
 * @see <a href="http://www.sos-berlin.com/jira/browse/JS-801">JS-801</a> */
public class JS801Test extends SchedulerTest {
    private static final OrderId orderId = new OrderId("test");

    @Test public void testJobChainWithoutDelay() {
        runSchedulerWithJobChain("without_delay");
    }

    @Test public void testJobChainWithDelay() {
        runSchedulerWithJobChain("with_delay");
    }

    @Test public void testJobChainWithAt() {
        String expectedErrorCode = "SCHEDULER-217";
        boolean ok;
        try {
            runSchedulerWithJobChain("with_at");
            ok = false;
        } catch (Exception x) {
            ok = x.getMessage().contains(expectedErrorCode);
        }
        if (!ok) fail("Exception "+expectedErrorCode+" expected");
    }

    private void runSchedulerWithJobChain(String jobChainName) {
        controller().activateScheduler();
        scheduler().executeXml("<order job_chain='"+jobChainName+"' id='"+orderId+"'/>");
        controller().waitForTermination(shortTimeout);
    }

    @EventHandler public void handleEvent(OrderFinishedEvent e) {
        if (e.getKey().getId().equals(orderId))
            controller().terminateScheduler();
    }
}
