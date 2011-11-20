package com.sos.scheduler.engine.tests.jira.js731;

import static org.hamcrest.Matchers.containsInAnyOrder;
import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertThat;

import org.junit.Test;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;

/** @see <a href='http://www.sos-berlin.com/jira/browse/JS-731'>JS-731</a> */
public class JS731Test extends SchedulerTest {
    @Test public void testOrderParametersNamesAndGet() throws InterruptedException {
        controller().startScheduler();
        String params = "<params><param name='a' value='ä'/><param name='B' value='B'/></params>";
        scheduler().executeXml("<add_order job_chain='a' id='1'>" + params + "</add_order>");
        controller().waitForTermination(shortTimeout);
    }

    @HotEventHandler public void handleEvent(OrderFinishedEvent e) {
        UnmodifiableVariableSet v = e.getOrder().getParameters();
        assertThat(v.get("a"), equalTo("ä"));
        assertThat(v.get("A"), equalTo("ä"));
        assertThat(v.get("b"), equalTo("B"));
        assertThat(v.get("B"), equalTo("B"));
        assertThat(v.getNames(), containsInAnyOrder("a", "B"));
        controller().terminateScheduler();
    }
}
