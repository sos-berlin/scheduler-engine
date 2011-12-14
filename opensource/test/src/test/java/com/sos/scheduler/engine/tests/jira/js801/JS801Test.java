package com.sos.scheduler.engine.tests.jira.js801;

import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Ignore;
import org.junit.Test;

/** JS-801 "JobScheduler crashes if order_state is set to a delayed jobchain_node by api".
 * @see <a href="http://www.sos-berlin.com/jira/browse/JS-801">JS-801</a> */
public class JS801Test extends SchedulerTest {
    //TODO Noch unvollständig

    @Test public void testWithError() {
        controller().activateScheduler("-e");
        scheduler().executeXml("<order job_chain='job_chain_with_error'/>");
        controller().waitForTermination(shortTimeout);
    }

    @Test public void testWithoutError() {
        controller().activateScheduler("-e");
        scheduler().executeXml("<order job_chain='job_chain_without_error'/>");
        controller().waitForTermination(shortTimeout);
    }
}
