package com.sos.scheduler.engine.kernelcpptest.jira.js782;

import static com.sos.scheduler.engine.kernel.util.XmlUtils.loadXml;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.stringXPath;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.w3c.dom.Document;

import com.sos.scheduler.engine.kernel.order.OrderId;
import com.sos.scheduler.engine.test.SchedulerTest;

/** @see <a href='http://www.sos-berlin.com/jira/browse/JS-782'>JS-782</a> */
public class JS782Test extends SchedulerTest {
    private static final OrderId orderId = new OrderId("testOrder");

    @Test public void suspendedOrderMovedToEndStateShouldBeOnBlacklist() throws InterruptedException {
        controller().setTerminateOnError(false);
        controller().startScheduler();
        doTest();
        controller().terminateScheduler();
    }

    private void doTest() {
        scheduler().executeXml("<modify_order job_chain='a' order='testOrder' suspended='true'/>");
        assertTrue("Order should be on blacklist", orderIsOnBlacklist());

        scheduler().executeXml("<modify_order job_chain='a' order='testOrder' suspended='false'/>");
        assertTrue("Order should not be blacklist", !orderIsOnBlacklist());
    }

    private boolean orderIsOnBlacklist() {
        Document doc = loadXml(scheduler().executeXml("<show_job_chain job_chain='a' what='job_chain_orders blacklist'/>"));
        return stringXPath(doc, "/spooler/answer/job_chain/blacklist/order/@order").equals(orderId.getString());
    }
}
