package com.sos.scheduler.engine.tests.jira.js782;

import com.sos.scheduler.engine.data.order.OrderId;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.configuration.TestConfigurationBuilder;
import org.junit.Test;
import org.w3c.dom.Document;

import static com.sos.scheduler.engine.common.xml.CppXmlUtils.stringXPath;
import static com.sos.scheduler.engine.common.xml.CppXmlUtils.loadXml;
import static org.junit.Assert.assertTrue;

/** @see <a href='http://www.sos-berlin.com/jira/browse/JS-782'>JS-782</a> */
public final class JS782IT extends SchedulerTest {
    private static final OrderId orderId = new OrderId("testOrder");

    public JS782IT() {
        super(new TestConfigurationBuilder(JS782IT.class).terminateOnError(false).build());
    }

    @Test public void suspendedOrderMovedToEndStateShouldBlacklisted() {
        controller().activateScheduler();
        doTest();
        controller().terminateScheduler();
    }

    private void doTest() {
        scheduler().executeXml("<modify_order job_chain='a' order='testOrder' suspended='true'/>");
        assertTrue("Order should be blacklisted", orderIsBlacklisted());

        scheduler().executeXml("<modify_order job_chain='a' order='testOrder' suspended='false'/>");
        assertTrue("Order should not blacklisted", !orderIsBlacklisted());
    }

    private boolean orderIsBlacklisted() {
        Document doc = loadXml(scheduler().executeXml("<show_job_chain job_chain='a' what='job_chain_orders blacklist'/>"));
        return stringXPath(doc, "/spooler/answer/job_chain/blacklist/order/@order").equals(orderId.string());
    }
}
